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
#define TRC_DIFF_IDS    31


/** Statistics for each key which makes differences */
typedef struct trc_diff_key_stats {
    CIRCLEQ_ENTRY(trc_diff_key_stats)   links;  /**< List links */

    const char     *key;    /**< Key */
    unsigned int    count;  /**< How many times this key is used to
                                 explain the difference */
} trc_diff_key_stats;

/** List of statistics for all keys */
typedef CIRCLEQ_HEAD(trc_diff_keys_stats, trc_diff_key_stats)
    trc_diff_keys_stats;


/** Set of tags to compare */
typedef struct trc_diff_set {
    TAILQ_ENTRY(trc_diff_set)   links; /**< List links */

    unsigned int    id;         /**< ID of the list */
    unsigned int    db_uid;     /**< TRC DB User ID */
    tqh_strings     tags;       /**< List of tags */
    char           *name;       /**< Name of the set */
    char           *log;        /**< Raw log filename */
    te_bool         show_keys;  /**< Show table with keys which explain
                                     differences */
    tqh_strings     ignore;     /**< List of exclusions */

    trc_diff_keys_stats keys_stats; /**< Per-key statistics */

} trc_diff_set;

/** List with set of tags to compare */
typedef TAILQ_HEAD(trc_diff_sets, trc_diff_set) trc_diff_sets;


/** Status of expected testing result from TRC point of view. */
typedef enum trc_test_status {
    TRC_TEST_PASSED = 0,    /**< PASSED results are expected */
    TRC_TEST_PASSED_UNE,    /**< PASSED results are unexpected */
    TRC_TEST_FAILED,        /**< FAILED results are expected */
    TRC_TEST_FAILED_UNE,    /**< FAILED results are unexpected */
    TRC_TEST_UNSTABLE,      /**< PASSED and FAILED results are expected */
    TRC_TEST_SKIPPED,       /**< SKIPPD result is expected */
    TRC_TEST_UNSPECIFIED,   /**< Expected result is unspecified */

    TRC_TEST_STATUS_MAX     /**< Total number of statuses */
} trc_test_status;

/** Status of expected testing result comparison. */
typedef enum trc_diff_status {
    TRC_DIFF_MATCH = 0,         /**< Expected results match */
    TRC_DIFF_NO_MATCH,          /**< Expected results do not match */
    TRC_DIFF_NO_MATCH_IGNORE,   /**< Expected results do not match
                                     (but ignore is requested) */

    TRC_DIFF_STATUS_MAX         /**< Total number of statuses */
} trc_diff_status;

typedef enum trc_diff_summary_status {
    TRC_DIFF_SUMMARY_PASSED = 0,
    TRC_DIFF_SUMMARY_PASSED_UNE,
    TRC_DIFF_SUMMARY_FAILED,
    TRC_DIFF_SUMMARY_FAILED_UNE,
    TRC_DIFF_SUMMARY_TOTAL,
    TRC_DIFF_SUMMARY_STATUS_MAX,
} trc_diff_summary_status;

typedef enum trc_diff_summary_component {
    TRC_DIFF_SUMMARY_MATCH = 0,
    TRC_DIFF_SUMMARY_NEW,
    TRC_DIFF_SUMMARY_OLD,
    TRC_DIFF_SUMMARY_SKIPPED_MATCH,
    TRC_DIFF_SUMMARY_SKIPPED_NEW,
    TRC_DIFF_SUMMARY_SKIPPED_OLD,
    TRC_DIFF_SUMMARY_COMPONENT_MAX,
} trc_diff_summary_component;

struct trc_diff_entry;

typedef struct trc_diff_stats_counter_list_entry {
    TAILQ_ENTRY(trc_diff_stats_counter_list_entry) links;
    struct trc_test       *test;
    char                  *hash;
    unsigned int           count;
} trc_diff_stats_counter_list_entry;

typedef TAILQ_HEAD(trc_diff_stats_counter_list_head,
                   trc_diff_stats_counter_list_entry)
                   trc_diff_stats_counter_list_head;

/** Type of simple counter. */
typedef struct trc_diff_stats_counter_s {
    unsigned int                        counter;
    trc_diff_stats_counter_list_head    entries;
} trc_diff_stats_counter;

/**
 * Set X vs set Y statistics are three dimension array of simple
 * counters. Indices are the results of the corresponding set together
 * with equal/different knowledge, when main result is the same.
 */
typedef trc_diff_stats_counter
    trc_diff_stats_counters[TRC_TEST_STATUS_MAX]
                           [TRC_TEST_STATUS_MAX]
                           [TRC_DIFF_STATUS_MAX];

/**
 * TRC differencies statistics are two dimension array of statistics per
 * set X vs set Y statistics.
 * 
 * A half of this array is used in fact (the first index is always
 * greater than the second one).
 */
typedef trc_diff_stats_counters trc_diff_stats[TRC_DIFF_IDS]
                                              [TRC_DIFF_IDS - 1];


/** Element of the list with TRC diff results. */
typedef struct trc_diff_entry {
    TAILQ_ENTRY(trc_diff_entry) links;  /**< List links */

    unsigned int    level;      /**< Level of the entry in the tree */
    te_bool         is_iter;    /**< Is a test or an iteration? */
    union {
        const trc_test      *test;   /**< Test entry data */
        const trc_test_iter *iter;   /**< Test iteration data */
    } ptr;                      /**< Pointer to test or iteration data */

    /** Expected result for each diff ID */
    const trc_exp_result   *results[TRC_DIFF_IDS];

    /** Expected result inheritance flags */
    unsigned int            inherit[TRC_DIFF_IDS];
#define TRC_DIFF_INHERIT    0x1 /**< Result should be inherited */
#define TRC_DIFF_INHERITED  0x2 /**< Result is inherited */

    /** Lists of keys per set which explain the differences */
    tqh_strings             keys[TRC_DIFF_IDS];
} trc_diff_entry;

/** Result of the TRC diff processing */
typedef TAILQ_HEAD(trc_diff_result, trc_diff_entry) trc_diff_result;


/**
 * TRC diff tool context.
 *
 * @note The object is quiet big and it is highly not recommented to
 *       allocate it on stack.
 */
typedef struct trc_diff_ctx {

    unsigned int        flags;      /**< Processing control flags */
    te_trc_db          *db;         /**< TRC database handle */
    trc_diff_sets       sets;       /**< Sets to compare */

    trc_diff_stats      stats;      /**< Grand total statistics */
    trc_diff_result     result;     /**< Result details */

    tqh_strings         tests_include; /* List of test paths to include */
    tqh_strings         tests_exclude; /* List of test paths to exclude */
} trc_diff_ctx;

extern char *
trc_diff_iter_hash_get(const trc_test_iter *test_iter, int db_uid);

/**
 * Find set in sets list by specified ID.
 *
 * @param sets          List of compared sets
 * @param id            Identifier of the set
 * @param create        create the set, if it is not found
 *
 * @return Status code.
 */
extern trc_diff_set *
trc_diff_find_set(trc_diff_sets *sets, unsigned int id, te_bool create);

/**
 * Set name of the compared set with specified ID.
 *
 * @param sets          List of compared sets
 * @param id            Identifier of the set
 * @param name          New name of the set
 *
 * @return Status code.
 */
extern te_errno trc_diff_set_name(trc_diff_sets *sets,
                                  unsigned int   id,
                                  const char    *name);

/**
 * Set name of the compared set with specified ID.
 *
 * @param sets  List of compared sets
 * @param id    Identifier of the set
 * @param log   Log filename
 for the set
 *
 * @return Status code.
 */
extern te_errno trc_diff_set_log(trc_diff_sets *sets,
                                 unsigned int   id,
                                 const char    *log);

/**
 * Enable showing keys of the compared set with specified ID.
 *
 * @param sets          List of compared sets
 * @param id            Identifier of the set
 *
 * @return Status code.
 */
extern te_errno trc_diff_show_keys(trc_diff_sets *sets,
                                   unsigned int   id);

/**
 * Add tag in the compared set with specified ID.
 *
 * @param sets          List of compared sets
 * @param id            Identifier of the set
 * @param tag           Name of a new tag
 *
 * @return Status code.
 */
extern te_errno trc_diff_add_tag(trc_diff_sets *sets,
                                 unsigned int   id,
                                 const char    *tag);

/**
 * Add ignore pattern for the compared set with specified ID.
 *
 * @param sets          List of compared sets
 * @param id            Identifier of the set
 * @param ignore        A new ignore pattern
 *
 * @return Status code.
 */
extern te_errno trc_diff_add_ignore(trc_diff_sets *sets,
                                    unsigned int   id,
                                    const char    *ignore);

/**
 * Free compared sets
 *
 * @param sets          List of compared sets
 */
extern void trc_diff_free_sets(trc_diff_sets *sets);


/**
 * Allocate a new TRC diff context.
 */
extern trc_diff_ctx *trc_diff_ctx_new(void);

/**
 * Free resources allocated in TRC diff context.
 *
 * @param ctx           Context to be freed
 */
extern void trc_diff_ctx_free(trc_diff_ctx *ctx);

/**
 * Are two expected results equal (including keys and notes)?
 *
 * @param lhv           Left hand value
 * @param rhv           Right hand value
 */
extern te_bool trc_diff_is_exp_result_equal(const trc_exp_result *lhv,
                                            const trc_exp_result *rhv);

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
 * @param header        HTML header file name to include into the report
 * @param title         Title of the report
 * @param summary_only  Generate only summary report
 *
 * @return Status code.
 */
extern te_errno trc_diff_report_to_html(trc_diff_ctx *ctx,
                                        const char   *filename,
                                        const char   *header,
                                        const char   *title,
                                        te_bool summary_only);


/**
 * Process TE log files specified for each diff set.
 *
 * @param ctx           TRC diff context
 *
 * @return Status code.
 */
extern te_errno trc_diff_process_logs(trc_diff_ctx *ctx);

/**
 * Filter test results by specified include/exclude rules.
 *
 * @param ctx           TRC diff context
 *
 * @return Status code.
 */
extern te_errno trc_diff_filter_logs(trc_diff_ctx *ctx);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_DIFF_H__ */
