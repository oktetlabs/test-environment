/** @file
 * @brief Minimal test
 *
 * Minial test scenario that yields a verdict.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page minimal_verdict Verdict test
 *
 * @objective Demo of verdict registering
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME    "verdict"

#include "te_config.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Register verdicts");
    RING_VERDICT("Test verdict 1");
    RING_VERDICT("Test verdict 2");

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
