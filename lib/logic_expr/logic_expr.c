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
        /* we should have exact lenght match!!! */
        else if (c - s->v == strlen(str))
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
                (endptr - re->u.value) != strlen(re->u.value))
                result = is_str_in_set(re->u.value, set);
            VERB("%s(): %s -> %d", __FUNCTION__, re->u.value, result);
            break;
        }
        case LOGIC_EXPR_NOT:
            result = -logic_expr_match(re->u.unary, set);
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
