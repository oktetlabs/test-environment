/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) OKTET Ltd. All rights reserved. */
/** @file
 * @brief Test for default arguments in package.xml
 *
 * Testing default arguments correctness
 */

/** @page tools_default_args default args test
 *
 * @objective Testing default arguments correctness
 *
 * Do some sanity checks that default arguments in package.xml work
 * correctly.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "default_args"

#include "te_config.h"
#include <string.h>
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    int test_arg1 = -1;
    int test_arg2 = -1;

    TEST_START;
    TEST_GET_INT_PARAM(test_arg1);
    TEST_GET_INT_PARAM(test_arg2);

    TEST_STEP("Print values of default arguments");

    RING("Argument test_arg1 has value: %d, test_arg2 has value: %d",
         test_arg1, test_arg2);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
