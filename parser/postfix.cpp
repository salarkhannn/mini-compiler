/*
 * Postfix expression evaluator (Module 2).
 * Reads space-separated tokens: operands (numbers) and operators (+, -, *, /).
 * Uses a value stack and prints each push/pop step.
 *
 * Example input:  3 4 + 2 *
 * Expected output: 14
 */
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>


static bool isNumber(const std::string& tok) {
    if (tok.empty()) return false;
    size_t i = (tok[0] == '-' || tok[0] == '+') ? 1 : 0;
    bool hasDot = false, hasDigit = false;
    for (; i < tok.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(tok[i]))) { hasDigit = true; continue; }
        if (tok[i] == '.' && !hasDot)                         { hasDot = true; continue; }
        return false;
    }
    return hasDigit;
}

static void printStack(const std::stack<double>& st) {
    std::stack<double> tmp = st;
    std::string repr = "[ ";
    std::vector<double> v;
    while (!tmp.empty()) { v.push_back(tmp.top()); tmp.pop(); }
    for (auto it = v.rbegin(); it != v.rend(); ++it) repr += std::to_string(*it) + " ";
    repr += "]";
    std::cout << "  stack: " << repr << "\n";
}

static double evaluate(const std::string& expr) {
    std::istringstream ss(expr);
    std::string tok;
    std::stack<double> st;

    while (ss >> tok) {
        if (isNumber(tok)) {
            double val = std::stod(tok);
            st.push(val);
            std::cout << "  push " << val << "\n";
            printStack(st);
        } else if (tok.size() == 1 &&
                   (tok[0] == '+' || tok[0] == '-' ||
                    tok[0] == '*' || tok[0] == '/')) {
            if (st.size() < 2)
                throw std::runtime_error("insufficient operands for '" + tok + "'");
            double b = st.top(); st.pop();
            double a = st.top(); st.pop();
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
            std::cout << "  pop " << a << ", pop " << b
                      << ", " << tok << " -> " << r << "\n";
            st.push(r);
            printStack(st);
        } else {
            throw std::runtime_error("unrecognized token '" + tok + "'");
        }
    }

    if (st.size() != 1)
        throw std::runtime_error("malformed expression: " +
                                 std::to_string(st.size()) + " values remain on stack");
    return st.top();
}

int main() {
    std::string line;
    std::cout << "Postfix Evaluator (enter expression, Ctrl-D to quit)\n";
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        try {
            double result = evaluate(line);
            std::cout << "Result: " << result << "\n\n";
        } catch (const std::exception& e) {
            std::cerr << "error: " << e.what() << "\n\n";
        }
    }
}
