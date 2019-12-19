/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#define TE_TEST_NAME  "tc_register"

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cache.h"

te_errno
cb_func(const char *oid, void *opaque)
{
    UNUSED(oid);
    UNUSED(opaque);

    ERROR("Not implemented yet");
    return TE_ENOSYS;
}

int
main(int argc, char **argv)
{
    char       **areas;
    char       **methods;
    int          num_areas;
    int          num_methods;
    int          i;
    int          j;

    TEST_START;
    TEST_GET_STRING_LIST_PARAM(areas, num_areas);
    TEST_GET_STRING_LIST_PARAM(methods, num_methods);

    TEST_STEP("Register methods on area");

    for (i = 0; i < num_areas; i++)
    {
        for (j = 0; j < num_methods; j++)
        {
            RING("Register method '%s' on area '%s'", methods[j], areas[i]);
            CHECK_RC(tapi_cache_register(methods[j], areas[i], cb_func));
        }
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
