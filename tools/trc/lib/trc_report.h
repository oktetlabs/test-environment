/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator: report tool
 *
 * Definition of TRC report tool types and related routines.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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

/**
 * Get number of run test iterations with a given result status
 *
 * @param   s       TRC statistics
 * @param   status  Test iteration result status
 *
 * @return  Number of corresponding iterations or 0
 */
static inline unsigned int
get_stats_by_status(const trc_report_stats *s, te_test_status status)
{
    switch (status)
    {
        case TE_TEST_PASSED:
            return s->pass_exp + s->pass_une;
        case TE_TEST_FAILED:
            return s->fail_exp + s->fail_une;
        default:
            return 0;
    }
}

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
    TRC_REPORT_KEEP_ARTIFACTS   = 0x40000,  /**< Keep entries with artifacts
                                                 regardless hiding options */
    TRC_REPORT_NO_STATS_NOT_RUN = 0x100, /**< Hide entries with
                                              unexpected not run
                                              statistic */
    TRC_REPORT_NO_KEYS          = 0x200, /**< Hide actual key entries */
    TRC_REPORT_KEYS_ONLY        = 0x400, /**< Show only keys table */
    TRC_REPORT_KEYS_FAILURES    = 0x800, /**< Keys table for failures */
    TRC_REPORT_KEYS_SANITY      = 0x1000, /**< Perform keys sanity check */
    TRC_REPORT_KEYS_EXPECTED    = 0x2000, /**< Keys for expected
                                               behaviour */
    TRC_REPORT_KEYS_UNEXPECTED  = 0x4000, /**< Keys for unexpected
                                               behaviour */
    TRC_REPORT_WILD_VERBOSE     = 0x80000, /**< Show wildcards for
                                                distinct verdicts,
                                                result statuses */

    /** Do not report unspecified key, if test passed with verdict */
    TRC_REPORT_KEYS_SKIP_PASSED_UNSPEC = 0x8000,
    /** Do not report unspecified key, if test failed w/wo verdict */
    TRC_REPORT_KEYS_SKIP_FAILED_UNSPEC = 0x10000,

    /* DB processing options */
    TRC_REPORT_UPDATE_DB        = 0x20000, /**< Update TRC database */
};

/* Mask for keys-related flags */
#define TRC_REPORT_KEYS_MASK \
    (TRC_REPORT_NO_KEYS | TRC_REPORT_KEYS_FAILURES |        \
     TRC_REPORT_KEYS_SANITY | TRC_REPORT_KEYS_EXPECTED |    \
     TRC_REPORT_KEYS_UNEXPECTED)

/** Result of test iteration run */
typedef struct trc_report_test_iter_entry {
    TAILQ_ENTRY(trc_report_test_iter_entry) links;  /**< List links */

    int             tin;        /**< Test Identification Number */
    int             test_id;    /**< Test ID */
    char           *hash;       /**< Test arguments hash */
    te_test_result  result;     /**< Obtained result */
    te_bool         is_exp;     /**< Does obtained result match one of
                                     expected? */
    unsigned int    args_max;   /**< Maximum number of arguments
                                     the space is allocated for */
    trc_report_argument *args;  /**< Actual arguments */

    unsigned int       args_n;  /**< Number of arguments */
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
    unsigned int        flags;          /**< Report options */
    unsigned int        parsing_flags;  /**< Log parsing options */
    te_trc_db          *db;             /**< TRC database handle */
    tqh_strings         tags;           /**< TRC tags specified by user */
    tqh_strings         merge_fns;      /**< Logs to merge with main log */
    tqh_strings         cut_paths;      /**< Testpaths to cut from
                                             main log */
    trc_report_stats    stats;          /**< Grand total statistics */
    unsigned int        db_uid;         /**< TRC database user ID */
    const char         *html_logs_path; /**< Path to HTML logs */
    const char         *show_cmd_file;  /**< Show cmd used to generate
                                             the report */
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

    char                        *name;      /**< Test name */
    char                        *path;      /**< Test path */
    char                        *key_path;  /**< Test path with key
                                                 appended */
    int                          count;     /**< Amount of iteration
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

extern te_errno trc_report_to_perl(trc_report_ctx *gctx,
                                   const char *filename);

/** Maximum length of test iteration ID */
#define TRC_REPORT_ITER_ID_LEN 128

/**
 * Return iteration ID (based on test ID, if available, or TIN).
 *
 * @note This function returns pointer to static variable, it is not
 *       thread-safe, every call overwrites previously returned value.
 *       It is supposed that in HTML logs a file for the iteration is
 *       named "node_<ID>.html".
 *
 * @param iter      Pointer to structure describing iteration.
 *
 * @return String ID (empty if neither test ID nor TIN are available).
 */
extern const char *trc_report_get_iter_id(
                                const trc_report_test_iter_entry *iter);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_REPORT_H__ */
