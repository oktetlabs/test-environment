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

#ifndef __TE_TOOLS_TRC_DB_H__
#define __TE_TOOLS_TRC_DB_H__

#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#include <libxml/tree.h>

#include "te_defs.h"


/** Enumeration of possible test results */
typedef enum trc_test_result {
    TRC_TEST_PASSED,      /**< Test should pass */
    TRC_TEST_FAILED,      /**< Test should fail */
    TRC_TEST_CORED,
    TRC_TEST_KILLED,
    TRC_TEST_FAKED,
    TRC_TEST_SKIPPED,     /**< Test should be skipped */
    TRC_TEST_UNSPEC       /**< Expected test result is not specified yet */
} trc_test_result;

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
    xmlNodePtr                       node;   /**< XML node with this element */
} test_args;


/* Forward */
struct test_run;

/** Head of the list with tests */
typedef struct test_runs {
    TAILQ_HEAD(_test_runs, test_run) head;;
    xmlNodePtr                       node;   /**< XML node with this element */
} test_runs;


/** Test iteration */
typedef struct test_iter {
    TAILQ_ENTRY(test_iter)  links;  /**< List links */

    xmlNodePtr      node;           /**< XML node with this element */
    trc_stats       stats;          /**< Statistics */

    test_args       args;           /**< Iteration arguments */
    trc_test_result exp_result;     /**< The expected result */
    char           *notes;          /**< Some notes */
    test_runs       tests;          /**< Children tests of the session */

    trc_test_result got_result;     /**< Got test result */
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
    char           *name;           /**< Test name */
    char           *notes;          /**< Some notes */

    char           *objective;      /**< Test objective */
    xmlNodePtr      obj_node;       /**< XML node with objective */
    te_bool         obj_update;     /**< Whether objective of the test
                                         should be updated */
    char           *obj_link;       /**< Test objective link */

    test_iters      iters;          /**< Iterations of the test */
} test_run;


/** Testing results comparison database */
typedef struct trc_database {
    test_runs   tests;
    trc_stats   stats;
} trc_database;


/** Testing results comparison database */
extern trc_database trc_db;
/** Should database be update */
extern te_bool trc_update_db;
/** Should database be initialized from scratch */
extern te_bool trc_init_db;


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
 *
 * @return Status code
 */
extern int trc_dump_db(const char *filename);

/**
 * Parse TE log in XML format.
 *
 * @param filename      Name of the file with log
 *
 * @return Status code
 */
extern int trc_parse_log(const char *filename);

/**
 * Prepare TRC report in HTML format.
 *
 * @param filename      Name of the file to put report
 * @param db            DB to be processed
 */
extern int trc_report_to_html(const char *filename, trc_database *db);

#endif /* !__TE_TOOLS_TRC_DB_H__ */
