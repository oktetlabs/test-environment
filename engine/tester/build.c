/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Interaction with Builder.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Build"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_errno.h"
#include "te_queue.h"
#include "logger_api.h"
#include "te_builder_ts.h"

#include "tester_flags.h"
#include "tester_build.h"


/* See description in tester_build.h */
te_errno
tester_build_suite(const test_suite_info *suite, te_bool verbose)
{
    te_errno rc;

    RING("Build Test Suite '%s' from '%s'",
         suite->name, suite->src);
    rc = builder_build_test_suite(suite->name, suite->src);
    if (rc != 0)
    {
        const char *pwd = getenv("PWD");

        ERROR("Build of Test Suite '%s' from '%s' failed, see "
              "%s/builder.log.%s.{1,2}",
              suite->name, suite->src, pwd, suite->name);
        if (verbose)
        {
            fprintf(stderr,
                    "Build of Test Suite '%s' from '%s' failed, see\n"
                    "%s/builder.log.%s.{1,2}\n",
                    suite->name, suite->src, pwd, suite->name);
        }
        return rc;
    }
    return 0;
}

/* See description in tester_build.h */
te_errno
tester_build_suites(const test_suites_info *suites, te_bool verbose)
{
    te_errno                rc;
    const test_suite_info  *suite;

    TAILQ_FOREACH(suite, suites, links)
    {
        if (suite->src != NULL)
        {
            rc = tester_build_suite(suite, verbose);
            if (rc != 0)
                return rc;
        }
    }
    return 0;
}
