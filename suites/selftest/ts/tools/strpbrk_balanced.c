/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for balanced strpbrk() equivalents
 *
 * Testing balanced strpbrk() equivalents.
 */

/** @page tools_strpbrk_balanced te_strpbrk_balanced() test
 *
 * @objective Testing balanced strpbrk() equivalents.
 *
 * Test the correctness of balanced strpbrk() equivalents.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/strpbrk_balanced"

#include <string.h>
#include "te_config.h"

#include "tapi_test.h"
#include "te_string.h"
#include "te_str.h"

int
main(int argc, char **argv)
{
    const char *opening;
    const char *closing;
    const char *escape;
    const char *separator;
    const char *input;
    tapi_test_expected_result leftmost;
    tapi_test_expected_result rightmost;
    const char *actual;
    te_errno status;

    TEST_START;
    TEST_GET_STRING_PARAM(opening);
    TEST_GET_STRING_PARAM(closing);
    TEST_GET_OPT_STRING_PARAM(escape);
    TEST_GET_OPT_STRING_PARAM(separator);
    TEST_GET_STRING_PARAM(input);
    TEST_GET_EXPECTED_RESULT_PARAM(leftmost);
    TEST_GET_EXPECTED_RESULT_PARAM(rightmost);

    TEST_STEP("Checking leftmost search");
    status = te_strpbrk_balanced(input, *opening, *closing,
                                 escape == NULL ? '\0' : *escape,
                                 separator, &actual);
    if (!tapi_test_check_expected_result(&leftmost, status, actual))
        TEST_VERDICT("Unexpected leftmost result");

    TEST_STEP("Checking rightmost search");
    status = te_strpbrk_rev_balanced(input, *opening, *closing,
                                     escape == NULL ? '\0' : *escape,
                                     separator, &actual);
    if (!tapi_test_check_expected_result(&rightmost, status, actual))
        TEST_VERDICT("Unexpected rightmost result");

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
