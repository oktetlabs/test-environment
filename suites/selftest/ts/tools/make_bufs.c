/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs. All rights reserved. */
/** @file
 * @brief Test for buffer generation functions.
 *
 * Testing buffer generation functions.
 */

/** @page tools_make_bufs te_make_printable_buf test
 *
 * @objective Testing buffer generation functions.
 *
 * Test the correctness of te_make_printable_buf function.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/make_bufs"

#include "te_config.h"

#include <string.h>
#include <ctype.h>

#include "tapi_test.h"
#include "tapi_mem.h"
#include "te_bufs.h"

int
main(int argc, char **argv)
{
    unsigned int i;
    unsigned int min_len;
    unsigned int max_len;
    unsigned int n_iterations;
    char *buf = NULL;

    TEST_START;

    TEST_GET_UINT_PARAM(min_len);
    TEST_GET_UINT_PARAM(max_len);
    TEST_GET_UINT_PARAM(n_iterations);

    TEST_STEP("Generate random printable buffers");
    for (i = 0; i < n_iterations; i++)
    {
        size_t len;
        const char *iter;

        CHECK_NOT_NULL(buf = te_make_printable_buf(min_len, max_len, &len));
        if (len < min_len || len > max_len)
        {
            TEST_VERDICT("The generated length is outside of the valid range: "
                         "%zu", len);
        }
        if (buf[len - 1] != '\0')
            TEST_VERDICT("The buffer is not zero terminated");
        if (strlen(buf) + 1 != len)
            TEST_VERDICT("The reported length %zu is incorrect", len);
        for (iter = buf; *iter != '\0'; iter++)
        {
            if (!isprint(*iter))
            {
                TEST_VERDICT("%u'th character is not printable, code %02x",
                             (unsigned)(iter - buf), (unsigned)*iter);
            }
        }
        free(buf);
        buf = NULL;
    }

    TEST_SUCCESS;

cleanup:
    free(buf);

    TEST_END;
}
