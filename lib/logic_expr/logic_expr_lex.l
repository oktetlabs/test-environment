/** @file
 * @brief Logical Expressions
 *
 * Lexer for logical expressions specification.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs.
 */
%{
#include "te_config.h"

#include "logic_expr.h"
#include "logic_expr_gram.h"


#define YY_INPUT(buf, result, max_size) \
    do {                                                        \
        char c = logic_expr_int_str[logic_expr_int_str_pos++];  \
                                                                \
        result = (c == '\0') ? YY_NULL : (buf[0] = c, 1);       \
    } while (0)

extern const char *logic_expr_int_str;
extern int logic_expr_int_str_pos;

%}

%option 8bit
%option batch
%option warn nodefault
%option noinput
%option nounput
%option noyywrap
%option bison-bridge
%option prefix="logic_expr_int_"

%%
[[:space:]]+                    {}
"("                             { return OPEN; }
")"                             { return CLOSE; }
"&"                             { return AND; }
"|"                             { return OR; }
">"                             { return GT; }
">="                            { return GE; }
"<"                             { return LT; }
"<="                            { return LE; }
"!="                            { return NEQ; }
"!"                             { return NOT; }
"="                             { return EQ; }
[-_[:alnum:].${}]+              {
                                    yylval->str = strdup(yytext);
                                    return TOKEN;
                                }
"\""                            { return DOUBLE_QUOTE; }
.                               { return NOMATCH; }
%%
