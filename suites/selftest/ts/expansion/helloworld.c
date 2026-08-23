/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Objective expansion test
 *
 * A test that does nothing but be run, so that the objective the
 * Tester logs for it can be looked at.
 */

/** @page expansion_helloworld Objective expansion test
 *
 * @objective Demo of the objective that package.xml overrides.
 *
 * The test itself does nothing: what is being checked is the
 * objective the Tester logs for every iteration of it, which the
 * package file writes with variable references in it.
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
