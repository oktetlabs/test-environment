/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs. All rights reserved. */
/** @file
 * @brief Test for buffer comparison function.
 *
 * Testing buffer comparison function.
 */

/** @page tools_compare_bufs te_compare_bufs test
 *
 * @objective Testing buffer comparison function.
 *
 * Test the correctness of te_compare_bufs function.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/compare_bufs"

#include "te_config.h"

#include "tapi_test.h"
#include "tapi_mem.h"
#include "te_bufs.h"

#define BUFFER_SIZE 64

int
main(int argc, char **argv)
{
    uint8_t *buf1 = te_make_buf_by_len(BUFFER_SIZE);
    uint8_t *buf2 = NULL;
    unsigned diff_at;

    TEST_START;

    TEST_STEP("Compare a buffer to itself");
    if (!te_compare_bufs(buf1, BUFFER_SIZE, 1, buf1, BUFFER_SIZE, TE_LL_RING))
        TEST_VERDICT("The buffer is not equal to itself");

    TEST_STEP("Compare a buffer to a duplicate buffer");
    buf2 = tapi_malloc(2 * BUFFER_SIZE);
    memcpy(buf2, buf1, BUFFER_SIZE);
    memcpy(buf2 + BUFFER_SIZE, buf1, BUFFER_SIZE);
    if (!te_compare_bufs(buf1, BUFFER_SIZE, 2, buf2, 2 * BUFFER_SIZE,
                         TE_LL_RING))
    {
        TEST_VERDICT("The buffer is not equal to itself");
    }

    TEST_STEP("Compare two buffers with a single difference");
    diff_at = rand_range(0, BUFFER_SIZE - 1);
    buf2[diff_at] = ~buf2[diff_at];
    if (te_compare_bufs(buf1, BUFFER_SIZE, 1, buf2, BUFFER_SIZE, TE_LL_RING))
        TEST_VERDICT("The unequal buffers are equal");

    TEST_STEP("Compare a buffer with a corrupted duplicate");
    buf2[diff_at] = ~buf2[diff_at];
    buf2[BUFFER_SIZE + diff_at] = ~buf2[diff_at];
    if (te_compare_bufs(buf1, BUFFER_SIZE, 2, buf2, 2 * BUFFER_SIZE,
                        TE_LL_RING))
    {
        TEST_VERDICT("The unequal buffers are equal");
    }

    TEST_STEP("Compare a buffer with a double-corrupted duplicate");
    buf2[diff_at] = ~buf2[diff_at];
    if (te_compare_bufs(buf1, BUFFER_SIZE, 2, buf2, 2 * BUFFER_SIZE,
                        TE_LL_RING))
    {
        TEST_VERDICT("The unequal buffers are equal");
    }

    TEST_STEP("Compare a buffer with its truncated version");
    if (te_compare_bufs(buf1, BUFFER_SIZE, 1, buf1, BUFFER_SIZE - 1,
                        TE_LL_RING))
    {
        TEST_VERDICT("The buffers of unequal size are equal");
    }

    TEST_STEP("Compare a buffer with its truncated corrupted version");
    if (te_compare_bufs(buf1, BUFFER_SIZE, 1, buf2, BUFFER_SIZE - 1,
                        TE_LL_RING))
    {
        TEST_VERDICT("The unequal buffers are equal");
    }

    TEST_STEP("Compare a buffer with its extended version");
    if (te_compare_bufs(buf2, BUFFER_SIZE, 1, buf2, BUFFER_SIZE + 1,
                        TE_LL_RING))
    {
        TEST_VERDICT("The buffers of unequal size are equal");
    }

    TEST_SUCCESS;

cleanup:

    free(buf1);
    free(buf2);
    TEST_END;
}
