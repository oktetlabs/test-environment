/** @file
 * @brief A sample test with environment
 *
 * Get the environment
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 */

/** @page minimal_env Get the environment
 *
 * @objective Get the environment
 *
 * @par Scenario:
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 */

#define TE_TEST_NAME "env"

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
#include <net/if.h>

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    const struct if_nameindex  *iut_if = NULL;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_IF(iut_if);

    RING("IUT interface is %s", iut_if->if_name);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
