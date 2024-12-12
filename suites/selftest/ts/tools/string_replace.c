/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for string segment replacement.
 *
 * Testing string segment replacing routines.
 */

/** @page tools_string_replace te_string.h test
 *
 * @objective Testing string segment replacement routines.
 *
 * Test the correctness of string segment replacement functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/string_replace"

#include "te_config.h"

#include "tapi_test.h"
#include "te_string.h"
#include "te_str.h"
#include "te_bufs.h"

static void
make_replacement(char *orig, size_t orig_len,
                 size_t seg_start, size_t seg_len,
                 char *rep, size_t rep_len,
                 size_t n_exp, const struct iovec *exp_vec)
{
    te_string str = TE_STRING_INIT;

    te_string_append_buf(&str, orig, orig_len);
    te_string_replace_buf(&str, seg_start, seg_len, rep, rep_len);

    if (!te_compare_iovecs(n_exp, exp_vec, 1,
                           (const struct iovec[1]){
                               {.iov_base = str.ptr, .iov_len = str.len + 1}
                           }, TE_LL_RING))
    {
        TEST_VERDICT("The replacement was not correct");
    }

    te_string_free(&str);
}

static void
check_inner_replace_buf(size_t buf_len, char *buf,
                        size_t rep_len, char *rep)
{

    size_t seg_start;
    size_t seg_end;

    seg_start = rand_range(0, buf_len - 1);
    seg_end = rand_range(seg_start, buf_len - 1);

    make_replacement(buf, buf_len,
                     seg_start, seg_end - seg_start + 1,
                     rep, rep_len,
                     3, (const struct iovec[3]){
                         {.iov_base = buf, .iov_len = seg_start},
                         {.iov_base = rep, .iov_len = rep_len},
                         {.iov_base = buf + seg_end + 1,
                          .iov_len = buf_len - seg_end}
                     });
}

static void
check_append_replace_buf(size_t buf_len, char *buf,
                         size_t rep_len, char *rep)
{
    size_t seg_start;
    size_t seg_len;

    seg_start = rand_range(0, buf_len);
    seg_len = rand_range(0, buf_len);
    make_replacement(buf, buf_len,
                     buf_len + seg_start, seg_len,
                     rep, rep_len,
                     3, (const struct iovec[3]){
                         {.iov_base = buf, .iov_len = buf_len},
                         {.iov_base = NULL,
                          .iov_len = seg_start},
                         {.iov_base = rep, .iov_len = rep_len + 1}
                     });
}

static void
check_insert_buf(size_t buf_len, char *buf,
                 size_t rep_len, char *rep)
{
    size_t seg_start;

    seg_start = rand_range(0, buf_len - 1);

    make_replacement(buf, buf_len,
                     seg_start, 0,
                     rep, rep_len,
                     3, (const struct iovec[3]){
                         {.iov_base = buf, .iov_len = seg_start},
                         {.iov_base = rep, .iov_len = rep_len},
                         {.iov_base = buf + seg_start,
                          .iov_len = buf_len - seg_start + 1}
                     });
}

static void
check_delete_buf(size_t buf_len, char *buf)
{

    size_t seg_start;
    size_t seg_end;

    seg_start = rand_range(0, buf_len - 1);
    seg_end = rand_range(seg_start, buf_len - 1);

    make_replacement(buf, buf_len,
                     seg_start, seg_end - seg_start + 1,
                     NULL, 0,
                     2, (const struct iovec[2]){
                         {.iov_base = buf, .iov_len = seg_start},
                         {.iov_base = buf + seg_end + 1,
                          .iov_len = buf_len - seg_end}
                     });
}

static void
check_zero_buf(size_t buf_len, char *buf, size_t rep_len)
{

    size_t seg_start;
    size_t seg_end;

    seg_start = rand_range(0, buf_len - 1);
    seg_end = rand_range(seg_start, buf_len - 1);

    make_replacement(buf, buf_len,
                     seg_start, seg_end - seg_start + 1,
                     NULL, rep_len,
                     3, (const struct iovec[3]){
                         {.iov_base = buf, .iov_len = seg_start},
                         {.iov_base = NULL, .iov_len = rep_len},
                         {.iov_base = buf + seg_end + 1,
                          .iov_len = buf_len - seg_end
                         }
                     });
}

static void
check_replace_suffix_buf(size_t buf_len, char *buf,
                        size_t rep_len, char *rep)
{

    size_t seg_start;
    size_t surplus;

    seg_start = rand_range(0, buf_len - 1);
    surplus = rand_range(1, buf_len);

    make_replacement(buf, buf_len,
                     seg_start, SIZE_MAX - seg_start,
                     rep, rep_len,
                     2, (const struct iovec[2]){
                         {.iov_base = buf, .iov_len = seg_start},
                         {.iov_base = rep, .iov_len = rep_len + 1}
                     });

    make_replacement(buf, buf_len,
                     seg_start, buf_len + surplus,
                     rep, rep_len,
                     2, (const struct iovec[2]){
                         {.iov_base = buf, .iov_len = seg_start},
                         {.iov_base = rep, .iov_len = rep_len + 1}
                     });
}

static void
check_cut(size_t buf_len, char *buf, size_t max_len)
{
    te_string str = TE_STRING_INIT;
    size_t midpoint;
    size_t real_midpoint;

    te_string_append_buf(&str, buf, buf_len);
    midpoint = rand_range(0, max_len);
    real_midpoint = midpoint > buf_len ? buf_len : midpoint;

    te_string_cut_beginning(&str, midpoint);

    if (str.len != buf_len - real_midpoint)
        TEST_VERDICT("Prefix of unexpected length was cut");
    if (memcmp(str.ptr, buf + real_midpoint, str.len + 1) != 0)
        TEST_VERDICT("The remaining suffix is invalid");

    te_string_reset(&str);
    te_string_append_buf(&str, buf, buf_len);
    te_string_cut(&str, midpoint);

    if (str.len != buf_len - real_midpoint)
        TEST_VERDICT("Suffix of unexpected length was cut");
    if (memcmp(str.ptr, buf, str.len) != 0)
        TEST_VERDICT("The remaining prefix is invalid");
    if (str.ptr[str.len] != '\0')
        TEST_VERDICT("The remaining prefix is not NUL-terminated");

    te_string_free(&str);
}

static void
check_replace_fmt(size_t orig_len, char *orig, size_t rep_len, char *rep)
{
    size_t seg_start = rand_range(0, orig_len);
    size_t seg_end = rand_range(seg_start, orig_len);
    te_string str = TE_STRING_INIT;
    size_t fmt_len;

    te_string_append_buf(&str, orig, orig_len);
    fmt_len = te_string_replace(&str, seg_start, seg_end - seg_start + 1,
                                "%s%s", rep, rep);
    if (fmt_len != 2 * rep_len)
        TEST_VERDICT("Invalid replacement length");

    if (!te_compare_iovecs(4, (const struct iovec[4]){
                {.iov_base = orig, .iov_len = seg_start},
                {.iov_base = rep, .iov_len = rep_len},
                {.iov_base = rep, .iov_len = rep_len},
                {.iov_base = orig + MIN(orig_len, seg_end + 1),
                 .iov_len = orig_len - MIN(orig_len - 1, seg_end)
                }
            },
            1, (const struct iovec[1]){
                {.iov_base = str.ptr,.iov_len = str.len + 1}
            }, TE_LL_RING))
    {
        TEST_VERDICT("The replacement was not correct");
    }

    fmt_len = te_string_replace(&str, seg_start, rep_len, NULL);
    if (fmt_len != 0)
        TEST_VERDICT("Non-zero replacement length on delete");
    if (!te_compare_iovecs(3, (const struct iovec[3]){
                {.iov_base = orig, .iov_len = seg_start},
                {.iov_base = rep, .iov_len = rep_len},
                {.iov_base = orig + MIN(orig_len, seg_end + 1),
                 .iov_len = orig_len - MIN(orig_len - 1, seg_end)
                }
            },
            1, (const struct iovec[1]){
                {.iov_base = str.ptr,.iov_len = str.len + 1}
            }, TE_LL_RING))
    {
        TEST_VERDICT("The replacement was not correct");
    }

    te_string_free(&str);
}


int
main(int argc, char **argv)
{
    unsigned int i;
    unsigned int max_len;
    unsigned int n_iterations;

    TEST_START;
    TEST_GET_UINT_PARAM(max_len);
    TEST_GET_UINT_PARAM(n_iterations);

    for (i = 0; i < n_iterations; i++)
    {
        size_t buf_len;
        char *buf = te_make_printable_buf(2, max_len + 1, &buf_len);
        size_t rep_len;
        char *rep = te_make_printable_buf(1, max_len + 1, &rep_len);

        /* Subtract the terminating NUL character */
        buf_len -= 1;
        rep_len -= 1;

        TEST_STEP("Iteration #%u", i);
        TEST_SUBSTEP("Checking plain replacement");
        check_inner_replace_buf(buf_len, buf, rep_len, rep);

        TEST_SUBSTEP("Checking replacement that does append");
        check_append_replace_buf(buf_len, buf, rep_len, rep);

        TEST_SUBSTEP("Checking replacement that does insert");
        check_insert_buf(buf_len, buf, rep_len, rep);

        TEST_SUBSTEP("Checking replacement that does delete");
        check_delete_buf(buf_len, buf);

        TEST_SUBSTEP("Checking replacement that insert zeroes");
        check_zero_buf(buf_len, buf, rep_len);

        TEST_SUBSTEP("Checking suffix replacement");
        check_replace_suffix_buf(buf_len, buf, rep_len, rep);

        TEST_SUBSTEP("Checking prefix and suffix cutting");
        check_cut(buf_len, buf, max_len);

        TEST_SUBSTEP("Checking formatted replacement");
        check_replace_fmt(buf_len, buf, rep_len, rep);

        free(buf);
        free(rep);
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
