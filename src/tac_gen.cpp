#include "tac_gen.h"
#include <cmath>
#include <stdexcept>

void TACGenerator::generate(ProgramNode* node) {
    for (const auto& gv : node->globalVars)
        emit(gv.get());

    for (const auto& fn : node->functions)
        emit(fn.get());
}

void TACGenerator::emit(FuncDeclNode* node) {
    instructions.push_back({TACOp::LABEL, node->name, "", "", ""});
    for (const auto& p : node->params)
        instructions.push_back({TACOp::PARAM, p.name, p.name, "", ""});
    emit(node->body.get());
    if (node->returnType == Type::VOID)
        instructions.push_back({TACOp::RETURN, "", "", "", ""});
}

std::string TACGenerator::emit(ExprNode* node) {
    if (auto* n = dynamic_cast<IntLiteralNode*>(node))   return emit(n);
    if (auto* n = dynamic_cast<FloatLiteralNode*>(node)) return emit(n);
    if (auto* n = dynamic_cast<IdentifierNode*>(node))   return emit(n);
    if (auto* n = dynamic_cast<UnaryOpNode*>(node))      return emit(n);
    if (auto* n = dynamic_cast<BinaryOpNode*>(node))     return emit(n);
    if (auto* n = dynamic_cast<CallExprNode*>(node))     return emit(n);
    return "";
}

void TACGenerator::emit(StmtNode* node) {
    if (!node) return;
    if (auto* n = dynamic_cast<VarDeclNode*>(node)) emit(n);
    else if (auto* n = dynamic_cast<AssignNode*>(node))   emit(n);
    else if (auto* n = dynamic_cast<BlockNode*>(node))    emit(n);
    else if (auto* n = dynamic_cast<CallStmtNode*>(node)) emit(n);
    else if (auto* n = dynamic_cast<IfNode*>(node))       emit(n);
    else if (auto* n = dynamic_cast<WhileNode*>(node))    emit(n);
    else if (auto* n = dynamic_cast<ReturnNode*>(node))   emit(n);
    else if (auto* n = dynamic_cast<PrintNode*>(node))    emit(n);
}

void TACGenerator::emit(CallStmtNode* node) {
    emit(static_cast<ExprNode*>(node->call.get()));
}

std::string TACGenerator::emit(IntLiteralNode*   n) { return std::to_string(n->value); }
std::string TACGenerator::emit(FloatLiteralNode* n) { return std::to_string(n->value); }
std::string TACGenerator::emit(IdentifierNode*   n) { return n->name; }

std::string TACGenerator::emit(UnaryOpNode* node) {
    std::string src = emit(node->operand.get());
    std::string dst = freshTemp();
    instructions.push_back({TACOp::UNARY, dst, src, "", node->op});
    return dst;
}

std::string TACGenerator::emit(BinaryOpNode* node) {
    std::string lhs = emit(node->left.get());
    std::string rhs = emit(node->right.get());
    std::string dst = freshTemp();
    instructions.push_back({TACOp::BINARY, dst, lhs, rhs, node->op});
    return dst;
}

std::string TACGenerator::emit(CallExprNode* node) {
    for (const auto& arg : node->args) {
        std::string v = emit(arg.get());
        instructions.push_back({TACOp::PARAM, "", v, "", ""});
    }
    std::string dst = freshTemp();
    instructions.push_back({TACOp::CALL, dst, node->funcName,
                             std::to_string(node->args.size()), ""});
    return dst;
}

void TACGenerator::emit(VarDeclNode*) {
    // Variable declarations only affect the symbol table; no TAC needed.
}

void TACGenerator::emit(AssignNode* node) {
    std::string val = emit(node->expr.get());
    instructions.push_back({TACOp::ASSIGN, node->name, val, "", ""});
}

void TACGenerator::emit(BlockNode* node) {
    for (const auto& s : node->stmts) emit(s.get());
}

void TACGenerator::emit(IfNode* node) {
    std::string cond     = emit(node->condition.get());
    std::string thenLbl  = freshLabel();
    std::string endLbl   = freshLabel();

    if (node->elseBlock) {
        std::string elseLbl = freshLabel();
        instructions.push_back({TACOp::IF_GOTO, thenLbl, cond, "", ""});
        instructions.push_back({TACOp::GOTO,    elseLbl, "", "", ""});
        instructions.push_back({TACOp::LABEL,   thenLbl, "", "", ""});
        emit(node->thenBlock.get());
        instructions.push_back({TACOp::GOTO,    endLbl, "", "", ""});
        instructions.push_back({TACOp::LABEL,   elseLbl, "", "", ""});
        emit(node->elseBlock.get());
        instructions.push_back({TACOp::LABEL,   endLbl, "", "", ""});
    } else {
        instructions.push_back({TACOp::IF_GOTO, thenLbl, cond, "", ""});
        instructions.push_back({TACOp::GOTO,    endLbl, "", "", ""});
        instructions.push_back({TACOp::LABEL,   thenLbl, "", "", ""});
        emit(node->thenBlock.get());
        instructions.push_back({TACOp::LABEL,   endLbl, "", "", ""});
    }
}

void TACGenerator::emit(WhileNode* node) {
    std::string testLbl = freshLabel();
    std::string bodyLbl = freshLabel();
    std::string endLbl  = freshLabel();

    instructions.push_back({TACOp::LABEL,   testLbl, "", "", ""});
    std::string cond = emit(node->condition.get());
    instructions.push_back({TACOp::IF_GOTO, bodyLbl, cond, "", ""});
    instructions.push_back({TACOp::GOTO,    endLbl, "", "", ""});
    instructions.push_back({TACOp::LABEL,   bodyLbl, "", "", ""});
    emit(node->body.get());
    instructions.push_back({TACOp::GOTO,    testLbl, "", "", ""});
    instructions.push_back({TACOp::LABEL,   endLbl, "", "", ""});
}

void TACGenerator::emit(ReturnNode* node) {
    if (node->expr) {
        std::string val = emit(node->expr.get());
        instructions.push_back({TACOp::RETURN, "", val, "", ""});
    } else {
        instructions.push_back({TACOp::RETURN, "", "", "", ""});
    }
}

void TACGenerator::emit(PrintNode* node) {
    std::string val = emit(node->expr.get());
    instructions.push_back({TACOp::PRINT, "", val, "", ""});
}
