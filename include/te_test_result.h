/** @file
 * @brief Test Environment
 *
 * Test result representation.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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

#ifndef __TE_TEST_RESULT_H__
#define __TE_TEST_RESULT_H__

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Statuses of test execution visible for the world.
 */
typedef enum te_test_status {
    TE_TEST_INCOMPLETE = 0, /**< Test execution has not been finished */

    TE_TEST_EMPTY,      /**< Session is empty */
    TE_TEST_SKIPPED,    /**< The test is skipped because of target
                             requirements specified by user */
    TE_TEST_FAKED,      /**< Test execution is just faked by the Tester */
    TE_TEST_PASSED,     /**< Discovered IUT behaviour is correct from
                             the test point of view */
    TE_TEST_FAILED,     /**< Discovered IUT behaviour is incorrect
                             from test point of view or some internal
                             error occur (test executable not found,
                             unexpected configuration changes, etc) */

    TE_TEST_STATUS_MAX  /**< Dummy test status */
} te_test_status;

/**
 * Convert test status to string representation.
 *
 * @param status        Test status
 *
 * @return Pointer to string representation.
 */
static inline const char *
te_test_status_to_str(te_test_status status)
{
    switch (status)
    {
#define TE_TEST_STATUS_TO_STR(_status) \
        case TE_TEST_ ## _status:   return #_status;

        TE_TEST_STATUS_TO_STR(INCOMPLETE);
        TE_TEST_STATUS_TO_STR(EMPTY);
        TE_TEST_STATUS_TO_STR(SKIPPED);
        TE_TEST_STATUS_TO_STR(FAKED);
        TE_TEST_STATUS_TO_STR(PASSED);
        TE_TEST_STATUS_TO_STR(FAILED);

#undef TE_TEST_STATUS_TO_STR

        default:
            assert(0);
            return "<UNKNOWN>";
    }
}

/**
 * Verdict generated by the test during execution.
 */
typedef struct te_test_verdict {
    TAILQ_ENTRY(te_test_verdict)    links;  /**< Links to create queue
                                                 of verdicts */
    char                           *str;    /**< Verdict string */
} te_test_verdict;

/**
 * Test result representation: status plus verdicts.
 */
typedef struct te_test_result {
    te_test_status                  status;     /**< Status of the test
                                                     execution */
    TAILQ_HEAD(, te_test_verdict)   verdicts;   /**< Verdicts generated
                                                     by the test during
                                                     execution */
} te_test_result;


/** Verdicts generated by Testing Results Comparator */
typedef enum trc_verdict {
    TRC_VERDICT_UNKNOWN,    /**< Test/iteration is unknown for TRC */
    TRC_VERDICT_UNEXPECTED, /**< Obtained result is equal to nothing
                                 in set of expected results */
    TRC_VERDICT_EXPECTED,   /**< Obtained result is equal to one of
                                 expected results */
} trc_verdict;


/** Initialize test result by defaults. */
static inline void
te_test_result_init(te_test_result *result)
{
    result->status = TE_TEST_INCOMPLETE;
    TAILQ_INIT(&result->verdicts);
}

/** Free resourses allocated for test result verdicts. */
static inline void
te_test_result_free_verdicts(te_test_result *result)
{
    te_test_verdict *v;

    while ((v = result->verdicts.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&result->verdicts, v, links);
        free(v->str);
        free(v);
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TEST_RESULT_H__ */
