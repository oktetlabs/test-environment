/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2024 OKTET Ltd. All rights reserved. */
/** @file
 * @brief TS-provided subtree
 */

/** @page ts_subtree TS-provided subtree
 *
 * @objective Check that TS-provided subtrees work correctly
 *
 * This test suite includes a library that, when linked to a TA,
 * should export an additional subtree for that agent.
 *
 * This functionality has only been implemented for Unix TAs, so
 * the test is going to skip other types of agents.
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "cs/ts_subtree"

#ifndef TEST_START_VARS
#define TEST_START_VARS TEST_START_ENV_VARS
#endif

#ifndef TEST_START_SPECIFIC
#define TEST_START_SPECIFIC TEST_START_ENV
#endif

#ifndef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC TEST_END_ENV
#endif

#include "te_config.h"
#include "tapi_cfg.h"
#include "tapi_test.h"
#include "tapi_env.h"

static te_errno
check_subtree(const char *ta, void *cookie)
{
    te_errno  rc;
    char     *value = NULL;

    UNUSED(cookie);

    rc = cfg_get_string(&value, "/agent:%s/ts_lib_helloworld:", ta);
    if (rc != 0)
    {
        ERROR("Failed to get the ts_lib_helloworld instance from %s: %r", ta, rc);
        return rc;
    }

    RING("Successfully got '%s' from TA %s", value, ta);

    free(value);
    return 0;
}

int
main(int argc, char **argv)
{
    TEST_START;

    CHECK_RC(rcf_foreach_ta(check_subtree, NULL));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
