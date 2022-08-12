/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#define TE_TEST_NAME  "tc_actualize_sub"

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cache.h"

#define WORKAREA "foo/bar"

static te_errno
cb_common(const char *method, const char *oid, void *opaque)
{
    te_errno   rc;
    cfg_handle handle;

    UNUSED(opaque);

    rc = cfg_find_str(oid, &handle);
    if (rc != 0)
    {
        rc = cfg_add_instance_str(oid, &handle, CFG_VAL(NONE, NULL));
        if (rc != 0)
            return rc;
    }
    rc = cfg_add_instance_child_fmt(NULL, CFG_VAL(STRING, method), handle,
                                    "/baz:%s", method);
    if (rc != 0)
        return rc;

    return cfg_add_instance_child_fmt(NULL, CFG_VAL(STRING, method), handle,
                                    "/qux:%s", method);
}

static te_errno
cb_m1(const char *oid, void *opaque)
{
    RING("It is a callback %s() working on '%s'", __FUNCTION__, oid);

    return cb_common("m1", oid, opaque);
}

static te_errno
cb_m2(const char *oid, void *opaque)
{
    RING("It is a callback %s() working on '%s'", __FUNCTION__, oid);

    return cb_common("m2", oid, opaque);
}

static te_errno
cb_m3(const char *oid, void *opaque)
{
    RING("It is a callback %s() working on '%s'", __FUNCTION__, oid);

    return cb_common("m3", oid, opaque);
}

static const struct {
    const char *method;
    tapi_cache_cb cb_func;
} cbs[] = {
    { .method = "m1", .cb_func = cb_m1 },
    { .method = "m2", .cb_func = cb_m2 },
    { .method = "m3", .cb_func = cb_m3 },
};

static void
create_area(const char *area)
{
    te_errno rc;

    rc = cfg_find_fmt(NULL, "%s/%s", TAPI_CACHE_ROOT_INST, area);
    if (rc != 0)
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, NULL),
                                  "%s/%s", TAPI_CACHE_ROOT_INST, area);

    CHECK_RC(rc);
}

int
main(int argc, char **argv)
{
    char **areas;
    size_t num_areas;
    char **subinstances;
    size_t num_subinstances;
    char **methods;
    size_t num_methods;
    size_t i;
    size_t j;
    size_t k;

    TEST_START;
    TEST_GET_STRING_LIST_PARAM(areas, num_areas);
    TEST_GET_STRING_LIST_PARAM(subinstances, num_subinstances);
    TEST_GET_STRING_LIST_PARAM(methods, num_methods);

    TEST_STEP("Register all supported methods on area");
    for (i = 0; i < TE_ARRAY_LEN(cbs); i++)
    {
        RING("Register method '%s' on area '%s'", cbs[i].method, WORKAREA);
        CHECK_RC(tapi_cache_register(cbs[i].method, WORKAREA, cbs[i].cb_func));
    }

    TEST_STEP("Actualize an area");
    for (i = 0; i < num_areas; i++)
    {
        create_area(areas[i]);
        for (j = 0; j < num_subinstances; j++)
        {
            for (k = 0; k < num_methods; k++)
            {
                RING("Actualize area '%s:%s' with method '%s'",
                     areas[i], subinstances[j], methods[k]);
                CHECK_RC(tapi_cache_actualize(methods[k], NULL, "%s/bar:%s",
                                              areas[i], subinstances[j]));
            }
        }
    }

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    TEST_END;
}
