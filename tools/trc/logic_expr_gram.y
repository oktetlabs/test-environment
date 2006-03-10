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
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "trc_log.h"

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


/**
 * Create binary logical expression.
 *
 * @param type      AND or OR
 * @param lhv       Left hand value
 * @param rhv       Right hand value
 *
 * @return Pointer to allocated value or NULL.
 */
static logic_expr *
logic_expr_binary(logic_expr_type type, logic_expr *lhv, logic_expr *rhv)
{
    logic_expr *p;

    assert(type == LOGIC_EXPR_AND ||
           type == LOGIC_EXPR_OR);
    assert(lhv != NULL);
    assert(rhv != NULL);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("%s(): calloc(1, %u) failed", __FUNCTION__, sizeof(*p));
        return NULL;
    }
    p->type = type;
    p->u.binary.lhv = lhv;
    p->u.binary.rhv = rhv;
    
    return p;
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
%nonassoc NOT

%token OPEN CLOSE
%token <str> TOKEN

%type <expr> expr

%%
expr:
    TOKEN
    {
        logic_expr *p = calloc(1, sizeof(*p));

        if ((p == NULL) || ($1 == NULL))
        {
            ERROR("%s(): calloc(1, %u) failed", __FUNCTION__, sizeof(*p));
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
            ERROR("%s(): calloc(1, %u) failed", __FUNCTION__, sizeof(*p));
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
    ;
%%

const char *logic_expr_int_str;
int logic_expr_int_str_pos;

/* See the description in logic_expr.h */
int
logic_expr_parse(const char *str, logic_expr **expr)
{
    logic_expr_int_str = str;
    logic_expr_int_str_pos = 0;
    logic_expr_int_root = NULL;

    if (logic_expr_int_parse() != 0)
    {
        ERROR("Failed to parse requirements expression string '%s'",
              str);
        return EINVAL;
    }

    *expr = logic_expr_int_root;

    return 0;
}

