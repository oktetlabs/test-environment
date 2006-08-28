/** @file
 * @brief Tester Subsystem
 *
 * Test execution result representation and auxiluary routes
 * declaration.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * $Id: tester_defs.h 29007 2006-06-09 15:12:42Z arybchik $
 */

#ifndef __TE_TESTER_RESULT_H__
#define __TE_TESTER_RESULT_H__

#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_queue.h"
#include "logger_api.h"
#include "te_test_result.h"

#include "tester_defs.h"


/**
 * Result of the test execution.
 */
typedef struct tester_test_result {
    LIST_ENTRY(tester_test_result)  links;  /**< List links */

    test_id         id;         /**< Test ID */
    te_test_result  result;     /**< Result */
    trc_verdict     expected;   /**< Is the result expected? */
} tester_test_result;

/** List of results of tests which are in progress. */
typedef struct tester_test_results {
    /** Head of the list */
    LIST_HEAD(, tester_test_result) list;
    /**
     * Mutual exclusion device to protect the list used in Tester main
     * thread and verdicts listener.
     */
    pthread_mutex_t lock;
} tester_test_results;

/* Forward declaration */
struct tester_verdicts_listener;
/** Verdicts listener control data. */
typedef struct tester_verdicts_listener tester_verdicts_listener;


/**
 * Initialize the list of results.
 *
 * @param results       List to be initialized
 *
 * @return Status code.
 */
static inline te_errno
tester_test_results_init(tester_test_results *results)
{
    LIST_INIT(&results->list);

    if (pthread_mutex_init(&results->lock, NULL) != 0)
    {
        te_errno rc = TE_OS_RC(TE_TESTER, errno);

        ERROR("%s(): pthread_mutex_init() failed: %r",
              __FUNCTION__, rc);
        return rc;
    }

    return 0;
}

/**
 * Insert a new test results in the list.
 *
 * @param results       List with location for results of the tests
 *                      which are in progress
 * @param result        Location for a new test
 */
static inline void
tester_test_result_add(tester_test_results *results,
                       tester_test_result *result)
{
    pthread_mutex_lock(&results->lock);
    LIST_INSERT_HEAD(&results->list, result, links);
    pthread_mutex_unlock(&results->lock);
}

/**
 * Delete test results from the list.
 *
 * @param results       List with location for results of the tests
 *                      which are in progress
 * @param result        Result to be extracted
 */
static inline void
tester_test_result_del(tester_test_results *results,
                       tester_test_result *result)
{
    pthread_mutex_lock(&results->lock);
    LIST_REMOVE(result, links);
    pthread_mutex_unlock(&results->lock);
}

/**
 * Start verdicts listener.
 *
 * @param ctx           Location for verdicts listener control data
 * @param results       List of tests which are in progress to store
 *                      received verdicts
 *
 * @return Status code.
 */
extern te_errno tester_verdicts_listener_start(
                    tester_verdicts_listener **ctx,
                    tester_test_results       *results);

/**
 * Stop verdicts listener.
 *
 * @param ctx           Location of verdicts listener control data
 *
 * @return Status code.
 */
extern te_errno tester_verdicts_listener_stop(
                    tester_verdicts_listener **ctx);

#endif /* !__TE_TESTER_RESULT_H__ */
