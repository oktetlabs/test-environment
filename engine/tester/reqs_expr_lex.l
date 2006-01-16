%{
#include "te_config.h"
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "reqs.h"
#include "reqs_expr.h"


#define YY_INPUT(buf, result, max_size) \
    do {                                                    \
        char c = reqs_expr_str[reqs_expr_str_pos++];        \
                                                            \
        result = (c == '\0') ? YY_NULL : (buf[0] = c, 1);   \
    } while (0)

extern const char *reqs_expr_str;
extern int reqs_expr_str_pos;

%}

%option 8bit
%option batch
%option warn nodefault
%option nounput
%option noyywrap
%option bison-bridge bison-locations
%option prefix="reqs_expr_"

%%
[[:space:]]+                    {}
"("                             { return OPEN; }
")"                             { return CLOSE; }
"&"                             { return AND; }
"|"                             { return OR; }
"!"                             { return NOT; }
[-_[:alnum:].]+                 {
                                    yylval->str = strdup(yytext);
                                    return TOKEN;
                                }
.*                              { REJECT; }
%%
