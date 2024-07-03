/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs. All rights reserved. */
/** @file
 * @brief Test for buffer generation functions.
 *
 * Testing buffer generation functions.
 */

/** @page tools_make_bufs te_make_printable_buf test
 *
 * @objective Testing buffer generation functions.
 *
 * Test the correctness of te_make_printable_buf function.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/make_bufs"

#include "te_config.h"

#include <string.h>
#include <regex.h>

#include "tapi_test.h"
#include "tapi_mem.h"
#include "te_bufs.h"

static void
check_valid_buf(size_t min_len, size_t max_len, const char *spec,
                const char *exp_match)
{
    char *buf = NULL;
    size_t len;
    regex_t re;

    if (regcomp(&re, exp_match, REG_EXTENDED | REG_NOSUB) != 0)
        TE_FATAL_ERROR("Cannot compile '%s'", exp_match);

    CHECK_NOT_NULL(buf = te_make_spec_buf(min_len, max_len, &len, spec));
    if (len < min_len || len > max_len)
    {
        TEST_VERDICT("The generated length is outside of "
                     "the valid range: %zu", len);
    }

    if (buf[len - 1] != '\0')
        TEST_VERDICT("The buffer is not zero terminated");
    if (strlen(buf) + 1 != len)
    {
        TEST_VERDICT("The reported length %zu is incorrect: should be %zu",
                     len, strlen(buf) + 1);
    }

    if (len > 1)
    {
        if (regexec(&re, buf, 0, NULL, 0) != 0)
        {
            ERROR("'%s' does not match RE '%s'", buf, exp_match);
            TEST_VERDICT("The buffer contains invalid characters");
        }
    }

    free(buf);
    regfree(&re);
}

int
main(int argc, char **argv)
{
    unsigned int i;
    unsigned int min_len;
    unsigned int max_len;
    unsigned int n_iterations;
    bool embedded_zeroes = false;

    static const char *specs[][3] = {
        {"digits", TE_FILL_SPEC_DECIMAL, "^[[:digit:]]+$"},
        {"hex digits", TE_FILL_SPEC_HEX, "^[[:xdigit:]]+$"},
        {"letters", TE_FILL_SPEC_WORD, "^[[:alpha:]]+$"},
        {"C id", TE_FILL_SPEC_C_ID, "^[[:alpha:]_][[:alnum:]_]*$"},
        {"filename", TE_FILL_SPEC_FILENAME,
         "^[[:alnum:]_%+=@^-][[:alnum:]_%+=@^.-]*$"},
        {"URI chunk", TE_FILL_SPEC_URI_CHUNK, "^[[:alnum:]_.~-]+$"},
        {"XML name", TE_FILL_SPEC_XML_NAME, "^[[:alpha:]_][[:alnum:]_.-]*"},
        {"JSON literal", TE_FILL_SPEC_JSON_STR, "^[][ !#-Z^-~]+$"},
        {"XML text", TE_FILL_SPEC_XML_TEXT, "^[][\n !#-%(-;=?-~]+$"},
        {"printable characters", TE_FILL_SPEC_PRINTABLE, "^[[:print:]]+$"},
        {"text", TE_FILL_SPEC_TEXT, "^[\n\t[:print:]]+$"},
        {"ASCII characters", TE_FILL_SPEC_ASCII, "^[\x1-\x7F]+$"},
        {"([a-z^d-x^k-p])\\0", "([a-z^d-x^k-p])`\0", "^[abcklmnopyz]+$"},
        {"([^`\\0\\x1-\\x1F\\x7F-\\xFF^\\n\\t])",
         "([^`\0\x1-\x1F\x7F-\xFF^\n\t])`\0", "^[\n\t[:print:]]+$"},
        {"x(yz)`\\0", "x(yz)`\0", "^x(yz)*(yz?)?$"},
    };
    unsigned int specno;

    TEST_START;

    TEST_GET_UINT_PARAM(min_len);
    TEST_GET_UINT_PARAM(max_len);
    TEST_GET_UINT_PARAM(n_iterations);


    TEST_STEP("Generate random buffers");
    for (specno = 0; specno < TE_ARRAY_LEN(specs); specno++)
    {
        TEST_SUBSTEP("Checking buffer constraint: %s", specs[specno][0]);
        for (i = 0; i < n_iterations; i++)
        {
            check_valid_buf(min_len, max_len, specs[specno][1],
                            specs[specno][2]);
        }
    }

    TEST_STEP("Check embedded zeroes");
    for (i = 0; i < n_iterations && !embedded_zeroes; i++)
    {
        size_t len;
        char *buf = te_make_buf(min_len, max_len, &len);
        size_t j;

        for (j = 0; j < len - 1; j++)
        {
            if (buf[j] == '\0')
            {
                embedded_zeroes = true;
                break;
            }
        }
        free(buf);
    }
    if (!embedded_zeroes)
        TEST_VERDICT("No embedded zeroes after %zu iterations", n_iterations);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
