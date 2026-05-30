#pragma once

#include "ast.h"
#include "tac.h"

class TACGenerator {
    int tempCnt  = 0;
    int labelCnt = 0;

    std::string freshTemp()  { return "t" + std::to_string(++tempCnt);  }
    std::string freshLabel() { return "L" + std::to_string(++labelCnt); }

    std::string emit(ExprNode* node);
    void        emit(StmtNode* node);
    void        emit(CallStmtNode*     node);
    void        emit(VarDeclNode*    node);
    void        emit(AssignNode*     node);
    void        emit(BlockNode*      node);
    void        emit(IfNode*         node);
    void        emit(WhileNode*      node);
    void        emit(ReturnNode*     node);
    void        emit(PrintNode*      node);
    void        emit(FuncDeclNode*   node);
    std::string emit(IntLiteralNode*   node);
    std::string emit(FloatLiteralNode* node);
    std::string emit(IdentifierNode*   node);
    std::string emit(UnaryOpNode*      node);
    std::string emit(BinaryOpNode*     node);
    std::string emit(CallExprNode*     node);

public:
    TACProgram instructions;

    void generate(ProgramNode* node);
};
