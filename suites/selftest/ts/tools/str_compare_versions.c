/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs. All rights reserved. */
/** @file
 * @brief Test for version comparison function.
 *
 * Testing version comparison function.
 */

/** @page tools_str_compare_versions te_str_compare_versions test
 *
 * @objective Testing version comparison function.
 *
 * Test the correctness of te_str_compare_versions function.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/str_compare_versions"

#include "te_config.h"
#include "tapi_test.h"
#include "te_str.h"

static const char*
versions_comparison_res_to_str(int relation)
{
    return relation > 0 ? ">" : relation < 0 ? "<" : "=";
}

/**
 * Does comparison and check the result with expectation.
 *
 * @param v1     First version.
 * @param expect String representation of expected relationship.
 * @param v2     Second version.
 *
 */
static void
check_str_versions_comparison(const char* v1, const char* expect,
                              const char* v2)
{
    int actual;

    CHECK_RC(te_str_compare_versions(v1, v2, &actual));

    if (strcmp(expect, versions_comparison_res_to_str(actual)) != 0)
    {
        TEST_VERDICT("Got wrong relationship between the versions: "
                     "expected '%s' %s '%s' but got '%s' %s '%s'",
                     v1, expect, v2,
                     v1, versions_comparison_res_to_str(actual), v2);
    }
}

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Check that version 1 is earlier than version 2");
    check_str_versions_comparison("", "<", "1");
    check_str_versions_comparison("1", "<", "2");
    check_str_versions_comparison("1.0", "<", "2");
    check_str_versions_comparison("2.1", "<", "2.2");
    check_str_versions_comparison("1.1", "<", "1.1.1");
    check_str_versions_comparison("1.2", "<", "1.11");
    check_str_versions_comparison("1.00", "<", "1.01");
    check_str_versions_comparison("1.1.0", "<", "1.1.1");
    check_str_versions_comparison("1.0.1", "<", "1.0.02");
    check_str_versions_comparison("1.10.0", "<", "1.10.1");
    check_str_versions_comparison("10.10.20", "<", "10.10.101");
    check_str_versions_comparison("10.10.100", "<", "10.10.101");

    TEST_STEP("Check that version 1 is later than version 2");
    check_str_versions_comparison("1", ">", "");
    check_str_versions_comparison("2", ">", "1");
    check_str_versions_comparison("2", ">", "1.0");
    check_str_versions_comparison("1.2", ">", "1.1");
    check_str_versions_comparison("1.1.1", ">", "1.1");
    check_str_versions_comparison("1.11", ">", "1.2");
    check_str_versions_comparison("1.01", ">", "1.00");
    check_str_versions_comparison("1.1.1", ">", "1.1.0");
    check_str_versions_comparison("1.0.02", ">", "1.0.1");
    check_str_versions_comparison("1.10.1", ">", "1.10.0");
    check_str_versions_comparison("10.10.101", ">", "10.10.20");
    check_str_versions_comparison("10.10.101", ">", "10.10.100");

    TEST_STEP("Check that version 1 is the same as version 2");
    check_str_versions_comparison("", "=", "");
    check_str_versions_comparison("1", "=", "1");
    check_str_versions_comparison("0.1", "=", "0.1");
    check_str_versions_comparison("0.0.1", "=", "0.0.1");
    check_str_versions_comparison("0.0.0", "=", "0.0.0");
    check_str_versions_comparison("00.00.00", "=", "00.00.00");

    TEST_SUCCESS;

cleanup:

    TEST_END;
}