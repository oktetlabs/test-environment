/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_expand.h list facilities.
 *
 * Testing list variable expansion routines.
 */

/** @page tools_expand_list te_expand.h lost test
 *
 * @objective Testing list variable expansion routines.
 *
 * Test the correctness of list variable expanding functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/expand_list"

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
    const char *template;
    char **values1;
    size_t n_values1;
    char **values2;
    size_t n_values2;
    unsigned int index_value;
    size_t i;
    const char *expanded;
    te_kvpair_h kvpairs;
    te_string actual = TE_STRING_INIT;

    te_kvpair_init(&kvpairs);

    TEST_START;
    TEST_GET_STRING_PARAM(template);
    TEST_GET_STRING_PARAM(expanded);
    TEST_GET_STRING_LIST_PARAM(values1, n_values1);
    TEST_GET_STRING_LIST_PARAM(values2, n_values2);
    for (i = n_values1; i > 0; i--)
        te_kvpair_push(&kvpairs, "var1", "%s", values1[i - 1]);
    for (i = n_values2; i > 0; i--)
        te_kvpair_push(&kvpairs, "var2", "%s", values2[i - 1]);
    TEST_GET_UINT_PARAM(index_value);
    te_kvpair_add(&kvpairs, "index", "%u", index_value);

    CHECK_RC(te_string_expand_kvpairs(template, NULL, &kvpairs, &actual));
    if (strcmp(expanded, actual.ptr) != 0)
    {
        ERROR("Expected '%s', got '%s'", expanded, actual.ptr);
        TEST_VERDICT("Unexpected expansion");
    }

    TEST_SUCCESS;

cleanup:

    te_kvpair_fini(&kvpairs);
    te_string_free(&actual);
    /* FIXME: there should be a way to free string list params automatically */
    free(values1[0]);
    free(values2[0]);
    free(values1);
    free(values2);
    TEST_END;
}
