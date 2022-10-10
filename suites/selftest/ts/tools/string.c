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

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
