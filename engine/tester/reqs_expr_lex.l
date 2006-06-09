/** @file
 * @brief Tester Subsystem
 *
 * Lexer for requirements expressions specification.
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
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "tester_reqs.h"
#include "reqs_expr_gram.h"


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
