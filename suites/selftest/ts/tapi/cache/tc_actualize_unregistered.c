/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME  "tc_actualize_unregistered"

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cache.h"


int
main(int argc, char **argv)
{
    const char *area;
    const char *method;
    te_errno    res;

    TEST_START;
    TEST_GET_STRING_PARAM(area);
    TEST_GET_STRING_PARAM(method);

    TEST_STEP("Actualize unregistered area");
    RING("Actualize area '%s' with method '%s'", area, method);
    res = tapi_cache_actualize(method, NULL, "%s", area);
    if (res == 0)
        TEST_VERDICT("Unregistered area has been actualized unexpectedly");
    else if (TE_RC_GET_ERROR(res) != TE_ENOENT)
        CHECK_RC(res);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
