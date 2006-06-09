/** @file
 * @brief Tester Subsystem
 *
 * Gramma for requirements expressions specification.
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

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#define TE_LGR_USER     "Requirements Expression Parser"
#include "logger_api.h"

#include "tester_reqs.h"

#include "reqs_expr_gram.h"
#include "reqs_expr_lex.h"


static reqs_expr *reqs_expr_root;

/**
 * Parser error reporting.
 *
 * @param str       Error description string
 */
static void
reqs_expr_error(const char *str)
{
    ERROR("%s", str);
}

%}

%pure-parser
%locations
%name-prefix="reqs_expr_"

%union {
    char       *str;
    reqs_expr  *expr;
}

%right OR
%right AND
%nonassoc NOT

%token OPEN CLOSE
%token <str> TOKEN

%type <expr> expr

%%
expr:
    TOKEN
    {
        reqs_expr *p = calloc(1, sizeof(*p));

        if ((p == NULL) || ($1 == NULL))
        {
            ERROR("%s(): calloc(1, %u) failed", __FUNCTION__, sizeof(*p));
            YYABORT;
        }
        p->type = TESTER_REQS_EXPR_VALUE;
        p->u.value = $1;
        reqs_expr_root = $$ = p;
    }
    | OPEN expr CLOSE
    {
        reqs_expr_root = $$ = $2;
    }
    | NOT expr
    {
        reqs_expr *p = calloc(1, sizeof(*p));

        if (p == NULL)
        {
            ERROR("%s(): calloc(1, %u) failed", __FUNCTION__, sizeof(*p));
            YYABORT;
        }
        p->type = TESTER_REQS_EXPR_NOT;
        p->u.unary = $2;
        reqs_expr_root = $$ = p;
    }
    | expr AND expr
    {
        reqs_expr_root = $$ =
            reqs_expr_binary(TESTER_REQS_EXPR_AND, $1, $3);
        if (reqs_expr_root == NULL)
            YYABORT;
    }
    | expr OR expr
    {
        reqs_expr_root = $$ =
            reqs_expr_binary(TESTER_REQS_EXPR_OR, $1, $3);
        if (reqs_expr_root == NULL)
            YYABORT;
    }
    ;
%%

const char *reqs_expr_str;
int reqs_expr_str_pos;

/* See description in tester_reqs.h */
te_errno
tester_reqs_expr_parse(const char *str, reqs_expr **expr)
{
    reqs_expr_str = str;
    reqs_expr_str_pos = 0;
    reqs_expr_root = NULL;

    if (reqs_expr_parse() != 0)
    {
        ERROR("Failed to parse requirements expression string '%s'",
              str);
        return TE_EINVAL;
    }

    *expr = reqs_expr_root;

    return 0;
}
