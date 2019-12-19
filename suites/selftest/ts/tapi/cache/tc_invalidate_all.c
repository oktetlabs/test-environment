/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#define TE_TEST_NAME  "tc_invalidate_all"

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cache.h"

#define WORKAREA "foo"

/* Callbacks opaque */
typedef struct opaque_t {
    const char **subinstances;
    int          num_subinstances;
} opaque_t;

static te_errno
cb_common(const char *method, const char *oid, void *opaque)
{
    te_errno   rc;
    opaque_t  *op = opaque;
    int        i;
    cfg_handle handle_root;
    cfg_handle handle;

    rc = cfg_find_str(oid, &handle_root);
    if (rc != 0)
    {
        rc = cfg_add_instance_str(oid, &handle_root, CFG_VAL(NONE, NULL));
        if (rc != 0)
            return rc;
    }
    for (i = 0; i < op->num_subinstances; i++)
    {
        if (strcmp(op->subinstances[i], "nil") == 0)
        {
            handle = handle_root;
        }
        else
        {
            rc = cfg_find_fmt(&handle, "%s/bar:%s", oid, op->subinstances[i]);
            if (rc != 0)
            {
                rc = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                        "%s/bar:%s", oid, op->subinstances[i]);
                if (rc != 0)
                    break;
            }
        }
        rc = cfg_add_instance_child_fmt(NULL, CFG_VAL(STRING, method), handle,
                                        "/baz:%s", method);
        if (rc != 0)
            break;
        rc = cfg_add_instance_child_fmt(NULL, CFG_VAL(STRING, method), handle,
                                        "/qux:%s", method);
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

/* It depends on cb_common() implementation */
static te_bool
test_invalidation(const char *area, const char **subs, int num_subs,
                  const char **methods, size_t num_methods)
{
    te_bool success = TRUE;
    te_errno rc;
    int i;

    rc = cfg_find_fmt(NULL, "%s/%s", TAPI_CACHE_ROOT_INST, area);
    if (rc != 0)
    {
        success = FALSE;
        ERROR_VERDICT("Unexpectedly removed registered area '%s': %r",
                      area, rc);
    }

    for (i = 0; i < num_subs; i++)
    {
        if (strcmp(subs[i], "nil") != 0)
        {
            rc = cfg_find_fmt(NULL, "%s/%s/bar:%s",
                              TAPI_CACHE_ROOT_INST, area, subs[i]);
            if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
            {
                success = FALSE;
                ERROR_VERDICT("Failed to invalidate area '%s/bar:%s': %r",
                              area, subs[i], rc);
            }
        }
        else
        {
            const char *leafs[] = {"baz", "qux"};
            size_t j, k;

            for (j = 0; j < num_methods; j++)
            {
                for (k = 0; k < TE_ARRAY_LEN(leafs); k++)
                {
                    rc = cfg_find_fmt(NULL, "%s/%s/%s:%s", TAPI_CACHE_ROOT_INST,
                                      area, leafs[k], methods[j]);
                    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
                    {
                        success = FALSE;
                        ERROR_VERDICT("Failed to invalidate leaf '%s/%s:%s': "
                                      "%r", area, leafs[k], methods[j], rc);
                    }
                }
            }
        }
    }

    return success;
}


int
main(int argc, char **argv)
{
    char   **areas;
    size_t   num_areas;
    char   **subinstances;
    size_t   num_subinstances;
    char   **methods;
    size_t   num_methods;
    te_bool  inv_method_by_method;
    size_t   i;
    size_t   j;
    opaque_t op;
    te_bool  test_ok = TRUE;

    TEST_START;
    TEST_GET_STRING_LIST_PARAM(areas, num_areas);
    TEST_GET_STRING_LIST_PARAM(subinstances, num_subinstances);
    TEST_GET_STRING_LIST_PARAM(methods, num_methods);
    TEST_GET_BOOL_PARAM(inv_method_by_method);
    op.subinstances = (const char **)subinstances;
    op.num_subinstances = num_subinstances;

    TEST_STEP("Register all supported methods on area");
    for (i = 0; i < TE_ARRAY_LEN(cbs); i++)
    {
        RING("Register method '%s' on area '%s'", cbs[i].method, WORKAREA);
        CHECK_RC(tapi_cache_register(cbs[i].method, WORKAREA, cbs[i].cb_func));
    }

    TEST_STEP("Actualize an area");
    for (i = 0; i < num_areas; i++)
    {
        for (j = 0; j < num_methods; j++)
        {
            RING("Actualize area '%s' with method '%s'", areas[i], methods[j]);
            CHECK_RC(tapi_cache_actualize(methods[j], &op, "%s", areas[i]));
        }
    }

    TEST_STEP("Invalidate an area");
    for (i = 0; i < num_areas; i++)
    {
        CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
        if (inv_method_by_method)
        {
            for (j = 0; j < num_methods; j++)
                CHECK_RC(tapi_cache_invalidate(methods[j], "%s", areas[i]));
        }
        else
        {
            CHECK_RC(tapi_cache_invalidate(NULL, "%s", areas[i]));
        }
        /* Check invalidation results */
        test_ok = test_invalidation((const char *)areas[i],
                                    (const char **)subinstances,
                                    num_subinstances,
                                    (const char **)methods, num_methods);
    }

    if (!test_ok)
        TEST_FAIL("Invalidation works improperly");

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    TEST_END;
}
