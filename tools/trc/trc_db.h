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
typedef enum test_result_e {
    TEST_PASS,      /**< Test should pass */
    TEST_FAIL,      /**< Test should fail */
    TEST_UNSPEC     /**< Expected test result is not specified yet */
} test_result;


/** Testing results conparator statistics */
typedef struct trc_stats {
    unsigned int    not_run;
    unsigned int    new_run;
    unsigned int    skipped;
    unsigned int    pass_exp;
    unsigned int    pass_une;
    unsigned int    fail_exp;
    unsigned int    fail_une;
} trc_stats;


/** Test argument */
typedef struct test_arg {
    TAILQ_ENTRY(test_arg)   links;  /**< List links */
    
    xmlNodePtr  node;   /**< XML node with this element */

    char       *name;   /**< Argument name */
    char       *value;  /**< Argument value */
} test_arg;

/** Head of the list with test arguments */
typedef TAILQ_HEAD(test_args, test_arg) test_args;


/* Forward */
struct test_run;

/** Head of the list with tests */
typedef TAILQ_HEAD(test_runs, test_run) test_runs;


/** Test iteration */
typedef struct test_iter {
    TAILQ_ENTRY(test_iter)  links;  /**< List links */

    xmlNodePtr      node;           /**< XML node with this element */
    trc_stats       stats;          /**< Statistics */

    test_args       args;           /**< Iteration arguments */
    unsigned int    n;              /**< Number of iterations
                                         with the same parameters */
    test_result     result;         /**< The expected result */
    char           *notes;          /**< Some notes */

    test_runs       tests;          /**< Children tests of the session */
} test_iter;

/** Head of the list with test iterations */
typedef TAILQ_HEAD(test_iters, test_iter) test_iters;


/** Test run */
typedef struct test_run {
    TAILQ_ENTRY(test_run)   links;  /**< List links */

    xmlNodePtr      node;           /**< XML node with this element */
    trc_stats       stats;          /**< Statistics */

    char           *name;           /**< Test name */
    char           *objective;      /**< Test objective */
    te_bool         is_package;     /**< Is it a package or a session? */

    test_iters      iters;          /**< Iterations of the test */
} test_run;


/** Testing results comparison database */
typedef struct trc_database {
    test_runs   tests;
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
 *
 * @return Status code
 */
extern int trc_dump_db(const char *filename);

#endif /* !__TE_TOOLS_TRC_DB_H__ */
