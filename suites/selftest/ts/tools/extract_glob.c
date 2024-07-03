/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for a te_file_extract_glob() function
 *
 * Testing te_file_extract_glob() correctness
 */

/** @page tools_extract_glob te_file_extract_glob test
 *
 * @objective Testing te_file_extract_glob() correctness.
 *
 * Test that te_file_extract_glob() correctly extracts parts of filenames.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/extract_glob"

#include "te_config.h"

#include <unistd.h>
#include <fnmatch.h>

#include "tapi_test.h"
#include "te_str.h"
#include "te_file.h"

static void
do_extract(bool is_basename, const char *filename, const char *pattern,
           const char *expect)
{
    char *result = te_file_extract_glob(filename, pattern, is_basename);

    if (result == NULL)
    {
        if (expect != NULL)
            TEST_VERDICT("Nothing is extracted, expected '%s'", expect);
    }
    else
    {
        if (expect == NULL)
            TEST_VERDICT("Nothing is expected, extracted '%s'", result);
        if (strcmp(expect, result) != 0)
            TEST_VERDICT("Expected '%s', extracted '%s'", expect, result);

        free(result);
    }
}


int
main(int argc, char **argv)
{
    const char *filename;
    const char *pattern;
    const char *expect_full;
    const char *expect_base;


    TEST_START;

    TEST_GET_STRING_PARAM(filename);
    TEST_GET_STRING_PARAM(pattern);
    TEST_GET_STRING_PARAM(expect_full);
    TEST_GET_STRING_PARAM(expect_base);

    if (strcmp(expect_full, "NULL") == 0)
        expect_full = NULL;
    if (strcmp(expect_base, "NULL") == 0)
        expect_base = NULL;

    TEST_STEP("Checking full pathname");
    do_extract(false, filename, pattern, expect_full);

    TEST_STEP("Checking basename");
    do_extract(true, filename, pattern, expect_base);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
