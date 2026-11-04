/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief A test binding an env net by its name
 *
 * Check that an env net named after a configuration net binds to it
 *
 * Copyright (C) 2026 OKTET Labs Ltd. All rights reserved.
 */

/** @page minimal_env_by_name Bind an env net by its name
 *
 * @objective Check that with @c TE_ENV_BIND_BY_NAME=yes the env net
 *            @c net2 is bound to the configuration net @c /net:net2
 *            and not to @c /net:net1, which fits as well and comes
 *            first.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "env_by_name"

#ifndef TEST_START_VARS
/**
 * Test suite specific variables of the test @b main() function.
 */
#define TEST_START_VARS TEST_START_ENV_VARS
#endif

#ifndef TEST_START_SPECIFIC
/**
 * Test suite specific the first actions of the test.
 */
#define TEST_START_SPECIFIC TEST_START_ENV
#endif

#ifndef TEST_END_SPECIFIC
/**
 * Test suite specific part of the last action of the test @b main()
 * function.
 */
#define TEST_END_SPECIFIC TEST_END_ENV
#endif

#include "tapi_test.h"
#include "tapi_env.h"

int
main(int argc, char **argv)
{
    const char              prefix[] = "/net:net2/";
    const tapi_env_if      *iut_if = NULL;
    const cfg_net_node_t   *node;
    char                   *oid = NULL;

    TEST_START;

    TEST_STEP("Get the configuration net node of @p iut_if");
    TEST_GET_ENV_IF(iut_if);
    node = tapi_env_get_if_net_node(iut_if);
    CHECK_NOT_NULL(node);
    CHECK_RC(cfg_get_oid_str(node->handle, &oid));
    RING("Interface 'iut_if' is bound to %s", oid);

    TEST_STEP("Check that the node belongs to @c /net:net2");
    if (strncmp(oid, prefix, strlen(prefix)) != 0)
        TEST_FAIL("Env net 'net2' is bound to %s, not to /net:net2", oid);

    TEST_SUCCESS;

cleanup:
    free(oid);

    TEST_END;
}
