/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Sample of compound parameters.
 *
 * Sample of compound parameters.
 *
*/

/** @page minimal_compound_params Test compound parameters
 *
 * @objective Test that compound parameters are properly handled.
 *
 * The test does almost nothing by itself but it expects that
 * parameters have structure defined by @c <field>.
 *
 * @par Test sequence:
 *
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "minimal/compound_params"

#include "te_config.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    const char *fields_first;
    const char *fields_second;
    const char *fields_second1;
    te_vec multiple = TE_VEC_INIT(const char *);
    te_vec states = TE_VEC_INIT(int);
    bool boolean_true;
    bool boolean_false;
    const char *enum_first;
    const char *enum_second;
    const char *check_enum_first;
    const char *check_enum_second;
    const char *simple;

    const char **iter;
    int *iptr;

    TEST_START;

    TEST_STEP("Getting compound parameters");

    TEST_GET_STRING_PARAM(fields_first);
    TEST_GET_STRING_PARAM(fields_second);
    TEST_GET_STRING_PARAM(fields_second1);

    TEST_GET_PARAMS_VECTOR(multiple, test_get_string_param);
    TEST_GET_PARAMS_VECTOR(states, test_get_enum_param,
                           (struct param_map_entry []){
                               ETHDEV_STATE_MAPPING_LIST,
                               {NULL, 0}
                           });

    TEST_GET_BOOL_PARAM(boolean_true);
    TEST_GET_BOOL_PARAM(boolean_false);

    TEST_GET_STRING_PARAM(enum_first);
    TEST_GET_STRING_PARAM(enum_second);

    TEST_GET_STRING_PARAM(check_enum_first);
    TEST_GET_STRING_PARAM(check_enum_second);

    TEST_GET_STRING_PARAM(simple);

    RING("first = %s, second = %s %s",
         fields_first, fields_second, fields_second1);
    TE_VEC_FOREACH(&multiple, iter)
        RING("multiple = %s", *iter);
    TE_VEC_FOREACH(&states, iptr)
        RING("states = %d", *iptr);

    RING("enum first = %s, second = %s",
         enum_first, enum_second);

    RING("check enum first = %s, second = %s",
         check_enum_first, check_enum_second);

    RING("simple = %s", simple);

    if (boolean_false)
        TEST_VERDICT("boolean_false should be FALSE");

    if (!boolean_true)
        TEST_VERDICT("boolean_true should be TRUE");

    TEST_SUCCESS;

cleanup:

    te_vec_free(&multiple);
    te_vec_free(&states);
    TEST_END;
}
