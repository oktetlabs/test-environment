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
#if HAVE_CONFIG_H
#include "config.h"
#endif

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
#include "trc_log.h"
#include "logic_expr.h"


/**
 * Non-recursive free of logical expressions.
 *
 * @param expr      Expression to be freed
 */
static void
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


/**
 * Has the set of strings specified string?
 *
 * @param str       A string
 * @param set       Set of string
 *
 * @return Index in the set (starting from 1) or 0.
 */
static int
is_str_in_set(const char *str, const tqh_string *set)
{
    const tqe_string   *s;
    unsigned int        i;

    assert(str != NULL);

    for (s = (set != NULL) ? set->tqh_first : NULL, i = 1;
         s != NULL;
         s = s->links.tqe_next, i++)
    {
        if (strcmp(str, s->str) == 0)
        {
            return i;
        }
    }
    return 0;
}

/* See the description in logic_expr.h */
int
logic_expr_match(const logic_expr *re, const tqh_string *set)
{
    int result;

    assert(re != NULL);
    switch (re->type)
    {
        case LOGIC_EXPR_VALUE:
            result = is_str_in_set(re->u.value, set);
            VERB("%s(): %s -> %d", __FUNCTION__, re->u.value, result);
            break;

        case LOGIC_EXPR_NOT:
            result = !logic_expr_match(re->u.unary, set);
            VERB("%s(): ! -> %d", __FUNCTION__, result);
            break;

        case LOGIC_EXPR_AND:
        {
            int lhr = logic_expr_match(re->u.binary.lhv, set);
            int rhr = (lhr == 0) ? 0 :
                          logic_expr_match(re->u.binary.rhv, set);

            if (lhr == 0 || rhr == 0)
                result = 0;
            else
                result = MIN(lhr, rhr);
            VERB("%s(): && -> %d", __FUNCTION__, result);
            break;
        }

        case LOGIC_EXPR_OR:
        {
            int lhr = logic_expr_match(re->u.binary.lhv, set);
            int rhr = logic_expr_match(re->u.binary.rhv, set);

            if (lhr == 0)
                result = rhr;
            else if (rhr == 0)
                result = lhr;
            else
                result = MIN(lhr, rhr);
            VERB("%s(): || -> %d", __FUNCTION__, result);
            break;
        }

        default:
            ERROR("Invalid type of requirements expression");
            assert(FALSE);
            result = 0;
            break;
    }
    return result;
}
