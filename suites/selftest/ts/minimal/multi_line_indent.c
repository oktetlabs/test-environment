/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Minimal test
 *
 * Minial test scenario.
 *
 * Copyright (C) 2025 OKTET Ltd. All rights reserved.
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
    TEST_START;

    TEST_GET_STRING_PARAM(test_line);

    TEST_STEP("Print the value of 'test_line' parameter");
    RING("Value of 'test_line11' param is: %s", test_line);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
