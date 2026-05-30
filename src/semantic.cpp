#include "semantic.h"

ErrorHandler errorHandler;

Type SemanticAnalyzer::widenType(Type a, Type b) const {
    if (a == Type::FLOAT || b == Type::FLOAT) return Type::FLOAT;
    if (a == Type::INT   && b == Type::INT)   return Type::INT;
    return Type::UNKNOWN;
}

void SemanticAnalyzer::analyze(ProgramNode* node) {
    for (const auto& g : node->globalVars)
        visit(g.get());

    for (const auto& f : node->functions) {
        std::vector<Type> ptypes;
        ptypes.reserve(f->params.size());
        for (const auto& p : f->params) ptypes.push_back(p.type);
        if (!symbolTable.insert(f->name, f->returnType, f->line, true, ptypes))
            errorHandler.report(ErrorPhase::SEMANTIC, f->line, 0,
                                "redefinition of function '" + f->name + "'");
    }

    for (const auto& f : node->functions)
        visit(f.get());
}

void SemanticAnalyzer::visit(FuncDeclNode* node) {
    symbolTable.enterScope();
    currentReturnType = node->returnType;
    for (const auto& p : node->params) {
        if (!symbolTable.insert(p.name, p.type, node->line))
            errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                                "duplicate parameter '" + p.name + "'");
    }
    visit(node->body.get());
    symbolTable.exitScope();
    currentReturnType = Type::UNKNOWN;
}

void SemanticAnalyzer::visit(BlockNode* node) {
    symbolTable.enterScope();
    for (const auto& s : node->stmts) visit(s.get());
    symbolTable.exitScope();
}

void SemanticAnalyzer::visit(StmtNode* node) {
    if (!node) return;
    if (auto v = dynamic_cast<VarDeclNode*>(node)) visit(v);
    else if (auto a  = dynamic_cast<AssignNode*>(node))   visit(a);
    else if (auto b  = dynamic_cast<BlockNode*>(node))    visit(b);
    else if (auto cs = dynamic_cast<CallStmtNode*>(node)) visit(cs);
    else if (auto i  = dynamic_cast<IfNode*>(node))       visit(i);
    else if (auto w  = dynamic_cast<WhileNode*>(node))    visit(w);
    else if (auto r  = dynamic_cast<ReturnNode*>(node))   visit(r);
    else if (auto p  = dynamic_cast<PrintNode*>(node))    visit(p);
}

void SemanticAnalyzer::visit(VarDeclNode* node) {
    if (node->type == Type::VOID)
        errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                            "variable cannot have type void");
    for (const auto& name : node->names) {
        if (!symbolTable.insert(name, node->type, node->line))
            errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                                "redeclaration of '" + name + "' in current scope");
    }
}

void SemanticAnalyzer::visit(AssignNode* node) {
    visit(node->expr.get());
    Symbol* sym = symbolTable.lookup(node->name);
    if (!sym) {
        errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                            "use of undeclared variable '" + node->name + "'");
        return;
    }
    if (sym->isFunction) {
        errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                            "cannot assign to function name '" + node->name + "'");
        return;
    }
    Type rhs = node->expr->exprType;
    if (rhs != Type::UNKNOWN && sym->type != rhs) {
        if (sym->type == Type::INT && rhs == Type::FLOAT) {
            errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                                "narrowing conversion: float to int in assignment to '" +
                                node->name + "'");
        }
        // int -> float is a legal implicit widening; no error.
    }
}

void SemanticAnalyzer::visit(IfNode* node) {
    visit(node->condition.get());
    visit(node->thenBlock.get());
    if (node->elseBlock) visit(node->elseBlock.get());
}

void SemanticAnalyzer::visit(WhileNode* node) {
    visit(node->condition.get());
    visit(node->body.get());
}

void SemanticAnalyzer::visit(ReturnNode* node) {
    if (node->expr) {
        visit(node->expr.get());
        if (currentReturnType == Type::VOID) {
            errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                                "void function must not return a value");
        } else if (node->expr->exprType != Type::UNKNOWN &&
                   node->expr->exprType != currentReturnType) {
            errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                                "return type mismatch: expected " +
                                typeToString(currentReturnType) + ", got " +
                                typeToString(node->expr->exprType));
        }
    } else {
        if (currentReturnType != Type::VOID && currentReturnType != Type::UNKNOWN)
            errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                                "non-void function must return a value");
    }
}

void SemanticAnalyzer::visit(PrintNode* node) {
    visit(node->expr.get());
}

void SemanticAnalyzer::visit(CallStmtNode* node) {
    visit(static_cast<ExprNode*>(node->call.get()));
}

void SemanticAnalyzer::visit(ExprNode* node) {
    if (!node) return;
    if (auto i  = dynamic_cast<IntLiteralNode*>(node))   visit(i);
    else if (auto f  = dynamic_cast<FloatLiteralNode*>(node)) visit(f);
    else if (auto id = dynamic_cast<IdentifierNode*>(node))   visit(id);
    else if (auto u  = dynamic_cast<UnaryOpNode*>(node))      visit(u);
    else if (auto b  = dynamic_cast<BinaryOpNode*>(node))     visit(b);
    else if (auto c  = dynamic_cast<CallExprNode*>(node))     visit(c);
}

void SemanticAnalyzer::visit(IntLiteralNode*   n) { n->exprType = Type::INT;   }
void SemanticAnalyzer::visit(FloatLiteralNode* n) { n->exprType = Type::FLOAT; }

void SemanticAnalyzer::visit(IdentifierNode* node) {
    Symbol* sym = symbolTable.lookup(node->name);
    if (!sym) {
        errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                            "use of undeclared identifier '" + node->name + "'");
        node->exprType = Type::UNKNOWN;
    } else if (sym->isFunction) {
        errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                            "'" + node->name + "' is a function; use call syntax");
        node->exprType = Type::UNKNOWN;
    } else {
        node->exprType = sym->type;
    }
}

void SemanticAnalyzer::visit(UnaryOpNode* node) {
    visit(node->operand.get());
    node->exprType = node->operand->exprType;
}

void SemanticAnalyzer::visit(BinaryOpNode* node) {
    visit(node->left.get());
    visit(node->right.get());

    Type lt = node->left->exprType;
    Type rt = node->right->exprType;

    if (lt == Type::UNKNOWN || rt == Type::UNKNOWN) {
        node->exprType = Type::UNKNOWN;
        return;
    }

    bool isRelational = (node->op == "==" || node->op == "!=" ||
                         node->op == "<"  || node->op == ">"  ||
                         node->op == "<=" || node->op == ">=");

    if (node->op == "^") {
        node->exprType = widenType(lt, rt);
    } else if (isRelational) {
        if (lt != rt && !(lt == Type::INT && rt == Type::FLOAT) &&
                        !(lt == Type::FLOAT && rt == Type::INT)) {
            errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                                "type mismatch in comparison");
        }
        node->exprType = Type::INT;
    } else {
        if (lt != rt) {
            // Implicit widening: int op float -> float
            node->exprType = widenType(lt, rt);
        } else {
            node->exprType = lt;
        }
    }
}

void SemanticAnalyzer::visit(CallExprNode* node) {
    for (const auto& arg : node->args) visit(arg.get());

    Symbol* sym = symbolTable.lookup(node->funcName);
    if (!sym) {
        errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                            "call to undeclared function '" + node->funcName + "'");
        node->exprType = Type::UNKNOWN;
        return;
    }
    if (!sym->isFunction) {
        errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                            "'" + node->funcName + "' is not a function");
        node->exprType = Type::UNKNOWN;
        return;
    }
    if (sym->paramTypes.size() != node->args.size()) {
        errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                            "'" + node->funcName + "' expects " +
                            std::to_string(sym->paramTypes.size()) +
                            " argument(s), got " +
                            std::to_string(node->args.size()));
    } else {
        for (size_t i = 0; i < node->args.size(); ++i) {
            Type at = node->args[i]->exprType;
            Type pt = sym->paramTypes[i];
            if (at != Type::UNKNOWN && at != pt &&
                !(at == Type::INT && pt == Type::FLOAT)) {
                errorHandler.report(ErrorPhase::SEMANTIC, node->line, 0,
                                    "argument " + std::to_string(i + 1) +
                                    " type mismatch in call to '" + node->funcName + "'");
            }
        }
    }
    node->exprType = sym->type;
}
