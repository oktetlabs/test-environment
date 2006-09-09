/** @file
 * @brief Testing Results Comparator
 *
 * Generator of two set of tags comparison report in HTML format.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * 
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *  
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_defs.h"
#include "logger_api.h"

#include "trc_diff.h"


static te_bool trc_diff_tests_has_diff(const test_runs *tests);

/**
 * Add key used for specified tags set.
 *
 * @param tags          Set of tags for which the key is used
 * @param key           Key value
 *
 * @retval 0            Success
 * @retval TE_ENOMEM    Memory allocation failure
 */
static int
tad_diff_key_add(trc_tags_entry *tags, const char *key)
{
    trc_diff_key_stats *p;

    for (p = keys_stats.cqh_first;
         (p != (void *)&keys_stats) &&
         (p->tags != tags || strcmp(p->key, key) != 0);
         p = p->links.cqe_next);

    if (p == (void *)&keys_stats)
    {
        p = malloc(sizeof(*p));
        if (p == NULL)
            return TE_ENOMEM;

        p->tags = tags;
        p->key = key;
        p->count = 0;

        CIRCLEQ_INSERT_TAIL(&keys_stats, p, links);
    }

    p->count++;

    return 0;
}

/**
 * Add iterations keys in set of keys with make differencies.
 *
 * @param iter          Test iteration
 *
 * @retval 0            Success
 * @retval TE_ENOMEM    Memory allocation failure
 */
static int
trc_diff_key_add_iter(const test_iter *iter)
{
    trc_tags_entry *tags;
    int             rc;

    for (tags = tags_diff.tqh_first;
         tags != NULL;
         tags = tags->links.tqe_next)
    {
        const char *key = iter->diff_exp[tags->id].key;

        if (key == NULL)
            key = "";

        rc = tad_diff_key_add(tags, key);
        if (rc != 0)
            return rc;
    }
    return 0;
}

static te_bool
trc_diff_exclude_by_key(const test_iter *iter)
{
    tqe_string     *p;
    trc_tags_entry *tags;
    te_bool         exclude = FALSE;

    for (p = trc_diff_exclude_keys.tqh_first;
         p != NULL && !exclude;
         p = p->links.tqe_next)
    {
        for (tags = tags_diff.tqh_first;
             tags != NULL;
             tags = tags->links.tqe_next)
        {
            if (iter->diff_exp[tags->id].key != NULL &&
                strlen(iter->diff_exp[tags->id].key) > 0)
            {
                exclude = (strncmp(iter->diff_exp[tags->id].key,
                                   p->v, strlen(p->v)) == 0);
                if (!exclude)
                    break;
            }
        }
    }
    return exclude;
}


/**
 * Map test result, match and exclude status to statistics index.
 *
 * @param result        Result
 * @param match         Do results match?
 * @param exclude       Does exclude of such differencies requested?
 *
 * @return Index of statistics counter.
 */
static trc_diff_stats_index
trc_diff_result_to_stats_index(trc_test_result result,
                               te_bool match, te_bool exclude)
{
    switch (result)
    {
        case TRC_TEST_PASSED:
            if (match)
                return TRC_DIFF_STATS_PASSED;
            else if (exclude)
                return TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE;
            else
                return TRC_DIFF_STATS_PASSED_DIFF;

        case TRC_TEST_FAILED:
            if (match)
                return TRC_DIFF_STATS_FAILED;
            else if (exclude)
                return TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE;
            else
                return TRC_DIFF_STATS_FAILED_DIFF;

        case TRC_TEST_SKIPPED:
            return TRC_DIFF_STATS_SKIPPED;

        default:
            return TRC_DIFF_STATS_OTHER;
    }
}

/**
 * Update total statistics for sets X and Y based on an iteration data.
 *
 * @param iter          Test iteration
 * @param tags_x        Set X of tags
 * @param tags_y        Set Y of tags
 */
static void
trc_diff_iter_stats(trc_diff_stats       *stats,
                    const test_iter      *iter,
                    const trc_tags_entry *tags_x,
                    const trc_tags_entry *tags_y)
{
    te_bool match;
    te_bool exclude;

    /* 
     * Do nothing if an index of the first set is greater or equal to the
     * index of the second set.
     */
    if (tags_x->id >= tags_y->id)
        return;

    /*
     * Exclude iterations of test packages
     */
    if (iter->tests.head.tqh_first != NULL)
        return;

    match = (iter->diff_exp[tags_x->id].value ==
             iter->diff_exp[tags_y->id].value) &&
            tq_strings_equal(&iter->diff_exp[tags_x->id].verdicts,
                             &iter->diff_exp[tags_y->id].verdicts);

    exclude = !match && trc_diff_exclude_by_key(iter);

    assert(tags_y->id > 0);

    stats[tags_x->id][tags_y->id - 1]
         [trc_diff_result_to_stats_index(iter->diff_exp[tags_x->id].value,
                                         match, exclude)]
         [trc_diff_result_to_stats_index(iter->diff_exp[tags_y->id].value,
                                         match, exclude)]++;
}
                    

/**
 * Do test iterations have different expected results?
 *
 * @param[in]  test         Test
 * @param[out] all_equal    Do @e all iterations have output?
 */
static te_bool
trc_diff_iters_has_diff(test_run *test, te_bool *all_equal)
{
    te_bool         has_diff;
    te_bool         iter_has_diff;
    trc_exp_result *iter_result;
    trc_tags_entry *tags_i;
    trc_tags_entry *tags_j;
    test_iter      *p;

    *all_equal = TRUE;
    for (has_diff = FALSE, p = test->iters.head.tqh_first;
         p != NULL;
         has_diff = has_diff || p->output, p = p->links.tqe_next)
    {
        iter_has_diff = FALSE;
        iter_result = NULL;

        for (tags_i = tags_diff.tqh_first;
             tags_i != NULL;
             tags_i = tags_i->links.tqe_next)
        {
            if (test->diff_exp[tags_i->id] == TRC_TEST_UNSET)
            {
                test->diff_exp[tags_i->id] = p->diff_exp[tags_i->id].value;
                test->diff_verdicts[tags_i->id] =
                    &p->diff_exp[tags_i->id].verdicts;
            }
            else if (test->diff_exp[tags_i->id] == TRC_TEST_MIXED)
            {
                /* Nothing to do */
            }
            else if (test->diff_exp[tags_i->id] !=
                     p->diff_exp[tags_i->id].value ||
                     !tq_strings_equal(test->diff_verdicts[tags_i->id],
                                       &p->diff_exp[tags_i->id].verdicts))
            {
                test->diff_exp[tags_i->id] = TRC_TEST_MIXED;
                test->diff_verdicts[tags_i->id] = NULL;
                *all_equal = FALSE;
            }

            if (iter_result == NULL)
            {
                iter_result = p->diff_exp + tags_i->id;
            }
            else if (iter_has_diff)
            {
                /* Nothing to do */
            }
            else if (iter_result->value != p->diff_exp[tags_i->id].value ||
                     !tq_strings_equal(&iter_result->verdicts,
                                       &p->diff_exp[tags_i->id].verdicts))
            {
                iter_has_diff = TRUE;
            }

            for (tags_j = tags_diff.tqh_first;
                 tags_j != NULL;
                 tags_j = tags_j->links.tqe_next)
            {
                if (!test->aux)
                    trc_diff_iter_stats(p, tags_i, tags_j);
            }
        }

        /* The routine should be called first to be called in any case */
        p->output = trc_diff_tests_has_diff(&p->tests) ||
                    (test->type == TRC_TEST_SCRIPT && iter_has_diff &&
                     !trc_diff_exclude_by_key(p));
        /*<
         * Iteration is output, if its tests have differencies or
         * expected results of the test iteration are different and it
         * shouldn't be excluded because of keys pattern.
         */

        if (p->output && test->type == TRC_TEST_SCRIPT)
        {
            trc_diff_key_add_iter(p);
        }
    }

    return has_diff;
}

/**
 * Do tests in the set have different expected results?
 *
 * @param tests         Set of tests
 */
static te_bool
trc_diff_tests_has_diff(const test_runs *tests)
{
    trc_tags_entry *entry;
    test_run       *p;
    te_bool         has_diff;
    te_bool         all_iters_equal;

    for (has_diff = FALSE, p = tests->head.tqh_first;
         p != NULL;
         has_diff = has_diff || p->diff_out, p = p->links.tqe_next)
    {
        /* Initialize expected result of the test as whole */
        for (entry = tags_diff.tqh_first;
             entry != NULL;
             entry = entry->links.tqe_next)
        {
            p->diff_exp[entry->id] = TRC_TEST_UNSET;
        }

        /* Output the test, if  iteration has differencies. */
        p->diff_out = trc_diff_iters_has_diff(p, &all_iters_equal);

        /**
         * Output test iterations if and only if test should be output
         * itself and:
         *  - set of iterations is empty, or
         *  - all iterations are not equal, or
         *  - it is not leaf of the tests tree.
         */
        p->diff_out_iters = p->diff_out &&
            (p->iters.head.tqh_first == NULL ||
             !all_iters_equal ||
             p->iters.head.tqh_first->tests.head.tqh_first != NULL);
    }

    return has_diff;
}

/**
 * Initialize TRC diff context.
 *
 * @param ctx           Context to be initialized 
 */
void
trc_diff_ctx_init(trc_diff_ctx *ctx)
{
    flags = 0;
    TAILQ_INIT(&ctx->sets);
    TAILQ_INIT(&ctx->exclude_keys);

    memset(&ctx->stats, 0, sizeof(ctx->stats));
    CIRCLEQ_INIT(&ctx->keys_stats);
}

/**
 * Free resources allocated in TRC diff context.
 *
 * @param ctx           Context to be freed
 */
void
trc_diff_ctx_free(trc_diff_ctx *ctx)
{
    trc_diff_free_tags(&trc->sets);
    tq_strings_free(&ctx->exclude_keys, free);
}


/** See descriptino in trc_diff.h */
te_errno
trc_diff_do(trc_diff_ctx *ctx, const te_trc_db *db)
{
    te_bool has_diff;

    /* Preprocess tests tree and gather statistics */
    has_diff = trc_diff_tests_has_diff(&db->tests);

    return 0;
}
