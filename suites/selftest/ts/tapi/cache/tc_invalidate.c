/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_TEST_NAME  "tc_invalidate"

#include "te_defs.h"
#include "logger_api.h"
#include "te_string.h"
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
    char   **areas;
    size_t   num_areas;
    char   **subinstances;
    size_t   num_subinstances;
    char   **methods;
    size_t   num_methods;
    char   **inv_methods;
    size_t   num_inv_methods;
    const char *inv_area;
    char   **expected_missing;
    int      num_expected_missing;
    size_t   i;
    size_t   j;
    opaque_t op;
    te_bool  test_ok = TRUE;

    TEST_START;
    TEST_GET_STRING_LIST_PARAM(areas, num_areas);
    TEST_GET_STRING_LIST_PARAM(subinstances, num_subinstances);
    TEST_GET_STRING_LIST_PARAM(methods, num_methods);
    TEST_GET_STRING_LIST_PARAM(inv_methods, num_inv_methods);
    TEST_GET_STRING_PARAM(inv_area);
    TEST_GET_STRING_LIST_PARAM(expected_missing, num_expected_missing);
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
    CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    for (j = 0; j < num_inv_methods; j++)
        CHECK_RC(tapi_cache_invalidate(inv_methods[j], "%s", inv_area));

    TEST_STEP("Verify invalidation");
    for (i = 0; i < num_areas; i++)
    {
        for (j = 0; j < num_subinstances; j++)
        {
            size_t k;

            for (k = 0; k < num_methods; k++)
            {
                size_t l;
                unsigned expected_rc = 0;
                te_string ar = TE_STRING_INIT_STATIC(CFG_OID_MAX);

                if (strcmp(subinstances[j],"nil") == 0)
                {
                    CHECK_RC(te_string_append(&ar, "%s", areas[i]));
                }
                else
                {
                    CHECK_RC(te_string_append(&ar, "%s/bar:%s",
                                              areas[i], subinstances[j]));
                }

                rc = cfg_find_fmt(NULL, "%s/%s/baz:%s",
                                  TAPI_CACHE_ROOT_INST, ar.ptr, methods[k]);

                for (l = 0; l < num_inv_methods; l++)
                {
                    int m;

                    if (strcmp(inv_methods[l], methods[k]) != 0)
                        continue;

                    for (m = 0; m < num_expected_missing; m++)
                    {
                        if (strcmp(expected_missing[m], ar.ptr) == 0)
                            expected_rc = TE_ENOENT;
                    }
                }

                if (TE_RC_GET_ERROR(rc) != expected_rc)
                {
                    test_ok = FALSE;

                    switch (TE_RC_GET_ERROR(rc))
                    {
                        case 0:
                            ERROR_VERDICT("Area '%s' of method '%s' has not "
                                          "been invalidated",
                                          ar.ptr, methods[k]);
                            break;

                        case TE_ENOENT:
                            ERROR_VERDICT("Area '%s' of method '%s' has been "
                                          "unexpectedly invalidated",
                                          ar.ptr, methods[k]);
                            break;

                        default:
                            CHECK_RC(rc);
                    }
                }
            }
        }
    }

    if (!test_ok)
        TEST_FAIL("Invalidation works improperly");

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    TEST_END;
}
