/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Minimal test
 *
 * Minial test scenario.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page minimal_helloworld Hello World test
 *
 * @objective Demo of minima Hello World test
 *
 * For each test @p TEST_STEP() is required. This need to generate
 * documetation of test steps.
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME    "helloworld"

#include "te_config.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Print \"Hello, World!\"");
    RING("Hello, World!");

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
