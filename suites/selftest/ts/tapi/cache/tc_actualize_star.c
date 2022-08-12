/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#define TE_TEST_NAME  "tc_actualize_star"

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cache.h"


/* Callbacks opaque */
typedef struct opaque_t {
    const char **instances;
    int          num_instances;
} opaque_t;

static te_errno
cb_common(const char *method, const char *oid, void *opaque)
{
    te_errno    rc;
    opaque_t   *op = opaque;
    int         i;
    cfg_handle  handle;
    char       *name;

    assert(oid != NULL);

    name = strrchr(oid, ':');
    if (name == NULL || strcmp(name, ":*") != 0)
    {
        rc = cfg_find_str(oid, &handle);
        if (rc != 0)
            return rc;
        rc = cfg_add_instance_child_fmt(NULL, CFG_VAL(STRING, method), handle,
                                        "/baz:%s", method);
    }
    else for (i = 0; i < op->num_instances; i++)
    {
        /* It is expected "*" symbol at the oid string end */
        rc = cfg_find_fmt(&handle, "%.*s%s", (int)(strlen(oid) - 1), oid,
                          op->instances[i]);
        if (rc != 0)
        {
            rc = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                      "%.*s%s", (int)(strlen(oid) - 1), oid,
                                      op->instances[i]);
            if (rc != 0)
                break;
        }
        rc = cfg_add_instance_child_fmt(NULL, CFG_VAL(STRING, method), handle,
                                        "/baz:%s", method);
        if (rc != 0)
            break;
    }
    return rc;
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

int
main(int argc, char **argv)
{
    char **areas;
    size_t num_areas;
    char **instances;
    size_t num_instances;
    char **methods;
    size_t num_methods;
    size_t    i;
    size_t    j;
    opaque_t op;

    TEST_START;
    TEST_GET_STRING_LIST_PARAM(areas, num_areas);
    TEST_GET_STRING_LIST_PARAM(instances, num_instances);
    TEST_GET_STRING_LIST_PARAM(methods, num_methods);
    op.instances = (const char **)instances;
    op.num_instances = num_instances;

    TEST_STEP("Register all supported methods on area");
    for (i = 0; i < TE_ARRAY_LEN(cbs); i++)
    {
        RING("Register method '%s' on area '%s'", cbs[i].method, "foo");
        CHECK_RC(tapi_cache_register(cbs[i].method, "foo", cbs[i].cb_func));
        RING("Register method '%s' on area '%s'", cbs[i].method, "foo/bar");
        CHECK_RC(tapi_cache_register(cbs[i].method, "foo/bar", cbs[i].cb_func));
    }

    TEST_STEP("Actualize areas by pattern \"*\"");
    for (i = 0; i < num_areas; i++)
    {
        for (j = 0; j < num_methods; j++)
        {
            RING("Actualize area '%s' with method '%s'", areas[i], methods[j]);
            CHECK_RC(tapi_cache_actualize(methods[j], &op, "%s", areas[i]));
        }
    }

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    TEST_END;
}
