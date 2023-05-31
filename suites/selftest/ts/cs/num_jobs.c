/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Testing get number of jobs depends on CPU cores or threads number.
 *
 * Testing get number of jobs depends on CPU cores or threads number.
 */

/** @page cs-num_jobs Testing get number of jobs depends on CPU cores or threads
 *                    number.
 *
 * @objective Check that number counted correctly.
 *
 * @par Scenario:
 */

#define TE_TEST_NAME "cs/num_jobs"

#ifndef TEST_START_VARS
/**
 * Test suite specific variables of the test @b main() function.
 */
#define TEST_START_VARS TEST_START_ENV_VARS
#endif

#ifndef TEST_START_SPECIFIC
/**
 * Test suite specific the first actions of the test.
 */
#define TEST_START_SPECIFIC TEST_START_ENV
#endif

#ifndef TEST_END_SPECIFIC
/**
 * Test suite specific part of the last action of the test @b main()
 * function.
 */
#define TEST_END_SPECIFIC TEST_END_ENV
#endif

#include "te_config.h"
#include "te_str.h"

#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_cfg_cpu.h"

static void
get_pair_expr_result(te_string *res_expr, unsigned int *res_value,
                     const char *ta, unsigned int value, unsigned int factor,
                     const char *type, unsigned int divisor, int displacement,
                     unsigned int max, unsigned int min)
{
    int expr_value = 1;
    size_t threads = 0;

    if (value != 0)
    {
        te_string_append(res_expr, "%u", value);
        *res_value = value;
        return;
    }

    if (factor != 0)
    {
        te_string_append(res_expr, "%u", factor);
        expr_value *= factor;
    }

    te_string_append(res_expr, "%s", type);

    if (te_str_strip_prefix(type, TAPI_CFG_CPU_NPROC_FACTOR) != NULL)
        tapi_cfg_get_all_threads(ta, &threads , NULL);
    else if (te_str_strip_prefix(type, TAPI_CFG_CPU_NCORES_FACTOR) != NULL)
        tapi_cfg_get_cpu_cores(ta, &threads, NULL);
    else
        TEST_VERDICT("Failed to parse type");

    expr_value *= threads;

    if (divisor != 0)
    {
        te_string_append(res_expr, "/%u", divisor);
        expr_value /= divisor;
    }
    if (displacement != 0)
    {
        te_string_append(res_expr, "%+d", displacement);
        expr_value += displacement;
    }
    if (max != 0)
    {
        te_string_append(res_expr, "<%u", max);
        expr_value = MIN((int)max, expr_value);
    }
    if (min != 0)
    {
        te_string_append(res_expr, ">%u", min);
        expr_value = MAX((int)min, expr_value);
    }

    *res_value = MAX(1, expr_value);
}

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;

    te_string expr = TE_STRING_INIT;
    unsigned int expected_value;
    unsigned int calculated_value;

    unsigned int value;
    const char *type;
    unsigned int factor;
    unsigned int divisor;
    int displacement;
    unsigned int max;
    unsigned int min;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_UINT_PARAM(value);
    TEST_GET_OPT_STRING_PARAM(type);
    TEST_GET_UINT_PARAM(factor);
    TEST_GET_UINT_PARAM(divisor);
    TEST_GET_INT_PARAM(displacement);
    TEST_GET_UINT_PARAM(max);
    TEST_GET_UINT_PARAM(min);

    TEST_STEP("Generate expression and expected result by params");
    get_pair_expr_result(&expr, &expected_value, pco_iut->ta,
                         value, factor, te_str_empty_if_null(type),
                         divisor, displacement, max, min);

    TEST_STEP("Calculate jobs number by expression");
    RING("Expression: \"%s\"", expr.ptr);
    CHECK_RC(tapi_cfg_cpu_calculate_numjobs(pco_iut->ta, expr.ptr,
                                            &calculated_value));

    TEST_STEP("Compare result with expected value");
    if (calculated_value != expected_value)
    {
        TEST_VERDICT("Calculated %u, but expected %u",
                     calculated_value, expected_value);
    }

    TEST_SUCCESS;

cleanup:
    te_string_free(&expr);

    TEST_END;
}
