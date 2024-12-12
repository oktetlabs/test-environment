/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for subtring find and replace.
 *
 * Testing substring find and replace routines.
 */

/** @page tools_substring te_string.h test
 *
 * @objective Testing substring find and replace routines.
 *
 * Test the correctness of substring find and replace functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/substrings"

#include "te_config.h"

#include "tapi_test.h"
#include "te_string.h"
#include "te_str.h"
#include "te_bufs.h"

static void
build_source_string(te_string *dst, const char *base, const char *chunk,
                    unsigned int n_repl)
{
    te_string_append(dst, "%s", base);
    while (n_repl-- > 0)
    {
        te_string_append(dst, "%s", chunk);
        te_string_append(dst, "%s", base);
    }
}

static void
find_substrings(const char *base, const char *search, unsigned int n_repl)
{
    te_string str = TE_STRING_INIT;
    te_substring_t substr = TE_SUBSTRING_INIT(&str);

    build_source_string(&str, base, search, n_repl);

    do {
        if (!te_substring_is_valid(&substr))
            TEST_VERDICT("The substring is not valid in the middle of search");

        te_substring_advance(&substr);
        if (!te_substring_is_valid(&substr))
            TEST_VERDICT("The substring is not valid in the middle of search");

        te_substring_find(&substr, search);
    } while(n_repl-- > 0);

    if (te_substring_is_valid(&substr))
        TEST_VERDICT("The substring is still valid after the test");

    te_string_free(&str);
}

static void
replace_substrings(char *base, char *chunk,
                   char *repl, unsigned int n_repl,
                   bool repl_all)
{
    te_string str = TE_STRING_INIT;
    struct iovec exp_vec[1 + 2 * n_repl];
    unsigned int i;
    size_t repl_len = strlen(repl);
    size_t chunk_len = strlen(chunk);

    build_source_string(&str, base, chunk, n_repl);
    exp_vec[0].iov_base = base;
    exp_vec[0].iov_len = strlen(base);

    for (i = 0; i < n_repl; i++)
    {
        if (i == 0 || repl_all)
        {
            exp_vec[1 + 2 * i].iov_base = repl;
            exp_vec[1 + 2 * i].iov_len = repl_len;
        }
        else
        {
            exp_vec[1 + 2 * i].iov_base = chunk;
            exp_vec[1 + 2 * i].iov_len = chunk_len;
        }
        exp_vec[2 * (i + 1)] = exp_vec[0];
    }
    exp_vec[2 * n_repl].iov_len++;

    if (repl_all)
        te_string_replace_all_substrings(&str, repl, chunk);
    else
        te_string_replace_substring(&str, repl, chunk);
    if (!te_compare_iovecs(1 + 2 * n_repl, exp_vec,
                           1, (const struct iovec[1]){
                               {.iov_base = str.ptr,
                                .iov_len = str.len + 1,
                               }
                           }, TE_LL_RING))
        TEST_VERDICT("Improper replacement");

    te_string_free(&str);
}


int
main(int argc, char **argv)
{
    unsigned int i;
    unsigned int min_len;
    unsigned int max_len;
    unsigned int n_iterations;
    unsigned int max_repl;

    TEST_START;
    TEST_GET_UINT_PARAM(min_len);
    TEST_GET_UINT_PARAM(max_len);
    TEST_GET_UINT_PARAM(n_iterations);
    TEST_GET_UINT_PARAM(max_repl);

    for (i = 0; i < n_iterations; i++)
    {
        char *base = te_make_printable_buf(min_len, max_len + 1, NULL);
        char *chunk = te_make_printable_buf(min_len, max_len + 1, NULL);
        char *rep = te_make_printable_buf(min_len, max_len + 1, NULL);
        unsigned int n_repl = rand_range(0, max_repl);

        /*
         * Extremely unlikely if min_len is not too small,
         * but skipping the iteration to avoid even the tiniest
         * chance of a false negative.
         */
        if (strstr(base, chunk) != NULL)
        {
            free(base);
            free(chunk);
            free(rep);
            continue;
        }

        TEST_STEP("Iteration #%u", i);

        TEST_SUBSTEP("Check for non-destructive string search");
        find_substrings(base, chunk, n_repl);

        TEST_SUBSTEP("Check for single replacement");
        replace_substrings(base, chunk, rep, n_repl, false);

        TEST_SUBSTEP("Check for total replacement");
        replace_substrings(base, chunk, rep, n_repl, true);

        free(base);
        free(chunk);
        free(rep);
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
