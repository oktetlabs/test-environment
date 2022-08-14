/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME  "tc_operation_del"

#include "te_defs.h"
#include "logger_api.h"
#include "te_string.h"
#include "tapi_test.h"
#include "tapi_cache.h"

/* Callbacks opaque */
typedef struct opaque_t {
    te_bool expected_found;
    int     num_found;
} opaque_t;

static te_errno
cb_func(cfg_handle handle, void *opaque)
{
    opaque_t *op = opaque;
    char     *oid;
    te_errno  rc;

    op->num_found++;

    rc = cfg_get_oid_str(handle, &oid);
    if (op->expected_found)
        RING("Found '%s'", oid);
    else
        ERROR("Unexpectedly found '%s'", oid);

    return rc;
}

int
main(int argc, char **argv)
{
    const char  *instance;
    char       **expected_existing;
    int          num_expected_existing;
    char       **expected_missing;
    int          num_expected_missing;
    int          i;
    opaque_t     op;
    te_bool      test_ok = TRUE;

    TEST_START;

    TEST_GET_STRING_PARAM(instance);
    TEST_GET_STRING_LIST_PARAM(expected_existing, num_expected_existing);
    TEST_GET_STRING_LIST_PARAM(expected_missing, num_expected_missing);

    CLEANUP_CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    TEST_STEP("Remove instance from the cache");
    CHECK_RC(tapi_cache_del("%s", instance));
    TEST_STEP("Check whether it has not removed unexpected instances");
    op.expected_found = TRUE;
    for (i = 0; i < num_expected_existing; i++)
    {
        op.num_found = 0;
        CHECK_RC(tapi_cache_find(cb_func, &op, "%s", expected_existing[i]));
        if (op.num_found == 0)
        {
            test_ok = FALSE;
            ERROR("Unexpected removed '%s'", expected_existing[i]);
            ERROR_VERDICT("Unexpected instances have been removed");
        }
    }
    TEST_STEP("Check whether it has removed requested instances properly");
    op.expected_found = FALSE;
    for (i = 0; i < num_expected_missing; i++)
    {
        op.num_found = 0;
        CHECK_RC(tapi_cache_find(cb_func, &op, "%s", expected_missing[i]));
        if (op.num_found != 0)
        {
            test_ok = FALSE;
            ERROR_VERDICT("Requested instances have not been removed");
        }
    }

    if (!test_ok)
        TEST_FAIL("Delete operation works improperly");

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    TEST_END;
}
