%{
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

#include "reqs.h"

#include "reqs_expr.h"
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

%left OR;
%left AND;
%nonassoc NOT;

%token OPEN CLOSE;
%token <str> TOKEN;

%type <expr> expr;

%%
expr:
    TOKEN
    {
        reqs_expr *p = calloc(1, sizeof(*p));

        if ((p == NULL) || ($1 == NULL))
        {
            /* TODO */
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
            /* TODO */
        }
        p->type = TESTER_REQS_EXPR_NOT;
        p->u.unary = $2;
        reqs_expr_root = $$ = p;
    }
    | expr AND expr
    {
        reqs_expr *p = calloc(1, sizeof(*p));

        if (p == NULL)
        {
            /* TODO */
        }
        p->type = TESTER_REQS_EXPR_AND;
        p->u.binary.lhv = $1;
        p->u.binary.rhv = $3;
        reqs_expr_root = $$ = p;
    }
    | expr OR expr
    {
        reqs_expr *p = calloc(1, sizeof(*p));

        if (p == NULL)
        {
            /* TODO */
        }
        p->type = TESTER_REQS_EXPR_OR;
        p->u.binary.lhv = $1;
        p->u.binary.rhv = $3;
        reqs_expr_root = $$ = p;
    }
    ;
%%

const char *reqs_expr_str;
int reqs_expr_str_pos;

/* See description in reqs.h */
int
tester_reqs_expr_parse(const char *str, reqs_expr **expr)
{
    reqs_expr_str = str;
    reqs_expr_str_pos = 0;
    reqs_expr_root = NULL;

    if (reqs_expr_parse() != 0)
    {
        ERROR("Failed to parse requirements expression string '%s'",
              str);
        return EINVAL;
    }

    *expr = reqs_expr_root;

    return 0;
}
