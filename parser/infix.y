%{
/*
 * Infix expression evaluator with exponentiation (^), log(), exp().
 * Demonstrates: operator precedence, right-associative ^, math functions.
 * This is a self-contained demo; it does not link into the main pipeline.
 */
#include <cstdio>
#include <cmath>
#include <cstring>

int  yylex();
void yyerror(const char* s);
%}

%union { double val; }

%token <val> NUM
%token LOG EXP

%type <val> expr term factor base

%left  '+' '-'
%left  '*' '/'
%right POW

%start input
%%

input
    : /* empty */
    | input line
    ;

line
    : expr '\n' { printf("  = %.10g\n\n", $1); }
    | '\n'
    | error '\n' { yyerrok; }
    ;

expr
    : expr '+' term { $$ = $1 + $3; }
    | expr '-' term { $$ = $1 - $3; }
    | term          { $$ = $1; }
    ;

term
    : term '*' factor { $$ = $1 * $3; }
    | term '/' factor { $$ = ($3 == 0.0) ? (yyerror("division by zero"), 0.0) : $1 / $3; }
    | factor          { $$ = $1; }
    ;

/* Right-associative exponentiation via recursive rule. */
factor
    : base '^' factor { $$ = pow($1, $3); }
    | base            { $$ = $1; }
    ;

base
    : '(' expr ')'    { $$ = $2; }
    | LOG '(' expr ')' { $$ = log($3); }
    | EXP '(' expr ')' { $$ = exp($3); }
    | '-' base         { $$ = -$2; }
    | NUM              { $$ = $1; }
    ;

%%

void yyerror(const char* s) { fprintf(stderr, "error: %s\n", s); }
