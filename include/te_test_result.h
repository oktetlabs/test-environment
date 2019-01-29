/** @file
 * @brief Test Environment
 *
 * Test result representation.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Statuses of test execution visible for the world.
 *
 * @attention Order of statuses is important and used by TE modules.
 */
typedef enum te_test_status {
    TE_TEST_INCOMPLETE = 0, /**< Test execution has not been finished */

    TE_TEST_UNSPEC,     /**< Unspecified result in term of TRC */
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
        TE_TEST_STATUS_TO_STR(UNSPEC);
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

/** List of verdicts. */
typedef TAILQ_HEAD(te_test_verdicts, te_test_verdict) te_test_verdicts;

/**
 * Test result representation: status plus verdicts.
 */
typedef struct te_test_result {
    te_test_status      status;     /**< Status of the test execution */
    te_test_verdicts    verdicts;   /**< Verdicts generated by the test
                                         during execution */
    te_test_verdicts    artifacts;  /**< Test artifacts (similar to verdicts
                                         but not taken into account when
                                         test results are matched to TRC
                                         database) */
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
    TAILQ_INIT(&result->artifacts);
}

/** Free resources allocated for test result verdict */
static inline void
te_test_result_free_verdict(te_test_verdict *verdict)
{
    free(verdict->str);
    free(verdict);
}

/** Free resourses allocated for test result verdicts. */
static inline void
te_test_result_free_verdicts(te_test_verdicts *verdicts)
{
    te_test_verdict *v;

    if (verdicts == NULL)
        return;

    while ((v = TAILQ_FIRST(verdicts)) != NULL)
    {
        TAILQ_REMOVE(verdicts, v, links);
        te_test_result_free_verdict(v);
    }
}

/**
 * Clean memory allocated for te_test_result members.
 *
 * @param result    TE test result
 */
static inline void
te_test_result_clean(te_test_result *result)
{
    if (result == NULL)
        return;

    te_test_result_free_verdicts(&result->verdicts);
    te_test_result_free_verdicts(&result->artifacts);
}

/**
 * Free test result.
 *
 * @param result    TE test result
 */
static inline void
te_test_result_free(te_test_result *result)
{
    te_test_result_clean(result);
    free(result);
}

/**
 * Copy test verdicts.
 *
 * @param dst       Where copied verdicts should be placed
 *                  (should be initialized already).
 * @param src       From where to copy verdicts.
 *
 * @return Status code.
 */
static inline te_errno
te_test_result_verdicts_cpy(te_test_verdicts *dst, te_test_verdicts *src)
{
    te_test_verdict *v;
    te_test_verdict *dup_verdict;

    TAILQ_FOREACH(v, src, links)
    {
       dup_verdict = calloc(1, sizeof(te_test_verdict));
       if (dup_verdict == NULL)
            return TE_ENOMEM;

       if (v->str != NULL)
       {
           dup_verdict->str = strdup(v->str);
           if (dup_verdict->str == NULL)
           {
              free(dup_verdict);
              return TE_ENOMEM;
           }
       }
       TAILQ_INSERT_TAIL(dst, dup_verdict, links);
    }

    return 0;
}


/**
 * Duplicate test result.
 *
 * @param result    Test result
 *
 * @return
 *      Copy of result
 */
static inline te_test_result *
te_test_result_dup(te_test_result *result)
{
    te_test_result  *dup;

    if (result == NULL)
        return NULL;

    dup = calloc(1, sizeof(te_test_result));
    dup->status = result->status;

    TAILQ_INIT(&dup->verdicts);
    TAILQ_INIT(&dup->artifacts);

    if (te_test_result_verdicts_cpy(&dup->verdicts,
                                    &result->verdicts) != 0 ||
        te_test_result_verdicts_cpy(&dup->artifacts,
                                    &result->artifacts) != 0)
    {
        te_test_result_free(dup);
        return NULL;
    }

    return dup;
}

/**
 * Copy test result.
 *
 * @param dest    Where to copy (will be initialized by this function)
 * @param src     What to copy
 */
static inline void
te_test_result_cpy(te_test_result *dest, te_test_result *src)
{
    te_test_result_init(dest);
    dest->status = src->status;

    te_test_result_verdicts_cpy(&dest->verdicts, &src->verdicts);
    te_test_result_verdicts_cpy(&dest->artifacts, &src->artifacts);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TEST_RESULT_H__ */
