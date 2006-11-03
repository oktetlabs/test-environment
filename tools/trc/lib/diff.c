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
    SLIST_ENTRY(trc_diff_state) links;  /**< List links */
    trc_diff_entry     *entry;      /**< Pointer to the entry in
                                         result list */
    te_bool             has_diff;   /**< Have children differences? */
    unsigned int        children;   /**< Number of children */
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
    {
        entry->results[i] = NULL;
        entry->inherit[i] = 0;
        TAILQ_INIT(entry->keys + i);
    }
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
    {
        /* If result is not inherited, clean up */
        if (~entry->inherit[i] & TRC_DIFF_INHERITED)
        {
            entry->results[i] = NULL;
            entry->inherit[i] &= ~TRC_DIFF_INHERIT;
        }
        assert(TAILQ_EMPTY(entry->keys + i));
    }
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
        if (parent->inherit[i] & TRC_DIFF_INHERIT)
        {
            entry->results[i] = parent->results[i];
            entry->inherit[i] = parent->inherit[i] | TRC_DIFF_INHERITED;
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


/**
 * Get expected results for all sets to compare.
 *
 * @param sets          Sets to compare
 * @param walker        Current position in DB
 * @param entry         Entry to fill in
 */
static void
trc_diff_entry_exp_results(const trc_diff_sets    *sets,
                           const te_trc_db_walker *walker,
                           trc_diff_entry         *entry)
{
    trc_diff_set *set;

    assert(walker != NULL);
    assert(sets != NULL);
    assert(entry != NULL);

    TAILQ_FOREACH(set, sets, links)
    {
        /* Check if the result is not inherited from parent */
        if (entry->results[set->id] == NULL)
        {
            entry->results[set->id] =
                trc_db_walker_get_exp_result(walker, &set->tags);
            assert(entry->results[set->id] != NULL);
            if (trc_is_exp_result_skipped(entry->results[set->id]))
            {
                /* Skipped results should be inherited */
                entry->inherit[set->id] = TRC_DIFF_INHERIT;
            }
        }
    }
}

/**
 * Analogue of strcmp() with posibility to compare NULL strings.
 * If both strings are NULL, 0 is returned.
 * If only one stirng is NULL, -1 or 1 is returned correspondingly.
 */
static int
str_or_null_cmp(const char *s1, const char *s2)
{
    if (s1 == NULL && s2 == NULL)
        return 0;
    if (s1 == NULL)
        return 1;
    if (s2 == NULL)
        return -1;

    return strcmp(s1, s2);
}

/**
 * Are two expected results equal (including keys and notes)?
 *
 * @param lhv           Left hand value
 * @param rhv           Right hand value
 */
te_bool
trc_diff_is_exp_result_equal(const trc_exp_result *lhv,
                             const trc_exp_result *rhv)
{
    const trc_exp_result_entry *p;
    const trc_exp_result_entry *q;

    assert(lhv != NULL);
    assert(rhv != NULL);

    if (lhv == rhv)
        return TRUE;

    if ((str_or_null_cmp(lhv->key, rhv->key) != 0) ||
        (str_or_null_cmp(lhv->notes, rhv->notes) != 0))
        return FALSE;

    /* 
     * Check that each entry in left-hand value has equal entry in
     * right-hand value.
     */
    TAILQ_FOREACH(p, &lhv->results, links)
    {
        q = trc_is_result_expected(rhv, &p->result);
        if ((q == NULL) ||
            (str_or_null_cmp(p->key, q->key) != 0) ||
            (str_or_null_cmp(p->notes, q->notes) != 0))
        {
            /* 
             * The expected result entry does not correspond to any
             * in another expected result or key/notes do not match.
             */
            return FALSE;
        }
    }

    /* 
     * Check that each entry in right-hand value has equal entry in
     * left-hand value.
     */
    TAILQ_FOREACH(p, &rhv->results, links)
    {
        q = trc_is_result_expected(lhv, &p->result);
        if ((q == NULL) ||
            (str_or_null_cmp(p->key, q->key) != 0) ||
            (str_or_null_cmp(p->notes, q->notes) != 0))
        {
            /* 
             * The expected result entry does not correspond to any
             * in another expected result or key/notes do not match.
             */
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * Derive group result from its items.
 *
 * @param sets          Compared sets
 * @param group         Group entry
 * @param item          Item of the group entry
 * @param init          Initialize unset or not
 *
 * @return Is group homogeneous?
 */
static te_bool
trc_diff_group_exp_result(const trc_diff_sets  *sets,
                          trc_diff_entry       *group,
                          const trc_diff_entry *item,
                          te_bool               init)
{
    const trc_diff_set *p;
    te_bool             all_equal = TRUE;

    assert(sets != NULL);
    assert(group != NULL);
    assert(item != NULL);

    TAILQ_FOREACH(p, sets, links)
    {
        /* Item result may be NULL if it is a group itself */
        if (item->results[p->id] == NULL)
        {
            group->results[p->id] = NULL;
        }
        else if (group->results[p->id] != NULL)
        {
            if (!trc_diff_is_exp_result_equal(item->results[p->id],
                                              group->results[p->id]))
            {
                group->results[p->id] = NULL;
            }
        }
        else if (init)
        {
            group->results[p->id] = item->results[p->id];
        }
        all_equal = all_equal && (group->results[p->id] != NULL);
    }
    return all_equal;
}

/**
 * Increment statistics for the key.
 *
 * @param keys_stats    List with per-key statistics
 * @param key           Key
 *
 * @retval 0            Success
 * @retval TE_ENOMEM    Memory allocation failure
 */
static te_errno
tad_diff_key_stat_inc(trc_diff_keys_stats *keys_stats, const char *key)
{
    trc_diff_key_stats *p;

    assert(keys_stats != NULL);

    if (key == NULL)
        key = "";

    for (p = keys_stats->cqh_first;
         (p != (void *)keys_stats) && (strcmp(p->key, key) != 0);
         p = p->links.cqe_next);

    if (p == (void *)keys_stats)
    {
        p = TE_ALLOC(sizeof(*p));
        if (p == NULL)
            return TE_ENOMEM;

        p->key = key;
        p->count = 0;

        CIRCLEQ_INSERT_TAIL(keys_stats, p, links);
    }

    p->count++;

    return 0;
}

/**
 * Check key of the found difference against patterns to ignore.
 *
 * @param set           Set which participate in comparison
 * @param key           Key which explains the difference
 *
 * @return Should the difference be ignored?
 */
static te_bool
trc_diff_check_key(trc_diff_set *set, const char *key)
{
    tqe_string     *p;
    te_bool         ignore = FALSE;

    if (key == NULL)
        key = "";
 
    for (p = TAILQ_FIRST(&set->ignore);
         p != NULL && !ignore;
         p = TAILQ_NEXT(p, links))
    {
        assert(p->v != NULL);
        ignore = (strncmp(key, p->v, strlen(p->v)) == 0);
        VERB("%s(): key=%s vs ignore=%s -> %u\n",
             __FUNCTION__, key, p->v, ignore);
    }

    return ignore;
}

/**
 * Map TE test status to TRC test status.
 *
 * @param status        TE test status
 *
 * @return TRC test status.
 */
static trc_test_status
test_status_te2trc(const te_test_status status)
{
    switch (status)
    {
        case TE_TEST_PASSED:    return TRC_TEST_PASSED;
        case TE_TEST_FAILED:    return TRC_TEST_FAILED;
        case TE_TEST_SKIPPED:   return TRC_TEST_SKIPPED;
        case TE_TEST_UNSPEC:    return TRC_TEST_UNSPECIFIED;

        default:
            assert(FALSE);
            return TRC_TEST_STATUS_MAX;
    }
}

/**
 * Merge two TRC test statuses.
 */
static trc_test_status
trc_test_status_merge(trc_test_status result, trc_test_status add)
{
    /* 
     * In initial state accumulated result is equal to
     * TRC_TEST_STATUS_MAX.
     */
    assert(result <= TRC_TEST_STATUS_MAX);
    /* 
     * It is stange to have unspecified result together
     * with any other entry.
     */
    assert(result != TRC_TEST_UNSPECIFIED);
    /*
     * Added result has to be valid and can't be unstable.
     */
    assert(add < TRC_TEST_STATUS_MAX);
    assert(add != TRC_TEST_UNSTABLE);

    if (result == add)
    {
        /* The most common case */
        return result;
    }
    else if (result == TRC_TEST_STATUS_MAX)
    {
        /* The first result, initialize accumulated result */
        return add;
    }
    else if (add == TRC_TEST_UNSPECIFIED)
    {
        /* 
         * It is stange to have unspecified result together
         * with any other entry.
         */
        assert(FALSE);
        return TRC_TEST_STATUS_MAX;
    }
    else if (result == TRC_TEST_UNSTABLE)
    {
        /*
         * Result has already been classified as unstable, so nothing
         * can change it.
         */
        return result;
    }
    else if (((result == TRC_TEST_PASSED) && (add == TRC_TEST_FAILED)) ||
             ((result == TRC_TEST_FAILED) && (add == TRC_TEST_PASSED)))
    {
        /* Mixture of PASSED/FAILED, so UNSTABLE */
        return TRC_TEST_UNSTABLE;
    }
    /*
     * If SKIPPED goes with some other result, it is ignored.
     */
    else if (result == TRC_TEST_SKIPPED)
    {
        return add;
    }
    else if (add == TRC_TEST_SKIPPED)
    {
        return result;
    }
    else
    {
        /* Really unexpected situation */
        assert(FALSE);
        return TRC_TEST_STATUS_MAX;
    }
}

/**
 * Increment statistic for two compared sets.
 * 
 * @param stats         Location of the statistics to update
 * @param set_i         Index of the first set
 * @param status_i      Status for the first set
 * @param set_j         Index of the second set
 * @param status_j      Status for the second set
 * @param diff          Comparison result
 */
static void
trc_diff_stats_inc(trc_diff_stats *stats,
                   unsigned int set_i, trc_test_status status_i,
                   unsigned int set_j, trc_test_status status_j,
                   trc_diff_status diff)
{
    assert(stats != NULL);
    assert(set_i < TRC_DIFF_IDS);
    assert(set_j < TRC_DIFF_IDS);
    assert(set_i != set_j);
    assert(status_i < TRC_TEST_STATUS_MAX);
    assert(status_j < TRC_TEST_STATUS_MAX);
    assert(diff < TRC_DIFF_STATUS_MAX);
    assert((diff != TRC_DIFF_MATCH) || (status_i == status_j));

    ((*stats)[set_i][set_j - 1][status_i][status_j][diff])++;
}

/**
 * Are two expected results equal?
 *
 * @param set1          The first set to compare
 * @param result1       Expected result for the first set
 * @param keys1         List of keys for the first set
 * @param set2          The second set to compare
 * @param result2       Expected result for the second set
 * @param keys1         List of keys for the second set
 * @param stats         Statistics to update or NULL
 *
 * @return 
 */
static te_bool
trc_diff_compare(trc_diff_set *set1, const trc_exp_result *result1,
                 tqh_strings *keys1,
                 trc_diff_set *set2, const trc_exp_result *result2,
                 tqh_strings *keys2,
                 trc_diff_stats *stats)
{
    const trc_exp_result_entry *p;
    trc_test_status             status1 = TRC_TEST_STATUS_MAX;
    trc_test_status             status2 = TRC_TEST_STATUS_MAX;
    trc_diff_status             diff = TRC_DIFF_MATCH;
    te_bool                     ignore;
    te_bool                     main_key_used;

    assert(set1 != NULL);
    assert(result1 != NULL);
    assert(set2 != NULL);
    assert(result2 != NULL);

    /* 
     * Check that each entry in the expecred result for the first set
     * has equal entry in the expected result for the second set.
     */
    main_key_used = FALSE;
    TAILQ_FOREACH(p, &result1->results, links)
    {
        /* 
         * If pointers to expected result for the first and the second
         * sets are equal, expected results are definitely equal.
         */
        if ((result1 != result2) &&
            trc_is_result_expected(result2, &p->result) == NULL)
        {
            /* 
             * The expected result entry from the first set does not
             * match any entry from the second set.
             * Check key and collect per-key statistics.
             */
            ignore = trc_diff_check_key(set1, (p->key != NULL) ?
                                                  p->key : result1->key);
            if (!ignore)
            {
                if (p->key != NULL)
                {
                    tad_diff_key_stat_inc(&set1->keys_stats, p->key);
                    tq_strings_add_uniq(keys1, p->key);
                }
                else if (!main_key_used)
                {
                    main_key_used = TRUE;
                    tad_diff_key_stat_inc(&set1->keys_stats, result1->key);
                    if (result1->key != NULL)
                        tq_strings_add_uniq(keys1, result1->key);
                }
                diff = TRC_DIFF_NO_MATCH;
            }
            else if (diff == TRC_DIFF_NO_MATCH)
            {
                /* Nothing can change it */
            }
            else if (diff == TRC_DIFF_MATCH)
            {
                diff = TRC_DIFF_NO_MATCH_IGNORE;
            }
        }
        status1 = trc_test_status_merge(status1,
                      test_status_te2trc(p->result.status));
    }

    /* 
     * No entries in expected result for the first set, therefore,
     * it is unspecified.
     */
    if (status1 == TRC_TEST_STATUS_MAX)
        status1 = TRC_TEST_UNSPECIFIED;

    /* 
     * If pointers are equal, expected results are equal and we have
     * all information required to update statistics.
     * Since results are equal, no per-key statistics should be updated.
     */
    if (result1 == result2)
    {
        if (stats != NULL)
            trc_diff_stats_inc(stats, set1->id, status1,
                               set2->id, status1, TRC_DIFF_MATCH);
        return TRC_DIFF_MATCH;
    }

    /* 
     * Check that each entry in the expecred result for the second set
     * has equal entry in the expected result for the first set.
     */
    main_key_used = FALSE;
    TAILQ_FOREACH(p, &result2->results, links)
    {
        if (trc_is_result_expected(result1, &p->result) == NULL)
        {
            /* 
             * The expected result entry does not correspond to any
             * in another expected result. Therefore, this entry is 
             * unexpected.
             */
            ignore = trc_diff_check_key(set2, (p->key != NULL) ?
                                                  p->key : result2->key);
            if (!ignore)
            {
                if (p->key != NULL)
                {
                    tad_diff_key_stat_inc(&set2->keys_stats, p->key);
                    tq_strings_add_uniq(keys2, p->key);
                }
                else if (!main_key_used)
                {
                    main_key_used = TRUE;
                    tad_diff_key_stat_inc(&set2->keys_stats, result2->key);
                    if (result2->key != NULL)
                        tq_strings_add_uniq(keys2, result2->key);
                }
                diff = TRC_DIFF_NO_MATCH;
            }
            else if (diff == TRC_DIFF_NO_MATCH)
            {
                /* Nothing can change it */
            }
            else if (diff == TRC_DIFF_MATCH)
            {
                diff = TRC_DIFF_NO_MATCH_IGNORE;
            }
        }
        status2 = trc_test_status_merge(status2,
                      test_status_te2trc(p->result.status));
    }

    /* 
     * No entries in expected result for the second set, therefore,
     * it is unspecified.
     */
    if (status2 == TRC_TEST_STATUS_MAX)
        status2 = TRC_TEST_UNSPECIFIED;

    if (stats != NULL)
        trc_diff_stats_inc(stats, set1->id, status1, set2->id, status2,
                           diff);

    return diff;
}

/**
 * Compare expected results.
 *
 * @param sets          Compared sets
 * @param parent        Parent test entry
 * @parma entry         Test iteration entry
 * @param stats         Grand total statistics to update
 *
 * @return Comparison status.
 * @retval -1           All expected results are SKIPPED
 * @retval 0            No differences to be shown (no differences or
 *                      all differences are ignored)
 * @retval 1            There are some differences to be shown
 */
static int
trc_diff_entry_has_diff(const trc_diff_sets *sets,
                        trc_diff_entry      *parent,
                        trc_diff_entry      *entry,
                        trc_diff_stats      *stats)
{
    trc_diff_set   *p;
    trc_diff_set   *q;
    te_bool         diff = FALSE;

    assert(sets != NULL);
    assert(parent != NULL);
    assert(entry != NULL);

    TAILQ_FOREACH(p, sets, links)
    {
        for (q = TAILQ_NEXT(p, links); q != NULL; q = TAILQ_NEXT(q, links))
        {
            diff = diff ||
                   (trc_diff_compare(p, entry->results[p->id],
                                     parent->keys + p->id,
                                     q, entry->results[q->id],
                                     parent->keys + q->id,
                                     stats) == TRC_DIFF_NO_MATCH);
            /* 
             * Do not terminate comparison if the difference is found.
             * We need to update statistics for all pairs.
             */
        }
        /*
         * If comparison of the first entry with the rest does not
         * show any differences, we can be sure that all entries
         * match. However, special processing is required to update
         * statistics. 
         */
    }

    if (diff)
    {
        return 1;
    }
    else
    {
        if (!TAILQ_EMPTY(sets) &&
            entry->results[TAILQ_FIRST(sets)->id] != NULL &&
            trc_is_exp_result_skipped(
                entry->results[TAILQ_FIRST(sets)->id]))
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
}


/* See descriptino in trc_diff.h */
te_errno
trc_diff_do(trc_diff_ctx *ctx)
{
    SLIST_HEAD(, trc_diff_state) states;

    te_errno                rc;
    te_bool                 start;
    unsigned int            level;
    trc_diff_state         *state;
    te_trc_db_walker       *walker;
    trc_db_walker_motion    motion;
    trc_diff_entry         *parent;
    unsigned int            children;
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

    SLIST_INIT(&states);

    /* Traverse the tree */
    rc = 0;
    start = TRUE;
    level = 0;
    parent = entry = NULL;
    has_diff = FALSE;
    children = 0;
    while ((rc == 0) &&
           ((motion = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT))
    {
        VERB("M=%u, l=%u, p=%p, e=%p, to_result=%u hide_children=%u\n",
             motion, level, parent, entry, entry_to_result,
             hide_children);
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
                    state->children = children;
                    SLIST_INSERT_HEAD(&states, state, links);

                    /* Current 'entry' is a parent to a new one */
                    assert(entry != NULL);
                    parent = entry;
                    entry = NULL;
                    parent->level = level;
                    TAILQ_INSERT_TAIL(&ctx->result, parent, links);
                    /* Ignore 'entry_to_result' for non-leaf nodes */
                    has_diff = FALSE;
                    children = 0;
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
                children++;
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
                    /* 
                     * We have to get expected results for packages,
                     * since it may be skipped and should be inhireted
                     * by its tests.
                     */
                    trc_diff_entry_exp_results(&ctx->sets, walker, entry);

                    /*
                     * Analisys of differences is interesting for
                     * test scripts only (leaves of the tree).
                     */
                    if (trc_db_walker_get_test(walker)->type ==
                            TRC_TEST_SCRIPT)
                    {
                        switch (trc_diff_entry_has_diff(&ctx->sets,
                                    parent, entry,
                                    (trc_db_walker_get_test(walker)->aux) ?
                                        NULL : &ctx->stats))
                        {
                            case -1:
                                break;

                            case 0:
                                /* 
                                 * Some children does not have differences.
                                 * Therefore, it is necessary to show which
                                 * one has differences.
                                 */
                                hide_children = FALSE;
                                break;

                            case 1:
                                entry->ptr.iter =
                                    trc_db_walker_get_iter(walker);
                                entry_to_result = has_diff = TRUE;
                                if (!trc_diff_group_exp_result(&ctx->sets,
                                         parent, entry,
                                         motion == TRC_DB_WALKER_SON))
                                {
                                    /* Group is not homogeneous */
                                    hide_children = FALSE;
                                }
                                break;

                            default:
                                assert(FALSE);
                        }
                    }
                }
                else
                {
                    VERB("%s\n", trc_db_walker_get_test(walker)->name);
                }
                break;

            case TRC_DB_WALKER_FATHER:
                if (entry_to_result && entry != NULL)
                {
                    /* The last child should be added in the result */
                    entry->level = level;
                    TAILQ_INSERT_TAIL(&ctx->result, entry, links);
                    /* Keep 'entry_to_result' to force allocation */
                }
                else
                {
                    /* Free extra entry allocated to process children */
                    free(entry);
                }
                entry = NULL;
                if (has_diff)
                {
                    /* Some differences have been discovered */
                    if (parent->is_iter)
                        parent->ptr.iter = trc_db_walker_get_iter(walker);
                    else
                        parent->ptr.test = trc_db_walker_get_test(walker);

                    if (hide_children)
                    {
                        /* 
                         * It is allowed to hide all children,
                         * therefore remove them from result.
                         */
                        while ((entry = TAILQ_NEXT(parent, links)) != NULL)
                        {
                            TAILQ_REMOVE(&ctx->result, entry, links);
                            free(entry);
                        }
                    }
                    else if (children == 1 && !parent->is_iter)
                    {
                        /* 
                         * Test group has only one iteration. Therefore,
                         * it is not interesting to look at parameters.
                         */
                        entry = TAILQ_NEXT(parent, links);
                        assert(entry != NULL);
                        TAILQ_REMOVE(&ctx->result, entry, links);
                        free(entry);
                        entry = NULL;
                    }
                    entry_to_result = TRUE;
                }
                else
                {
                    /* No differences in children */
                    /* Nothing should be added after the parent */
                    assert(TAILQ_NEXT(parent, links) == NULL);
                    /* Remove the parent from result */
                    TAILQ_REMOVE(&ctx->result, parent, links);
                    /* Reuse this parent entry for its brothers */
                    entry = parent;
                    assert(!entry_to_result);
                }
                
                /* Extract state from the stack and restore */
                state = SLIST_FIRST(&states);
                assert(state != NULL);
                SLIST_REMOVE(&states, state, trc_diff_state, links);
                parent = state->entry;
                has_diff = has_diff || state->has_diff;
                children = state->children;
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
trc_diff_ctx *
trc_diff_ctx_new(void)
{
    trc_diff_ctx *ctx = TE_ALLOC(sizeof(*ctx));

    if (ctx != NULL)
    {
        ctx->flags = 0;
        ctx->db = NULL;
        TAILQ_INIT(&ctx->sets);

        memset(&ctx->stats, 0, sizeof(ctx->stats));
        TAILQ_INIT(&ctx->result);
    }
    return ctx;
}

/* See descriptino in trc_diff.h */
void
trc_diff_ctx_free(trc_diff_ctx *ctx)
{
    trc_diff_entry *p;

    trc_diff_free_sets(&ctx->sets);

    while ((p = TAILQ_FIRST(&ctx->result)) != NULL)
    {
        TAILQ_REMOVE(&ctx->result, p, links);
        free(p);
    }
    free(ctx);
}
