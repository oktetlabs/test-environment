/** @file
 * @brief Logical Expression Gramma
 *
 * Gramma for logical expressions specification.
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

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_printf.h"
#include "logger_api.h"

#include "logic_expr.h"
#include "logic_expr_gram.h"
#include "logic_expr_lex.h"


static logic_expr *logic_expr_int_root;

/**
 * Parser error reporting.
 *
 * @param str       Error description string
 */
static void
logic_expr_int_error(const char *str)
{
    ERROR("%s", str);
}
%}

%pure-parser
%locations
%name-prefix="logic_expr_int_"

%union {
    char       *str;
    logic_expr *expr;
}

%right OR
%right AND
%right GT
%right GE
%right LT
%right LE
%nonassoc NOT

%token OPEN CLOSE
%token <str> TOKEN
%token NOMATCH


%type <expr> expr

%%
expr:
    TOKEN
    {
        logic_expr *p = calloc(1, sizeof(*p));

        if ((p == NULL) || ($1 == NULL))
        {
            ERROR("%s(): calloc(1, %"TE_PRINTF_SIZE_T"u) failed", 
                  __FUNCTION__, sizeof(*p));
            return -1;
        }
        p->type = LOGIC_EXPR_VALUE;
        p->u.value = $1;
        logic_expr_int_root = $$ = p;
    }
    | OPEN expr CLOSE
    {
        logic_expr_int_root = $$ = $2;
    }
    | NOT expr
    {
        logic_expr *p = calloc(1, sizeof(*p));

        if (p == NULL)
        {
            ERROR("%s(): calloc(1, %"TE_PRINTF_SIZE_T"u) failed", 
                  __FUNCTION__, sizeof(*p));
            return -1;
        }
        p->type = LOGIC_EXPR_NOT;
        p->u.unary = $2;
        logic_expr_int_root = $$ = p;
    }
    | expr AND expr
    {
        logic_expr_int_root = $$ =
            logic_expr_binary(LOGIC_EXPR_AND, $1, $3);
        if (logic_expr_int_root == NULL)
        {
            return -1;
        }
    }
    | expr OR expr
    {
        logic_expr_int_root = $$ = logic_expr_binary(LOGIC_EXPR_OR, $1, $3);
        if (logic_expr_int_root == NULL)
        {
            return -1;
        }
    }
    | expr GT expr
    {
        logic_expr_int_root = $$ = logic_expr_binary(LOGIC_EXPR_GT, $1, $3);
        if (logic_expr_int_root == NULL)
        {
            return -1;
        }
    }
    | expr GE expr
    {
        logic_expr_int_root = $$ = logic_expr_binary(LOGIC_EXPR_GE, $1, $3);
        if (logic_expr_int_root == NULL)
        {
            return -1;
        }
    }
    | expr LT expr
    {
        logic_expr_int_root = $$ = logic_expr_binary(LOGIC_EXPR_LT, $1, $3);
        if (logic_expr_int_root == NULL)
        {
            return -1;
        }
    }
    | expr LE expr
    {
        logic_expr_int_root = $$ = logic_expr_binary(LOGIC_EXPR_LE, $1, $3);
        if (logic_expr_int_root == NULL)
        {
            return -1;
        }
    }
    ;
%%

const char *logic_expr_int_str;
int logic_expr_int_str_pos;

/* See the description in logic_expr.h */
te_errno
logic_expr_parse(const char *str, logic_expr **expr)
{
    logic_expr_int_str = str;
    logic_expr_int_str_pos = 0;
    logic_expr_int_root = NULL;

    if (logic_expr_int_parse() != 0)
    {
        ERROR("Failed to parse logical expression string '%s'",
              str);
        return TE_EINVAL;
    }

    *expr = logic_expr_int_root;

    return 0;
}
