%{
#include <cstdio>
#include <cstring>
#include <string>
#include "ast.h"
#include "error_handler.h"
#include "parser.tab.h"  /* generated into src/ by bison -d */

extern FILE* tokenOut;

static void emitToken(const char* kind, const char* lexeme) {
    if (!tokenOut) return;
    std::fprintf(tokenOut, "%-16s  %-24s  line %d\n", kind, lexeme, yylineno);
}
%}

%option yylineno
%option noyywrap
%option nounput
%option noinput

%%

[ \t\r\n]+                   { /* whitespace */ }
"//"[^\n]*                   { /* single-line comment */ }
"/*"([^*]|\*+[^*/])*\*+"/"  { /* block comment */ }

"int"                        { emitToken("INT",    yytext); return INT;    }
"float"                      { emitToken("FLOAT",  yytext); return FLOAT;  }
"void"                       { emitToken("VOID",   yytext); return VOID;   }
"if"                         { emitToken("IF",     yytext); return IF;     }
"else"                       { emitToken("ELSE",   yytext); return ELSE;   }
"while"                      { emitToken("WHILE",  yytext); return WHILE;  }
"return"                     { emitToken("RETURN", yytext); return RETURN; }
"print"                      { emitToken("PRINT",  yytext); return PRINT;  }

[0-9]+\.[0-9]+               {
                                 emitToken("FLOAT_LIT", yytext);
                                 yylval.float_val = std::stod(yytext);
                                 return FLOAT_LITERAL;
                             }
[0-9]+                       {
                                 emitToken("INT_LIT", yytext);
                                 yylval.int_val = std::atoi(yytext);
                                 return INT_LITERAL;
                             }
[a-zA-Z_][a-zA-Z0-9_]*      {
                                 emitToken("ID", yytext);
                                 yylval.str_val = strdup(yytext);
                                 return ID;
                             }

"=="                         { emitToken("EQ",     yytext); return EQ;     }
"!="                         { emitToken("NEQ",    yytext); return NEQ;    }
"<="                         { emitToken("LE",     yytext); return LE;     }
">="                         { emitToken("GE",     yytext); return GE;     }
"<"                          { emitToken("LT",     yytext); return LT;     }
">"                          { emitToken("GT",     yytext); return GT;     }
"="                          { emitToken("ASSIGN", yytext); return ASSIGN; }
"+"                          { emitToken("PLUS",   yytext); return PLUS;   }
"-"                          { emitToken("MINUS",  yytext); return MINUS;  }
"*"                          { emitToken("MUL",    yytext); return MUL;    }
"/"                          { emitToken("DIV",    yytext); return DIV;    }
"^"                          { emitToken("POW",    yytext); return POW;    }
"("                          { emitToken("LPAREN", yytext); return LPAREN; }
")"                          { emitToken("RPAREN", yytext); return RPAREN; }
"{"                          { emitToken("LBRACE", yytext); return LBRACE; }
"}"                          { emitToken("RBRACE", yytext); return RBRACE; }
";"                          { emitToken("SEMI",   yytext); return SEMI;   }
","                          { emitToken("COMMA",  yytext); return COMMA;  }

.                            {
                                 errorHandler.report(ErrorPhase::LEXICAL, yylineno, 0,
                                     std::string("unrecognized character '") + yytext + "'");
                             }

%%
