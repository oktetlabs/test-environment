/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for Base-64 encoding/decoding routines.
 *
 * Testing Base-64 encoder/decoder.
 */

/** @page tools_base64 Base-64 encoder/decoder test
 *
 * @objective Testing Base-64 encoding/decoding routines.
 *
 * Test the correctness of Base-64 encode/decoder.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/base64"

#include "te_config.h"

#include "tapi_test.h"
#include "te_bufs.h"
#include "te_string.h"

static void
check_encoding(const uint8_t *src, size_t len, const char *exp_base64,
               te_bool url_safe)
{
    te_string encoded = TE_STRING_INIT;
    te_string decoded = TE_STRING_INIT;

    te_string_encode_base64(&encoded, len, src, url_safe);
    CHECK_NOT_NULL(encoded.ptr);
    RING("Encoded buffer: %s", encoded.ptr);
    if (exp_base64 != NULL)
    {
        if (strcmp(encoded.ptr, exp_base64) != 0)
        {
            ERROR("Expected Base-64 string: %s", exp_base64);
            TEST_VERDICT("Invalid Base-64 encoding");
        }
    }

    CHECK_RC(te_string_decode_base64(&decoded, encoded.ptr));
    CHECK_NOT_NULL(decoded.ptr);

    if (!te_compare_bufs(src, len, 1, decoded.ptr, decoded.len,
                         TE_LL_ERROR))
    {
        TEST_VERDICT("Decoded buffer differs from the original");
    }

    te_string_free(&decoded);
    te_string_free(&encoded);
}

int
main(int argc, char **argv)
{
    unsigned int min_len;
    unsigned int max_len;
    unsigned int n_iterations;
    te_bool url_safe;
    unsigned int i;

    TEST_START;
    TEST_GET_UINT_PARAM(min_len);
    TEST_GET_UINT_PARAM(max_len);
    TEST_GET_UINT_PARAM(n_iterations);
    TEST_GET_BOOL_PARAM(url_safe);

    TEST_STEP("Check validity of encoding of well-known strings");
#define CHECK_BASE64(_in, _out) \
    check_encoding(_in, sizeof(_in), _out, url_safe)
    TEST_SUBSTEP("All zeroes");
    CHECK_BASE64(((const uint8_t[]){'\0', '\0', '\0'}), "AAAA");
    CHECK_BASE64(((const uint8_t[]){'\0', '\0'}), "AAA=");
    CHECK_BASE64(((const uint8_t[]){'\0'}), "AA==");
    TEST_SUBSTEP("A known phrase");
    CHECK_BASE64((const uint8_t []){"A quick brown fox jumped over "
                                    "a sleeping dog"},
                 "QSBxdWljayBicm93biBmb3gganVtcG"
                 "VkIG92ZXIgYSBzbGVlcGluZyBkb2cA");
    TEST_SUBSTEP("All alpha chars in Base64");
    CHECK_BASE64(((const uint8_t[]){'\x00', '\x10', '\x83', '\x10', '\x51',
                                   '\x87', '\x20', '\x92', '\x8b', '\x30',
                                   '\xd3', '\x8f', '\x41', '\x14', '\x93',
                                   '\x51', '\x55', '\x97', '\x61', '\x96',
                                   '\x9b', '\x71', '\xd7', '\x9f', '\x82',
                                   '\x18', '\xa3', '\x92', '\x59', '\xa7',
                                   '\xa2', '\x9a', '\xab', '\xb2', '\xdb',
                                   '\xaf', '\xc3', '\x1c', '\xb3'}),
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    TEST_SUBSTEP("Digits and 62-63 chars in Base64");
    CHECK_BASE64(((const uint8_t[]){'\xd3', '\x5d', '\xb7', '\xe3', '\x9e',
                                    '\xbb', '\xf3', '\xdf', '\xbf'}),
        url_safe ? "0123456789-_" : "0123456789+/");
#undef CHECK_BASE64

    TEST_STEP("Verify decoding process Base64-encoded data correctly");
    for (i = 0; i < n_iterations; i++)
    {
        size_t len;
        uint8_t *buf = te_make_buf(min_len, max_len, &len);

        check_encoding(buf, len, NULL, url_safe);
        free(buf);
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
