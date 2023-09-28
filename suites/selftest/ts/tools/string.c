/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_string.h functions
 *
 * Testing string manipulating routines.
 */

/** @page tools_string te_string.h test
 *
 * @objective Testing string manipulating routines.
 *
 * Test the correctness of string-handling functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/string"

#include "te_config.h"

#include "tapi_test.h"
#include "te_string.h"
#include "te_str.h"

static void
check_padding(const char *before, const char *text, const char *after,
              size_t padlen, char padchar,
              const char *expected, size_t explen)
{
    te_string dest = TE_STRING_INIT;

    if (before != NULL)
        te_string_append(&dest, "%s", before);
    te_string_add_centered(&dest, text, padlen, padchar);
    if (after != NULL)
        te_string_append(&dest, "%s", after);

    if (strcmp(dest.ptr, expected) != 0)
        TEST_VERDICT("String improperly centered: '%s'", dest.ptr);
    if (dest.len != explen)
        TEST_VERDICT("Unexpected length of a string: %zd", dest.len);

    te_string_free(&dest);
}

static void
check_string_equality(void)
{
    struct {
        const char *str1;
        const char *str2;
        te_bool expected;
    } tests[] = {
        {"", "", TRUE},
        {"", "abc", FALSE},
        {"abc", "abc", TRUE},
        {"abc", "def", FALSE},
        {"abc", "ab", FALSE},
        {" abc", "abc", TRUE},
        {"abc", " abc", TRUE},
        {"  abc", "\n\t\nabc", TRUE},
        {"abc ", "abc", TRUE},
        {"abc", "abc ", TRUE},
        {"abc   ", "abc\n\t\t", TRUE},
        {"abc def", "abc\n\ndef", TRUE},
        {"abc def", "abcdef", FALSE},
        {"abcdef", "abc def", FALSE},
        {"abc def", "abc\n\nghi", FALSE},
        {"abc", "abcdef", FALSE},
        {"abc ", "abc def", FALSE},
        {"abdef", "abc", FALSE},
        {NULL, NULL, TRUE}
    };
    unsigned int i;

    for (i = 0; tests[i].str1 != NULL; i++)
    {
        te_bool result = te_str_is_equal_nospace(tests[i].str1, tests[i].str2);

        if (result != tests[i].expected)
        {
            ERROR("Strings '%s' and '%s' are%s equal", tests[i].str1,
                  tests[i].str2, result ? "" : " not");
            TEST_VERDICT("Strings are%s equal", result ? "" : " not");
        }
    }
}

static void
check_common_prefix(void)
{
    static struct {
        const char *str1;
        const char *str2;
        size_t exp_prefix;
    } tests[] = {
        {NULL, NULL, 0},
        {NULL, "a", 0},
        {"a", NULL, 0},
        {"", "", 0},
        {"", "abc", 0},
        {"abc", "", 0},
        {"abc", "abc", 3},
        {"a", "abc", 1},
        {"abc", "def", 0},
        {"abcd", "abce", 3}
    };
    unsigned int i;

    for (i = 0; i < TE_ARRAY_LEN(tests); i++)
    {
        size_t prefix = te_str_common_prefix(tests[i].str1, tests[i].str2);

        if (prefix != tests[i].exp_prefix)
        {
            ERROR("Common prefix length for '%s' and '%s' should be %zu, "
                  "but got %zu", te_str_empty_if_null(tests[i].str1),
                  te_str_empty_if_null(tests[i].str2), tests[i].exp_prefix,
                  prefix);
            TEST_VERDICT("Common prefix improperly calculated");
        }
    }
}

int
main(int argc, char **argv)
{
    char *buf;

    TEST_START;

    TEST_STEP("Test string centering");
    check_padding(NULL, "Label", NULL, 10, '-', "---Label--", 10);

    TEST_STEP("Test string centering with truncation");
    check_padding(NULL, "Really long label", NULL, 10, '+', "Really lon", 10);

    TEST_STEP("Test string centering + appending");
    check_padding("[", "Label", "]", 11, '*', "[***Label***]", 13);

    TEST_STEP("Checking raw2string");
    CHECK_NOT_NULL(buf = raw2string((const uint8_t[]){0, 1, 2, 3, 4, 5, 6, 7,
                                                      8, 9, 10, 11, 12, 13, 14,
                                                      15, 16}, 17));
    /* yes, '00' is the expected representation of 0 */
    if (strcmp(buf, "[ 00 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 "
               "0x9 0xa 0xb 0xc 0xd 0xe 0xf 0x10 ]") != 0)
    {
        TEST_VERDICT("Byte array improperly formatted: %s", buf);
    }

    free(buf);

    TEST_STEP("Checking string equality w/o spaces");
    check_string_equality();

    TEST_STEP("Checking common prefix");
    check_common_prefix();

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
