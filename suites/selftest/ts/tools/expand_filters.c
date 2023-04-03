/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_expand.h filters.
 *
 * Testing variable expansion with filters.
 */

/** @page tools_expand_filters te_expand.h filters test
 *
 * @objective Testing variable expansion with filters.
 *
 * Test the correctness of variable expanding filters.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/expand_filter"

#include <string.h>
#include "te_config.h"

#include "tapi_test.h"
#include "te_kvpair.h"
#include "te_expand.h"
#include "te_str.h"
#include "te_string.h"

int
main(int argc, char **argv)
{
    const char *input;
    const char *filter;
    const char *filtered;
    te_string template = TE_STRING_INIT;
    te_string expected = TE_STRING_INIT;
    te_kvpair_h kvpairs;
    te_string actual = TE_STRING_INIT;

    te_kvpair_init(&kvpairs);

    TEST_START;
    TEST_GET_OPT_STRING_PARAM(input);
    TEST_GET_STRING_PARAM(filter);
    TEST_GET_OPT_STRING_PARAM(filtered);

    te_string_append(&template, "${var|%s} ${var|%s:+unfiltered} "
                     "${novar|%s:-default}", filter, filter, filter);
    te_string_append(&expected, "%s unfiltered default",
                     te_str_empty_if_null(filtered));
    CHECK_RC(te_kvpair_add(&kvpairs, "var", "%s", te_str_empty_if_null(input)));

    CHECK_RC(te_string_expand_kvpairs(template.ptr, NULL, &kvpairs, &actual));
    if(strcmp(te_string_value(&expected), te_string_value(&actual)) != 0)
    {
        ERROR("Expected '%s', got '%s'",
              te_string_value(&expected),
              te_string_value(&actual));
        TEST_VERDICT("Unexpected expansion");
    }

    TEST_SUCCESS;

cleanup:

    te_kvpair_fini(&kvpairs);
    te_string_free(&template);
    te_string_free(&expected);
    te_string_free(&actual);
    TEST_END;
}
