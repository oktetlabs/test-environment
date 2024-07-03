/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Testing file read-write functions
 *
 * Testing te_file_read_string and te_file_write_string correctness.
 */

/** @page tools_file te_file_read_string and te_file_write_string test
 *
 * @objective Testing te_file_read_string and te_file_write_string correctness.
 *
 * @par Test sequence:
 *
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/file"

#include "te_config.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "tapi_test.h"
#include "te_file.h"

int
main(int argc, char **argv)
{
    te_string outbuf = TE_STRING_INIT;
    te_string inbuf = TE_STRING_INIT;
    te_string padding = TE_STRING_INIT;
    te_string expected = TE_STRING_INIT;
    const char *content;
    bool binary;
    char *path = NULL;
    te_errno chk_rc;

    TEST_START;
    TEST_GET_STRING_PARAM(content);
    TEST_GET_BOOL_PARAM(binary);

    TEST_STEP("Create a temporary file");
    CHECK_NOT_NULL((path = te_file_create_unique("/tmp/te_file_", NULL)));

    TEST_STEP("Write to a file");
    te_string_append(&outbuf, "%s", content);
    CHECK_RC(te_file_write_string(&outbuf, 0, 0, 0, "%s", path));

    TEST_STEP("Append padding to a file");
    if (binary)
        te_string_append_buf(&padding, (const char[]){'\0'}, 1);
    else
        te_string_append(&padding, "\n\n\n\n");
    CHECK_RC(te_file_write_string(&padding, 0, O_APPEND, 0, "%s", path));

    TEST_STEP("Reading from a file");
    CHECK_RC(te_file_read_string(&inbuf, binary, 0, "%s", path));
    te_string_append_buf(&expected, outbuf.ptr, outbuf.len);
    if (binary)
        te_string_append_buf(&expected, padding.ptr, padding.len);
    if (!te_compare_bufs(expected.ptr, expected.len, 1,
                         inbuf.ptr, inbuf.len, TE_LL_ERROR))
    {
        TEST_VERDICT("The contents of file being read is different "
                     "from written");
    }

    TEST_STEP("Checking for maxsize limit");
    chk_rc = te_file_read_string(&inbuf, binary, strlen(content), "%s", path);
    if (chk_rc != TE_EFBIG)
        TEST_VERDICT("The maximum size of a file is not detected: %r", chk_rc);
    if (inbuf.len != expected.len)
        TEST_VERDICT("Buffer not rewound after error");

    if (binary)
    {
        TEST_STEP("Checking embedded zero detection");
        chk_rc = te_file_read_string(&inbuf, false, 0, "%s", path);
        if (chk_rc != TE_EILSEQ)
            TEST_VERDICT("Embedded zeroes are not detected: %r", chk_rc);
    }
    else
    {
        TEST_STEP("Reading text as binary");
        te_string_reset(&inbuf);
        te_string_reset(&expected);
        te_string_append_buf(&expected, outbuf.ptr, outbuf.len);
        te_string_append_buf(&expected, padding.ptr, padding.len);

        CHECK_RC(te_file_read_string(&inbuf, true, 0, "%s", path));
        if (!te_compare_bufs(expected.ptr, expected.len, 1,
                             inbuf.ptr, inbuf.len, TE_LL_ERROR))
        {
            TEST_VERDICT("The contents of file being read as binary "
                         "differs from written");
        }
    }

    TEST_STEP("Checking replicated writing");
    te_string_reset(&inbuf);
    te_string_reset(&expected);
    /* Not a typo: indeed append two copies of outbuf */
    te_string_append_buf(&expected, outbuf.ptr, outbuf.len);
    te_string_append_buf(&expected, outbuf.ptr, outbuf.len);
    te_string_append_buf(&expected, outbuf.ptr, 1);

    CHECK_RC(te_file_write_string(&outbuf, expected.len, O_TRUNC, 0,
                                  "%s", path));
    CHECK_RC(te_file_read_string(&inbuf, binary, 0, "%s", path));
    if (!te_compare_bufs(expected.ptr, expected.len, 1,
                         inbuf.ptr, inbuf.len, TE_LL_ERROR))
        TEST_VERDICT("The content of file differs from a replicated write");

    TEST_SUCCESS;

cleanup:

    te_string_free(&expected);
    te_string_free(&outbuf);
    te_string_free(&inbuf);
    te_string_free(&padding);

    if (path != NULL)
    {
        unlink(path);
        free(path);
    }

    TEST_END;
}
