/** @file
 * @brief Testing Results Comparator
 *
 * TRC database walker API implementation.
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

#include "te_config.h"

#include "te_errno.h"
#include "te_alloc.h"
#include "logger_api.h"

#include "te_trc.h"
#include "trc_db.h"


/** Internal data of the TRC database walker */
struct te_trc_db_walker {
    te_trc_db      *db;         /**< TRC database pointer */
    te_bool         is_iter;    /**< Is current position an iteration? */
    trc_test       *test;       /**< Test entry */
    trc_test_iter  *iter;       /**< Test iteration */
    unsigned int    unknown;    /**< Unknown depth counter */
};


/* See the description in te_trc.h */
te_trc_db_walker *
trc_db_new_walker(struct te_trc_db *trc_db)
{
    te_trc_db_walker   *walker;

    walker = TE_ALLOC(sizeof(*walker));
    if (walker == NULL)
        return NULL;

    walker->db = trc_db;
    walker->is_iter = TRUE;
    walker->test = NULL;
    walker->iter = NULL;

    return walker;
}

/* See the description in te_trc.h */
te_bool
trc_db_walker_step_test(te_trc_db_walker *walker, const char *test_name)
{
    assert(walker->is_iter);

    if (walker->unknown > 0)
    {
        walker->unknown++;
    }
    else
    {
        trc_tests  *tests = (walker->iter == NULL) ? &walker->db->tests :
                                                     &walker->iter->tests;

        for (walker->test = tests->head.tqh_first;
             walker->test != NULL &&
             strcmp(walker->test->name, test_name) != 0;
             walker->test = walker->test->links.tqe_next);

        if (walker->test == NULL)
            walker->unknown++;
    }

    walker->is_iter = FALSE;

    return (walker->unknown == 0);
}

/**
 * Match TRC database arguments vs arguments specified by caller.
 *
 * @param db_args       List with TRC database arguments
 * @parma n_args        Number of elements in @a names and @a values
 *                      arrays
 * @param names         Array with names of arguments
 * @param values        Array with values of arguments
 *
 * @return Is arguments match?
 */
static te_bool
test_iter_args_match(trc_test_iter_args *db_args,
                     unsigned int        n_args,
                     const char        **names,
                     const char        **values)
{
    uint8_t             match[n_args];
    trc_test_iter_arg  *arg;
    te_bool             arg_match;
    unsigned int        i;

    memset(match, 0, sizeof(match));
    for (arg = db_args->head.tqh_first, arg_match = TRUE;
         arg_match && arg != NULL;
         arg = arg->links.tqe_next)
    {
        arg_match = FALSE;
        for (i = 0; i < n_args; ++i)
        {
            if (match[i] == 0 &&
                strcmp(names[i], arg->name) == 0 &&
                strcmp(values[i], arg->value) == 0)
            {
                match[i] = 1;
                arg_match = TRUE;
                break;
            }
        }
    }
    if (!arg_match || memchr(match, 0, n_args) != NULL)
        return FALSE;
    else
        return TRUE;
}

/* See the description in te_trc.h */
te_bool
trc_db_walker_step_iter(te_trc_db_walker  *walker,
                        unsigned int       n_args,
                        const char       **names,
                        const char       **values)
{
    assert(!walker->is_iter);

    if (walker->unknown > 0)
    {
        walker->unknown++;
    }
    else
    {
        for (walker->iter = walker->test->iters.head.tqh_first;
             walker->iter != NULL &&
             !test_iter_args_match(&walker->iter->args,
                                   n_args, names, values);
             walker->iter = walker->iter->links.tqe_next);

        if (walker->iter == NULL)
            walker->unknown++;
    }

    walker->is_iter = TRUE;

    return (walker->unknown == 0);
}

/* See the description in te_trc.h */
void
trc_db_walker_step_back(te_trc_db_walker *walker)
{
    if (walker->unknown > 0)
    {
        walker->unknown--;
    }
    else if (walker->is_iter)
    {
        assert(walker->iter != NULL);
        walker->test = walker->iter->parent;
        walker->is_iter = FALSE;
    }
    else
    {
        assert(walker->test != NULL);
        walker->iter = walker->test->parent;
        walker->is_iter = TRUE;
    }
}

/* See the description in te_trc.h */
const trc_exp_result *
tester_db_walker_get_exp_result(te_trc_db_walker *walker)
{
    assert(walker->is_iter);

    if (walker->unknown > 0)
    {
        /* Test iteration is unknown. No expected result. */
        return NULL;
    }

    return NULL;
}
