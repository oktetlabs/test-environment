/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Minimal test command monitor
 *
 * Minimal test scenario that just log test iterations.
 *
 * Copyright (C) 2024 ARK NETWORKS LLC. All rights reserved.
 */

/** @page monitors_test_mon Trivial test command monitor
 *
 * @objective Demo of test command monitor
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME "test_mon"

#include "te_config.h"
#include "tapi_test.h"

static bool stop = false;

static void
test_sigusr1_handler(int signum)
{
    if (signum == SIGUSR1)
    {
        RING("Handle SIGUSR1 to carefully stop test command monitor");
        stop = true;
    }
    else
    {
        TEST_FAIL("Failed to handle unexpected signal \"%s\" (%d)",
                  strsignal(signum), signum);
    }
}

int
main(int argc, char **argv)
{
    int i;

    TEST_START;

    TEST_STEP("Register handler to stop test command monitor");
    signal(SIGUSR1, test_sigusr1_handler);

    TEST_STEP("Dump test iteration while session is active");
    for (i = 0; !stop; i++)
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
