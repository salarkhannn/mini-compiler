/*
 * Hand-written lexer for the infix calculator.
 * Provides yylex() and main() so the Bison-generated parser can call it.
 */
#include <cstdio>
#include <cmath>
#include <cctype>
#include <cstring>
#include <string>
#include "infix.tab.h"


int yylex() {
    int c;
    while ((c = getchar()) == ' ' || c == '\t') {}
    if (c == EOF) return 0;
    if (c == '\n') return '\n';

    if (std::isdigit(c) || c == '.') {
        char buf[64];
        int i = 0;
        buf[i++] = static_cast<char>(c);
        while (std::isdigit(c = getchar()) || c == '.') buf[i++] = static_cast<char>(c);
        ungetc(c, stdin);
        buf[i] = '\0';
        yylval.val = std::stod(buf);
        return NUM;
    }

    if (std::isalpha(c)) {
        char buf[16];
        int i = 0;
        buf[i++] = static_cast<char>(c);
        while (std::isalpha(c = getchar())) buf[i++] = static_cast<char>(c);
        ungetc(c, stdin);
        buf[i] = '\0';
        if (std::strcmp(buf, "log") == 0) return LOG;
        if (std::strcmp(buf, "exp") == 0) return EXP;
        std::fprintf(stderr, "error: unknown function '%s'\n", buf);
        return 0;
    }

    return c;
}

int main() {
    std::printf("Infix Calculator (supports +,-,*,/,^,log(),exp())\n");
    std::printf("Enter expressions one per line. Ctrl-D to quit.\n");
    return yyparse();
}
