/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs. All rights reserved. */
/** @file
 * @brief Test for buffer comparison functions.
 *
 * Testing buffer comparison functions.
 */

/** @page tools_compare_bufs te_compare_bufs test
 *
 * @objective Testing buffer comparison functions.
 *
 * Test the correctness of te_compare_bufs and
 * te_compare_iovecs functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/compare_bufs"

#include "te_config.h"

#include "tapi_test.h"
#include "tapi_mem.h"
#include "te_bufs.h"

static bool
do_compare(void *exp_buf, size_t exp_len, unsigned n_copies,
           void *actual, size_t actual_len,
           bool expected)
{
    struct iovec exp_vec[2 * n_copies];
    struct iovec actual_vec[2];
    size_t split_at;
    unsigned int i;

    TEST_SUBSTEP("Comparing plain buffers");
    if (te_compare_bufs(exp_buf, exp_len, n_copies,
                        actual, actual_len, TE_LL_RING) != expected)
        return false;

    TEST_SUBSTEP("Comparing iovecs");

    split_at = rand_range(0, actual_len);
    actual_vec[0].iov_base = actual;
    actual_vec[0].iov_len = split_at;
    actual_vec[1].iov_base = (uint8_t *)actual + split_at;
    actual_vec[1].iov_len = actual_len - split_at;

    for (i = 0; i < n_copies; i++)
    {
        split_at = rand_range(0, exp_len);
        exp_vec[2 * i].iov_base = exp_buf;
        exp_vec[2 * i].iov_len = split_at;
        exp_vec[2 * i + 1].iov_base = (uint8_t *)exp_buf + split_at;
        exp_vec[2 * i + 1].iov_len = exp_len - split_at;
    }

    return te_compare_iovecs(2 * n_copies, exp_vec, 2, actual_vec,
                             TE_LL_RING) == expected;
}

static void
compare_iovec_hole(void *buf, size_t buf_len)
{
    size_t hole_start;
    size_t hole_end;
    struct iovec exp_vec[3];
    struct iovec actual_vec[1] = {{.iov_base = buf, .iov_len = buf_len}};

    hole_start = rand_range(0, buf_len - 1);
    hole_end = rand_range(hole_start, buf_len - 1);
    memset((uint8_t *)buf + hole_start, '\0', hole_end - hole_start + 1);

    exp_vec[0].iov_base = buf;
    exp_vec[0].iov_len = hole_start;
    exp_vec[1].iov_base = NULL;
    exp_vec[1].iov_len = hole_end - hole_start + 1;
    exp_vec[2].iov_base = (uint8_t *)buf + hole_end + 1;
    exp_vec[2].iov_len = buf_len - hole_end - 1;

    if (!te_compare_iovecs(3, exp_vec, 1, actual_vec, TE_LL_RING))
    {
        TEST_VERDICT("The iovec with a hole is not equal to the buffer");
    }
}

int
main(int argc, char **argv)
{
    unsigned int min_len;
    unsigned int max_len;
    unsigned int n_iterations;
    unsigned int i;
    size_t buf_len;
    uint8_t *buf1 = NULL;
    uint8_t *buf2 = NULL;
    unsigned diff_at;

    TEST_START;

    TEST_GET_UINT_PARAM(min_len);
    TEST_GET_UINT_PARAM(max_len);
    TEST_GET_UINT_PARAM(n_iterations);

    for (i = 0; i < n_iterations; i++)
    {
        buf1 = te_make_buf(min_len, max_len, &buf_len);
        buf2 = NULL;

        TEST_STEP("Compare a buffer to itself");
        if (!do_compare(buf1, buf_len, 1, buf1, buf_len, true))
            TEST_VERDICT("The buffer is not equal to itself");

        TEST_STEP("Compare a buffer to a duplicate buffer");
        buf2 = tapi_malloc(2 * buf_len);
        memcpy(buf2, buf1, buf_len);
        memcpy(buf2 + buf_len, buf1, buf_len);
        if (!do_compare(buf1, buf_len, 2, buf2, 2 * buf_len,
                        true))
        {
            TEST_VERDICT("The buffer is not equal to itself");
        }

        TEST_STEP("Compare two buffers with a single difference");
        diff_at = rand_range(0, buf_len - 1);
        buf2[diff_at] = ~buf2[diff_at];
        if (!do_compare(buf1, buf_len, 1, buf2, buf_len, false))
            TEST_VERDICT("The unequal buffers are equal");

        TEST_STEP("Compare a buffer with a corrupted duplicate");
        buf2[diff_at] = ~buf2[diff_at];
        buf2[buf_len + diff_at] = ~buf2[diff_at];
        if (!do_compare(buf1, buf_len, 2, buf2, 2 * buf_len,
                        false))
        {
            TEST_VERDICT("The unequal buffers are equal");
        }

        TEST_STEP("Compare a buffer with a double-corrupted duplicate");
        buf2[diff_at] = ~buf2[diff_at];
        if (!do_compare(buf1, buf_len, 2, buf2, 2 * buf_len,
                       false))
        {
            TEST_VERDICT("The unequal buffers are equal");
        }

        TEST_STEP("Compare a buffer with its truncated version");
        if (!do_compare(buf1, buf_len, 1, buf1, buf_len - 1,
                       false))
        {
            TEST_VERDICT("The buffers of unequal size are equal");
        }

        TEST_STEP("Compare a buffer with its truncated corrupted version");
        if (!do_compare(buf1, buf_len, 1, buf2, buf_len - 1,
                        false))
        {
            TEST_VERDICT("The unequal buffers are equal");
        }

        TEST_STEP("Compare a buffer with its extended version");
        if (!do_compare(buf2, buf_len, 1, buf2, buf_len + 1,
                        false))
        {
            TEST_VERDICT("The buffers of unequal size are equal");
        }

        TEST_STEP("Compare a buffer with a hole");
        compare_iovec_hole(buf1, buf_len);

        free(buf1);
        free(buf2);
    }
    TEST_SUCCESS;

cleanup:

    TEST_END;
}
