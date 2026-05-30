#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>

enum class Type { INT, FLOAT, VOID, UNKNOWN };

inline std::string typeToString(Type t) {
    switch (t) {
        case Type::INT:     return "int";
        case Type::FLOAT:   return "float";
        case Type::VOID:    return "void";
        case Type::UNKNOWN: return "unknown";
    }
    return "unknown";
}

struct ASTNode {
    int line = 0;
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;

protected:
    static void printIndent(int n) {
        for (int i = 0; i < n; ++i) std::cout << "  ";
    }
};

struct ExprNode : public ASTNode {
    Type exprType = Type::UNKNOWN;
};

struct StmtNode : public ASTNode {};

struct IntLiteralNode : public ExprNode {
    int value;
    IntLiteralNode(int v, int l) : value(v) { line = l; exprType = Type::INT; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "IntLit(" << value << ")\n";
    }
};

struct FloatLiteralNode : public ExprNode {
    double value;
    FloatLiteralNode(double v, int l) : value(v) { line = l; exprType = Type::FLOAT; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "FloatLit(" << value << ")\n";
    }
};

struct IdentifierNode : public ExprNode {
    std::string name;
    IdentifierNode(const std::string& n, int l) : name(n) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Ident(" << name << ")\n";
    }
};

struct UnaryOpNode : public ExprNode {
    std::string op;
    std::unique_ptr<ExprNode> operand;
    UnaryOpNode(const std::string& o, std::unique_ptr<ExprNode> n, int l)
        : op(o), operand(std::move(n)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "UnaryOp(" << op << ")\n";
        operand->print(indent + 1);
    }
};

struct BinaryOpNode : public ExprNode {
    std::string op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;
    BinaryOpNode(const std::string& o, std::unique_ptr<ExprNode> l,
                 std::unique_ptr<ExprNode> r, int ln)
        : op(o), left(std::move(l)), right(std::move(r)) { line = ln; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "BinaryOp(" << op << ")\n";
        left->print(indent + 1);
        right->print(indent + 1);
    }
};

struct CallExprNode : public ExprNode {
    std::string funcName;
    std::vector<std::unique_ptr<ExprNode>> args;
    CallExprNode(const std::string& fn, std::vector<std::unique_ptr<ExprNode>> a, int l)
        : funcName(fn), args(std::move(a)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Call(" << funcName << ")\n";
        for (const auto& arg : args) arg->print(indent + 1);
    }
};

struct VarDeclNode : public StmtNode {
    Type type;
    std::vector<std::string> names;
    VarDeclNode(Type t, std::vector<std::string> n, int l)
        : type(t), names(std::move(n)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "VarDecl(" << typeToString(type) << ": ";
        for (size_t i = 0; i < names.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << names[i];
        }
        std::cout << ")\n";
    }
};

struct AssignNode : public StmtNode {
    std::string name;
    std::unique_ptr<ExprNode> expr;
    AssignNode(const std::string& n, std::unique_ptr<ExprNode> e, int l)
        : name(n), expr(std::move(e)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Assign(" << name << ")\n";
        expr->print(indent + 1);
    }
};

struct BlockNode : public StmtNode {
    std::vector<std::unique_ptr<StmtNode>> stmts;
    BlockNode(std::vector<std::unique_ptr<StmtNode>> s, int l)
        : stmts(std::move(s)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Block\n";
        for (const auto& s : stmts) s->print(indent + 1);
    }
};

struct IfNode : public StmtNode {
    std::unique_ptr<ExprNode>  condition;
    std::unique_ptr<BlockNode> thenBlock;
    std::unique_ptr<BlockNode> elseBlock;
    IfNode(std::unique_ptr<ExprNode> c, std::unique_ptr<BlockNode> t,
           std::unique_ptr<BlockNode> e, int l)
        : condition(std::move(c)), thenBlock(std::move(t)), elseBlock(std::move(e)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent); std::cout << "If\n";
        printIndent(indent + 1); std::cout << "Cond:\n";
        condition->print(indent + 2);
        printIndent(indent + 1); std::cout << "Then:\n";
        thenBlock->print(indent + 2);
        if (elseBlock) {
            printIndent(indent + 1); std::cout << "Else:\n";
            elseBlock->print(indent + 2);
        }
    }
};

struct WhileNode : public StmtNode {
    std::unique_ptr<ExprNode>  condition;
    std::unique_ptr<BlockNode> body;
    WhileNode(std::unique_ptr<ExprNode> c, std::unique_ptr<BlockNode> b, int l)
        : condition(std::move(c)), body(std::move(b)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent); std::cout << "While\n";
        printIndent(indent + 1); std::cout << "Cond:\n";
        condition->print(indent + 2);
        printIndent(indent + 1); std::cout << "Body:\n";
        body->print(indent + 2);
    }
};

struct ReturnNode : public StmtNode {
    std::unique_ptr<ExprNode> expr;
    ReturnNode(std::unique_ptr<ExprNode> e, int l) : expr(std::move(e)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent); std::cout << "Return\n";
        if (expr) expr->print(indent + 1);
    }
};

struct PrintNode : public StmtNode {
    std::unique_ptr<ExprNode> expr;
    PrintNode(std::unique_ptr<ExprNode> e, int l) : expr(std::move(e)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent); std::cout << "Print\n";
        expr->print(indent + 1);
    }
};

struct CallStmtNode : public StmtNode {
    std::unique_ptr<CallExprNode> call;
    explicit CallStmtNode(std::unique_ptr<CallExprNode> c, int l)
        : call(std::move(c)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "CallStmt(" << call->funcName << ")\n";
        for (const auto& arg : call->args) arg->print(indent + 1);
    }
};

struct Param {
    Type        type;
    std::string name;
};

struct FuncDeclNode : public ASTNode {
    Type                       returnType;
    std::string                name;
    std::vector<Param>         params;
    std::unique_ptr<BlockNode> body;
    FuncDeclNode(Type rt, const std::string& n, std::vector<Param> p,
                 std::unique_ptr<BlockNode> b, int l)
        : returnType(rt), name(n), params(std::move(p)), body(std::move(b)) { line = l; }
    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "FuncDecl(" << typeToString(returnType) << " " << name << "(";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << typeToString(params[i].type) << " " << params[i].name;
        }
        std::cout << "))\n";
        body->print(indent + 1);
    }
};

struct ProgramNode : public ASTNode {
    std::vector<std::unique_ptr<VarDeclNode>>  globalVars;
    std::vector<std::unique_ptr<FuncDeclNode>> functions;
    ProgramNode(std::vector<std::unique_ptr<VarDeclNode>>  gv,
                std::vector<std::unique_ptr<FuncDeclNode>> f)
        : globalVars(std::move(gv)), functions(std::move(f)) {}
    void print(int indent = 0) const override {
        printIndent(indent); std::cout << "Program\n";
        for (const auto& g : globalVars) g->print(indent + 1);
        for (const auto& f : functions)  f->print(indent + 1);
    }
};
