/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test the scalar types dynamic casting
 *
 * Testing the dynamic casting of scalar types.
 */

/** @page tools_te_scalar_type te_scalar_dynamic_cast test
 *
 * @objective Testing the dynamic casting of scalar types.
 *
 * Test the correctness of scalar types casting.
 *
 * @par Test sequence:
 */

#define TE_TEST_NAME    "tools/te_scalar_type"

#include "te_config.h"

#include "tapi_test.h"
#include "te_numeric.h"
#include "te_enum.h"
#include "te_string.h"

#define FIELD(type_, c_type_, ...) c_type_ v_##type_;
typedef union any_scalar {
    TE_SCALAR_TYPE_LIST(FIELD)
} any_scalar;
#undef FIELD

static void
check_cast_func(intmax_t val, te_scalar_type src_type, te_scalar_type dst_type,
                bool expect_trunc)
{
    te_errno rc;
    any_scalar src;
    any_scalar dst;

#define CASE(scalar_type_, src_type_, ...) \
    case(scalar_type_):                         \
    {                                           \
        src.v_##scalar_type_ = (src_type_)val;  \
        break;                                  \
    }
    switch(src_type)
    {
        TE_SCALAR_TYPE_LIST(CASE)
    }
    #undef CASE

    rc = te_scalar_dynamic_cast(src_type, &src, dst_type, &dst);
    if (rc != (expect_trunc ? TE_EOVERFLOW : 0))
        TEST_VERDICT("Unexpected result %r", rc);

    if (!expect_trunc)
    {
        any_scalar src_back;

        CHECK_RC(te_scalar_dynamic_cast(dst_type, &dst, src_type, &src_back));
        if (memcmp(&src, &src_back, te_scalar_type_sizeof(src_type)) != 0)
            TEST_VERDICT("The value was not preserved");
    }
}

static te_scalar_type
str2scalar_type(const char *type)
{
    int scalar_type;

    scalar_type = te_enum_map_from_str(te_scalar_type_names, type, INT_MIN);
    if (scalar_type == INT_MIN)
        TEST_FAIL("Invalid value of type test parameter: '%s'", type);

    return (te_scalar_type)scalar_type;
}

int
main(int argc, char **argv)
{
    const char *src_type;
    const char *dst_type;
    int src_scalar_type;
    int dst_scalar_type;
    uintmax_t src_max;
    uintmax_t dst_max;
    intmax_t src_min;
    intmax_t dst_min;

    bool is_trunc = false;

    TEST_START;
    TEST_GET_STRING_PARAM(src_type);
    TEST_GET_STRING_PARAM(dst_type);

    src_scalar_type = str2scalar_type(src_type);
    dst_scalar_type = str2scalar_type(dst_type);

    src_max = te_scalar_type_max(src_scalar_type);
    dst_max = te_scalar_type_max(dst_scalar_type);
    src_min = te_scalar_type_min(src_scalar_type);
    dst_min = te_scalar_type_min(dst_scalar_type);

    TEST_STEP("Test the casting of maximum value");
    /* src_max > dst_max => src_size > dst_size or src_sign != dst_sign */
    check_cast_func((intmax_t)src_max, src_scalar_type, dst_scalar_type,
                    src_max > dst_max);

    TEST_STEP("Test the casting of minimum value");
    /* src_min < dst_min => src_size > dst_size or src_sign != dst_sign */
    check_cast_func(src_min, src_scalar_type, dst_scalar_type,
                    src_min < dst_min);

    TEST_STEP("Test the casting of '1' value");
    check_cast_func(1, src_scalar_type, dst_scalar_type, false);

    TEST_STEP("Test the casting of '-1' value");
    if (te_scalar_type_is_signed(src_scalar_type))
        is_trunc = te_scalar_type_is_signed(dst_scalar_type) ? false : true;
    else
        is_trunc = (src_max > dst_max);

    check_cast_func(-1, src_scalar_type, dst_scalar_type, is_trunc);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
