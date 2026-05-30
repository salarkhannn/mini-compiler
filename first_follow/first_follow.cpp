/*
 * First & Follow set computation + LL(1) parsing table construction.
 *
 * Implements the target grammar from the project spec:
 *   E  -> T E'
 *   E' -> + T E' | - T E' | ε
 *   T  -> F T'
 *   T' -> * F T' | / F T' | ε
 *   F  -> ( E ) | id | num
 *
 * Outputs FIRST sets, FOLLOW sets, and the complete LL(1) parsing table.
 */
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>

static const std::string EPSILON = "ε";
static const std::string END_OF_INPUT = "$";

using SymSet    = std::set<std::string>;
using FirstMap  = std::map<std::string, SymSet>;
using FollowMap = std::map<std::string, SymSet>;
// LL(1) table: table[NonTerminal][Terminal] = production RHS
using LL1Table  = std::map<std::string, std::map<std::string, std::string>>;

// Grammar rule: lhs -> rhs (rhs is a vector of symbols, EPSILON denotes ε)
struct Rule {
    std::string              lhs;
    std::vector<std::string> rhs;
};

static const std::vector<std::string> nonTerminals = { "E", "E'", "T", "T'", "F" };
static const std::vector<std::string> terminals    = { "+", "-", "*", "/", "(", ")", "id", "num", "$" };

static const std::vector<Rule> grammar = {
    { "E",  { "T", "E'" } },
    { "E'", { "+", "T", "E'" } },
    { "E'", { "-", "T", "E'" } },
    { "E'", { EPSILON } },
    { "T",  { "F", "T'" } },
    { "T'", { "*", "F", "T'" } },
    { "T'", { "/", "F", "T'" } },
    { "T'", { EPSILON } },
    { "F",  { "(", "E", ")" } },
    { "F",  { "id" } },
    { "F",  { "num" } },
};

static bool isNonTerminal(const std::string& s) {
    return std::find(nonTerminals.begin(), nonTerminals.end(), s) != nonTerminals.end();
}

// Compute FIRST sets using the fixed-point algorithm.
static FirstMap computeFirst() {
    FirstMap first;
    for (const auto& nt : nonTerminals) first[nt] = {};

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& rule : grammar) {
            size_t before = first[rule.lhs].size();

            if (rule.rhs.size() == 1 && rule.rhs[0] == EPSILON) {
                if (first[rule.lhs].insert(EPSILON).second) changed = true;
                continue;
            }

            bool allNullable = true;
            for (const auto& sym : rule.rhs) {
                if (!isNonTerminal(sym)) {
                    first[rule.lhs].insert(sym);
                    allNullable = false;
                    break;
                }
                SymSet& fs = first[sym];
                for (const auto& t : fs)
                    if (t != EPSILON) first[rule.lhs].insert(t);
                if (fs.count(EPSILON) == 0) { allNullable = false; break; }
            }
            if (allNullable) first[rule.lhs].insert(EPSILON);

            if (first[rule.lhs].size() != before) changed = true;
        }
    }
    return first;
}

// Compute FOLLOW sets using the fixed-point algorithm.
static FollowMap computeFollow(const FirstMap& first) {
    FollowMap follow;
    for (const auto& nt : nonTerminals) follow[nt] = {};
    follow["E"].insert(END_OF_INPUT);  // start symbol

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& rule : grammar) {
            for (size_t i = 0; i < rule.rhs.size(); ++i) {
                const std::string& sym = rule.rhs[i];
                if (!isNonTerminal(sym)) continue;

                size_t before = follow[sym].size();

                // Add FIRST(β) \ {ε} to FOLLOW(sym) where β = rhs[i+1...]
                bool allNullable = true;
                for (size_t j = i + 1; j < rule.rhs.size(); ++j) {
                    const std::string& next = rule.rhs[j];
                    if (!isNonTerminal(next)) {
                        follow[sym].insert(next);
                        allNullable = false;
                        break;
                    }
                    const SymSet& fs = first.at(next);
                    for (const auto& t : fs)
                        if (t != EPSILON) follow[sym].insert(t);
                    if (fs.count(EPSILON) == 0) { allNullable = false; break; }
                }
                if (allNullable) {
                    for (const auto& t : follow[rule.lhs])
                        follow[sym].insert(t);
                }

                if (follow[sym].size() != before) changed = true;
            }
        }
    }
    return follow;
}

// Build the LL(1) parsing table.
static LL1Table buildTable(const FirstMap& first, const FollowMap& follow) {
    LL1Table table;
    for (const auto& rule : grammar) {
        // Compute FIRST of this production's RHS.
        SymSet rhsFirst;
        bool allNullable = true;
        for (const auto& sym : rule.rhs) {
            if (sym == EPSILON) break;
            if (!isNonTerminal(sym)) {
                rhsFirst.insert(sym);
                allNullable = false;
                break;
            }
            const SymSet& fs = first.at(sym);
            for (const auto& t : fs)
                if (t != EPSILON) rhsFirst.insert(t);
            if (fs.count(EPSILON) == 0) { allNullable = false; break; }
        }
        if (allNullable) rhsFirst.insert(EPSILON);

        std::string rhsStr;
        for (const auto& s : rule.rhs) rhsStr += (rhsStr.empty() ? "" : " ") + s;

        for (const auto& t : rhsFirst) {
            if (t != EPSILON) {
                table[rule.lhs][t] = rhsStr;
            } else {
                for (const auto& f : follow.at(rule.lhs))
                    table[rule.lhs][f] = EPSILON;
            }
        }
    }
    return table;
}

static void printSet(const std::string& label, const SymSet& s) {
    std::cout << std::setw(4) << label << " -> { ";
    bool first = true;
    for (const auto& t : s) {
        if (!first) std::cout << ", ";
        std::cout << t;
        first = false;
    }
    std::cout << " }\n";
}

static void printTable(const LL1Table& table) {
    const int W = 14;
    std::cout << "\nLL(1) Parsing Table\n";
    std::cout << std::string((terminals.size() + 1) * W, '-') << "\n";
    std::cout << std::setw(W) << "NT \\ Terminal";
    for (const auto& t : terminals) std::cout << std::setw(W) << t;
    std::cout << "\n";
    std::cout << std::string((terminals.size() + 1) * W, '-') << "\n";

    for (const auto& nt : nonTerminals) {
        std::cout << std::setw(W) << nt;
        for (const auto& t : terminals) {
            auto rowIt = table.find(nt);
            if (rowIt != table.end()) {
                auto colIt = rowIt->second.find(t);
                if (colIt != rowIt->second.end()) {
                    std::string entry = nt + " -> " + colIt->second;
                    std::cout << std::setw(W) << entry;
                    continue;
                }
            }
            std::cout << std::setw(W) << "-";
        }
        std::cout << "\n";
    }
}

int main() {
    FirstMap  first  = computeFirst();
    FollowMap follow = computeFollow(first);
    LL1Table  table  = buildTable(first, follow);

    std::cout << "=== FIRST Sets ===\n";
    for (const auto& nt : nonTerminals) printSet(nt, first[nt]);

    std::cout << "\n=== FOLLOW Sets ===\n";
    for (const auto& nt : nonTerminals) printSet(nt, follow[nt]);

    printTable(table);
}
