/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#define TE_TEST_NAME  "tc_actualize_indirect"

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cache.h"

#define ACT_ERROR_TYPE_MAPPING_LIST \
    { "0",      0 },                \
    { "ENOENT", TE_ENOENT },        \
    { "ECHILD", TE_ECHILD }

#define TEST_GET_ACT_ERROR_TYPE(_var_name) \
    TEST_GET_ENUM_PARAM(_var_name, ACT_ERROR_TYPE_MAPPING_LIST)

static te_errno
cb_common(const char *method, const char *oid, void *opaque)
{
    te_errno   rc;
    cfg_handle handle;

    UNUSED(opaque);

    rc = cfg_find_str(oid, &handle);
    if (rc != 0)
        return rc;

    rc = cfg_add_instance_child_fmt(NULL, CFG_VAL(STRING, method), handle,
                                    "/baz:%s", method);
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

static void
init_test(const char **init_inst, int num_init_inst,
          const char **init_subinst, int num_init_subinst)
{
    int i, j;
    cfg_handle handle;

    CHECK_RC(tapi_cache_del("%s", TAPI_CACHE_ALL));
    for (i = 0; i < num_init_inst; i++)
    {
        CHECK_RC(cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                    "%s/foo:%s", TAPI_CACHE_ROOT_INST, init_inst[i]));

        for (j = 0; j < num_init_subinst; j++)
        {
            CHECK_RC(cfg_add_instance_child_fmt(NULL, CFG_VAL(NONE, NULL),
                                        handle, "/bar:%s", init_subinst[j]));
        }

        CHECK_RC(cfg_add_instance_fmt(NULL, CFG_VAL(NONE, NULL),
                    "%s/foo1:%s", TAPI_CACHE_ROOT_INST, init_inst[i]));
    }
}

static te_bool
test_act(const char **init_inst, int num_init_inst,
         const char **init_subinst, int num_init_subinst,
         const char **methods, int num_methods,
         const char **expected_act, int num_expected_act)
{
    te_bool  success = TRUE;
    unsigned foo_expected = TE_ENOENT;
    unsigned foo_bar_expected = TE_ENOENT;
    unsigned foo1_expected = TE_ENOENT;
    te_errno rc;
    int i, j, k;

    for (i = 0; i < num_expected_act; i++)
    {
        if (strcmp(expected_act[i], "foo") == 0)
        {
            foo_expected = 0;
        }
        else if (strcmp(expected_act[i], "foo/bar") == 0)
        {
            foo_bar_expected = 0;
        }
        else if (strcmp(expected_act[i], "foo1") == 0)
        {
            foo1_expected = 0;
        }
        else if (strcmp(expected_act[i], "nil") != 0)
        {
            TEST_FAIL("Unexpected area_reg: '%s'", expected_act[i]);
        }
    }

    for (i = 0; i < num_init_inst; i++)
    {
        for (k = 0; k < num_methods; k++)
        {
            rc = cfg_find_fmt(NULL, "%s/foo:%s/baz:%s", TAPI_CACHE_ROOT_INST,
                              init_inst[i], methods[k]);
            if (TE_RC_GET_ERROR(rc) != foo_expected)
            {
                success = FALSE;
                ERROR_VERDICT("Unexpected status of instance 'foo:%s/baz:%s': "
                              "%r", init_inst[i], methods[k], rc);
            }

            rc = cfg_find_fmt(NULL, "%s/foo1:%s/baz:%s", TAPI_CACHE_ROOT_INST,
                              init_inst[i], methods[k]);
            if (TE_RC_GET_ERROR(rc) != foo1_expected)
            {
                success = FALSE;
                ERROR_VERDICT("Unexpected status of instance 'foo1:%s/baz:%s': "
                              "%r", init_inst[i], methods[k], rc);
            }
        }

        for (j = 0; j < num_init_subinst; j++)
        {
            for (k = 0; k < num_methods; k++)
            {
                rc = cfg_find_fmt(NULL, "%s/foo:%s/bar:%s/baz:%s",
                                  TAPI_CACHE_ROOT_INST,
                                  init_inst[i], init_subinst[j], methods[k]);
                if (TE_RC_GET_ERROR(rc) != foo_bar_expected)
                {
                    success = FALSE;
                    ERROR_VERDICT("Unexpected status of instance "
                                  "'%s/foo:%s/bar:%s/baz:%s': %r",
                                 init_inst[i], init_subinst[j], methods[k], rc);
                }
            }
        }
    }

    return success;
}


int
main(int argc, char **argv)
{
    char **init_inst;
    size_t num_init_inst;
    char **init_subinst;
    size_t num_init_subinst;
    char **methods;
    size_t num_methods;
    char **area_regs;
    size_t num_area_regs;
    char **expected_act;
    size_t num_expected_act;
    const char  *area_act;
    size_t   i, j;
    unsigned expected_error;
    te_bool  test_ok = TRUE;

    TEST_START;
    TEST_GET_STRING_LIST_PARAM(init_inst, num_init_inst);
    TEST_GET_STRING_LIST_PARAM(init_subinst, num_init_subinst);
    TEST_GET_STRING_LIST_PARAM(methods, num_methods);
    TEST_GET_STRING_LIST_PARAM(area_regs, num_area_regs);
    TEST_GET_STRING_LIST_PARAM(expected_act, num_expected_act);
    TEST_GET_STRING_PARAM(area_act);
    TEST_GET_ACT_ERROR_TYPE(expected_error);

    TEST_STEP("Create root instances");
    init_test((const char **)init_inst, num_init_inst,
              (const char **)init_subinst, num_init_subinst);

    TEST_STEP("Register all supported methods on area");
    for (i = 0; i < TE_ARRAY_LEN(cbs); i++)
    {
        for (j = 0; j < num_area_regs; j++)
        {
            if (strcmp(area_regs[j], "nil") == 0)
                continue;
            RING("Register method '%s' on area '%s'",
                 cbs[i].method, area_regs[j]);
            CHECK_RC(tapi_cache_register(cbs[i].method, area_regs[j],
                                         cbs[i].cb_func));
        }
    }

    TEST_STEP("Actualize an area");
    for (i = 0; i < num_methods; i++)
    {
        if (strcmp(area_act, "nil") == 0)
            area_act = TAPI_CACHE_ALL;
        RING("Actualize area '%s' with method '%s'", area_act, methods[i]);
        rc = tapi_cache_actualize(methods[i], NULL, "%s", area_act);
        if (TE_RC_GET_ERROR(rc) != expected_error)
        {
            ERROR_VERDICT("Unexpected actualization error: method '%s'",
                          methods[i]);
            test_ok = FALSE;
        }
        if (TE_RC_GET_ERROR(rc) != TE_ENOENT &&
            TE_RC_GET_ERROR(rc) != TE_ECHILD)
        {
            CHECK_RC(rc);
        }
    }

    TEST_STEP("Check the actualization");
    if (!test_act((const char **)init_inst, num_init_inst,
                  (const char **)init_subinst, num_init_subinst,
                  (const char **)methods, num_methods,
                  (const char **)expected_act, num_expected_act))
    {
        TEST_FAIL("Indirect actualization works improperly");
    }

    if (!test_ok)
        TEST_FAIL("Unexpected actualization status");

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    TEST_END;
}
