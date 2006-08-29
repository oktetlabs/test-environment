/** @file
 * @brief Logical Expressions
 *
 * Lexer for logical expressions specification.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
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
