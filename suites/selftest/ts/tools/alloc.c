/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs. All rights reserved. */
/** @file
 * @brief Test for allocator functions.
 *
 * Testing allocator functions.
 */

/** @page tools_alloc te_alloc.h test
 *
 * @objective Testing allocator functions.
 *
 * Test the correctness of allocator functions.
 *
 * The test is more useful when run under valgrind.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/alloc"

#include "te_config.h"

#include <string.h>

#include "tapi_test.h"
#include "te_bufs.h"
#include "te_alloc.h"

static void
check_adjust_extent(size_t buf_size, size_t offset, size_t length,
                    te_bool exp_adjusted, size_t exp_length)
{
    te_bool adjusted = te_alloc_adjust_extent(buf_size, offset, &length);

    if (exp_adjusted != adjusted)
    {
        TEST_VERDICT("Length %s when it %s",
                     adjusted ? "adjusted" : "not adjusted",
                     exp_adjusted ? "should" : "shouldn't");
    }

    if (length != exp_length)
    {
        TEST_VERDICT("Expected length %zu, actually %zu",
                     exp_length, length);
    }
}

int
main(int argc, char **argv)
{
    unsigned int i;
    unsigned int min_len;
    unsigned int max_len;
    unsigned int n_iterations;

    TEST_START;

    TEST_GET_UINT_PARAM(min_len);
    TEST_GET_UINT_PARAM(max_len);
    TEST_GET_UINT_PARAM(n_iterations);

    TEST_STEP("Testing memdup");
    for (i = 0; i < n_iterations; i++)
    {
        size_t len;
        void *buf = te_make_buf(min_len, max_len, &len);
        void *copy;

        copy = TE_MEMDUP(buf, len);
        if (!te_compare_bufs(buf, len, 1, copy, len, TE_LL_ERROR))
            TEST_VERDICT("The copy differs from the original");
        free(copy);
        free(buf);
    }

    TEST_STEP("Testing strdup");
    for (i = 0; i < n_iterations; i++)
    {
        size_t len;
        char *buf = te_make_printable_buf(min_len, max_len, &len);
        char *copy;
        size_t copy_len;

        copy = TE_STRDUP(buf);
        copy_len = strlen(copy) + 1;
        if (!te_compare_bufs(buf, len, 1, copy, copy_len, TE_LL_ERROR))
            TEST_VERDICT("The copy differs from the original");
        free(copy);
        free(buf);
    }

    TEST_STEP("Testing strndup");
    for (i = 0; i < n_iterations; i++)
    {
        size_t len;
        char *buf = te_make_printable_buf(min_len, max_len, &len);
        char *copy;
        size_t copy_len;
        size_t trunc_at = rand_range(0, 10);

        copy = TE_STRNDUP(buf, len + trunc_at);
        copy_len = strlen(copy) + 1;
        if (!te_compare_bufs(buf, len, 1, copy, copy_len, TE_LL_ERROR))
            TEST_VERDICT("The copy differs from the original");
        free(copy);
        free(buf);
    }

    TEST_STEP("Testing strndup with limit");
    for (i = 0; i < n_iterations; i++)
    {
        size_t len;
        char *buf = te_make_printable_buf(min_len, max_len, &len);
        char *copy;
        size_t trunc_at;

        trunc_at = rand_range(0, len - 1);
        copy = TE_STRNDUP(buf, trunc_at);
        if (copy[trunc_at] != '\0')
            TEST_VERDICT("The copy is not null terminated");
        if (!te_compare_bufs(buf, trunc_at, 1, copy, trunc_at, TE_LL_ERROR))
            TEST_VERDICT("The copy differs from the original");
        free(copy);
        free(buf);
    }

    TEST_STEP("Checking overflow checker");
    for (i = 0; i < n_iterations; i++)
    {
        /* Mask for the lower-half bits of size_t */
        static const size_t low_mask =
            ((size_t)1 << (sizeof(size_t) * CHAR_BIT / 2)) - 1;
        static const size_t high_bit =
            (size_t)1 << (sizeof(size_t) * CHAR_BIT - 1);
        size_t nmemb = (size_t)rand() & low_mask;
        size_t size = (size_t)rand() & low_mask;

        if (!te_is_valid_alloc(nmemb, size))
        {
            ERROR("%zu * %zu erroneously detected as overflow",
                  nmemb, size);
            TEST_VERDICT("Check for overflow failed");
        }

        if (te_is_valid_alloc(nmemb | high_bit, size + 2))
        {
            ERROR("%zu * %zu not detected as overflow",
                  nmemb | high_bit, size + 1);
            TEST_VERDICT("Check for overflow failed");
        }

        if (te_is_valid_alloc(nmemb + 2, size | high_bit))
        {
            ERROR("%zu * %zu not detected as overflow",
                  nmemb + 1, size | high_bit);
            TEST_VERDICT("Check for overflow failed");
        }
    }

    TEST_STEP("Checking extent limiter");
    TEST_SUBSTEP("no overflow, no adjustment");
    for (i = 0; i < n_iterations; i++)
    {
        size_t buf_size = (size_t)rand_range(1, UINT16_MAX);
        size_t start = (size_t)rand_range(0, buf_size - 1);
        size_t length = (size_t)rand_range(0, buf_size - start);

        check_adjust_extent(buf_size, start, length, FALSE, length);
    }

    TEST_SUBSTEP("no overflow, with adjustment");
    for (i = 0; i < n_iterations; i++)
    {
        size_t buf_size = (size_t)rand_range(1, UINT16_MAX);
        size_t start = (size_t)rand_range(0, buf_size - 1);
        size_t length = buf_size - start + (size_t)rand_range(1, UINT16_MAX);

        check_adjust_extent(buf_size, start, length, TRUE, buf_size - start);
    }

    TEST_SUBSTEP("overflow, with adjustment");
    for (i = 0; i < n_iterations; i++)
    {
        size_t buf_size = (size_t)rand_range(1, UINT16_MAX);
        size_t start = (size_t)rand_range(0, buf_size - 1);

        check_adjust_extent(buf_size, start, SIZE_MAX, TRUE, buf_size - start);
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
