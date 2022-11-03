/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Sample of parameters.
 *
 * Sample of parameters.
 *
*/

/** @page minimal_parameters Test parameters
 *
 * @objective Test that various types of parameters are properly handled.
 *
 * @par Test sequence:
 *
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "minimal/parameters"

#include "te_config.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    /*
     * Variables are intentionally initialized to values that are not equal
     * to provided parameter values.
     */
    const char *str_param = NULL;
    int int_param = 0;
    unsigned int uint_param = 0;
    double dbl_param = 0.0;
    double unit_param = 0.0;
    uintmax_t bin_unit_param = 0;
    te_bool true_param = FALSE;
    te_bool false_param = TRUE;
    const char *opt_str_none_param = "";
    const char *opt_str_val_param = NULL;
    te_optional_uint_t opt_uint_none_param = TE_OPTIONAL_UINT_VAL(0);
    te_optional_uint_t opt_uint_val_param = TE_OPTIONAL_UINT_UNDEF;
    te_optional_double_t opt_dbl_none_param = TE_OPTIONAL_DOUBLE_VAL(0.0);
    te_optional_double_t opt_dbl_val_param = TE_OPTIONAL_DOUBLE_UNDEF;
    te_optional_double_t opt_unit_none_param = TE_OPTIONAL_DOUBLE_VAL(0.0);
    te_optional_double_t opt_unit_val_param = TE_OPTIONAL_DOUBLE_UNDEF;

    TEST_START;

    TEST_STEP("Getting required parameters");

    TEST_GET_STRING_PARAM(str_param);
    TEST_GET_INT_PARAM(int_param);
    TEST_GET_UINT_PARAM(uint_param);
    TEST_GET_DOUBLE_PARAM(dbl_param);
    TEST_GET_VALUE_UNIT_PARAM(unit_param);
    TEST_GET_VALUE_BIN_UNIT_PARAM(bin_unit_param);
    TEST_GET_BOOL_PARAM(true_param);
    TEST_GET_BOOL_PARAM(false_param);

    TEST_STEP("Getting optional parameters");
    TEST_GET_OPT_STRING_PARAM(opt_str_none_param);
    TEST_GET_OPT_STRING_PARAM(opt_str_val_param);
    TEST_GET_OPT_UINT_PARAM(opt_uint_none_param);
    TEST_GET_OPT_UINT_PARAM(opt_uint_val_param);
    TEST_GET_OPT_DOUBLE_PARAM(opt_dbl_none_param);
    TEST_GET_OPT_DOUBLE_PARAM(opt_dbl_val_param);
    TEST_GET_OPT_VALUE_UNIT_PARAM(opt_unit_none_param);
    TEST_GET_OPT_VALUE_UNIT_PARAM(opt_unit_val_param);

    TEST_STEP("Verify parameter values");

    CHECK_NOT_NULL(str_param);
    if (strcmp(str_param, "value") != 0)
        TEST_VERDICT("'str_param' has unexpected value: '%s'", str_param);

#define CHECK_NUMERIC_PARAM(_name, _expected, _fmt) \
    do {                                                                \
        if (_name != (_expected))                                       \
            TEST_VERDICT("'%s' has unexpected value: " _fmt,            \
                         #_name, _name);                                \
    } while(0)

    CHECK_NUMERIC_PARAM(int_param, 42, "%d");
    CHECK_NUMERIC_PARAM(uint_param, 42u, "%u");
    CHECK_NUMERIC_PARAM(dbl_param, 42.0, "%g");
    CHECK_NUMERIC_PARAM(unit_param, 1e6, "%g");
    CHECK_NUMERIC_PARAM(bin_unit_param, 1ull << 20, "%ju");
#undef CHECK_NUMERIC_PARAM

    if (!true_param)
        TEST_VERDICT("'true_param' is FALSE");

    if (false_param)
        TEST_VERDICT("'false_param' is TRUE");

    if (opt_str_none_param != NULL)
        TEST_VERDICT("'opt_str_none_param' is not null");

    CHECK_NOT_NULL(opt_str_val_param);
    if (strcmp(opt_str_val_param, "value") != 0)
    {
        TEST_VERDICT("Unexpected value for 'opt_str_val_param': '%s'",
                     opt_str_val_param);
    }

#define CHECK_OPTNUMERIC_PARAMS(_nonename, _valname,  _expected, _fmt) \
    do {                                                                \
        if ((_nonename).defined)                                        \
        {                                                               \
            TEST_VERDICT("'%s' has a defined value: " _fmt,             \
                         #_nonename, (_nonename).value);                \
        }                                                               \
                                                                        \
        if (!(_valname).defined)                                        \
        {                                                               \
            TEST_VERDICT("'%s' does not have a defined value",          \
                         #_valname);                                    \
        }                                                               \
                                                                        \
        if ((_valname).value != (_expected))                            \
        {                                                               \
            TEST_VERDICT("'%s' has unexpected value: " _fmt,            \
                         #_valname, (_valname).value);                  \
        }                                                               \
    } while(0)

    CHECK_OPTNUMERIC_PARAMS(opt_uint_none_param, opt_uint_val_param, 42, "%u");
    CHECK_OPTNUMERIC_PARAMS(opt_dbl_none_param, opt_dbl_val_param, 42.0, "%g");
    CHECK_OPTNUMERIC_PARAMS(opt_unit_none_param, opt_unit_val_param,
                            1e6, "%g");
#undef CHECK_OPTNUMERIC_PARAMS

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
