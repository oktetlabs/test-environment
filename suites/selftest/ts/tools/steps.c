/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Ltd. */
/** @file
 * @brief Test for scenario step macros
 *
 * Exercise nested and conditional scenario steps.
 */

/** @page tools_steps Scenario step macros test
 *
 * @objective Exercise every scenario step shape: substeps, pushed
 *            step groups several levels deep, steps in loops and
 *            conditionals, and a reset.
 *
 * The test does nothing but walk its own scenario; it exists as a
 * living fixture for the tools that read scenarios back out of the
 * source and the log: the doxygen filter, tests-info.xml generation
 * and the scenario drift check.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/steps"

#include "te_config.h"

#include "tapi_test.h"

int
main(int argc, char *argv[])
{
    unsigned int loops;
    bool deep;
    unsigned int i;

    TEST_START;
    TEST_PARAM_DOC(loops,
        "How many exchange rounds to walk through:",
        "- `1`: a single round",
        "- `3`: a short burst");
    TEST_GET_UINT_PARAM(loops);
    TEST_PARAM_DOC(deep, "Whether to descend into the deep branch");
    TEST_GET_BOOL_PARAM(deep);

    TEST_STEP("Prepare the %u-round exchange", loops);
    TEST_SUBSTEP("Reset counters");
    TEST_SUBSTEP("Arm the timer");

    TEST_STEP("Run the exchange");
    for (i = 0; i < loops; i++)
    {
        TEST_SUBSTEP("Announce round %u", i);
        TEST_STEP_PUSH("Validate round %u", i);
        TEST_STEP_PUSH("Check the header");
        TEST_STEP_NEXT("Check the payload");
        TEST_STEP_POP("");
        TEST_STEP_POP("");
    }

    if (deep)
    {
        TEST_STEP("Descend the deep branch");
        TEST_STEP_PUSH("Level two");
        TEST_STEP_PUSH("Level three");
        TEST_STEP_PUSH("Level four");
        TEST_STEP_POP("");
        TEST_STEP_POP("");
        TEST_STEP_POP("");
    }
    else
    {
        TEST_STEP("Stay on the shallow path");
    }

    TEST_STEP_RESET();
    TEST_STEP("Wrap up after the reset");

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
