/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI File Test Suite.
 *
 * Create a local file with a given filling.
 *
 */

/** @page file_create_pattern Test for pattern-filled file creation
 *
 * @objective Test that tapi_file_create_pattern works correctly.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "file_create_pattern"

#include "file_suite.h"
#include "te_string.h"
#include "te_file.h"
#include "te_bufs.h"

int
main(int argc, char **argv)
{
    te_string expected = TE_STRING_INIT;
    te_string inbuf = TE_STRING_INIT;
    unsigned int length;
    const char *pattern;
    char *fname = NULL;

    TEST_START;
    TEST_GET_STRING_PARAM(pattern);
    TEST_GET_UINT_PARAM(length);

    TEST_STEP("Create a local file");
    CHECK_NOT_NULL((fname = tapi_file_create_pattern(length, *pattern)));
    te_string_reserve(&expected, length + 1);
    expected.len = length;
    memset(expected.ptr, *pattern, expected.len);

    TEST_STEP("Check the file contents");
    CHECK_RC(te_file_read_string(&inbuf, true, 0, "%s", fname));

    if (!te_compare_bufs(expected.ptr, expected.len, 1,
                         inbuf.ptr, inbuf.len, TE_LL_ERROR))
        TEST_VERDICT("Read and written data differ");

    TEST_SUCCESS;

cleanup:

    if (fname != NULL)
    {
        unlink(fname);
        free(fname);
    }
    te_string_free(&expected);
    te_string_free(&inbuf);
    TEST_END;
}
