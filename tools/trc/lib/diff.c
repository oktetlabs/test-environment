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

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "te_alloc.h"

#include "trc_diff.h"


/**
 * Element of the stack with TRC diff states.
 */
typedef struct trc_diff_state {
    LIST_ENTRY(trc_diff_state)  links;  /**< List links */
    trc_diff_entry     *entry;      /**< Pointer to the entry in
                                         result list */
    te_bool             has_diff;   /**< Have children differences? */
} trc_diff_state;


/**
 * Initialize TRC diff result entry.
 *
 * @param entry         Pointer to an entry to be initialized
 * @param is_iter       Is a test or an iteration?
 */
static void
trc_diff_entry_init(trc_diff_entry *entry, te_bool is_iter)
{
    unsigned int    i;

    assert(entry != NULL);

    entry->is_iter = is_iter;
    if (entry->is_iter)
        entry->ptr.iter = NULL;
    else
        entry->ptr.test = NULL;

    for (i = 0; i < TE_ARRAY_LEN(entry->results); ++i)
        entry->results[i] = NULL;
}

/**
 * Clean up TRC diff result entry.
 *
 * @param entry         Pointer to an entry to clean up 
 */
static void
trc_diff_entry_cleanup(trc_diff_entry *entry)
{
    unsigned int    i;

    assert(entry != NULL);

    if (entry->is_iter)
        entry->ptr.iter = NULL;
    else
        entry->ptr.test = NULL;

    for (i = 0; i < TE_ARRAY_LEN(entry->results); ++i)
        if (!entry->inherit[i])
            entry->results[i] = NULL;
}

/**
 * Inherit requested expected results from parent to entry.
 *
 * @param parent        Parent entry 
 * @param entry         Entry to fill in
 */
static void
trc_diff_entry_inherit(const trc_diff_entry *parent,
                       trc_diff_entry       *entry)
{
    unsigned int    i;

    assert(parent != NULL);
    assert(entry != NULL);

    for (i = 0; i < TE_ARRAY_LEN(entry->results); ++i)
    {
        if (parent->inherit[i])
        {
            entry->results[i] = parent->results[i];
            entry->inherit[i] = parent->inherit[i];
            assert(entry->results[i] != NULL);
        }
    }
}

/**
 * Allocate a new TRC diff result entry and inherit requested results
 * from parent.
 *
 * @param parent        Parent entry 
 *
 * @return Pointer to an allocated entry.
 */
static trc_diff_entry *
trc_diff_entry_new(const trc_diff_entry *parent)
{
    trc_diff_entry *p = TE_ALLOC(sizeof(*p));

    if (p != NULL)
    {
        if (parent == NULL)
        {
            trc_diff_entry_init(p, FALSE);
        }
        else
        {
            trc_diff_entry_init(p, !parent->is_iter);
            trc_diff_entry_inherit(parent, p);
        }
    }

    return p;
}

#if 0
/**
 * Add key used for specified tags set.
 *
 * @param keys_stats    List with all information about keys
 * @param tags          Set of tags for which the key is used
 * @param key           Key value
 *
 * @retval 0            Success
 * @retval TE_ENOMEM    Memory allocation failure
 */
static int
tad_diff_key_add(trc_diff_keys_stats *keys_stats,
                 trc_tags_entry *tags, const char *key)
{
    trc_diff_key_stats *p;

    for (p = keys_stats->cqh_first;
         (p != (void *)keys_stats) &&
         (p->tags != tags || strcmp(p->key, key) != 0);
         p = p->links.cqe_next);

    if (p == (void *)keys_stats)
    {
        p = malloc(sizeof(*p));
        if (p == NULL)
            return TE_ENOMEM;

        p->tags = tags;
        p->key = key;
        p->count = 0;

        CIRCLEQ_INSERT_TAIL(keys_stats, p, links);
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
 * Do test iterations have different expected results?
 *
 * @param[in]  ctx          TRC diff tool context
 * @param[in]  test         Test
 * @param[out] all_equal    Do @e all iterations have output?
 */
static te_bool
trc_diff_iters_has_diff(trc_diff_ctx *ctx, test_run *test,
                        te_bool *all_equal)
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

            if (!test->aux)
            {
                for (tags_j = tags_diff.tqh_first;
                     tags_j != NULL;
                     tags_j = tags_j->links.tqe_next)
                {
                    trc_diff_iter_stats(&ctx->stats, p, tags_i, tags_j);
                }
            }
        }

        /* The routine should be called first to be called in any case */
        p->output = trc_diff_tests_has_diff(ctx, &p->tests) ||
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
 * @param ctx           TRC diff tool context
 * @param tests         Set of tests
 */
static te_bool
trc_diff_tests_has_diff(trc_diff_ctx *ctx, const test_runs *tests)
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
        p->diff_out = trc_diff_iters_has_diff(ctx, p, &all_iters_equal);

        /**
         * Output test iterations if and only if test should be output
         * itself and:
         *  - all iterations are not equal, or
         *  - it is not leaf of the tests tree.
         */
        p->diff_out_iters = p->diff_out &&
            (!all_iters_equal ||
             p->iters.head.tqh_first->tests.head.tqh_first != NULL);
    }

    return has_diff;
}
#endif



/**
 * Get expected results for all sets to compare.
 *
 * @param diffs         Sets to compare
 * @param walker        Current position in DB
 * @param entry         Entry to fill in
 */
static void
trc_diff_entry_exp_results(const trc_diff_tags_list *diffs,
                           const te_trc_db_walker   *walker,
                           trc_diff_entry           *entry)
{
    trc_diff_tags_entry *tags;

    assert(walker != NULL);
    assert(diffs != NULL);
    assert(entry != NULL);

    for (tags = diffs->tqh_first;
         tags != NULL;
         tags = tags->links.tqe_next)
    {
        /* Check if the result is not inherited from parent */
        if (entry->results[tags->id] == NULL)
        {
            entry->results[tags->id] =
                trc_db_walker_get_exp_result(walker, &tags->tags);
            assert(entry->results[tags->id] != NULL);
        }
    }
}

#if 0
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
 * @param diff_i        Set X of tags
 * @param diff_j        Set Y of tags
 */
static void
trc_diff_iter_stats(trc_diff_stats       *stats,
                    const test_iter      *iter,
                    const unsigned int    diff_i,
                    const unsigned int    diff_j)
{
    te_bool match;
    te_bool exclude;

    /* 
     * Do nothing if an index of the first set is greater or equal to the
     * index of the second set.
     */
    if (diff_i >= diff_j)
        return;

    /*
     * Exclude iterations of test packages
     */
    if (iter->tests.head.tqh_first != NULL)
        return;

    match = trc_is_exp_result_equal(iter->results[diff_i],
                                    iter->results[diff_j]);

    exclude = !match && trc_diff_exclude_by_key(iter);

    assert(diff_j > 0);

    stats[diff_i][diff_j - 1]
         [trc_diff_result_to_stats_index(iter->results[diff_i],
                                         match, exclude)]
         [trc_diff_result_to_stats_index(iter->results[diff_j],
                                         match, exclude)]++;
}
#endif

/* See descriptino in trc_diff.h */
te_errno
trc_diff_do(trc_diff_ctx *ctx)
{
    LIST_HEAD(, trc_diff_state) states;

    te_errno                rc;
    te_bool                 start;
    unsigned int            level;
    trc_diff_state         *state;
    te_trc_db_walker       *walker;
    trc_db_walker_motion    motion;
    trc_diff_entry         *parent;
    te_bool                 has_diff;
    te_bool                 hide_children = FALSE; /* Just to make
                                                      compiler happy */
    trc_diff_entry         *entry;
    te_bool                 entry_to_result = FALSE; /* Just to make
                                                        compiler happy */

    if (ctx == NULL || ctx->db == NULL)
        return TE_EINVAL;

    walker = trc_db_new_walker(ctx->db);
    if (walker == NULL)
        return TE_ENOMEM;

    LIST_INIT(&states);

    /* Traverse the tree */
    rc = 0;
    start = TRUE;
    level = 0;
    parent = entry = NULL;
    has_diff = FALSE;
    while ((rc == 0) &&
           ((motion = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT))
    {
        assert(!start || motion == TRC_DB_WALKER_SON);
        switch (motion)
        {
            case TRC_DB_WALKER_SON:
                if (start)
                {
                    start = FALSE;
                }
                else
                {
                    /* 
                     * Save current 'parent' and its 'has_diff' in the
                     * stack.
                     */
                    state = TE_ALLOC(sizeof(*state));
                    if (state == NULL)
                    {
                        rc = TE_ENOMEM;
                        break;
                    }
                    state->entry = parent;
                    state->has_diff = has_diff;
                    LIST_INSERT_HEAD(&states, state, links);

                    /* Current 'entry' is a parent to a new one */
                    assert(entry != NULL);
                    parent = entry;
                    entry = NULL;
                    parent->level = level;
                    TAILQ_INSERT_TAIL(&ctx->result, parent, links);
                    /* Ignore 'entry_to_result' for non-leaf nodes */
                    has_diff = FALSE;
                    /* May be its children are leaves of the tree */
                    hide_children = TRUE;
                    /* Moved to the next level */
                    level++;
                }
                /* Fake 'entry_to_result' condition to force allocation */
                assert(entry == NULL);
                entry_to_result = TRUE;
                /*@fallthrough@*/

            case TRC_DB_WALKER_BROTHER:
                if (entry_to_result)
                {
                    if (entry != NULL)
                    {
                        entry->level = level;
                        TAILQ_INSERT_TAIL(&ctx->result, entry, links);
                    }

                    entry = trc_diff_entry_new(parent);
                    if (entry == NULL)
                    {
                        rc = TE_ENOMEM;
                        break;
                    }
                    entry_to_result = FALSE;
                }
                else
                {
                    trc_diff_entry_cleanup(entry);
                }
                if (entry->is_iter)
                {
                    trc_diff_entry_exp_results(&ctx->sets, walker, entry);
#if 0
                    if (trc_diff_entry_has_diff(entry))
                    {
                        entry_to_result = has_diff = TRUE;
                    }
                    else if ()
                    {
                    }
                    else
                    {
                        hide_children = FALSE;
                    }
#endif
                }
                break;

            case TRC_DB_WALKER_PARENT:
                /* Free extra entry allocated to process children */
                free(entry); entry = NULL;
                if (has_diff)
                {
                    /* Some differences have been discovered */
                    if (hide_children)
                    {
                        /* 
                         * It is allowed to hide all children,
                         * therefore remove them from result.
                         */
                        while ((entry = parent->links.tqe_next) != NULL)
                        {
                            TAILQ_REMOVE(&ctx->result, entry, links);
                            free(entry);
                        }
                    }
                }
                else
                {
                    /* No differences in children */
                    /* Nothing should be added after the parent */
                    assert(parent->links.tqe_next == NULL);
                    /* Remove the parent from result */
                    TAILQ_REMOVE(&ctx->result, parent, links);
                    /* Reuse this parent entry for its brothers */
                    entry = parent;
                }
                
                /* Extract state from the stack and restore */
                state = states.lh_first;
                assert(state != NULL);
                LIST_REMOVE(state, links);
                parent = state->entry;
                has_diff = has_diff || state->has_diff;
                /* Never hide children who have own children */
                hide_children = FALSE;
                free(state);
                /* Previous level */
                assert(level > 0);
                level--;
                break;

            default:
                assert(FALSE);
        }
    }

    if (entry_to_result && entry != NULL)
    {
        entry->level = level;
        TAILQ_INSERT_TAIL(&ctx->result, entry, links);
    }
    else
        free(entry);

    trc_db_free_walker(walker);

    return rc;
}


/* See descriptino in trc_diff.h */
void
trc_diff_ctx_init(trc_diff_ctx *ctx)
{
    ctx->flags = 0;
    ctx->db = NULL;
    TAILQ_INIT(&ctx->sets);
    TAILQ_INIT(&ctx->exclude_keys);

    memset(&ctx->stats, 0, sizeof(ctx->stats));
    CIRCLEQ_INIT(&ctx->keys_stats);
    TAILQ_INIT(&ctx->result);
}

/* See descriptino in trc_diff.h */
void
trc_diff_ctx_free(trc_diff_ctx *ctx)
{
    trc_diff_entry *p;

    trc_diff_free_tags(&ctx->sets);
    tq_strings_free(&ctx->exclude_keys, free);

    while ((p = ctx->result.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&ctx->result, p, links);
        free(p);
    }
}
