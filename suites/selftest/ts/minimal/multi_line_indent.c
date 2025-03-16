/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Minimal test
 *
 * Minial test scenario that checks indentation stripping.
 */

/** @page minimal_multi_line_indent Multi-line indentation test
 *
 * @objective Check that indentation of multi-line is stripped correctly by TE.
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME    "multi_line_indent"

#include "te_config.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    const char *test_line;
    const char *check_line = "first\n"
                             "\n"
                             "    sub-line\n"
                             "second";
    TEST_START;

    TEST_GET_STRING_PARAM(test_line);

    TEST_STEP("Print the value of 'test_line' parameter");
    RING("Value of 'test_line' param is: %s", test_line);

    if (strcmp(test_line, check_line) != 0)
        TEST_VERDICT("Incorrect value of 'test_line' after stripping "
                     "indentation");

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
