#pragma once

#include "ast.h"
#include "symbol_table.h"
#include "error_handler.h"

class SemanticAnalyzer {
    SymbolTable symbolTable;
    Type        currentReturnType = Type::UNKNOWN;

    void visit(ExprNode*    node);
    void visit(StmtNode*    node);
    void visit(IntLiteralNode*   node);
    void visit(FloatLiteralNode* node);
    void visit(IdentifierNode*   node);
    void visit(UnaryOpNode*      node);
    void visit(BinaryOpNode*     node);
    void visit(CallExprNode*     node);
    void visit(CallStmtNode*    node);
    void visit(VarDeclNode*      node);
    void visit(AssignNode*       node);
    void visit(BlockNode*        node);
    void visit(IfNode*           node);
    void visit(WhileNode*        node);
    void visit(ReturnNode*       node);
    void visit(PrintNode*        node);
    void visit(FuncDeclNode*     node);

    Type widenType(Type a, Type b) const;

public:
    void analyze(ProgramNode* node);
};
