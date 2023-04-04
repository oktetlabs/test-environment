/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_expand.h functions
 *
 * Testing variable expansion routines.
 */

/** @page tools_expand te_expand.h test
 *
 * @objective Testing variable expansion routines.
 *
 * Test the correctness of variable expanding functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/expand"

#include <string.h>
#include "te_config.h"

#include "tapi_test.h"
#include "te_kvpair.h"
#include "te_expand.h"
#include "te_str.h"

/*
 * FIXME: the template should a parameter too, but
 * due to OL bug #12847 a parameter inside package.xml
 * cannot contain shell-like variable references
 */
static const char template[] = "var1=${var1} var2=${var2:-unknown} "
    "pos=${0} unknown=${unknown:-${var2}} known=${var2:+known} "
    "nested=${var2:+${unknown:-${var1}}} end";

int
main(int argc, char **argv)
{
    const char *var1;
    const char *var2;
    const char *posarg;
    const char *expanded;
    te_kvpair_h kvpairs;
    const char *posargs[10] = {NULL,};
    char *actual = NULL;

    te_kvpair_init(&kvpairs);

    TEST_START;
    TEST_GET_OPT_STRING_PARAM(var1);
    TEST_GET_OPT_STRING_PARAM(var2);
    TEST_GET_STRING_PARAM(posarg);
    TEST_GET_STRING_PARAM(expanded);
    posargs[0] = posarg;
    CHECK_RC(te_kvpair_add(&kvpairs, "var1", "%s",
                           te_str_empty_if_null(var1)));
    CHECK_RC(te_kvpair_add(&kvpairs, "var2", "%s",
                           te_str_empty_if_null(var2)));

    CHECK_RC(te_expand_kvpairs(template, posargs, &kvpairs, &actual));
    if (strcmp(expanded, actual) != 0)
    {
        ERROR("Expected '%s', got '%s'", expanded, actual);
        TEST_VERDICT("Unexpected expansion");
    }

    TEST_SUCCESS;

cleanup:

    te_kvpair_fini(&kvpairs);
    free(actual);
    TEST_END;
}
