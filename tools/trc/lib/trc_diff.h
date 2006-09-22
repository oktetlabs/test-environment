/** @file
 * @brief Testing Results Comparator: diff tool
 *
 * Definition of TRC diff tool types and related routines.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TRC_DIFF_H__
#define __TE_TRC_DIFF_H__

#include "te_defs.h"
#include "te_queue.h"
#include "te_errno.h"
#include "tq_string.h"
#include "te_trc.h"
#include "trc_db.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Number of IDs supported by TRC diff */
#define TRC_DIFF_IDS    10


/** Entry in the list of list of named tags */
typedef struct trc_diff_tags_entry {
    TAILQ_ENTRY(trc_diff_tags_entry)    links; /**< List links */

    unsigned int    id;         /**< ID of the list */
    tqh_strings     tags;       /**< List of tags */
    char           *name;       /**< Name of the set */
    te_bool         show_keys;  /**< Show table with keys which explain
                                     differences */
} trc_diff_tags_entry;

/** List of lists of named tags */
typedef TAILQ_HEAD(trc_diff_tags_list, trc_diff_tags_entry)
    trc_diff_tags_list;


/**
 * Types of statistics collected per set X vs set Y.
 */
typedef enum trc_diff_stats_index {
    TRC_DIFF_STATS_PASSED = 0,
    TRC_DIFF_STATS_PASSED_DIFF,
    TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE,
    TRC_DIFF_STATS_FAILED,
    TRC_DIFF_STATS_FAILED_DIFF,
    TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE,
    TRC_DIFF_STATS_SKIPPED,
    TRC_DIFF_STATS_OTHER,

    TRC_DIFF_STATS_MAX
} trc_diff_stats_index;

/** Type of simple counter. */
typedef unsigned int trc_diff_stats_counter;

/**
 * Set X vs set Y statistics are two dimension array of simple
 * counters. Indices are the results of the corresponding set together
 * with equal/different knowledge, when main result is the same.
 */
typedef trc_diff_stats_counter trc_diff_stats_counters[TRC_DIFF_STATS_MAX]
                                                      [TRC_DIFF_STATS_MAX];

/**
 * TRC differencies statistics are two dimension array of statistics per
 * set X vs set Y statistics.
 * 
 * A half of this array is used in fact (the first index is always
 * greater than the second one).
 */
typedef trc_diff_stats_counters trc_diff_stats[TRC_DIFF_IDS]
                                              [TRC_DIFF_IDS - 1];


/** Statistics for each key which makes differences */
typedef struct trc_diff_key_stats {
    CIRCLEQ_ENTRY(trc_diff_key_stats)   links;  /**< List links */

    tqh_strings    *tags;   /**< Set of tags for which this key is used */
    const char     *key;    /**< Key */
    unsigned int    count;  /**< How many times this key is added */
} trc_diff_key_stats;

/** List of statistics for all keys */
typedef CIRCLEQ_HEAD(trc_diff_keys_stats, trc_diff_key_stats)
    trc_diff_keys_stats;


/** Element of the list with TRC diff results. */
typedef struct trc_diff_entry {
    TAILQ_ENTRY(trc_diff_entry) links;  /**< List links */

    te_bool         is_iter;    /**< Is a test or an iteration? */
    union {
        trc_test       *test;   /**< Test entry data */
        trc_test_iter  *iter;   /**< Test iteration data */
    } ptr;                      /**< Pointer to test or iteration data */
    unsigned int    level;      /**< Level of the entry in the tree */

    /** Expected result for each diff ID */
    const trc_exp_result   *results[TRC_DIFF_IDS];
    /** Expected result inheritance flag */
    te_bool                 inherit[TRC_DIFF_IDS];
    
} trc_diff_entry;

/** Result of the TRC diff processing */
typedef TAILQ_HEAD(trc_diff_result, trc_diff_entry) trc_diff_result;


/**
 * TRC diff tool context.
 */
typedef struct trc_diff_ctx {
    /* Input */
    unsigned int        flags;          /**< Processing control flags */
    te_trc_db          *db;
    trc_diff_tags_list  sets;           /**< Sets to compare */
    tqh_strings         exclude_keys;   /**< Templates for keys to exclude
                                             some differencies from 
                                             consideration */
    /* Output */
    trc_diff_stats      stats;          /**< Statistics */
    trc_diff_keys_stats keys_stats;     /**< Per-key statistics */
    trc_diff_result     result;         /**< Result details */
} trc_diff_ctx;


/**
 * Set name of the TRC tags set with specified ID.
 *
 * @param tags      List of sets of tags
 * @param id        Identifier of the list to be used
 * @param name      Name of the tags set
 *
 * @return Status code.
 */
extern te_errno trc_diff_set_name(trc_diff_tags_list *tags,
                                  unsigned int        id,
                                  const char         *name);

/**
 * Enable showing keys of the TRC tags set with specified ID.
 *
 * @param tags      List of sets of tags
 * @param id        Identifier of the list to be used
 *
 * @return Status code.
 */
extern te_errno trc_diff_show_keys(trc_diff_tags_list *tags,
                                   unsigned int        id);


/**
 * Add tag in the end of the TRC tags set with specified ID.
 *
 * @param tags      List of lists of tags
 * @param id        Identifier of the list to be used
 * @param tag       Name of the tag
 *
 * @return Status code.
 */
extern te_errno trc_diff_add_tag(trc_diff_tags_list *tags,
                                 unsigned int        id,
                                 const char         *tag);

/**
 * Free TRC tags lists.
 *
 * @param tags      List of list of tags to be freed
 */
extern void trc_diff_free_tags(trc_diff_tags_list *tags);


/**
 * Initialize TRC diff context.
 *
 * @param ctx           Context to be initialized 
 */
extern void trc_diff_ctx_init(trc_diff_ctx *ctx);

/**
 * Free resources allocated in TRC diff context.
 *
 * @param ctx           Context to be freed
 */
extern void trc_diff_ctx_free(trc_diff_ctx *ctx);


/**
 * Process TRC database and generate in-memory report.
 *
 * @param ctx           TRC diff tool context
 *
 * @return Status code.
 */
extern te_errno trc_diff_do(trc_diff_ctx *ctx);

/**
 * Prepare TRC diff report in HTML format.
 *
 * @param ctx           TRC diff tool context
 * @param filename      Name of the file to put report or NULL if
 *                      report should be generated to stdout
 * @param title         Title of the report
 *
 * @return Status code.
 */
extern te_errno trc_diff_report_to_html(trc_diff_ctx *ctx,
                                        const char   *filename,
                                        const char   *title);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_DIFF_H__ */
