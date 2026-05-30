/*
 * Prefix expression evaluator (Module 2).
 * Reads tokens in prefix order: operator precedes its operands.
 * Evaluation is recursive-descent over a token stream.
 *
 * Example input:  * + 3 4 2
 * Expected output: 14
 */
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>

static std::vector<std::string> tokens;
static size_t pos = 0;

static bool isNumber(const std::string& tok) {
    if (tok.empty()) return false;
    size_t i = (tok[0] == '-' || tok[0] == '+') ? 1 : 0;
    bool hasDot = false, hasDigit = false;
    for (; i < tok.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(tok[i]))) { hasDigit = true; continue; }
        if (tok[i] == '.' && !hasDot) { hasDot = true; continue; }
        return false;
    }
    return hasDigit && i > (tok[0] == '-' || tok[0] == '+' ? 1u : 0u);
}

static double parseExpr(int depth);

static double parsePrimary(int depth) {
    if (pos >= tokens.size())
        throw std::runtime_error("unexpected end of expression");

    const std::string& tok = tokens[pos++];

    if (tok.size() == 1 &&
        (tok[0] == '+' || tok[0] == '-' ||
         tok[0] == '*' || tok[0] == '/')) {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "op '" << tok << "'\n";
        double a = parsePrimary(depth + 1);
        double b = parsePrimary(depth + 1);
        double r;
        switch (tok[0]) {
            case '+': r = a + b; break;
            case '-': r = a - b; break;
            case '*': r = a * b; break;
            case '/':
                if (b == 0.0) throw std::runtime_error("division by zero");
                r = a / b; break;
            default: r = 0;
        }
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "=> " << r << "\n";
        return r;
    }

    if (isNumber(tok)) {
        double v = std::stod(tok);
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "num " << v << "\n";
        return v;
    }

    throw std::runtime_error("unrecognized token '" + tok + "'");
}

int main() {
    std::string line;
    std::cout << "Prefix Evaluator (enter expression, Ctrl-D to quit)\n";
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        tokens.clear();
        pos = 0;
        std::istringstream ss(line);
        std::string tok;
        while (ss >> tok) tokens.push_back(tok);
        try {
            double result = parsePrimary(0);
            if (pos != tokens.size())
                throw std::runtime_error("extra tokens after expression");
            std::cout << "Result: " << result << "\n\n";
        } catch (const std::exception& e) {
            std::cerr << "error: " << e.what() << "\n\n";
            tokens.clear(); pos = 0;
        }
    }
}
