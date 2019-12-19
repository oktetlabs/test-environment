/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#define TE_TEST_NAME  "prologue"

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cache.h"

#define WORKAREA "foo:*"

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Invalidate all areas");
    CLEANUP_CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    CHECK_RC(tapi_cache_del("%s", WORKAREA));

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    TEST_END;
}
