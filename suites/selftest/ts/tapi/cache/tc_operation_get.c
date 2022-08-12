/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_TEST_NAME  "tc_operation_get"

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cache.h"

int
main(int argc, char **argv)
{
    const char   *instance;
    const char   *method;
    const char   *expected_value;
    char         *val;
    cfg_val_type  type = CVT_STRING;

    TEST_START;

    TEST_GET_STRING_PARAM(instance);
    TEST_GET_STRING_PARAM(method);
    TEST_GET_STRING_PARAM(expected_value);

    TEST_STEP("Get value from the cache");
    CHECK_RC(tapi_cache_get(&type, &val, "%s%s", instance, method));
    if (strcmp(val, expected_value) != 0)
    {
        ERROR_VERDICT("Unexpected instance value");
        TEST_FAIL("Value mismatch: obtained('%s') != expected('%s')",
                  val, expected_value);
    }

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
