%{
#include <cstdio>
#include <cmath>
#include <cctype>
#include <cstring>
#include "infix.tab.h"

int yylex() {
    int c;
    while ((c = getchar()) == ' ' || c == '\t') {}

    if (c == EOF) return 0;
    if (c == '\n') return '\n';

    // Number: integer or float.
    if (std::isdigit(c) || c == '.') {
        char buf[64];
        int i = 0;
        buf[i++] = c;
        while (std::isdigit(c = getchar()) || c == '.') buf[i++] = c;
        ungetc(c, stdin);
        buf[i] = '\0';
        yylval.val = std::stod(buf);
        return NUM;
    }

    // Keywords: log, exp.
    if (std::isalpha(c)) {
        char buf[16];
        int i = 0;
        buf[i++] = c;
        while (std::isalpha(c = getchar())) buf[i++] = c;
        ungetc(c, stdin);
        buf[i] = '\0';
        if (std::strcmp(buf, "log") == 0) return LOG;
        if (std::strcmp(buf, "exp") == 0) return EXP;
        fprintf(stderr, "error: unknown function '%s'\n", buf);
        return 0;
    }

    return c;
}
%}
