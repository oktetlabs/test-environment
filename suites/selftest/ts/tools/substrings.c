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
#include "te_rand.h"
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

        if (te_substring_find(&substr, search) != (n_repl != 0))
            TEST_VERDICT("Unexpected result of finding a substring");
        if (!!te_substring_compare_str(&substr, search) != (n_repl == 0))
            TEST_VERDICT("The found substring is not as expected");
        if (te_substring_compare(&substr, &substr) != 0)
            TEST_VERDICT("A substring is not equal to itself");
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
    {
        size_t performed = te_string_replace_all_substrings(&str, repl, chunk);

        if (performed != n_repl)
        {
            TEST_VERDICT("Number of actual replacements"
                         "differ from the expected");
        }
    }
    else
    {
        if (!te_string_replace_substring(&str, repl, chunk))
            TEST_VERDICT("The replacement is reported not to happen");
    }
    if (!te_compare_iovecs(1 + 2 * n_repl, exp_vec,
                           1, (const struct iovec[1]){
                               {.iov_base = str.ptr,
                                .iov_len = str.len + 1,
                               }
                           }, TE_LL_RING))
        TEST_VERDICT("Improper replacement");

    te_string_free(&str);
}

static void
replace_none(const char *base, const char *chunk, const char *rep)
{
    te_string str = TE_STRING_INIT;

    te_string_append(&str, "%s", base);

    if (te_string_replace_substring(&str, rep, chunk))
        TEST_VERDICT("Replacement is reported to happen");

    if (strcmp(str.ptr, base) != 0)
        TEST_VERDICT("Unexpected substring replacement");

    if (te_string_replace_all_substrings(&str, rep, chunk) != 0)
        TEST_VERDICT("Replacements are reported to happen");

    if (strcmp(str.ptr, base) != 0)
        TEST_VERDICT("Unexpected substring replacement");

    te_string_free(&str);
}

static void
modify_substring(char *base, char *chunk, char *rep)
{
    te_string str = TE_STRING_INIT;
    te_substring_t substr = TE_SUBSTRING_INIT(&str);
    size_t base_len = strlen(base);
    size_t chunk_len = strlen(chunk);
    size_t exp_rep_len = strlen(rep);
    size_t rep_len;

    build_source_string(&str, base, chunk, 1);
    if (!te_substring_find(&substr, chunk))
        TEST_VERDICT("A segment is not found");
    if (substr.start != base_len)
        TEST_VERDICT("A segment is found at unexpected position");
    if (substr.len != chunk_len)
        TEST_VERDICT("Unexpected length of the found segment");

    rep_len = te_substring_modify(&substr, TE_SUBSTRING_MOD_OP_PREPEND,
                                  "%s", rep);
    if (rep_len != exp_rep_len)
        TEST_VERDICT("Unexpected replacement length");
    if (!te_compare_iovecs(4, (const struct iovec[4]){
                {.iov_base = base, .iov_len = base_len},
                {.iov_base = rep, .iov_len = exp_rep_len},
                {.iov_base = chunk, .iov_len = chunk_len},
                {.iov_base = base, .iov_len = base_len + 1}},
            1,
            (const struct iovec[1]){
                {.iov_base = str.ptr, .iov_len = str.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Invalid string content after prepending");
    rep_len = te_substring_modify(&substr, TE_SUBSTRING_MOD_OP_APPEND,
                                  "%s", rep);
    if (rep_len != exp_rep_len)
        TEST_VERDICT("Unexpected replacement length");
    if (!te_compare_iovecs(5, (const struct iovec[5]){
                {.iov_base = base, .iov_len = base_len},
                {.iov_base = rep, .iov_len = exp_rep_len},
                {.iov_base = chunk, .iov_len = chunk_len},
                {.iov_base = rep, .iov_len = exp_rep_len},
                {.iov_base = base, .iov_len = base_len + 1}
            },
            1, (const struct iovec[1]){
                {.iov_base = str.ptr, .iov_len = str.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Invalid string content after appending");

    rep_len = te_substring_modify(&substr, TE_SUBSTRING_MOD_OP_REPLACE,
                                  "%s", rep);
    if (rep_len != exp_rep_len)
        TEST_VERDICT("Unexpected replacement length");
    if (!te_compare_iovecs(3, (const struct iovec[3]){
                {.iov_base = base, .iov_len = base_len},
                {.iov_base = rep, .iov_len = exp_rep_len},
                {.iov_base = base, .iov_len = base_len + 1}},
            1, (const struct iovec[1]){
                {.iov_base = str.ptr, .iov_len = str.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Invalid string content after replacement");

    if (substr.start != base_len)
        TEST_VERDICT("Substring has moved unexpectedly");
    if (substr.len != exp_rep_len)
        TEST_VERDICT("Unexpected length of the substring after replacement");

    te_string_free(&str);
}

static void
copy_substring(char *base, char *chunk, char *rep)
{
    te_string str1 = TE_STRING_INIT;
    te_substring_t substr1 = TE_SUBSTRING_INIT(&str1);
    te_string str2 = TE_STRING_INIT;
    te_substring_t substr2 = TE_SUBSTRING_INIT(&str2);
    size_t base_len = strlen(base);
    size_t chunk_len = strlen(chunk);
    size_t rep_len = strlen(rep);

    te_string_append(&str1, "%s%s", base, chunk);
    te_string_append(&str2, "%s%s", base, rep);

    substr1.start = substr2.start = base_len;
    substr1.len = chunk_len;
    substr2.len = rep_len;

    if (!te_substring_copy(&substr1, &substr2, TE_SUBSTRING_MOD_OP_PREPEND))
        TEST_VERDICT("Reported no copying");
    if (!te_compare_iovecs(3, (const struct iovec[3]){
                {.iov_base = base, .iov_len = base_len},
                {.iov_base = rep, .iov_len = rep_len},
                {.iov_base = chunk, .iov_len = chunk_len + 1}
            },
            1, (const struct iovec[1]){
                {.iov_base = str1.ptr, .iov_len = str1.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Invalid string content after prepending");

    if (!te_substring_copy(&substr1, &substr2, TE_SUBSTRING_MOD_OP_APPEND))
        TEST_VERDICT("Reported no copying");
    if (!te_compare_iovecs(4, (const struct iovec[4]){
                {.iov_base = base, .iov_len = base_len},
                {.iov_base = rep, .iov_len = rep_len},
                {.iov_base = chunk, .iov_len = chunk_len},
                {.iov_base = rep, .iov_len = rep_len + 1}
            },
            1, (const struct iovec[1]){
                {.iov_base = str1.ptr, .iov_len = str1.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Invalid string content after appending");

    if (!te_substring_copy(&substr1, &substr2, TE_SUBSTRING_MOD_OP_REPLACE))
        TEST_VERDICT("Reported no copying");
    if (!te_compare_iovecs(2, (const struct iovec[2]){
                {.iov_base = base, .iov_len = base_len},
                {.iov_base = rep, .iov_len = rep_len + 1}},
            1, (const struct iovec[1]){
                {.iov_base = str1.ptr, .iov_len = str1.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Invalid string content after replacement");

    if (te_substring_compare(&substr1, &substr2) != 0)
        TEST_VERDICT("Substrings differ after copying");

    te_substring_invalidate(&substr2);
    if (!te_substring_copy(&substr1, &substr2, TE_SUBSTRING_MOD_OP_REPLACE))
        TEST_VERDICT("Reported no copying");
    if (!te_compare_bufs(base, base_len + 1, 1,
                         str1.ptr, str1.len + 1, TE_LL_RING))
        TEST_VERDICT("Invalid string content after deletion");

    te_string_free(&str1);
    te_string_free(&str2);
}

static void
check_spans(unsigned int min_len, unsigned int max_len)
{
    static const char all_digits[] = "0123456789";
    size_t digits_len;
    char *digits = te_make_spec_buf(min_len, max_len, &digits_len,
                                    TE_FILL_SPEC_DECIMAL);
    size_t word_len;
    char *word = te_make_spec_buf(min_len, max_len, &word_len,
                                  TE_FILL_SPEC_WORD);
    te_string str = TE_STRING_INIT;
    te_substring_t substr = TE_SUBSTRING_INIT(&str);
    te_substring_t substr2;
    int n_spaces = rand_range(min_len, max_len);
    char next_ch;
    size_t skip_cnt;

    te_string_append(&str, "%s%s%*s", digits, word, n_spaces, "");

    next_ch = te_substring_span(&substr, all_digits, false);
    if (next_ch != word[0])
        TEST_VERDICT("Unexpected next character");
    if (substr.start != 0)
        TEST_VERDICT("Substring starting point has moved");
    if (substr.len != digits_len - 1)
        TEST_VERDICT("Incorrect span length");

    next_ch = te_substring_span(&substr, " ", true);
    if (next_ch != ' ')
        TEST_VERDICT("Unexpected next character: '%#x'", next_ch);
    if (substr.start != 0)
        TEST_VERDICT("Substring starting point has moved");
    if (substr.len != digits_len + word_len - 2)
        TEST_VERDICT("Incorrect span length");

    if (!te_substring_advance(&substr))
        TEST_VERDICT("Substring not advanced");

    substr2 = substr;
    skip_cnt = te_substring_skip(&substr, ' ', max_len);
    if (skip_cnt != (size_t)n_spaces)
        TEST_VERDICT("Invalid amount of spaces skipped");
    if (substr.start != str.len)
        TEST_VERDICT("The substring does not point to the end");
    if (substr.len != 0)
        TEST_VERDICT("The substring has non-zero length");

    next_ch = te_substring_span(&substr2, " ", false);
    if (next_ch != '\0')
        TEST_VERDICT("Unexpected next character");
    if (substr2.len != (size_t)n_spaces)
        TEST_VERDICT("Incorrect span length");
    skip_cnt = te_substring_skip(&substr2, ' ', 1);
    if (skip_cnt != 1)
        TEST_VERDICT("Invalid amount of spaces skipped");
    if (substr2.len != (size_t)n_spaces - 1)
        TEST_VERDICT("Invalid span length after skip");
    if (substr2.start != digits_len + word_len - 1)
        TEST_VERDICT("The substring has invalid start point after skip");

    free(digits);
    free(word);
    te_string_free(&str);
}

static void
insert_sep(char *base)
{
    te_string str = TE_STRING_INIT;
    te_substring_t substr = TE_SUBSTRING_INIT(&str);
    int pos;
    char sep;

    te_string_append(&str, "%s", base);
    pos = rand_range(1, str.len);
    sep = te_rand_range_exclude(' ', '~', str.ptr[pos - 1]);
    substr.start = pos;
    substr.len = rand_range(0, str.len - pos);

    if (!te_substring_insert_sep(&substr, sep, false))
        TEST_VERDICT("No separator inserted when it should");
    substr.start++;
    if (te_substring_insert_sep(&substr, sep, false))
        TEST_VERDICT("Separator inserted when it should not");

    if (te_compare_iovecs(3, (const struct iovec[3]){
                {.iov_base = base, .iov_len = pos},
                {.iov_base = (char[1]){sep}, .iov_len = 1},
                {.iov_base = base + 1, .iov_len = strlen(base) - pos}
            },
            1, (const struct iovec[1]){
                {.iov_base = str.ptr, .iov_len = str.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Unexpected content after inserting separator");
    substr.start = 0;
    substr.len = 0;
    sep = te_rand_range_exclude(' ', '~', str.ptr[0]);
    if (te_substring_insert_sep(&substr, sep, false))
        TEST_VERDICT("Separator inserted when it should not");
    if (!te_substring_insert_sep(&substr, sep, true))
        TEST_VERDICT("No separator inserted when it should");
    if (te_compare_iovecs(3, (const struct iovec[3]){
                {.iov_base = (char[1]){sep}, .iov_len = 1},
                {.iov_base = base, .iov_len = strlen(base) + 1}
            },
            1, (const struct iovec[1]){
                {.iov_base = str.ptr, .iov_len = str.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Unexpected content after inserting separator");

    te_string_free(&str);
}

static void
check_strip_prefix(void)
{
    static struct {
        const char *input;
        const char *prefix;
        const char *stem;
        bool exp_result;
    } tests[] = {
        {"", "", "", true},
        {"a", "", "a", true},
        {"a", "a", "", true},
        {"abc", "a", "bc", true},
        {"aabc", "a", "abc", true},
        {"abc", "c", "abc", false},
        {"abc", "abcd", "abc", false},
    };
    unsigned int i;

    for (i = 0; i < TE_ARRAY_LEN(tests); i++)
    {
        te_string inp = TE_STRING_INIT_RO_PTR(tests[i].input);
        te_substring_t inp_sub = TE_SUBSTRING_INIT(&inp);
        bool result;

        inp_sub.len = strlen(tests[i].input);
        result = te_substring_strip_prefix(&inp_sub, tests[i].prefix);
        if (result != tests[i].exp_result)
            TEST_VERDICT("Unexpected stripping result");
        if (te_substring_compare_str(&inp_sub, tests[i].stem) != 0)
            TEST_VERDICT("Unexpected suffix after stripping");
    }
}

static void
check_strip_suffix(void)
{
    static struct {
        const char *input;
        const char *suffix;
        const char *stem;
        bool exp_result;
    } tests[] = {
        {"", "", "", true},
        {"a", "", "a", true},
        {"a", "a", "", true},
        {"abc", "c", "ab", true},
        {"abcc", "c", "abc", true},
        {"abc", "d", "abc", false},
        {"abc", "abcd", "abc", false},
    };
    unsigned int i;

    for (i = 0; i < TE_ARRAY_LEN(tests); i++)
    {
        te_string inp = TE_STRING_INIT_RO_PTR(tests[i].input);
        te_substring_t inp_sub = TE_SUBSTRING_INIT(&inp);
        bool result;

        inp_sub.len = strlen(tests[i].input);
        result = te_substring_strip_suffix(&inp_sub, tests[i].suffix);
        if (result != tests[i].exp_result)
            TEST_VERDICT("Unexpected stripping result");
        if (te_substring_compare_str(&inp_sub, tests[i].stem) != 0)
            TEST_VERDICT("Unexpected prefix after stripping");
    }
}

static void
check_strip_numeric_suffix(void)
{
    static struct {
        const char *input;
        size_t inp_len;
        const char *stem;
        uintmax_t exp_suffix;
        bool exp_result;
    } tests[] = {
        {"", 0, "", 0, false},
        {"a", 1, "a", 0, false},
        {"abc", 3, "abc", 0, false},
        {"abc1", 4, "abc", 1, true},
        {"abc1", 3, "abc", 0, false},
        {"abc123", 6, "abc", 123, true},
        {"abc123", 5, "abc", 12, true},
        {"123", 3, "", 123, true},
        {"abc0", 4, "abc", 0, true},
        {"abc-1", 5, "abc-", 1, true},
        /* Let's hope we're not on a 128-bit platform */
        {"abc99999999999999999999999",
         sizeof("abc99999999999999999999999") - 1,
         "abc99999999999999999999999", 0, false},
    };
    unsigned int i;

    for (i = 0; i < TE_ARRAY_LEN(tests); i++)
    {
        te_string inp = TE_STRING_INIT_RO_PTR(tests[i].input);
        te_substring_t inp_sub = TE_SUBSTRING_INIT(&inp);
        uintmax_t suffix;
        bool result;

        inp_sub.len = tests[i].inp_len;
        result = te_substring_strip_uint_suffix(&inp_sub, &suffix);
        if (result != tests[i].exp_result)
            TEST_VERDICT("Unexpected stripping result");
        if (suffix != tests[i].exp_suffix)
            TEST_VERDICT("Unexpected suffix value");
        if (te_substring_compare_str(&inp_sub, tests[i].stem) != 0)
            TEST_VERDICT("Unexpected prefix after stripping");
    }
}

static void
check_empty_string(void)
{
    te_string empty = TE_STRING_INIT;
    te_substring_t substr = TE_SUBSTRING_INIT(&empty);
    te_substring_t substr1 = TE_SUBSTRING_INIT(&empty);

    if (!te_substring_is_valid(&substr))
        TEST_VERDICT("Substring of an empty string is invalid");

    if (!te_substring_past_end(&substr))
        TEST_VERDICT("Substring of an empty string is not past its end");

    if (te_substring_compare(&substr, &substr1) != 0)
        TEST_VERDICT("Empty substring is not equal to itself");

    if (te_substring_compare_str(&substr, "") != 0)
        TEST_VERDICT("Empty substring is not equal to empty string");

    if (te_substring_find(&substr, " "))
        TEST_VERDICT("Something is found in an empty string");
    if (te_substring_is_valid(&substr))
        TEST_VERDICT("A substring is expected to be invalid");

    substr = substr1;
    if (te_substring_span(&substr, " ", false) != '\0')
        TEST_VERDICT("A non-empty span in an empty string");
    if (substr.start != 0 || substr.len != 0)
        TEST_VERDICT("Empty substring changed unexpectedly");

    if (te_substring_span(&substr, " ", true) != '\0')
        TEST_VERDICT("A non-empty span in an empty string");
    if (substr.start != 0 || substr.len != 0)
        TEST_VERDICT("Empty substring changed unexpectedly");

    if (te_substring_skip(&substr, ' ', SIZE_MAX) != 0)
        TEST_VERDICT("Characters were skipped in an empty substring");

    if (te_substring_strip_prefix(&substr, " "))
        TEST_VERDICT("Non-empty prefix stripped from an empty substring");

    if (te_substring_strip_suffix(&substr, " "))
        TEST_VERDICT("Non-empty suffix stripped from an empty substring");

    te_string_free(&empty);
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
        unsigned int n_repl = rand_range(1, max_repl);

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

        TEST_SUBSTEP("Check for non-replacement");
        replace_none(base, chunk, rep);

        TEST_SUBSTEP("Check for various modification modes");
        modify_substring(base, chunk, rep);

        TEST_SUBSTEP("Check for substring copying");
        copy_substring(base, chunk, rep);

        TEST_SUBSTEP("Check for separator insertion");
        insert_sep(base);

        free(base);
        free(chunk);
        free(rep);

        TEST_SUBSTEP("Checking span detection");
        check_spans(min_len, max_len);
    }

    TEST_STEP("Check prefix/suffix stripping");
    TEST_SUBSTEP("Stripping simple prefix");
    check_strip_prefix();
    TEST_SUBSTEP("Stripping simple suffix");
    check_strip_suffix();
    TEST_SUBSTEP("Stripping numeric suffix");
    check_strip_numeric_suffix();

    TEST_STEP("Check empty string handling");
    check_empty_string();

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
