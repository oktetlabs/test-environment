%{
#include "te_config.h"
#if HAVE_CONFIG_H
#include <config.h>
#endif

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
%option nounput
%option noyywrap
%option bison-bridge bison-locations
%option prefix="logic_expr_int_"

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
