#pragma once

#include <string>
#include <vector>
#include <iostream>

enum class TACOp {
    ASSIGN,    // result = arg1
    BINARY,    // result = arg1 op arg2
    UNARY,     // result = op arg1
    GOTO,      // goto result
    IF_GOTO,   // if arg1 goto result
    IFNOT_GOTO,// ifnot arg1 goto result
    LABEL,     // result:
    PARAM,     // param arg1
    CALL,      // result = call arg1, arg2(count)
    RETURN,    // return arg1  (arg1 may be empty)
    PRINT,     // print arg1
    ARRAY_LOAD, // result = arg1[arg2]
    ARRAY_STORE // arg1[arg2] = result
};

struct TACInst {
    TACOp       op   = TACOp::ASSIGN;
    std::string result;
    std::string arg1;
    std::string arg2;
    std::string oper;

    void print(std::ostream& out = std::cout) const {
        switch (op) {
            case TACOp::ASSIGN:
                out << "    " << result << " = " << arg1 << "\n";
                break;
            case TACOp::BINARY:
                out << "    " << result << " = " << arg1 << " " << oper << " " << arg2 << "\n";
                break;
            case TACOp::UNARY:
                out << "    " << result << " = " << oper << arg1 << "\n";
                break;
            case TACOp::GOTO:
                out << "    goto " << result << "\n";
                break;
            case TACOp::IF_GOTO:
                out << "    if " << arg1 << " goto " << result << "\n";
                break;
            case TACOp::IFNOT_GOTO:
                out << "    ifnot " << arg1 << " goto " << result << "\n";
                break;
            case TACOp::LABEL:
                out << result << ":\n";
                break;
            case TACOp::PARAM:
                out << "    param " << arg1 << "\n";
                break;
            case TACOp::CALL:
                if (result.empty())
                    out << "    call " << arg1 << ", " << arg2 << "\n";
                else
                    out << "    " << result << " = call " << arg1 << ", " << arg2 << "\n";
                break;
            case TACOp::RETURN:
                if (arg1.empty())
                    out << "    return\n";
                else
                    out << "    return " << arg1 << "\n";
                break;
            case TACOp::PRINT:
                out << "    print " << arg1 << "\n";
                break;
            case TACOp::ARRAY_LOAD:
                out << "    " << result << " = " << arg1 << "[" << arg2 << "]\n";
                break;
            case TACOp::ARRAY_STORE:
                out << "    " << arg1 << "[" << arg2 << "] = " << result << "\n";
                break;
        }
    }
};

using TACProgram = std::vector<TACInst>;

inline void printTAC(const TACProgram& prog, std::ostream& out = std::cout) {
    for (const auto& inst : prog) inst.print(out);
}
