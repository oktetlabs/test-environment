/** @file
 * @brief Testing Results Comparator: report tool
 *
 * Definition of TRC report tool types and related routines.
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

#ifndef __TE_TRC_REPORT_H__
#define __TE_TRC_REPORT_H__

#include "te_defs.h"
#include "te_queue.h"
#include "te_errno.h"
#include "tq_string.h"
#include "te_trc.h"
#include "trc_db.h"

#ifdef __cplusplus
extern "C" {
#endif


/** Testing results comparator statistics */
typedef struct trc_report_stats {
    unsigned int    pass_exp;       /**< Passed as expected */
    unsigned int    pass_une;       /**< Passed unexpectedly */
    unsigned int    fail_exp;       /**< Failed as expected */
    unsigned int    fail_une;       /**< Failed unexpectedly */
    unsigned int    aborted;        /**< No usefull result */
    unsigned int    new_run;        /**< Run iterations with unknown
                                         expected result */
    unsigned int    not_run;        /**< Not run iterations */
    unsigned int    skip_exp;       /**< Skipped as expected */
    unsigned int    skip_une;       /**< Skipped unexpectedly */
    unsigned int    new_not_run;    /**< Not run iterations with unknown
                                         expected result */
} trc_report_stats;

/** Number of run test iterations */
#define TRC_STATS_RUN(s) \
    ((s)->pass_exp + (s)->pass_une + (s)->fail_exp + (s)->fail_une + \
     (s)->aborted + (s)->new_run)

/** Number of test iterations with specified result */
#define TRC_STATS_SPEC(s) \
    (TRC_STATS_RUN(s) + (s)->skip_exp + (s)->skip_une)

/** Number of test iterations with obtained unexpected results */
#define TRC_STATS_RUN_UNEXP(s) \
    ((s)->pass_une + (s)->fail_une + (s)->skip_une + (s)->aborted + \
     (s)->new_run)

/** Number of test iterations with unexpected results (including not run) */
#define TRC_STATS_UNEXP(s) \
    (TRC_STATS_RUN_UNEXP(s) + (s)->not_run + (s)->new_not_run)

/** Number of test iterations which have not been run in fact */
#define TRC_STATS_NOT_RUN(s) \
    ((s)->not_run + (s)->skip_exp + (s)->skip_une + (s)->new_not_run)


/** TRC report tool options */
enum trc_report_flags {
    /* HTML report options */
    TRC_REPORT_NO_TOTAL_STATS   = 0x01,  /**< Hide grand total
                                              statistics */
    TRC_REPORT_NO_PACKAGES_ONLY = 0x02,  /**< Hide packages only
                                              statistics */
    TRC_REPORT_NO_SCRIPTS       = 0x04,  /**< Hide scripts */
    TRC_REPORT_STATS_ONLY       = 0x08,  /**< Show statistics only */
    TRC_REPORT_NO_UNSPEC        = 0x10,  /**< Hide entries with no
                                              obtained result */
    TRC_REPORT_NO_SKIPPED       = 0x20,  /**< Hide skipped iterations */
    TRC_REPORT_NO_EXP_PASSED    = 0x40,  /**< Hide passed as expected
                                              iterations */
    TRC_REPORT_NO_EXPECTED      = 0x80,  /**< Hide all expected
                                              iterations */
    TRC_REPORT_NO_STATS_NOT_RUN = 0x100, /**< Hide entries with
                                              unexpected not run
                                              statistic */
    TRC_REPORT_NO_KEYS          = 0x200, /**< Hide actual key entries */
    TRC_REPORT_KEYS_ONLY        = 0x400, /**< Show only keys table */
    TRC_REPORT_KEYS_SANITY      = 0x800, /**< Perform keys sanity check */

    /* DB processing options */
    TRC_REPORT_UPDATE_DB        = 0x2000, /**< Update TRC database */
    TRC_REPORT_IGNORE_LOG_TAGS  = 0x4000, /**< Ignore TRC tags extracted
                                               from the log */
};

/** Result of test iteration run */
typedef struct trc_report_test_iter_entry {
    TAILQ_ENTRY(trc_report_test_iter_entry) links;  /**< List links */

    int             tin;        /**< Test Identification Number */
    te_test_result  result;     /**< Obtained result */
    te_bool         is_exp;     /**< Does obtained result match one of
                                     expected? */
} trc_report_test_iter_entry;

/** Data attached to test iterations */
typedef struct trc_report_test_iter_data {
    const trc_exp_result   *exp_result; /**< Expected result */
    trc_report_stats        stats;      /**< Statistics */

    /** Head of the list with results of test iteration executions */
    TAILQ_HEAD(, trc_report_test_iter_entry)    runs;

} trc_report_test_iter_data;

/** Data attached to test entry */
typedef struct trc_report_test_data {
    trc_report_stats        stats;      /**< Statistics */
} trc_report_test_data;

/** TRC report context */
typedef struct trc_report_ctx {
    unsigned int        flags;  /**< Report options */
    te_trc_db          *db;     /**< TRC database handle */
    tqh_strings         tags;   /**< TRC tags specified by user */
    trc_report_stats    stats;  /**< Grand total statistics */
    unsigned int        db_uid; /**< TRC database user ID */
} trc_report_ctx;

typedef struct trc_report_key_iter_entry {
    TAILQ_ENTRY(trc_report_key_iter_entry)  links; /**< List links */
    const trc_report_test_iter_entry       *iter;  /**< Link to iteration
                                                        entry */
} trc_report_key_iter_entry;

/** Auxilary structure to list iterations marked by specific key */
typedef struct trc_report_key_test_entry {
    TAILQ_ENTRY(trc_report_key_test_entry)   links; /**< List links */
    TAILQ_HEAD(, trc_report_key_iter_entry)  iters; /**< Iterations list */
    char                                    *name;  /**< Test name */
    char                                    *path;  /**< Test path */
    int                                      count; /**< Amount of iteration
                                                         failed due to
                                                         specific key */
} trc_report_key_test_entry;


/** Key list entry */
typedef struct trc_report_key_entry {
    TAILQ_ENTRY(trc_report_key_entry) links;       /**< List links */
    TAILQ_HEAD(, trc_report_key_test_entry) tests; /**< Tests list */
    char *name;                                    /**< Key name */
    int   count; /**< Amount of test iterations failed to specific key */
} trc_report_key_entry;

/** TRC report context */
typedef struct trc_report_key_ctx {
    FILE            *f;      /**< File created by popen of key script */
    unsigned int     flags;  /**< Report options */
} trc_report_key_ctx;

extern int trc_report_key_read_cb(void *ctx, char *buffer, int len);
extern int trc_report_key_close_cb(void *ctx);

/**
 * Initialize TRC report tool context.
 *
 * @param ctx           Context to be initialized
 */
extern void trc_report_init_ctx(trc_report_ctx *ctx);

/**
 * Process TE log file with obtained testing results.
 *
 * @param ctx           TRC report context
 * @param log           Name of the file with TE log in XML format
 *
 * @return Status code.
 */
extern te_errno trc_report_process_log(trc_report_ctx *ctx,
                                       const char     *log);

/**
 * Collect statistics after processing of TE log.
 *
 * @param ctx           TRC report context
 *
 * @return Status code.
 */
extern te_errno trc_report_collect_stats(trc_report_ctx *ctx);

/**
 * Output TRC report in HTML format.
 *
 * @param ctx           TRC report context
 * @param filename      Name of the file for HTML report
 * @param title         Report title or NULL
 * @param header        File with header to be added in HTML report or
 *                      NULL
 * @param flags         Report options
 *
 * @return Status code.
 */
extern te_errno trc_report_to_html(trc_report_ctx *ctx,
                                   const char     *filename,
                                   const char     *title,
                                   FILE           *header,
                                   unsigned int    flags);


/**
 * Free resources allocated for test iteration in TRC report.
 *
 * @param data          Test iteration data to be freed
 */
extern void trc_report_free_test_iter_data(trc_report_test_iter_data *data);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_REPORT_H__ */
