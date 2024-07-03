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
    uint64_t uint64_param = 0ull;
    double dbl_param = 0.0;
    double unit_param = 0.0;
    uintmax_t bin_unit_param = 0;
    bool true_param = false;
    bool false_param = true;
    const char *opt_str_none_param = "";
    const char *opt_str_val_param = NULL;
    te_optional_uint_t opt_uint_none_param = TE_OPTIONAL_UINT_VAL(0);
    te_optional_uint_t opt_uint_val_param = TE_OPTIONAL_UINT_UNDEF;
    te_optional_uintmax_t opt_uint64_none_param = TE_OPTIONAL_UINTMAX_VAL(0);
    te_optional_uintmax_t opt_uint64_val_param = TE_OPTIONAL_UINTMAX_UNDEF;
    te_optional_double_t opt_dbl_none_param = TE_OPTIONAL_DOUBLE_VAL(0.0);
    te_optional_double_t opt_dbl_val_param = TE_OPTIONAL_DOUBLE_UNDEF;
    te_optional_double_t opt_unit_none_param = TE_OPTIONAL_DOUBLE_VAL(0.0);
    te_optional_double_t opt_unit_val_param = TE_OPTIONAL_DOUBLE_UNDEF;
    te_optional_uintmax_t opt_bin_unit_none_param = TE_OPTIONAL_UINTMAX_VAL(0);
    te_optional_uintmax_t opt_bin_unit_val_param = TE_OPTIONAL_UINTMAX_UNDEF;
    tapi_test_expected_result good_result;
    tapi_test_expected_result good_result_noprefix;
    tapi_test_expected_result good_int_result;
    tapi_test_expected_result bad_result;
    tapi_test_expected_result bad_result_nomodule;
    tapi_test_expected_result bad_result_output;
    tapi_test_expected_result bad_int_result;

    TEST_START;

    TEST_STEP("Getting required parameters");

    TEST_GET_STRING_PARAM(str_param);
    TEST_GET_INT_PARAM(int_param);
    TEST_GET_UINT_PARAM(uint_param);
    TEST_GET_UINT64_PARAM(uint64_param);
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
    TEST_GET_OPT_UINT64_PARAM(opt_uint64_none_param);
    TEST_GET_OPT_UINT64_PARAM(opt_uint64_val_param);
    TEST_GET_OPT_DOUBLE_PARAM(opt_dbl_none_param);
    TEST_GET_OPT_DOUBLE_PARAM(opt_dbl_val_param);
    TEST_GET_OPT_VALUE_UNIT_PARAM(opt_unit_none_param);
    TEST_GET_OPT_VALUE_UNIT_PARAM(opt_unit_val_param);
    TEST_GET_OPT_VALUE_BIN_UNIT_PARAM(opt_bin_unit_none_param);
    TEST_GET_OPT_VALUE_BIN_UNIT_PARAM(opt_bin_unit_val_param);

    TEST_STEP("Getting expected result parameters");
    TEST_GET_EXPECTED_RESULT_PARAM(good_result);
    TEST_GET_EXPECTED_RESULT_PARAM(good_result_noprefix);
    TEST_GET_EXPECTED_RESULT_PARAM(good_int_result);
    TEST_GET_EXPECTED_RESULT_PARAM(bad_result);
    TEST_GET_EXPECTED_RESULT_PARAM(bad_result_nomodule);
    TEST_GET_EXPECTED_RESULT_PARAM(bad_result_output);
    TEST_GET_EXPECTED_RESULT_PARAM(bad_int_result);

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
    CHECK_NUMERIC_PARAM(uint64_param, 42ull, "%" PRIu64);
    CHECK_NUMERIC_PARAM(dbl_param, 42.0, "%g");
    CHECK_NUMERIC_PARAM(unit_param, 1e6, "%g");
    CHECK_NUMERIC_PARAM(bin_unit_param, 1ull << 20, "%ju");
#undef CHECK_NUMERIC_PARAM

    if (!true_param)
        TEST_VERDICT("'true_param' is false");

    if (false_param)
        TEST_VERDICT("'false_param' is true");

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
    CHECK_OPTNUMERIC_PARAMS(opt_uint64_none_param, opt_uint64_val_param, 42ull,
                            "%" PRIu64);
    CHECK_OPTNUMERIC_PARAMS(opt_dbl_none_param, opt_dbl_val_param, 42.0, "%g");
    CHECK_OPTNUMERIC_PARAMS(opt_unit_none_param, opt_unit_val_param,
                            1e6, "%g");
    CHECK_OPTNUMERIC_PARAMS(opt_bin_unit_none_param, opt_bin_unit_val_param,
                            1ull << 20, "%ju");
#undef CHECK_OPTNUMERIC_PARAMS

    TEST_STEP("Checking expected results");
#define CHECK_EXPECTED(_var, _rc, _value) \
    do {                                                                \
        if (!tapi_test_check_expected_result(&(_var), (_rc), (_value))) \
            TEST_VERDICT("Expected result for '%s' "                    \
                         "is not recognized", #_var);                   \
    } while (0)
#define CHECK_UNEXPECTED(_var, _rc, _value)                               \
    do {                                                                \
        if (tapi_test_check_expected_result(&(_var), (_rc), (_value)))  \
            TEST_VERDICT("Unexpected result for '%s' "                  \
                         "is recognized as expected", #_var);           \
    } while (0)
    CHECK_EXPECTED(good_result, 0, "value");
    CHECK_EXPECTED(good_result_noprefix, 0, "value");
    CHECK_UNEXPECTED(good_result, 0, "mismatched value");
    CHECK_UNEXPECTED(good_result, TE_ENOENT, NULL);

    if (!tapi_test_check_expected_int_result(&good_int_result, 0, 42))
        TEST_VERDICT("Expected integral result is not recognized");
    if (tapi_test_check_expected_int_result(&good_int_result, 0, 43))
        TEST_VERDICT("Unexpected integral result considered expected");

    CHECK_EXPECTED(bad_result, TE_RC(TE_TAPI, TE_ENOENT), NULL);
    CHECK_UNEXPECTED(bad_result, 0, NULL);
    CHECK_UNEXPECTED(bad_result, TE_RC(TE_TA_UNIX, TE_ENOENT), NULL);
    CHECK_UNEXPECTED(bad_result, TE_RC(TE_TAPI, TE_ENOENT), "value");
    CHECK_EXPECTED(bad_result_nomodule, TE_RC(TE_TAPI, TE_ENOENT), NULL);
    CHECK_EXPECTED(bad_result_nomodule, TE_RC(TE_TA_UNIX, TE_ENOENT), NULL);
    CHECK_EXPECTED(bad_result_output, TE_RC(TE_TAPI, TE_ENOENT), "value");
    CHECK_UNEXPECTED(bad_result_output, TE_RC(TE_TAPI, TE_ENOENT),
                     "mismatched value");
    CHECK_UNEXPECTED(bad_result_output, TE_RC(TE_TAPI, TE_ENOENT), NULL);

    if (!tapi_test_check_expected_int_result(&bad_int_result, TE_ENOENT, 42))
        TEST_VERDICT("Expected integral result is not recognized");
    if (tapi_test_check_expected_int_result(&bad_int_result, TE_ENOENT, 43))
        TEST_VERDICT("Unexpected integral result considered expected");
    if (!tapi_test_check_expected_int_result(&bad_result,
                                             TE_RC(TE_TAPI, TE_ENOENT), 42))
        TEST_VERDICT("Expected integral result is not recognized");
    if (!tapi_test_check_expected_int_result(&bad_result,
                                             TE_RC(TE_TAPI, TE_ENOENT), 43))
        TEST_VERDICT("Expected integral result is not recognized");
#undef CHECK_EXPECTED
#undef CHECK_UNEXPECTED

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
