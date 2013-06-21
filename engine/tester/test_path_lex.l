/** @file
 * @brief Tester Subsystem
 *
 * Lexer for test path specification.
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
#define TE_LGR_USER     "Test Path Lexer"
    
#include "te_config.h"
#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#include "logger_api.h"

#include "test_path.h"
#include "test_path_gram.h"
#if 0 /* Inclusion of the header leads to compilation errors */
#include "test_path_lex.h"
#endif


#define MAX_STR_CONST   512

/**
 * Override flex default fatal error action (exit).
 *
 * @param msg   Message to be reported
 */
#define YY_FATAL_ERROR(msg) \
    do {                                \
        ERROR("Flex error: %s", msg);   \
        yyterminate();                  \
        /* Unreachable */               \
        yy_fatal_error(msg);            \
    } while (0)

/**
 * Method to get input symbols.
 *
 * @param buf           Buffer to put input symbol
 * @param result        Result
 * @param max_size      Size of the buffer @a buf
 */
#define YY_INPUT(buf, result, max_size) \
    do {                                                    \
        char c = test_path_str[test_path_str_pos++];        \
                                                            \
        result = (c == '\0') ? YY_NULL : (buf[0] = c, 1);   \
    } while (0)


extern const char *test_path_str;
extern int test_path_str_pos;

%}

%option 8bit
%option batch
%option warn nodefault
%option nounput
%option noyywrap
%option bison-bridge bison-locations
%option prefix="test_path_"

%x STR
%x NUM 

%%

                        char string_buf[MAX_STR_CONST];
                        char *string_buf_ptr = NULL;

[[:space:]]+            {}
"/"                     { return SLASH; }
":"                     { return COLON; }
"="                     { return EQUAL; }
"~="                    { return GLOB; }
","                     { return COMMA; }
"{"                     { return OPEN; }
"}"                     { return CLOSE; }
"*"                     { BEGIN(NUM); return ITERATE; }
"%"                     { BEGIN(NUM); return SELECT; }
"+"                     { BEGIN(NUM); return STEP; }
<NUM>[[:digit:]]+       {
                            unsigned long  tmp;
                            char          *end;

                            BEGIN(INITIAL);
                            yylval->num = tmp = strtoul(yytext, &end, 10);
                            if (tmp == ULONG_MAX || *end != '\0')
                            {
                                ERROR("Invalid or too long number in "
                                      "string '%s'", yytext);
                                REJECT;
                            }
                            else if (yylval->num != tmp)
                            {
                                ERROR("Too big number specified in '%s'",
                                      yytext);
                                REJECT;
                            }
                            else
                            {
                                return NUMBER;
                            }
                        }
[-_[:alnum:].]+         {
                            yylval->str = strdup(yytext);
                            if (yylval->str == NULL)
                            {
                                ERROR("strdup(%s) failed", yytext);
                                REJECT;
                            }
                            return STRING;
                        }

\"                      {
                            string_buf_ptr = string_buf;
                            BEGIN(STR);
                        }
<STR>\"                 { 
                            BEGIN(INITIAL);
                            *string_buf_ptr = '\0';
                            yylval->str = strdup(string_buf);
                            if (yylval->str == NULL)
                            {
                                ERROR("strdup(%s) failed", string_buf);
                                REJECT;
                            }
                            return STRING;
                        }
<STR>\\.                {
                            *string_buf_ptr++ = yytext[1];
                            if (string_buf_ptr - string_buf ==
                                    MAX_STR_CONST)
                            {
                                ERROR("Too long quoted string");
                                REJECT;
                            }
                        }
<STR>.                  {
                            *string_buf_ptr++ = yytext[0];
                            if (string_buf_ptr - string_buf ==
                                    MAX_STR_CONST)
                            {
                                ERROR("Too long quoted string");
                                REJECT;
                            }
                        }

.*                      { REJECT; }
%%
