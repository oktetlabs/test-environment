/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Data types and API to interact with Builder.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TESTER_BUILD_H__
#define __TE_TESTER_BUILD_H__

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Information about Test Suite */
typedef struct test_suite_info {
    TAILQ_ENTRY(test_suite_info)    links;  /**< List links */

    char   *name;   /**< Name of the Test Suite */
    char   *src;    /**< Path where Test Suite sources are located */
    char   *bin;    /**< Path where Test Suite executables are located */
} test_suite_info;

/** Head of the list with information about Test Suites */
typedef TAILQ_HEAD(test_suites_info, test_suite_info) test_suites_info;


/**
 * Free test suite information.
 *
 * @param p         Information about test suite
 */
static inline void
test_suite_info_free(test_suite_info *p)
{
    free(p->name);
    free(p->src);
    free(p->bin);
    free(p);
}

/**
 * Free list of test suites information.
 *
 * @param suites    List of with test suites information
 */
static inline void
test_suites_info_free(test_suites_info *suites)
{
    test_suite_info *p;

    while ((p = TAILQ_FIRST(suites)) != NULL)
    {
        TAILQ_REMOVE(suites, p, links);
        test_suite_info_free(p);
    }
}

/**
 * Build Test Suite.
 *
 * @param suite         Test Suites
 * @param verbose       Be verbose in the case of build failure
 *
 * @return Status code.
 */
extern te_errno tester_build_suite(const test_suite_info *suite,
                                   te_bool                verbose);

/**
 * Build list of Test Suites.
 *
 * @param suites    List of Test Suites
 * @param verbose       Be verbose in the case of build failure
 *
 * @return Status code.
 */
extern te_errno tester_build_suites(const test_suites_info *suites,
                                    te_bool                 verbose);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_BUILD_H__ */
