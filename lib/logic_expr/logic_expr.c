/** @file
 * @brief Logical Expressions
 *
 * Match implementation.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "te_string.h"

#include "logic_expr.h"

#if 0
#undef TE_LOG_LEVEL
#define TE_LOG_LEVEL (TE_LL_WARN | TE_LL_ERROR | \
                      TE_LL_VERB | TE_LL_ENTRY_EXIT | TE_LL_RING)
#endif

/* See the description in logic_expr.h */
void
logic_expr_free_nr(logic_expr *expr)
{
    free(expr);
}

/* See the description in logic_expr.h */
void
logic_expr_free(logic_expr *expr)
{
    if (expr == NULL)
        return;

    switch (expr->type)
    {
        case LOGIC_EXPR_VALUE:
            free(expr->u.value);
            break;

        case LOGIC_EXPR_NOT:
            logic_expr_free(expr->u.unary);
            break;

        case LOGIC_EXPR_AND:
        case LOGIC_EXPR_OR:
        case LOGIC_EXPR_GT:
        case LOGIC_EXPR_GE:
        case LOGIC_EXPR_LE:
        case LOGIC_EXPR_LT:
        case LOGIC_EXPR_EQ:
            logic_expr_free(expr->u.binary.lhv);
            logic_expr_free(expr->u.binary.rhv);
            break;

        default:
            ERROR("Invalid type of logical expression");
            assert(0);
            break;
    }
    logic_expr_free_nr(expr);
}

/* See the description in logic_expr.h */
logic_expr *
logic_expr_dup(logic_expr *expr)
{
    logic_expr      *dup = NULL;

    if (expr == NULL)
        return NULL;

    dup = calloc(1, sizeof(logic_expr));
    dup->type = expr->type;

    switch (expr->type)
    {
        case LOGIC_EXPR_VALUE:
            dup->u.value = strdup(expr->u.value); 
            break;

        case LOGIC_EXPR_NOT:
            dup->u.unary = logic_expr_dup(expr->u.unary); 
            break;

        case LOGIC_EXPR_AND:
        case LOGIC_EXPR_OR:
        case LOGIC_EXPR_GT:
        case LOGIC_EXPR_GE:
        case LOGIC_EXPR_LE:
        case LOGIC_EXPR_LT:
        case LOGIC_EXPR_EQ:
            dup->u.binary.lhv = logic_expr_dup(expr->u.binary.lhv);
            dup->u.binary.rhv = logic_expr_dup(expr->u.binary.rhv);
            break;

        default:
            ERROR("Invalid type of logical expression");
            assert(0);
            break;
    }

    return dup;
}

/* See the description in logic_expr.h */
logic_expr *
logic_expr_binary(logic_expr_type type, logic_expr *lhv, logic_expr *rhv)
{
    logic_expr *p;

    assert(type == LOGIC_EXPR_AND ||
           type == LOGIC_EXPR_OR ||
           type == LOGIC_EXPR_GT ||
           type == LOGIC_EXPR_GE ||
           type == LOGIC_EXPR_LT ||
           type == LOGIC_EXPR_EQ ||
           type == LOGIC_EXPR_LE);
    assert(lhv != NULL);
    assert(rhv != NULL);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("%s(): calloc(1, %"TE_PRINTF_SIZE_T"u) failed", 
              __FUNCTION__, sizeof(*p));
        return NULL;
    }
    p->type = type;
    p->u.binary.lhv = lhv;
    p->u.binary.rhv = rhv;
    
    return p;
}


/**
 * Has the set of strings specified string?
 *
 * @param str       A string
 * @param set       Set of string
 *
 * @return Index in the set (starting from 1) or value associated
 *         with string if presented (in form "string:value") or -1
 *         if not found.
 */
static int
is_str_in_set(const char *str, const tqh_strings *set)
{
    const tqe_string   *s;
    unsigned int        i;

    assert(str != NULL);

    for (s = (set != NULL) ? TAILQ_FIRST(set) : NULL, i = 1;
         s != NULL;
         s = TAILQ_NEXT(s, links), i++)
    {
        char *c;

        if ((c = strchr(s->v, ':')) == NULL)
        {
            if (strcmp(str, s->v) == 0)
            {
                return i;
            }
        }
        /* we should have exact lenght match */
        else if (c - s->v == (int) strlen(str))
        {
            if (strncmp(str, s->v, c - s->v) == 0)
                return atoi(c + 1);
        }
    }
    return -1;
}

/* See the description in logic_expr.h */
int
logic_expr_match(const logic_expr *re, const tqh_strings *set)
{
    int result;

    assert(re != NULL);
    switch (re->type)
    {
        case LOGIC_EXPR_VALUE:
        {
            char *endptr;

            result = strtol(re->u.value, &endptr, 10);

            if (re->u.value == endptr ||
                (endptr - re->u.value) != (int) strlen(re->u.value))
                result = is_str_in_set(re->u.value, set);
            VERB("%s(): %s -> %d", __FUNCTION__, re->u.value, result);
            break;
        }
        case LOGIC_EXPR_NOT:
            result = logic_expr_match(re->u.unary, set) >= 0 ? -1 : 0;
            VERB("%s(): ! -> %d", __FUNCTION__, result);
            break;

        case LOGIC_EXPR_AND:
        {
            int lhr = logic_expr_match(re->u.binary.lhv, set);
            int rhr = (lhr == -1) ? -1 :
                          logic_expr_match(re->u.binary.rhv, set);

            if (lhr == -1 || rhr == -1)
                result = -1;
            else
                result = MIN(lhr, rhr);
            VERB("%s(): && -> %d", __FUNCTION__, result);
            break;
        }

        case LOGIC_EXPR_OR:
        {
            int lhr = logic_expr_match(re->u.binary.lhv, set);
            int rhr = logic_expr_match(re->u.binary.rhv, set);

            if (lhr == -1)
                result = rhr;
            else if (rhr == -1)
                result = lhr;
            else
                result = MIN(lhr, rhr);
            VERB("%s(): || -> %d", __FUNCTION__, result);
            break;
        }

        case LOGIC_EXPR_GT:
        {
            int lhr = logic_expr_match(re->u.binary.lhv, set);
            int rhr = logic_expr_match(re->u.binary.rhv, set);

            if (lhr > rhr)
                result = 1;
            else
                result = -1;
            VERB("%s(): %d > %d -> %d", __FUNCTION__,
                 lhr, rhr,
                 result);
            break;
        }

        case LOGIC_EXPR_GE:
        {
            int lhr = logic_expr_match(re->u.binary.lhv, set);
            int rhr = logic_expr_match(re->u.binary.rhv, set);

            if (lhr >= rhr && lhr != -1)
                result = 1;
            else
                result = -1;
            VERB("%s(): %d >= %d -> %d", __FUNCTION__,
                 lhr, rhr,
                 result);
            break;
        }

        case LOGIC_EXPR_LT:
        {
            int lhr = logic_expr_match(re->u.binary.lhv, set);
            int rhr = logic_expr_match(re->u.binary.rhv, set);

            if (lhr < rhr && lhr != -1)
                result = 1;
            else
                result = -1;
            VERB("%s(): %d < %d -> %d", __FUNCTION__,
                 lhr, rhr,
                 result);
            break;
        }
        case LOGIC_EXPR_LE:
        {
            int lhr = logic_expr_match(re->u.binary.lhv, set);
            int rhr = logic_expr_match(re->u.binary.rhv, set);

            if (lhr <= rhr && lhr != -1)
                result = 1;
            else
                result = -1;
            VERB("%s(): %d > %d -> %d", __FUNCTION__,
                 lhr, rhr,
                 result);
            break;
        }
        case LOGIC_EXPR_EQ:
        {
            int lhr = logic_expr_match(re->u.binary.lhv, set);
            int rhr = logic_expr_match(re->u.binary.rhv, set);

            if (lhr == rhr && lhr != -1)
                result = 1;
            else
                result = -1;
            VERB("%s(): %d == %d -> %d", __FUNCTION__,
                 lhr, rhr,
                 result);
            break;
        }


        default:
            ERROR("Invalid type of requirements expression");
            assert(FALSE);
            result = -1;
            break;
    }
    return result;
}

/**
 * Transform logical expression so that ! will not
 * be applied to nodes of type LOGIC_EXPR_OR or
 * LOGIC_EXPR_AND. For example, !(x&y) -> !x|!y.
 *
 * @param expr      Logical expression to be transformed
 *
 * @return 0 on success or error code
 */
static te_errno
logic_expr_not_prop(logic_expr **expr)
{
    logic_expr  *lhv;
    logic_expr  *rhv;
    logic_expr  *aux;

    switch ((*expr)->type)
    {
        case LOGIC_EXPR_NOT:
            if ((*expr)->u.unary->type == LOGIC_EXPR_AND)
                (*expr)->type = LOGIC_EXPR_OR;
            else if ((*expr)->u.unary->type == LOGIC_EXPR_OR)
                (*expr)->type = LOGIC_EXPR_AND;
            else if ((*expr)->u.unary->type == LOGIC_EXPR_NOT)
            {
                aux = (*expr)->u.unary->u.unary;
                logic_expr_free_nr(*expr);
                logic_expr_not_prop(&aux);
                *expr = aux;
            }
            else
                return 0;

            lhv = calloc(1, sizeof(*lhv));
            lhv->type = LOGIC_EXPR_NOT;
            lhv->u.unary = (*expr)->u.unary->u.binary.lhv;
            rhv = calloc(1, sizeof(*rhv));
            rhv->type = LOGIC_EXPR_NOT;
            rhv->u.unary = (*expr)->u.unary->u.binary.rhv;
            logic_expr_free_nr((*expr)->u.unary);

            logic_expr_not_prop(&lhv);
            logic_expr_not_prop(&rhv);
            (*expr)->u.binary.lhv = lhv;
            (*expr)->u.binary.rhv = rhv;
            
            break;

        case LOGIC_EXPR_AND:
        case LOGIC_EXPR_OR:
            logic_expr_not_prop(&(*expr)->u.binary.lhv);
            logic_expr_not_prop(&(*expr)->u.binary.rhv);
            break;

        case LOGIC_EXPR_VALUE:
        case LOGIC_EXPR_GT:
        case LOGIC_EXPR_GE:
        case LOGIC_EXPR_LT:
        case LOGIC_EXPR_LE:
        case LOGIC_EXPR_EQ:
            break;

        default:
            ERROR("Invalid type of requirements expression");
            assert(FALSE);
    }

    return 0;
}

/**
 * Find node of type LOGIC_EXPR_OR in subtree containing nodes
 * of type LOGIC_EXPR_AND and LOGIC_EXPR_OR such that
 * there are only nodes of type LOGIC_EXPR_AND in the path
 * connecting this node with the top of the tree.
 *
 * @param expr      Logical expression (top node of tree)
 * @param parent    Pointer to parent of node found (to be set)
 *
 * @return Pointer to node found or NULL
 */
static logic_expr *
find_or(logic_expr *expr, logic_expr **parent)
{
    logic_expr *result;

    if (expr->type == LOGIC_EXPR_OR)
        return expr;
    if (expr->type != LOGIC_EXPR_AND)
        return NULL;

    *parent = expr;

    result = find_or(expr->u.binary.lhv, parent);
    if (result == NULL)
    {
        *parent = expr;
        result = find_or(expr->u.binary.rhv, parent);
    }

    if (result == NULL)
        *parent = NULL;

    return result;
}

/**
 * Replace a&b&...&x&(y|z) with (a&b&...&x&y)|(a&b&...&x&z).
 *
 * @param and_expr      Node of logical expression tree
 *                      of type LOGIC_EXPR_AND
 * @param or_expr       Node of type LOGIC_EXPR_OR somewhere
 *                      in the subtree of @p and_expr such that
 *                      there are only nodes of type LOGIC_EXPR_AND
 *                      in the path connecting @p and_expr and @p
 *                      or_expr
 * @param or_parent     Parent node of @p or_expr
 *
 * @return 0 on success or error code
 */
static te_errno
and_or_replace(logic_expr **and_expr, logic_expr *or_expr,
               logic_expr *or_parent)
{
    logic_expr **aux;
    logic_expr  *lhv;
    logic_expr  *dup_expr;

    if (or_parent->u.binary.lhv == or_expr)
        aux = &or_parent->u.binary.lhv;
    else
        aux = &or_parent->u.binary.rhv;

    lhv = (*aux)->u.binary.lhv;
    *aux = (*aux)->u.binary.rhv;
    dup_expr = logic_expr_dup(*and_expr);
    
    logic_expr_free(*aux);
    logic_expr_free_nr(or_expr);
    *aux = lhv;

    *and_expr = logic_expr_binary(LOGIC_EXPR_OR,
                                  *and_expr, dup_expr);

    return 0;
}

/**
 * Transform a logical expression subtree containing nodes of type
 * LOGIC_EXPR_AND so that tree will become a list.
 *
 *      &&               &&
 *    /    \            /  \
 *   &&     &&   ->    &&   x
 *  /  \   /  \       /  \
 * x    y z    t     &&   y
 *                  /  \
 *                 z    t
 *
 * @param expr      Logic expression
 *
 * @return 0 on succes
 */
static te_errno
make_and_chain(logic_expr **expr)
{
    logic_expr *lhv;
    logic_expr *rhv;
    logic_expr *l_end;
    logic_expr *end_prev;

    if ((*expr)->type != LOGIC_EXPR_AND)
        return 0;

    lhv = (*expr)->u.binary.lhv;
    rhv = (*expr)->u.binary.rhv;

    make_and_chain(&lhv);
    make_and_chain(&rhv);

    if (lhv->type == LOGIC_EXPR_AND)
    {
        end_prev = lhv;
        l_end = lhv->u.binary.lhv;
        while (l_end->type == LOGIC_EXPR_AND)
        {
            end_prev = l_end;
            l_end = l_end->u.binary.lhv;
        }

        end_prev->u.binary.lhv = rhv;
        (*expr)->u.binary.lhv = lhv;
        (*expr)->u.binary.rhv = l_end;
    }
    else if (rhv->type == LOGIC_EXPR_AND)
    {
        (*expr)->u.binary.lhv = rhv;
        (*expr)->u.binary.rhv = lhv;
    }

    return 0;
}

/**
 * Just assignment operation for logical
 * expressions.
 *
 * @param p     Left side of assignment
 * @param q     Right side of assignment
 *
 * @return 0 on success or error code
 */
static te_errno
expr_eq_expr(logic_expr *p, logic_expr *q)
{
    p->type = q->type;
    if (q->type == LOGIC_EXPR_VALUE)
        p->u.value = q->u.value;
    else if (q->type == LOGIC_EXPR_NOT)
        p->u.unary = q->u.unary;
    else
    {
        p->u.binary.lhv = q->u.binary.lhv;
        p->u.binary.rhv = q->u.binary.rhv;
    }

    return 0;
}

/**
 * Sort logical conjunction in descending order.
 *
 * @param and_chain     Conjunction
 * @param comp_func     Function comparing logical
 *                      expressions
 *
 * @return 0 on succes or error code
 */
static te_errno
sort_and_chain(logic_expr *and_chain,
               int (*comp_func)(logic_expr *,
                                logic_expr *))
{
    logic_expr *p;
    logic_expr *q;
    logic_expr *aux;
    int         c = 0;
    int         rc = 0;

    if (and_chain->type != LOGIC_EXPR_AND)
        return 0;

    /*
     * Just bubble sort - there should not be
     * too much expressions.
     */
    do {
        c = 0;
        p = and_chain;
        q = p->u.binary.lhv;

        while (q != NULL)
        {
            rc = comp_func(p->u.binary.rhv,
                           (q->type == LOGIC_EXPR_AND) ?
                           q->u.binary.rhv : q);

            if (rc == -1)
            {
                aux = p->u.binary.rhv;
                if (q->type == LOGIC_EXPR_AND)
                {
                    p->u.binary.rhv = q->u.binary.rhv;
                    q->u.binary.rhv = aux;
                }
                else
                {
                    p->u.binary.rhv = q;
                    p->u.binary.lhv = aux;
                }
                c++;
            }
            else if (rc == 0)
            {
                /* Delete duplicate */
                if (q->type == LOGIC_EXPR_AND)
                {
                    p->u.binary.lhv = q->u.binary.lhv;
                    logic_expr_free(q->u.binary.rhv);
                }
                else
                {
                    logic_expr_free(p->u.binary.rhv);
                    expr_eq_expr(p, q);
                    logic_expr_free_nr(q);
                }

                q = p;
            }

            p = q;
            q = (p->type == LOGIC_EXPR_AND) ? p->u.binary.lhv :
                                              NULL;
        }
    } while (c > 0);

    return 0;
}

/**
 * Compare two conjunctions and determine whether one 
 * of them can merge another in disjunctive normal form.
 * Conjunctions should be sorted with help of
 * sort_and_chain() in descending order.
 *
 * @param chain1        First conjunction
 * @param chain2        Second conjunction
 * @param comp_func     Function performing comparison
 *                      of logical expressions
 * 
 * @return result of comparison
 *
 * @retval -2           There is not conjunction that can
 *                      merge another one.
 * @retval -1           The first conjunction can merge 
 *                      the second one.
 * @retval 1            The second conjunction can merge
 *                      the first one.
 */
static int
and_chains_cmp(logic_expr *chain1,
               logic_expr *chain2,
               int (*comp_func)(logic_expr *,
                                logic_expr *))
{
    logic_expr *p;
    logic_expr *q;
    logic_expr *p_cur;
    logic_expr *q_cur;
    int         rc = 0;
    te_bool     first_noeq = FALSE;
    te_bool     second_noeq = FALSE;

    p = chain1;
    q = chain2;

    while ((p != NULL && rc != -1) ||
           (q != NULL && rc != 1))
    {
        /* Get next element of the first
         * conjunction unless there is
         * "bigger" current element
         * in the second conjunction
         */
        if (rc != -1 && p != NULL)
        {
            if (p->type == LOGIC_EXPR_AND)
            {
                p_cur = p->u.binary.rhv;
                p = p->u.binary.lhv;
            }
            else
            {
                p_cur = p;
                p = NULL;
            }
        }

        /* Get next element of the second
         * conjunction unless there is
         * "bigger" current element
         * in the first conjunction
         */
        if (rc != 1 && q != NULL)
        {
            if (q->type == LOGIC_EXPR_AND)
            {
                q_cur = q->u.binary.rhv;
                q = q->u.binary.lhv;
            }
            else
            {
                q_cur = q;
                q = NULL;
            }
        }

        rc = comp_func(p_cur, q_cur);

        if (rc != 0)
        {
            /*
             * Our goal is determining which sorted in descending
             * order conjunction is substring of another one if
             * any. If we discover that current element in the
             * first conjunction is "less" than current element
             * in another one - we should pass some elements in
             * the first one in hope that eventually we will find
             * the same element as current element in the second
             * conjunction. If so, we will suppose that the
             * second conjunction is substring of the first one,
             * and will set first_noeq to TRUE. The same
             * considerations are applicable to the opposite case
             * when we should set second_noeq. If these variables
             * are set both it means that both conjunctions
             * have elements not presented in another one and
             * so no one can merge another one.
             */
            if (rc == 1)
                first_noeq = TRUE;
            else
                second_noeq = TRUE;

            if (first_noeq == TRUE && second_noeq == TRUE)
                return -2;
        }
        else if (p == NULL || q == NULL)
            break;
    }

    /* If there are remaining elements in the first
     * conjunction after we passed all elements
     * of the second - the second one is perhaps a
     * substring of the first one. */
    if (p != NULL)
        first_noeq = TRUE;

    /* The opposite case */
    if (q != NULL)
        second_noeq = TRUE;

    /* Comparison of conjunctions ended on
     * different last elements - no one can
     * be substring of another one
     */
    if (p == NULL && q == NULL && rc != 0)
        return -2;

    if (first_noeq == TRUE && second_noeq == TRUE)
        return -2;
    else if (first_noeq == TRUE)
        return 1;
    else
        return -1;
}

/** 
 * Simplify disjunctive normal form deleting duplicating 
 * conjunctions and also merging conjunctions of kind
 * x&y|x in x.
 *
 * @param dnf       Logic expression in disjunctive form
 * @param comp_func Function comparing logical expressions
 *
 * @return 0 on success or error code
 */
static te_errno
logic_expr_dnf_rem_dups(logic_expr **dnf,
                        int (*comp_func)(logic_expr *,
                                         logic_expr *))
{
    logic_expr *p;
    logic_expr *q;
    logic_expr *aux;
    logic_expr *p_m;
    logic_expr *p_cur;
    logic_expr *q_cur;
    int         rc = 0;

    if ((*dnf)->type == LOGIC_EXPR_AND)
    {
        /* Disjunctive form contains just one conjunction */
        sort_and_chain(*dnf, comp_func);
        return 0;
    }
    else if ((*dnf)->type == LOGIC_EXPR_OR)
    {
        p = *dnf;

        while (p->type == LOGIC_EXPR_OR)
        {
            sort_and_chain(p->u.binary.lhv, comp_func);
            p = p->u.binary.rhv;
        }

        if (p->type == LOGIC_EXPR_AND)
            sort_and_chain(p, comp_func);
        
        p = *dnf;

        while (p->type == LOGIC_EXPR_OR)
        {
            p_cur = p->u.binary.lhv;

            p_m = p;
            q = p_m->u.binary.rhv;

            while (q != NULL)
            {
                 if (q->type == LOGIC_EXPR_OR)
                    q_cur = q->u.binary.lhv;
                 else
                    q_cur = q;
                
                rc = and_chains_cmp(p_cur, q_cur, comp_func);
                if (rc == 0 || rc == -1)
                {
                    /* The first conjunction merges the second one */
                    logic_expr_free(q_cur);
                    if (q->type == LOGIC_EXPR_OR)
                    {
                        p_m->u.binary.rhv = q->u.binary.rhv;
                        logic_expr_free_nr(q);
                    }
                    else
                    {
                        aux = p_m->u.binary.lhv;
                        expr_eq_expr(p_m, p_m->u.binary.lhv);
                        logic_expr_free_nr(aux);
                    }
                    q = p_m;
                }
                else if (rc == 1)
                {

                    /* The second conjunction merges the first one */
                    logic_expr_free(p_cur);
                    expr_eq_expr(p, p->u.binary.rhv);
                    break;
                }

                p_m = q;
                q = (p_m->type == LOGIC_EXPR_OR) ?
                     p_m->u.binary.rhv : NULL; 
            }

            p = p->u.binary.rhv; 
        }
    }
    else
        return 0;

    return 0;
}

/**
 * This function just compares string representations
 * of logical expressions.
 *
 * @param p     Logical expression
 * @param q     Logical expression
 *
 * @return Result of strcmp()
 */
static int
logic_expr_cmp_simple(logic_expr *p,
                      logic_expr *q)
{
    char *p_str = logic_expr_to_str(p);
    char *q_str = logic_expr_to_str(q);
    int   rc;

    rc = strcmp(p_str, q_str);

    free(p_str);
    free(q_str);

    return rc;
}

/**
 * Transform logic expression into disjunctive
 * normal form:
 *          ||
 *        /     \
 *      &&       ||
 *     /  \     /  \
 *    &&   x   &&   ||
 *   /  \     ...  /  \
 * ...   y        &&   ...
 *               ...
 *
 * @param expr      Logical expression
 * @param comp_func Function comparing logical expressions
 *
 * @return 0 on success or error code
 */
static te_errno
logic_expr_dnf_gen(logic_expr **expr,
                   int (*comp_func)(logic_expr *,
                                    logic_expr *))
{
    logic_expr   *parent = NULL;
    logic_expr   *or_expr = NULL;
    logic_expr   *lhv;
    /*
     * Transform expression so that logical not
     * is not applied to subexpressions of kind
     * x&y and x|y (i.e. !(x|y) ->
     * !x&!y
     */
    logic_expr_not_prop(expr);
    
    switch ((*expr)->type)
    {
        case LOGIC_EXPR_AND:
            /*
             * Find or-subexpression in a tree and its parent.
             */
            or_expr = find_or(*expr, &parent);

            if (or_expr != NULL)
                /*
                 * Transform the tree so that or-node will be
                 * on the top.
                 */
                and_or_replace(expr, or_expr, parent);
            else
            {
                make_and_chain(expr);
                break;
            }
            /*@fallthrough@*/

        case LOGIC_EXPR_OR:
            /*
             * Remove or-nodes from the left subtree untill there will be
             * no such nodes in the subtree
             */
            while ((or_expr = find_or((*expr)->u.binary.lhv, &parent))
                   != NULL)
            {
                and_or_replace(&(*expr)->u.binary.lhv, or_expr, parent);
                lhv = (*expr)->u.binary.lhv;
                (*expr)->u.binary.lhv = lhv->u.binary.lhv;
                (*expr)->u.binary.rhv = logic_expr_binary(LOGIC_EXPR_OR,
                                            lhv->u.binary.rhv,
                                            (*expr)->u.binary.rhv);
            }

            make_and_chain(&(*expr)->u.binary.lhv);

            /*
             * Left subtree and current node are OK,
             * go to right subtree top and do things
             * again.
             */
            logic_expr_dnf_gen(&(*expr)->u.binary.rhv,
                               comp_func);

            break;

        default:
            break;
    }

    return 0;
}

/* See the description in logic_expr.h */
te_errno
logic_expr_dnf(logic_expr **expr,
               int (*comp_func)(logic_expr *,
                                logic_expr *))
{
    if (comp_func == NULL)
        comp_func = &logic_expr_cmp_simple;

    logic_expr_dnf_gen(expr, comp_func);
    logic_expr_dnf_rem_dups(expr, comp_func);

    /*
     * TODO: simplification !x&a|x&a -> a to be done.
     */
    return 0;
}

/* See the description in logic_expr.h */
te_errno
logic_expr_dnf_split(logic_expr *dnf, logic_expr ***array,
                     int *size)
{
    logic_expr *p;
    int         i;
    int         n;

    if (dnf->type != LOGIC_EXPR_OR)
    {
        *array = calloc(1, sizeof(logic_expr *));
        if (*array == NULL)
        {
            ERROR("%s(): failed to allocate memory",
                  __FUNCTION__);
            return TE_ENOMEM;
        }
        (*array)[0] = logic_expr_dup(dnf);
        *size = 1;
    }
    else
    {
        p = dnf;
        n = 1;

        while (p->type == LOGIC_EXPR_OR)
        {
            n++;
            p = p->u.binary.rhv;
        }

        *array = calloc(n, sizeof(logic_expr *));
        if (*array == NULL)
        {
            ERROR("%s(): failed to allocate memory",
                  __FUNCTION__);
            return TE_ENOMEM;
        }

        i = 0;
        p = dnf;
        while (p->type == LOGIC_EXPR_OR)
        {
            (*array)[i++] = logic_expr_dup(p->u.binary.lhv);
            p = p->u.binary.rhv;
        }

        (*array)[i] = logic_expr_dup(p);
        *size = n;
    }

    return 0;
}

/**
 * Get string representation of logical subexpression.
 *
 * @param expr    Logical expression
 * @param parent  Parent logical expression
 *
 * @return String representation
 */
static char *
logic_expr_to_str_gen(logic_expr *expr, logic_expr *parent)
{
    te_string  result;
    char      *format = NULL;
    char      *l_str = NULL;
    char      *r_str = NULL;

    if (expr == NULL)
        return NULL;

    memset(&result, 0, sizeof(result));
    result.ptr = NULL;
    
    switch(expr->type)
    {
        case LOGIC_EXPR_VALUE:
            te_string_append(&result, "%s", expr->u.value);
            break;

        case LOGIC_EXPR_NOT:
            if (expr->u.unary->type == LOGIC_EXPR_VALUE)
                format = "!%s";
            else
                format = "!(%s)";

            l_str = logic_expr_to_str_gen(expr->u.unary,
                                          expr);
            te_string_append(&result, format, l_str);
            break;

        case LOGIC_EXPR_OR:
            if (parent == NULL || parent->type == LOGIC_EXPR_OR ||
                parent->type == LOGIC_EXPR_NOT)
                format = "%s|%s";
            else
                format = "(%s|%s)";

            l_str = logic_expr_to_str_gen(expr->u.binary.lhv,
                                          expr),
            r_str = logic_expr_to_str_gen(expr->u.binary.rhv,
                                          expr);
            te_string_append(&result, format, l_str, r_str);
            break;

        case LOGIC_EXPR_AND:
            if (parent == NULL || parent->type == LOGIC_EXPR_AND ||
                parent->type == LOGIC_EXPR_NOT)
                format = "%s&%s";
            else
                format = "(%s&%s)";

            l_str = logic_expr_to_str_gen(expr->u.binary.lhv,
                                          expr),
            r_str = logic_expr_to_str_gen(expr->u.binary.rhv,
                                          expr);
            te_string_append(&result, format, l_str, r_str);
            break;

        case LOGIC_EXPR_GT:
            if (format == NULL)
                format = "%s>%s";
        case LOGIC_EXPR_GE:
            if (format == NULL)
                format = "%s>=%s";
        case LOGIC_EXPR_LT:
            if (format == NULL)
                format = "%s<%s";
        case LOGIC_EXPR_LE:
            if (format == NULL)
                format = "%s<=%s";
        case LOGIC_EXPR_EQ:
            if (format == NULL)
                format = "%s=%s";

            l_str = logic_expr_to_str_gen(expr->u.binary.lhv,
                                          expr),
            r_str = logic_expr_to_str_gen(expr->u.binary.rhv,
                                          expr);
            te_string_append(&result, format, l_str, r_str);
            break;

        default:
            ERROR("Invalid type of requirements expression");
            assert(FALSE);
    }

    free(l_str);
    free(r_str);
    return result.ptr;
}

/* See the description in logic_expr.h */
char *
logic_expr_to_str(logic_expr *expr)
{
    return logic_expr_to_str_gen(expr, NULL);
}
