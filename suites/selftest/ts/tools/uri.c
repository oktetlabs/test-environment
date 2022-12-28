/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for URI-handling functions
 *
 * Testing URI-handling routines.
 */

/** @page tools_uri URI handling test
 *
 * @objective Testing URI-handling routines.
 *
 * Test the correctness of URI-handling functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/uri"

#include "te_config.h"

#include <regex.h>

#include "tapi_test.h"
#include "te_string.h"
#include "te_kvpair.h"
#include "te_bufs.h"
#include "te_str.h"

static const char *mode_names[] = {
    [TE_STRING_URI_ESCAPE_BASE] = "generic URI",
    [TE_STRING_URI_ESCAPE_USER] = "userinfo",
    [TE_STRING_URI_ESCAPE_HOST] = "host",
    [TE_STRING_URI_ESCAPE_PATH_SEGMENT] = "path segment",
    [TE_STRING_URI_ESCAPE_PATH] = "path",
    [TE_STRING_URI_ESCAPE_QUERY] = "query string",
    [TE_STRING_URI_ESCAPE_QUERY_VALUE] = "query key/value",
    [TE_STRING_URI_ESCAPE_FRAG] = "fragment"
};

static void
check_unreserved(void)
{
    char *uri_part = te_make_spec_buf(10, 20, NULL,
                                      TE_FILL_SPEC_URI_CHUNK);
    te_string_uri_escape_mode mode;

    for (mode = TE_STRING_URI_ESCAPE_BASE;
         mode <= TE_STRING_URI_ESCAPE_FRAG;
         mode++)
    {
        te_string result = TE_STRING_INIT;

        te_string_append_escape_uri(&result, mode, uri_part);

        if (strcmp(result.ptr, uri_part))
        {
            ERROR("'%s' should not have changed, but got '%s'",
                  uri_part, result.ptr);
            te_string_free(&result);
            free(uri_part);
            TEST_VERDICT("Incorrect escaping for a %s", mode_names[mode]);
        }
        te_string_free(&result);
    }

    free(uri_part);
}

static void
check_obligatory_escaping(void)
{
    char input[16];
    te_string_uri_escape_mode mode;
    regex_t re;

    te_fill_spec_buf(input, sizeof(input) - 1,
                     "[\1-\x20\"%<>\\^``{|}\x7F-\xFF]");
    input[sizeof(input) - 1] = '\0';
    regcomp(&re, "^(%[0-9A-F]{2}){15}$", REG_EXTENDED | REG_NOSUB);

    for (mode = TE_STRING_URI_ESCAPE_BASE;
         mode <= TE_STRING_URI_ESCAPE_FRAG;
         mode++)
    {
        te_string result = TE_STRING_INIT;

        te_string_append_escape_uri(&result, mode, input);

        if (regexec(&re, result.ptr, 0, NULL, 0) != 0)
        {
            ERROR("'%s' should have been totally escaped, but got '%s'",
                  input, result.ptr);
            te_string_free(&result);
            regfree(&re);
            TEST_VERDICT("Incorrect escaping for a %s", mode_names[mode]);
        }

        te_string_free(&result);
    }

    regfree(&re);
}

static void
check_uri_escape(const char *input, te_string_uri_escape_mode mode,
                 const char *expected)
{
    te_string result = TE_STRING_INIT;

    te_string_append_escape_uri(&result, mode, input);

    if (strcmp(result.ptr, expected) != 0)
    {
        ERROR("'%s' should have been escaped to '%s', got '%s'",
              input, expected, result.ptr);
        te_string_free(&result);
        TEST_VERDICT("Incorrect escaping for a %s", mode_names[mode]);
    }
    te_string_free(&result);
}

static void
check_kvpair(const char *expected, ...)
{
    va_list kv;
    te_kvpair_h kvp;
    te_string result = TE_STRING_INIT;

    te_kvpair_init(&kvp);
    va_start(kv, expected);

    while (TRUE)
    {
        const char *key = va_arg(kv, const char *);
        const char *value;

        if (key == NULL)
            break;
        value = va_arg(kv, const char *);

        te_kvpair_add(&kvp, key, "%s", value);
    }
    va_end(kv);

    te_kvpair_to_uri_query(&kvp, &result);
    te_kvpair_fini(&kvp);

    if (strcmp(result.ptr, expected) != 0)
    {
        ERROR("Expected '%s', got '%s'", expected, result.ptr);
        te_string_free(&result);
        TEST_VERDICT("Invalid escaping for key-value pairs");
    }
    te_string_free(&result);
}

static void
check_join_uri_path(const char *expected, ...)
{
    te_string result = TE_STRING_INIT;
    te_vec vec = TE_VEC_INIT(const char *);
    va_list args;

    va_start(args, expected);

    while (TRUE)
    {
        const char *item = va_arg(args, const char *);

        if (item == NULL)
            break;

        TE_VEC_APPEND(&vec, item);
    }
    va_end(args);

    te_string_join_uri_path(&result, &vec);
    te_vec_free(&vec);

    if (strcmp(result.ptr, expected) != 0)
    {
        ERROR("Expected '%s', got '%s'", expected, result.ptr);
        te_string_free(&result);
        TEST_VERDICT("Invalid escaping for path segments");
    }
    te_string_free(&result);
}

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Checking URI non-escaping");
    check_unreserved();

    TEST_STEP("Checking obligatory URI escaping");
    check_obligatory_escaping();

    TEST_STEP("Checking specific URI escaping");
    check_uri_escape("user:(password)", TE_STRING_URI_ESCAPE_USER,
                     "user:(password)");
    check_uri_escape("user@domain", TE_STRING_URI_ESCAPE_USER,
                     "user%40domain");
    check_uri_escape("[ffff:ffff:ffff:ffff:ffff:ffff]",
                     TE_STRING_URI_ESCAPE_HOST,
                     "[ffff:ffff:ffff:ffff:ffff:ffff]");
    check_uri_escape("strange/host@domain",
                     TE_STRING_URI_ESCAPE_HOST,
                     "strange%2Fhost%40domain");
    check_uri_escape("/a/b/c?", TE_STRING_URI_ESCAPE_PATH,
                     "/a/b/c%3F");
    check_uri_escape("/a/b/c?", TE_STRING_URI_ESCAPE_PATH_SEGMENT,
                     "%2Fa%2Fb%2Fc%3F");
    check_uri_escape("a=b&c=d?;e=f#", TE_STRING_URI_ESCAPE_QUERY,
                     "a=b&c=d?;e=f%23");
    check_uri_escape("a=b&c=d?;e=f#", TE_STRING_URI_ESCAPE_QUERY_VALUE,
                     "a%3Db%26c%3Dd?%3Be%3Df%23");
    check_uri_escape("a=b&c=d?;e=f#", TE_STRING_URI_ESCAPE_FRAG,
                     "a=b&c=d?;e=f%23");

    TEST_STEP("Checking URI path joining");
    check_join_uri_path("a/b%2Fc/d%3F", "a", "b/c", "d?", NULL);

    TEST_STEP("Checking kvpair-to-query conversion");
    check_kvpair("a=b&c=d%3De&e=f%26", "a", "b", "c", "d=e", "e", "f&", NULL);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
