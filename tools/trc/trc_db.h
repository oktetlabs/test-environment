/** @file
 * @brief Testing Results Comparator
 *
 * Definition of database representation and functions.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TRC_DB_H__
#define __TE_TRC_DB_H__

#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#include <libxml/tree.h>

#include "te_defs.h"


/** Number of IDs supported by TRC-diff */
#define TRC_DIFF_IDS    10

/** Generate brief version of the diff report */
#define TRC_DIFF_BRIEF      0x01


/** Entry of the list of strings */
typedef struct le_string {
    LIST_ENTRY(le_string)   links;  /**< List links */

    char                   *str;    /**< String */
} le_string;

/** List of strings */
typedef LIST_HEAD(lh_string, le_string) lh_string;


/** Entry of the queue of strings */
typedef struct tqe_string {
    TAILQ_ENTRY(tqe_string)     links;  /**< List links */

    char                       *str;    /**< String */
} tqe_string;

/** Queue of strings */
typedef TAILQ_HEAD(tqh_string, tqe_string) tqh_string;


/**
 * Compare two tail queue of strings. Queues are equal, if each element
 * of the first queue is equal to the corresponding element of the
 * second queue.
 *
 * @param s1        The first tail queue
 * @param s1        The second tail queue
 *
 * @retval TRUE     Equal
 * @retval FALSE    Not equal
 */
static inline te_bool
tq_strings_equal(const tqh_string *s1, const tqh_string *s2)
{
    const tqe_string *p1;
    const tqe_string *p2;

    if (s1 == s2)
        return TRUE;
    if (s1 == NULL || s2 == NULL)
        return FALSE;

    for (p1 = s1->tqh_first, p2 = s2->tqh_first;
         p1 != NULL && p2 != NULL && strcmp(p1->str, p2->str) == 0;
         p1 = p1->links.tqe_next, p2 = p2->links.tqe_next);
    
    return (p1 == NULL) && (p2 == NULL);
}


/** Enumeration of possible test results */
typedef enum trc_test_result {
    TRC_TEST_PASSED,      /**< Test should pass */
    TRC_TEST_FAILED,      /**< Test should fail */
    TRC_TEST_CORED,
    TRC_TEST_KILLED,
    TRC_TEST_FAKED,
    TRC_TEST_SKIPPED,     /**< Test should be skipped */
    TRC_TEST_UNSPEC,      /**< Expected test result is not specified yet */
    TRC_TEST_MIXED,       /**< Different test result for iterations */
    TRC_TEST_UNSET,       /**< Uninitialized test result */
} trc_test_result;

/** Expected result with key information and notes */
typedef struct trc_exp_result {
    trc_test_result     value;      /**< The result itself */
    char               *key;        /**< BugID-like information */
    char               *notes;      /**< Some usefull notes */
    tqh_string          verdicts;   /**< List of verdicts */
} trc_exp_result;

typedef enum trc_test_type {
    TRC_TEST_SCRIPT,
    TRC_TEST_SESSION,
    TRC_TEST_PACKAGE,
} trc_test_type;


/** Testing results conparator statistics */
typedef struct trc_stats {
    unsigned int    pass_exp;
    unsigned int    pass_une;
    unsigned int    fail_exp;
    unsigned int    fail_une;
    unsigned int    aborted;
    unsigned int    new_run;
    unsigned int    not_run;
    unsigned int    skip_exp;
    unsigned int    skip_une;
    unsigned int    new_not_run;
} trc_stats;

#define TRC_STATS_RUN(s) \
    ((s)->pass_exp + (s)->pass_une + (s)->fail_exp + (s)->fail_une + \
     (s)->aborted + (s)->new_run)

#define TRC_STATS_SPEC(s) \
    (TRC_STATS_RUN(s) + (s)->skip_exp + (s)->skip_une)

#define TRC_STATS_UNEXP(s) \
    ((s)->pass_une + (s)->fail_une + (s)->skip_une + (s)->aborted + \
     (s)->new_run + (s)->not_run + (s)->new_not_run)

#define TRC_STATS_NOT_RUN(s) \
    ((s)->not_run + (s)->skip_exp + (s)->skip_une + (s)->new_not_run)


/** Test argument */
typedef struct test_arg {
    TAILQ_ENTRY(test_arg)   links;  /**< List links */
    
    xmlNodePtr  node;   /**< XML node with this element */

    char       *name;   /**< Argument name */
    char       *value;  /**< Argument value */
} test_arg;

/** Head of the list with test arguments */
typedef struct test_args {
    TAILQ_HEAD(_test_args, test_arg) head;
    xmlNodePtr  node;   /**< XML node with this element */
} test_args;


/* Forward */
struct test_run;

/** Head of the list with tests */
typedef struct test_runs {
    TAILQ_HEAD(_test_runs, test_run) head;
    xmlNodePtr  node;   /**< XML node with this element */
} test_runs;


/** Test iteration */
typedef struct test_iter {
    TAILQ_ENTRY(test_iter)  links;  /**< List links */

    xmlNodePtr      node;           /**< XML node with this element */
    trc_stats       stats;          /**< Statistics */
    te_bool         used;           /**< Is the iteration used?
                                         (for many iterations with
                                         the same arguments) */

    test_args       args;           /**< Iteration arguments */
    char           *notes;          /**< Common notes */
    trc_exp_result  exp_result;     /**< The expected result */
    test_runs       tests;          /**< Children tests of the session */

    trc_test_result got_result;     /**< Got test result */
    tqh_string      got_verdicts;   /**< Got list of verdicts */

    te_bool         got_as_expect;  /**< Got result/verdicts is equal 
                                         to expected */

    /* Fields specific for TRC diff */
    trc_exp_result  diff_exp[TRC_DIFF_IDS]; /**< The expected results */

    /* Processing helpers */
    te_bool         processed;  /**< Is iteration checked for output */
    unsigned int    proc_flags; /**< Flags for this processing results */
    te_bool         output;     /**< Should the iteration be output */
    char           *diff_keys;  /**< String with keys for all tag sets */

} test_iter;

/** Head of the list with test iterations */
typedef struct test_iters {
    TAILQ_HEAD(_test_iters, test_iter) head;
    xmlNodePtr                         node;
} test_iters;


/** Test run */
typedef struct test_run {
    TAILQ_ENTRY(test_run)   links;  /**< List links */

    xmlNodePtr      node;           /**< XML node with this element */
    trc_stats       stats;          /**< Statistics */

    trc_test_type   type;           /**< Type of the test */
    te_bool         aux;            /**< Is test auxiliary? */
    char           *name;           /**< Test name */
    char           *notes;          /**< Some notes */

    char           *objective;      /**< Test objective */
    xmlNodePtr      obj_node;       /**< XML node with objective */
    te_bool         obj_update;     /**< Whether objective of the test
                                         should be updated */
    char           *test_path;      /**< Full test path */

    test_iters      iters;          /**< Iterations of the test */

    /* Fields specific for TRC diff */
    te_bool         diff_out;       /**< Should the test be output */
    te_bool         diff_out_iters; /**< Should the test iterations
                                         be output */
    /** The expected results */
    trc_test_result     diff_exp[TRC_DIFF_IDS];
    /** The expected verdicts */
    tqh_string         *diff_verdicts[TRC_DIFF_IDS];
} test_run;


/** Testing results comparison database */
typedef struct trc_database {
    char       *version;    /**< Database version */
    test_runs   tests;      /**< Tree of tests */
    trc_stats   stats;      /**< Grand total statistics */
} trc_database;


/** Testing results comparison database */
extern trc_database trc_db;


/**
 * Parse XML file with expected testing results database.
 *
 * @param filename      Name of the file with DB
 *
 * @return Status code
 */
extern int trc_parse_db(const char *filename);

/**
 * Dump database with expected results.
 *
 * @param filename      Name of the file to save to
 * @param init          Is it DB initialization?
 *
 * @return Status code
 */
extern int trc_dump_db(const char *filename, te_bool init);

/**
 * Parse TE log in XML format.
 *
 * @param filename      Name of the file with log
 *
 * @return Status code
 */
extern int trc_parse_log(const char *filename);

/**
 * Free TRC resources allocated for TRC database.
 *
 * @param db        Database to be freed
 */
extern void trc_free_db(trc_database *db);

/** Output flags */
enum trc_out_flags {
    TRC_OUT_NO_TOTAL_STATS      = 0x01,
    TRC_OUT_NO_PACKAGES_ONLY    = 0x02,
    TRC_OUT_NO_SCRIPTS          = 0x04,
    TRC_OUT_STATS_ONLY          = 0x08,
    TRC_OUT_NO_UNSPEC           = 0x10,
    TRC_OUT_NO_SKIPPED          = 0x20,
    TRC_OUT_NO_EXP_PASSED       = 0x40,
    TRC_OUT_NO_EXPECTED         = 0x80,
};

/**
 * Prepare TRC report in HTML format.
 *
 * @param filename      Name of the file to put report
 * @param header        File with HTML header or NULL
 * @param db            DB to be processed
 * @param flags         Flags to control content (trc_out_flags)
 *
 * @return Status code
 */
extern int trc_report_to_html(const char *filename, FILE *header,
                              trc_database *db, unsigned int flags);

/**
 * Prepare TRC diff report in HTML format.
 *
 * @param db            DB to be processed
 * @param flags         Report options
 * @param filename      Name of the file to put report
 *
 * @return Status code
 */
extern int trc_diff_report_to_html(trc_database *db, unsigned int flags,
                                   const char *filename);

/**
 * Output verdicts to HTML.
 *
 * @param f         File
 * @param verdicts  Verdicts
 */
static inline void
trc_verdicts_to_html(FILE *f, const tqh_string *verdicts)
{
    const tqe_string   *v;

    if (verdicts == NULL || verdicts->tqh_first == NULL)
        return;

    fputs("<BR/><BR/>", f);
    for (v = verdicts->tqh_first; v != NULL; v = v->links.tqe_next)
    {
        if (v != verdicts->tqh_first)
            fputs("; ", f);
        fputs(v->str, f);
    }
}

#endif /* !__TE_TRC_DB_H__ */
