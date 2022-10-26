/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_intset.h functions
 *
 * Testing integral set functions
 */

/** @page tools_intset te_intset.h test
 *
 * @objective Testing integral set functions
 *
 * Check that intset parsing/unparsing works as expected.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/intset"

#include "te_config.h"

#include "tapi_test.h"
#include "te_intset.h"

static void
check_parse_unparse(const char *str, uint64_t expect)
{
    uint64_t result;
    char *backstr;

    CHECK_RC(te_bits_parse(str, &result));
    if (expect != result)
    {
        TEST_VERDICT("Expected bits are %" PRIx64, ", but got %" PRIx64,
                     expect, result);
    }
    CHECK_NOT_NULL(backstr = te_bits2string(result));
    if (strcmp(backstr, str) != 0)
    {
        TEST_VERDICT("Unparsed string '%s' is different from the original '%s'",
                     backstr, str);
    }
    free(backstr);
}

static void
check_subset(uint64_t sub, uint64_t super)
{
    te_bool result = te_intset_generic_is_subset(&te_bits_intset,
                                                 0, sizeof(uint64_t) *
                                                 CHAR_BIT - 1,
                                                 &sub, &super);
    te_bool expected = (sub & ~super) == 0;

    if (result != expected)
    {
        TEST_VERDICT("%" PRIx64 " is expected to be%s a subset of "
                     "%" PRIx64 ", but it is%s", sub,
                     expected ? "" : " not",
                     super,
                     result ? "" : " not");
    }
}
int
main(int argc, char **argv)
{
    uint64_t pristine;

    TEST_START;

    TEST_STEP("Parsing/unparsing empty set");
    pristine = 0;
    check_parse_unparse("", pristine);

    TEST_STEP("Parsing/unparsing singleton");
    pristine = 1ull << 1;
    check_parse_unparse("1", pristine);

    TEST_STEP("Parsing/unparsing list");
    pristine = 1ull << 1;
    pristine |= 1ull << 10;
    check_parse_unparse("1,10", pristine);

    TEST_STEP("Parsing/unparsing single range");
    pristine = 1ull << 1;
    pristine |= 1ull << 2;
    check_parse_unparse("1-2", pristine);

    TEST_STEP("Parsing/unparsing list of ranges");
    pristine = 1ull << 1;
    pristine |= 1ull << 2;
    pristine |= 1ull << 10;
    pristine |= 1ull << 11;
    pristine |= 1ull << 12;
    pristine |= 1ull << 32;
    check_parse_unparse("1-2,10-12,32", pristine);
    pristine |= 1ull << 33;
    check_parse_unparse("1-2,10-12,32-33", pristine);

    TEST_STEP("Unparsing/parsing the max element");
    pristine = 1ull << 63;
    check_parse_unparse("63", pristine);
    pristine |= 1ull << 62;
    check_parse_unparse("62-63", pristine);
    pristine |= 1ull << 61;
    check_parse_unparse("61-63", pristine);
    pristine &= ~(1ull << 62);
    check_parse_unparse("61,63", pristine);

    TEST_STEP("Checking subset detection");
    check_subset(0, 0);
    check_subset(0, 1 << 0);
    check_subset(1 << 0, 1 << 0);
    check_subset(1 << 0, (1 << 1) | (1 << 0));
    check_subset((1 << 1) | (1 << 0), UINT64_MAX);
    check_subset(UINT64_MAX, UINT64_MAX);
    check_subset(1, 0);
    check_subset(UINT64_MAX, 1);
    check_subset((1 << 2) | (1 << 0), (1 << 1) | (1 << 0));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
