/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Testing chunked read from a file
 *
 * Testing te_file_read_string chunked read correcteness
 */

/** @page tools_file_chunked te_file_read_string chunked read test
 *
 * @objective Testing te_file_read_string correctness on chunked reads.
 *
 * @par Test sequence:
 *
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/file_chunked"

#include "te_config.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "tapi_test.h"
#include "te_file.h"

static void
system_read_file(const char *pathname, te_string *dest)
{
    FILE *f = fopen(pathname, "r");
    char buf[1024];

    if (f == NULL)
    {
        TEST_SKIP("Cannot open '%s': %r",
                  pathname, te_rc_os2te(errno));
    }

    while (true)
    {
        size_t actual = fread(buf, 1, sizeof(buf), f);

        if (actual == 0)
        {
            if (ferror(f))
            {
                TEST_VERDICT("Cannot read from '%s': %r",
                             pathname, te_rc_os2te(errno));
            }
            break;
        }

        te_string_append_buf(dest, buf, actual);
    }
    te_string_chop(dest, "\n");
    fclose(f);
}

int
main(int argc, char **argv)
{
    te_string inbuf = TE_STRING_INIT;
    te_string expected = TE_STRING_INIT;
    const char *pathname = NULL;
    te_errno chk_rc;

    TEST_START;
    TEST_GET_STRING_PARAM(pathname);

    TEST_STEP("Reading a file via system functions");
    system_read_file(pathname, &expected);

    TEST_STEP("Reading from a file via TE");
    CHECK_RC(te_file_read_string(&inbuf, false, 0, "%s", pathname));

    if (!te_compare_bufs(expected.ptr, expected.len, 1,
                         inbuf.ptr, inbuf.len, TE_LL_ERROR))
    {
        TEST_VERDICT("The contents of file being read is different "
                     "from written");
    }

    TEST_STEP("Checking for maxsize limit");
    chk_rc = te_file_read_string(&inbuf, false, inbuf.len / 2, "%s",
                                 pathname);
    if (chk_rc != TE_EFBIG)
        TEST_VERDICT("The maximum size of a file is not detected: %r", chk_rc);
    if (inbuf.len != expected.len)
        TEST_VERDICT("Buffer not rewound after error");

    TEST_SUCCESS;

cleanup:

    te_string_free(&expected);
    te_string_free(&inbuf);

    TEST_END;
}
