/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * A test example
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page example An example test
 *
 *  @objective Example smells example whether it's called example or not.
 *
 */

#include "te_config.h"
#include "tapi_test.h"

#define TE_TEST_NAME "example"

int
main(int argc, char *argv[])
{
    TEST_START;

    RING("All is fine");
    TEST_SUCCESS;

cleanup:


    TEST_END;
}
