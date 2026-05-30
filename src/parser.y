%{
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include "ast.h"
#include "error_handler.h"

extern int  yylex();
extern int  yylineno;
extern char* yytext;
void yyerror(const char* s);

ProgramNode* rootAST = nullptr;

struct ProgramParts {
    std::vector<std::unique_ptr<VarDeclNode>>  globals;
    std::vector<std::unique_ptr<FuncDeclNode>> funcs;
};
%}

%code requires {
    #include "ast.h"
    #include <memory>
    #include <vector>
    struct ProgramParts;
}

%union {
    int    int_val;
    double float_val;
    char*  str_val;

    ProgramNode*    prog_node;
    FuncDeclNode*   func_node;
    VarDeclNode*    var_node;
    StmtNode*       stmt_node;
    ExprNode*       expr_node;
    BlockNode*      block_node;
    ProgramParts*   parts;

    std::vector<std::unique_ptr<StmtNode>>* stmt_list;
    std::vector<std::unique_ptr<ExprNode>>* expr_list;
    std::vector<Param>*                     param_list;
    std::vector<std::string>*               id_list;
    Param*                                  param;
    Type                                    type;
}

%token <int_val>   INT_LITERAL
%token <float_val> FLOAT_LITERAL
%token <str_val>   ID
%token INT FLOAT VOID IF ELSE WHILE RETURN PRINT
%token PLUS MINUS MUL DIV POW ASSIGN
%token EQ NEQ LT GT LE GE
%token LPAREN RPAREN LBRACE RBRACE SEMI COMMA

%type <prog_node>   program
%type <parts>       external_list
%type <var_node>    var_decl
%type <id_list>     id_list
%type <func_node>   func_decl
%type <type>        type_spec
%type <param_list>  params param_list
%type <param>       param
%type <block_node>  block
%type <stmt_list>   stmts
%type <stmt_node>   stmt
%type <expr_node>   expr cmp_expr add_expr mul_expr pow_expr unary_expr primary
%type <expr_list>   args arg_list

%start program
%define parse.error verbose

%%

program
    : external_list {
        rootAST = new ProgramNode(std::move($1->globals), std::move($1->funcs));
        delete $1;
        $$ = rootAST;
    }
    ;

external_list
    : /* empty */ { $$ = new ProgramParts(); }
    | external_list var_decl {
        $1->globals.push_back(std::unique_ptr<VarDeclNode>($2));
        $$ = $1;
    }
    | external_list func_decl {
        $1->funcs.push_back(std::unique_ptr<FuncDeclNode>($2));
        $$ = $1;
    }
    ;

type_spec
    : INT   { $$ = Type::INT;   }
    | FLOAT { $$ = Type::FLOAT; }
    | VOID  { $$ = Type::VOID;  }
    ;

var_decl
    : type_spec id_list SEMI {
        $$ = new VarDeclNode($1, std::move(*$2), yylineno);
        delete $2;
    }
    ;

id_list
    : ID {
        $$ = new std::vector<std::string>();
        $$->push_back(std::string($1));
        free($1);
    }
    | id_list COMMA ID {
        $1->push_back(std::string($3));
        free($3);
        $$ = $1;
    }
    ;

func_decl
    : type_spec ID LPAREN params RPAREN block {
        $$ = new FuncDeclNode($1, std::string($2), std::move(*$4),
                              std::unique_ptr<BlockNode>($6), yylineno);
        free($2);
        delete $4;
    }
    ;

params
    : /* empty */ { $$ = new std::vector<Param>(); }
    | param_list  { $$ = $1; }
    ;

param_list
    : param {
        $$ = new std::vector<Param>();
        $$->push_back(*$1);
        delete $1;
    }
    | param_list COMMA param {
        $1->push_back(*$3);
        delete $3;
        $$ = $1;
    }
    ;

param
    : type_spec ID {
        $$ = new Param{$1, std::string($2)};
        free($2);
    }
    ;

block
    : LBRACE stmts RBRACE {
        $$ = new BlockNode(std::move(*$2), yylineno);
        delete $2;
    }
    ;

stmts
    : /* empty */ { $$ = new std::vector<std::unique_ptr<StmtNode>>(); }
    | stmts stmt  {
        if ($2) $1->push_back(std::unique_ptr<StmtNode>($2));
        $$ = $1;
    }
    ;

stmt
    : var_decl { $$ = $1; }
    | ID ASSIGN expr SEMI {
        $$ = new AssignNode(std::string($1), std::unique_ptr<ExprNode>($3), yylineno);
        free($1);
    }
    | ID LPAREN args RPAREN SEMI {
        auto call = new CallExprNode(std::string($1), std::move(*$3), yylineno);
        free($1);
        delete $3;
        $$ = new CallStmtNode(std::unique_ptr<CallExprNode>(call), yylineno);
    }
    | IF LPAREN expr RPAREN block {
        $$ = new IfNode(std::unique_ptr<ExprNode>($3),
                        std::unique_ptr<BlockNode>($5), nullptr, yylineno);
    }
    | IF LPAREN expr RPAREN block ELSE block {
        $$ = new IfNode(std::unique_ptr<ExprNode>($3),
                        std::unique_ptr<BlockNode>($5),
                        std::unique_ptr<BlockNode>($7), yylineno);
    }
    | WHILE LPAREN expr RPAREN block {
        $$ = new WhileNode(std::unique_ptr<ExprNode>($3),
                           std::unique_ptr<BlockNode>($5), yylineno);
    }
    | RETURN expr SEMI {
        $$ = new ReturnNode(std::unique_ptr<ExprNode>($2), yylineno);
    }
    | RETURN SEMI {
        $$ = new ReturnNode(nullptr, yylineno);
    }
    | PRINT expr SEMI {
        $$ = new PrintNode(std::unique_ptr<ExprNode>($2), yylineno);
    }
    | block { $$ = $1; }
    | error SEMI { yyerrok; $$ = nullptr; }
    ;

/* Operator precedence is encoded in the grammar hierarchy rather than
   relying solely on %left/%right declarations, making it explicit and
   easy to verify correctness by inspection. */

expr
    : cmp_expr { $$ = $1; }
    | expr EQ  cmp_expr { $$ = new BinaryOpNode("==", std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    | expr NEQ cmp_expr { $$ = new BinaryOpNode("!=", std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    ;

cmp_expr
    : add_expr { $$ = $1; }
    | cmp_expr LT add_expr { $$ = new BinaryOpNode("<",  std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    | cmp_expr GT add_expr { $$ = new BinaryOpNode(">",  std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    | cmp_expr LE add_expr { $$ = new BinaryOpNode("<=", std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    | cmp_expr GE add_expr { $$ = new BinaryOpNode(">=", std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    ;

add_expr
    : mul_expr { $$ = $1; }
    | add_expr PLUS  mul_expr { $$ = new BinaryOpNode("+", std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    | add_expr MINUS mul_expr { $$ = new BinaryOpNode("-", std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    ;

mul_expr
    : pow_expr { $$ = $1; }
    | mul_expr MUL pow_expr { $$ = new BinaryOpNode("*", std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    | mul_expr DIV pow_expr { $$ = new BinaryOpNode("/", std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    ;

/* Right-associative exponentiation: a^b^c = a^(b^c) */
pow_expr
    : unary_expr { $$ = $1; }
    | unary_expr POW pow_expr { $$ = new BinaryOpNode("^", std::unique_ptr<ExprNode>($1), std::unique_ptr<ExprNode>($3), yylineno); }
    ;

unary_expr
    : primary { $$ = $1; }
    | MINUS primary { $$ = new UnaryOpNode("-", std::unique_ptr<ExprNode>($2), yylineno); }
    ;

primary
    : INT_LITERAL   { $$ = new IntLiteralNode($1, yylineno); }
    | FLOAT_LITERAL { $$ = new FloatLiteralNode($1, yylineno); }
    | ID {
        $$ = new IdentifierNode(std::string($1), yylineno);
        free($1);
    }
    | ID LPAREN args RPAREN {
        $$ = new CallExprNode(std::string($1), std::move(*$3), yylineno);
        free($1);
        delete $3;
    }
    | LPAREN expr RPAREN { $$ = $2; }
    ;

args
    : /* empty */ { $$ = new std::vector<std::unique_ptr<ExprNode>>(); }
    | arg_list    { $$ = $1; }
    ;

arg_list
    : expr {
        $$ = new std::vector<std::unique_ptr<ExprNode>>();
        $$->push_back(std::unique_ptr<ExprNode>($1));
    }
    | arg_list COMMA expr {
        $1->push_back(std::unique_ptr<ExprNode>($3));
        $$ = $1;
    }
    ;

%%

void yyerror(const char* s) {
    errorHandler.report(ErrorPhase::SYNTAX, yylineno, 0, s);
}
