#include "optimizer.h"
#include <unordered_map>
#include <unordered_set>
#include <cctype>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool isLiteral(const std::string& s) {
    if (s.empty()) return false;
    size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    bool hasDigit = false, hasDot = false;
    for (; i < s.size(); ++i) {
        char c = s[i];
        if (std::isdigit(static_cast<unsigned char>(c))) { hasDigit = true; continue; }
        if (c == '.' && !hasDot)                         { hasDot = true; continue; }
        return false;
    }
    return hasDigit;
}

static double asDouble(const std::string& s) { return std::stod(s); }

static std::string fmtNumber(double v, bool isRelational) {
    if (isRelational) return std::to_string(static_cast<int>(v));
    // Trim trailing zeros from floating representation only if needed.
    std::string s = std::to_string(v);
    if (s.find('.') != std::string::npos) {
        s.erase(s.find_last_not_of('0') + 1);
        if (s.back() == '.') s += "0";
    }
    return s;
}

// ---------------------------------------------------------------------------
// Pass 1: Constant folding + constant propagation (combined single pass)
// ---------------------------------------------------------------------------

void Optimizer::constantFoldAndPropagate(TACProgram& prog) {
    // Maps variable name -> literal value if it is known to be constant.
    std::unordered_map<std::string, std::string> constants;

    auto resolve = [&](const std::string& v) -> std::string {
        auto it = constants.find(v);
        return (it != constants.end()) ? it->second : v;
    };

    auto invalidate = [&](const std::string& v) { constants.erase(v); };

    for (auto& inst : prog) {
        // At any label or backward jump, we conservatively clear our constant map
        // because we don't know which path reached this label.
        if (inst.op == TACOp::LABEL || inst.op == TACOp::GOTO) {
            constants.clear();
            continue;
        }

        switch (inst.op) {
            case TACOp::ASSIGN: {
                inst.arg1 = resolve(inst.arg1);
                if (isLiteral(inst.arg1))
                    constants[inst.result] = inst.arg1;
                else
                    invalidate(inst.result);
                break;
            }
            case TACOp::BINARY: {
                inst.arg1 = resolve(inst.arg1);
                inst.arg2 = resolve(inst.arg2);
                if (isLiteral(inst.arg1) && isLiteral(inst.arg2)) {
                    double a = asDouble(inst.arg1);
                    double b = asDouble(inst.arg2);
                    double r = 0.0;
                    bool rel = false;
                    const std::string& op = inst.oper;
                    if      (op == "+")  r = a + b;
                    else if (op == "-")  r = a - b;
                    else if (op == "*")  r = a * b;
                    else if (op == "/")  r = (b == 0.0) ? 0.0 : a / b;
                    else if (op == "^")  r = std::pow(a, b);
                    else if (op == "==") { r = (a == b) ? 1.0 : 0.0; rel = true; }
                    else if (op == "!=") { r = (a != b) ? 1.0 : 0.0; rel = true; }
                    else if (op == "<")  { r = (a  < b) ? 1.0 : 0.0; rel = true; }
                    else if (op == ">")  { r = (a  > b) ? 1.0 : 0.0; rel = true; }
                    else if (op == "<=") { r = (a <= b) ? 1.0 : 0.0; rel = true; }
                    else if (op == ">=") { r = (a >= b) ? 1.0 : 0.0; rel = true; }

                    std::string lit = fmtNumber(r, rel);
                    inst.op   = TACOp::ASSIGN;
                    inst.arg1 = lit;
                    inst.arg2.clear();
                    inst.oper.clear();
                    constants[inst.result] = lit;
                } else {
                    invalidate(inst.result);
                }
                break;
            }
            case TACOp::UNARY: {
                inst.arg1 = resolve(inst.arg1);
                if (isLiteral(inst.arg1) && inst.oper == "-") {
                    double v = -asDouble(inst.arg1);
                    std::string lit = fmtNumber(v, false);
                    inst.op   = TACOp::ASSIGN;
                    inst.arg1 = lit;
                    inst.oper.clear();
                    constants[inst.result] = lit;
                } else {
                    invalidate(inst.result);
                }
                break;
            }
            case TACOp::IF_GOTO:
            case TACOp::IFNOT_GOTO: {
                inst.arg1 = resolve(inst.arg1);
                constants.clear(); // Branching point — conservative.
                break;
            }
            case TACOp::PARAM:
            case TACOp::PRINT:
                inst.arg1 = resolve(inst.arg1);
                break;
            case TACOp::RETURN:
                if (!inst.arg1.empty()) inst.arg1 = resolve(inst.arg1);
                break;
            case TACOp::CALL:
                invalidate(inst.result);
                break;
            default:
                break;
        }
    }
}

// ---------------------------------------------------------------------------
// Pass 2: Common Sub-expression Elimination (CSE)
// ---------------------------------------------------------------------------
// We only eliminate binary expressions within a basic block (no LABEL or GOTO
// boundary crossed). For each distinct (arg1 op arg2) pair we track the first
// temp that computed it, and reuse that temp thereafter.

void Optimizer::commonSubexpressionElimination(TACProgram& prog) {
    // Maps canonical expression string -> result temp that holds its value.
    std::unordered_map<std::string, std::string> available;

    auto invalidateContaining = [&](const std::string& var) {
        std::vector<std::string> toErase;
        for (const auto& kv : available) {
            // If the variable appears in the expression key, it's no longer valid.
            if (kv.first.find(var) != std::string::npos)
                toErase.push_back(kv.first);
        }
        for (const auto& k : toErase) available.erase(k);
        // Also erase any expression whose result is this variable.
        for (auto it = available.begin(); it != available.end(); ) {
            if (it->second == var) it = available.erase(it);
            else ++it;
        }
    };

    for (auto& inst : prog) {
        if (inst.op == TACOp::LABEL || inst.op == TACOp::GOTO) {
            available.clear();
            continue;
        }
        if (inst.op == TACOp::BINARY) {
            std::string key = inst.arg1 + " " + inst.oper + " " + inst.arg2;
            auto it = available.find(key);
            if (it != available.end() && it->second != inst.result) {
                // Reuse the previously computed value.
                inst.op   = TACOp::ASSIGN;
                inst.arg1 = it->second;
                inst.arg2.clear();
                inst.oper.clear();
            } else {
                available[key] = inst.result;
            }
        }
        // Any assignment to a variable invalidates expressions involving it.
        if (!inst.result.empty() &&
            (inst.op == TACOp::ASSIGN || inst.op == TACOp::BINARY ||
             inst.op == TACOp::UNARY  || inst.op == TACOp::CALL)) {
            invalidateContaining(inst.result);
            if (inst.op == TACOp::BINARY) {
                std::string key = inst.arg1 + " " + inst.oper + " " + inst.arg2;
                available[key] = inst.result;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Pass 3: Dead Code Elimination
// ---------------------------------------------------------------------------
// A temp variable is dead if it is written but never subsequently read.
// We do a backwards liveness scan.

void Optimizer::deadCodeElimination(TACProgram& prog) {
    std::unordered_set<std::string> live;

    // Backwards scan: anything not a compiler-generated temp is always live.
    auto isSyntheticTemp = [](const std::string& s) -> bool {
        if (s.empty() || s[0] != 't') return false;
        for (size_t i = 1; i < s.size(); ++i)
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        return true;
    };

    auto markLive = [&](const std::string& v) {
        if (!v.empty()) live.insert(v);
    };

    // First pass: collect all uses in reverse.
    for (int i = static_cast<int>(prog.size()) - 1; i >= 0; --i) {
        const auto& inst = prog[i];
        // Mark uses as live.
        if (!inst.arg1.empty())  markLive(inst.arg1);
        if (!inst.arg2.empty() && inst.op != TACOp::CALL) markLive(inst.arg2);
        if (inst.op == TACOp::CALL) markLive(inst.arg1); // function name is not a var

        // If the result is a temp and it is not live, this instruction is dead.
        if (!inst.result.empty() && isSyntheticTemp(inst.result) &&
            live.count(inst.result) == 0 &&
            inst.op != TACOp::LABEL && inst.op != TACOp::GOTO &&
            inst.op != TACOp::IF_GOTO && inst.op != TACOp::IFNOT_GOTO &&
            inst.op != TACOp::RETURN && inst.op != TACOp::PRINT &&
            inst.op != TACOp::CALL) {
            prog[i].op = TACOp::LABEL; // repurpose as a no-op marker
            prog[i].result = "__dead__";
            continue;
        }

        // The definition kills liveness of the result.
        if (!inst.result.empty()) live.erase(inst.result);
    }

    prog.erase(std::remove_if(prog.begin(), prog.end(),
                              [](const TACInst& i) {
                                  return i.op == TACOp::LABEL && i.result == "__dead__";
                              }),
               prog.end());
}

// ---------------------------------------------------------------------------
// Pass 4: Unreachable Block Elimination
// ---------------------------------------------------------------------------
// Any instruction after an unconditional GOTO that is not a label target
// is unreachable.

void Optimizer::unreachableBlockElimination(TACProgram& prog) {
    std::unordered_set<std::string> targets;
    for (const auto& inst : prog) {
        if (inst.op == TACOp::GOTO || inst.op == TACOp::IF_GOTO ||
            inst.op == TACOp::IFNOT_GOTO)
            targets.insert(inst.result);
        // Function entry labels referenced via CALL must also be reachable.
        if (inst.op == TACOp::CALL)
            targets.insert(inst.arg1);
    }

    TACProgram cleaned;
    cleaned.reserve(prog.size());
    bool reachable = true;

    for (const auto& inst : prog) {
        if (inst.op == TACOp::LABEL) {
            // Reachable if jumped to, called, or if this is the very first instruction.
            reachable = cleaned.empty() || targets.count(inst.result) > 0;
        }
        if (reachable) cleaned.push_back(inst);
        if (inst.op == TACOp::GOTO || inst.op == TACOp::RETURN) {
            reachable = false;
        }
    }

    prog.swap(cleaned);
}

// ---------------------------------------------------------------------------
// Pass 5: Loop-Invariant Code Motion (LICM)
// ---------------------------------------------------------------------------
// Identifies GOTO-back loops, collects invariant temporaries, and hoists them
// before the loop header.

void Optimizer::loopInvariantCodeMotion(TACProgram& prog) {
    // Build label -> index map.
    auto buildLabelMap = [&]() {
        std::unordered_map<std::string, size_t> m;
        for (size_t i = 0; i < prog.size(); ++i)
            if (prog[i].op == TACOp::LABEL) m[prog[i].result] = i;
        return m;
    };

    auto labelMap = buildLabelMap();

    for (size_t i = 0; i < prog.size(); ++i) {
        if (prog[i].op != TACOp::GOTO) continue;

        auto it = labelMap.find(prog[i].result);
        if (it == labelMap.end()) continue;

        size_t loopHeader = it->second;
        size_t loopBack   = i;
        if (loopHeader >= loopBack) continue; // forward jump, not a back-edge

        // Find the conditional guard right after the header (if any).
        size_t bodyStart = loopHeader + 1;
        for (size_t j = loopHeader + 1; j < loopBack; ++j)
            if (prog[j].op == TACOp::LABEL) bodyStart = j + 1;
        if (bodyStart >= loopBack) continue;

        // Collect the set of variables defined inside the loop body.
        std::unordered_set<std::string> defined;
        for (size_t j = bodyStart; j < loopBack; ++j) {
            const auto& inst = prog[j];
            if (!inst.result.empty() &&
                (inst.op == TACOp::ASSIGN || inst.op == TACOp::BINARY ||
                 inst.op == TACOp::UNARY  || inst.op == TACOp::CALL))
                defined.insert(inst.result);
        }

        // Invariant: result is a temp AND neither arg1 nor arg2 are in defined.
        auto isInvariant = [&](const TACInst& inst) -> bool {
            if (inst.result.empty() || inst.result[0] != 't') return false;
            if (inst.op != TACOp::ASSIGN && inst.op != TACOp::BINARY &&
                inst.op != TACOp::UNARY)  return false;
            bool a1ok = inst.arg1.empty() || defined.count(inst.arg1) == 0;
            bool a2ok = inst.arg2.empty() || defined.count(inst.arg2) == 0;
            return a1ok && a2ok;
        };

        std::vector<size_t> hoistIdx;
        for (size_t j = bodyStart; j < loopBack; ++j)
            if (isInvariant(prog[j])) hoistIdx.push_back(j);

        if (hoistIdx.empty()) continue;

        TACProgram hoisted;
        for (size_t idx : hoistIdx) {
            hoisted.push_back(prog[idx]);
            prog[idx].op     = TACOp::LABEL;
            prog[idx].result = "__hoisted__";
        }

        // Insert hoisted instructions before the loop header.
        prog.insert(prog.begin() + static_cast<std::ptrdiff_t>(loopHeader),
                    hoisted.begin(), hoisted.end());

        // Remove placeholder __hoisted__ markers.
        prog.erase(std::remove_if(prog.begin(), prog.end(),
                                  [](const TACInst& x) {
                                      return x.op == TACOp::LABEL &&
                                             x.result == "__hoisted__";
                                  }),
                   prog.end());

        // Rebuild label map after structural changes.
        labelMap = buildLabelMap();
        if (i > 0) --i; // Re-examine this position.
    }
}

// ---------------------------------------------------------------------------
// Public entry point — run all passes in dependency order.
// ---------------------------------------------------------------------------

void Optimizer::run(TACProgram& prog) {
    constantFoldAndPropagate(prog);
    commonSubexpressionElimination(prog);
    deadCodeElimination(prog);
    unreachableBlockElimination(prog);
    loopInvariantCodeMotion(prog);
}
