/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Minimal test
 *
 * Minial test scenario that just log test iterations.
 *
 * Copyright (C) 2024 ARK NETWORKS LLC. All rights reserved.
 */

/** @page monitors_trivial Trivial test to dump a coule test iterations
 *
 * @objective Auxiliary test to validate test command monitor
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME "trivial"

#include "te_config.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    int i;

    TEST_START;

    TEST_STEP("Dump some test iterations");
    for (i = 0; i < 3; i++)
    {
        RING("Test iteration #%d", i);
        te_motivated_msleep(100, "between test iteration");
    }

    TEST_STEP("Finish the test");

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
