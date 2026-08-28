/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Testing the ta_conf demo subtree
 */

/** @page cs-ta_conf_demo Demo of the ta_conf declarative tree
 *
 * @objective Check that a subtree registered via ta_conf_register()
 *            works end-to-end: every leaf type, the collection
 *            paths, instance lookup at depth, and commit grouping.
 *
 * @par Scenario:
 */

#define TE_TEST_NAME "cs/ta_conf_demo"

#ifndef TEST_START_VARS
#define TEST_START_VARS TEST_START_ENV_VARS
#endif

#ifndef TEST_START_SPECIFIC
#define TEST_START_SPECIFIC TEST_START_ENV
#endif

#ifndef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC TEST_END_ENV
#endif

#include "te_config.h"

#include "tapi_test.h"
#include "tapi_env.h"
#include "conf_api.h"

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;
    char *str = NULL;
    cfg_val_type vt;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Read the constant node");
    vt = CVT_STRING;
    CHECK_RC(cfg_get_instance_fmt(&vt, &str,
             "/agent:%s/ta_conf_demo:/ro_val:", pco_iut->ta));
    if (strcmp(str, "ro") != 0)
        TEST_VERDICT("ro_val reads back '%s'", str);
    free(str); str = NULL;

    TEST_STEP("Round-trip every typed leaf");
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(STRING, "hello world"),
             "/agent:%s/ta_conf_demo:/str_val:", pco_iut->ta));
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(INT32, -7),
             "/agent:%s/ta_conf_demo:/int_val:", pco_iut->ta));
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT32, 42),
             "/agent:%s/ta_conf_demo:/uint_val:", pco_iut->ta));
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(STRING, "1"),
             "/agent:%s/ta_conf_demo:/bool_val:", pco_iut->ta));
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(STRING, "green"),
             "/agent:%s/ta_conf_demo:/color:", pco_iut->ta));

    vt = CVT_STRING;
    CHECK_RC(cfg_get_instance_fmt(&vt, &str,
             "/agent:%s/ta_conf_demo:/str_val:", pco_iut->ta));
    if (strcmp(str, "hello world") != 0)
        TEST_VERDICT("str_val reads back '%s'", str);
    free(str); str = NULL;

    {
        int32_t int_val;

        vt = CVT_INT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &int_val,
                 "/agent:%s/ta_conf_demo:/int_val:", pco_iut->ta));
        if (int_val != -7)
            TEST_VERDICT("int_val reads back '%" PRId32 "'", int_val);
    }

    {
        uint32_t uint_val;

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &uint_val,
                 "/agent:%s/ta_conf_demo:/uint_val:", pco_iut->ta));
        if (uint_val != 42)
            TEST_VERDICT("uint_val reads back '%" PRIu32 "'", uint_val);
    }

    vt = CVT_STRING;
    CHECK_RC(cfg_get_instance_fmt(&vt, &str,
             "/agent:%s/ta_conf_demo:/bool_val:", pco_iut->ta));
    if (strcmp(str, "1") != 0)
        TEST_VERDICT("bool_val reads back '%s'", str);
    free(str); str = NULL;

    vt = CVT_STRING;
    CHECK_RC(cfg_get_instance_fmt(&vt, &str,
             "/agent:%s/ta_conf_demo:/color:", pco_iut->ta));
    if (strcmp(str, "green") != 0)
        TEST_VERDICT("color reads back '%s'", str);
    free(str); str = NULL;

    TEST_STEP("Every integer width round-trips through its own accessor");
    {
        static const char *const widths[] = { "i32_val", "u16_val",
                                              "u8_val" };
        int32_t  i32;
        uint16_t u16;
        uint8_t  u8;
        unsigned int i;

        CHECK_RC(cfg_set_instance_fmt(CFG_VAL(INT32, -2000000000),
                 "/agent:%s/ta_conf_demo:/i32_val:", pco_iut->ta));
        CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT16, 65535),
                 "/agent:%s/ta_conf_demo:/u16_val:", pco_iut->ta));
        CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT8, 255),
                 "/agent:%s/ta_conf_demo:/u8_val:", pco_iut->ta));

        /*
         * A plain set leaves the value just written in the
         * Configurator cache, and a get would be answered from there
         * without the agent's own get accessor ever running. Refresh
         * from the agent first, so what is checked below is what the
         * accessor of that width returns.
         */
        for (i = 0; i < TE_ARRAY_LEN(widths); i++)
        {
            CHECK_RC(cfg_synchronize_fmt(true,
                     "/agent:%s/ta_conf_demo:/%s:", pco_iut->ta,
                     widths[i]));
        }

        CHECK_RC(cfg_get_int32(&i32,
                 "/agent:%s/ta_conf_demo:/i32_val:", pco_iut->ta));
        if (i32 != -2000000000)
            TEST_VERDICT("i32_val reads back '%" PRId32 "'", i32);

        CHECK_RC(cfg_get_uint16(&u16,
                 "/agent:%s/ta_conf_demo:/u16_val:", pco_iut->ta));
        if (u16 != 65535)
            TEST_VERDICT("u16_val reads back '%" PRIu16 "'", u16);

        CHECK_RC(cfg_get_uint8(&u8,
                 "/agent:%s/ta_conf_demo:/u8_val:", pco_iut->ta));
        if (u8 != 255)
            TEST_VERDICT("u8_val reads back '%" PRIu8 "'", u8);
    }

    TEST_STEP("A value the node's width cannot hold is refused, not "
              "truncated: narrowing_u8/narrowing_i8 are declared one "
              "width narrower than their objects, so the value reaches "
              "the agent's codec instead of being stopped by the engine");
    {
        uint16_t u16_back;
        int16_t  i16_back;
        te_errno rc;

        CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT16, 200),
                 "/agent:%s/ta_conf_demo:/narrowing_u8:", pco_iut->ta));

        rc = cfg_set_instance_fmt(CFG_VAL(UINT16, 256),
                 "/agent:%s/ta_conf_demo:/narrowing_u8:", pco_iut->ta);
        if (TE_RC_GET_ERROR(rc) != TE_ERANGE)
        {
            TEST_VERDICT("narrowing_u8 set to 256 returned %r, "
                         "expected ERANGE", rc);
        }

        CHECK_RC(cfg_synchronize_fmt(true,
                 "/agent:%s/ta_conf_demo:/narrowing_u8:", pco_iut->ta));
        CHECK_RC(cfg_get_uint16(&u16_back,
                 "/agent:%s/ta_conf_demo:/narrowing_u8:", pco_iut->ta));
        if (u16_back != 200)
        {
            TEST_VERDICT("narrowing_u8 reads back '%" PRIu16 "' after a "
                         "refused set, expected the previous 200",
                         u16_back);
        }

        CHECK_RC(cfg_set_instance_fmt(CFG_VAL(INT16, -100),
                 "/agent:%s/ta_conf_demo:/narrowing_i8:", pco_iut->ta));

        rc = cfg_set_instance_fmt(CFG_VAL(INT16, -129),
                 "/agent:%s/ta_conf_demo:/narrowing_i8:", pco_iut->ta);
        if (TE_RC_GET_ERROR(rc) != TE_ERANGE)
        {
            TEST_VERDICT("narrowing_i8 set to -129 returned %r, "
                         "expected ERANGE", rc);
        }

        CHECK_RC(cfg_synchronize_fmt(true,
                 "/agent:%s/ta_conf_demo:/narrowing_i8:", pco_iut->ta));
        CHECK_RC(cfg_get_int16(&i16_back,
                 "/agent:%s/ta_conf_demo:/narrowing_i8:", pco_iut->ta));
        if (i16_back != -100)
        {
            TEST_VERDICT("narrowing_i8 reads back '%" PRId16 "' after a "
                         "refused set, expected the previous -100",
                         i16_back);
        }
    }

    TEST_STEP("Collection add, get, list, child lookup, del");
    CHECK_RC(cfg_add_instance_fmt(NULL, CFG_VAL(STRING, "bar"),
             "/agent:%s/ta_conf_demo:/item:foo", pco_iut->ta));

    vt = CVT_STRING;
    CHECK_RC(cfg_get_instance_fmt(&vt, &str,
             "/agent:%s/ta_conf_demo:/item:foo", pco_iut->ta));
    if (strcmp(str, "bar") != 0)
        TEST_VERDICT("item value reads back '%s'", str);
    free(str); str = NULL;

    vt = CVT_STRING;
    CHECK_RC(cfg_get_instance_fmt(&vt, &str,
             "/agent:%s/ta_conf_demo:/item:foo/depth:", pco_iut->ta));
    if (strcmp(str, "foo") != 0)
        TEST_VERDICT("depth (instance lookup) reads back '%s'", str);
    free(str); str = NULL;

    {
        unsigned int n_inst = 0;
        cfg_handle *insts = NULL;

        CHECK_RC(cfg_find_pattern_fmt(&n_inst, &insts,
                 "/agent:%s/ta_conf_demo:/item:*", pco_iut->ta));
        if (n_inst != 1)
            TEST_VERDICT("item list returned %u instances", n_inst);
        free(insts);
    }

    CHECK_RC(cfg_del_instance_fmt(false,
             "/agent:%s/ta_conf_demo:/item:foo", pco_iut->ta));

    TEST_STEP("Item value set (TA_CONF_COLL_STR_RW) replaces the "
              "added value");
    CHECK_RC(cfg_add_instance_fmt(NULL, CFG_VAL(STRING, "bar"),
             "/agent:%s/ta_conf_demo:/item:foo", pco_iut->ta));
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(STRING, "baz"),
             "/agent:%s/ta_conf_demo:/item:foo", pco_iut->ta));

    vt = CVT_STRING;
    CHECK_RC(cfg_get_instance_fmt(&vt, &str,
             "/agent:%s/ta_conf_demo:/item:foo", pco_iut->ta));
    if (strcmp(str, "baz") != 0)
        TEST_VERDICT("item value after set reads back '%s'", str);
    free(str); str = NULL;

    CHECK_RC(cfg_del_instance_fmt(false,
             "/agent:%s/ta_conf_demo:/item:foo", pco_iut->ta));

    TEST_STEP("List-only fixed instances (TA_CONF_LIST) and instance "
              "lookup at depth via a RO_UINT32 child");
    {
        unsigned int n_inst = 0;
        cfg_handle *insts = NULL;

        CHECK_RC(cfg_find_pattern_fmt(&n_inst, &insts,
                 "/agent:%s/ta_conf_demo:/fixed:*", pco_iut->ta));
        if (n_inst != 3)
            TEST_VERDICT("fixed list returned %u instances", n_inst);
        free(insts);
    }

    {
        uint32_t index_val;

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &index_val,
                 "/agent:%s/ta_conf_demo:/fixed:two/index:", pco_iut->ta));
        if (index_val != 1)
            TEST_VERDICT("fixed/two index reads back '%" PRIu32 "'",
                         index_val);
    }

    TEST_STEP("Read-only signed collection (TA_CONF_RO_COLL_INT32)");
    {
        static const struct {
            const char *name;
            int32_t     value;
        } cells[] = {
            { "neg",  -5 },
            { "zero",  0 },
            { "pos",   7 },
        };
        unsigned int i;

        for (i = 0; i < TE_ARRAY_LEN(cells); i++)
        {
            int32_t cell_val;

            vt = CVT_INT32;
            CHECK_RC(cfg_get_instance_fmt(&vt, &cell_val,
                     "/agent:%s/ta_conf_demo:/cell:%s", pco_iut->ta,
                     cells[i].name));
            if (cell_val != cells[i].value)
            {
                TEST_VERDICT("cell:%s reads back '%" PRId32 "'",
                             cells[i].name, cell_val);
            }
        }
    }

    TEST_STEP("Read-write fixed collection with no add/del "
              "(TA_CONF_RW_COLL_UINT32)");
    {
        unsigned int n_inst = 0;
        cfg_handle *insts = NULL;

        CHECK_RC(cfg_find_pattern_fmt(&n_inst, &insts,
                 "/agent:%s/ta_conf_demo:/dial:*", pco_iut->ta));
        if (n_inst != 2)
            TEST_VERDICT("dial list returned %u instances", n_inst);
        free(insts);
    }

    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT32, 11),
             "/agent:%s/ta_conf_demo:/dial:x", pco_iut->ta));
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT32, 22),
             "/agent:%s/ta_conf_demo:/dial:y", pco_iut->ta));

    {
        uint32_t x_val, y_val;

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &x_val,
                 "/agent:%s/ta_conf_demo:/dial:x", pco_iut->ta));
        if (x_val != 11)
            TEST_VERDICT("dial:x reads back '%" PRIu32 "'", x_val);

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &y_val,
                 "/agent:%s/ta_conf_demo:/dial:y", pco_iut->ta));
        if (y_val != 22)
            TEST_VERDICT("dial:y reads back '%" PRIu32 "'", y_val);
    }

    TEST_STEP("Staged-commit collection (TA_CONF_COLL_STR_RW_COMMIT): "
              "set() stages, commit() applies and bumps the counter");
    {
        uint32_t commits_before, commits_after;

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &commits_before,
                 "/agent:%s/ta_conf_demo:/citem_commits:", pco_iut->ta));

        CHECK_RC(cfg_add_instance_fmt(NULL, CFG_VAL(STRING, "v0"),
                 "/agent:%s/ta_conf_demo:/citem:c1", pco_iut->ta));

        vt = CVT_STRING;
        CHECK_RC(cfg_get_instance_fmt(&vt, &str,
                 "/agent:%s/ta_conf_demo:/citem:c1", pco_iut->ta));
        if (strcmp(str, "v0") != 0)
            TEST_VERDICT("citem:c1 after add reads back '%s'", str);
        free(str); str = NULL;

        CHECK_RC(cfg_set_instance_local_fmt(CFG_VAL(STRING, "v1"),
                 "/agent:%s/ta_conf_demo:/citem:c1", pco_iut->ta));
        CHECK_RC(cfg_commit_fmt("/agent:%s/ta_conf_demo:/citem:c1",
                                pco_iut->ta));
        CHECK_RC(cfg_synchronize_fmt(true,
                 "/agent:%s/ta_conf_demo:/citem:c1", pco_iut->ta));

        vt = CVT_STRING;
        CHECK_RC(cfg_get_instance_fmt(&vt, &str,
                 "/agent:%s/ta_conf_demo:/citem:c1", pco_iut->ta));
        if (strcmp(str, "v1") != 0)
            TEST_VERDICT("citem:c1 after commit reads back '%s'", str);
        free(str); str = NULL;

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &commits_after,
                 "/agent:%s/ta_conf_demo:/citem_commits:", pco_iut->ta));
        if (commits_after != commits_before + 1)
        {
            TEST_VERDICT("citem_commits went from %" PRIu32
                         " to %" PRIu32, commits_before, commits_after);
        }

        CHECK_RC(cfg_del_instance_fmt(false,
                 "/agent:%s/ta_conf_demo:/citem:c1", pco_iut->ta));
    }

    TEST_STEP("Generic unvalued collection (TA_CONF_NODE(), rx_rules "
              "shape): add/list/del plus a commit counter");
    {
        uint32_t commits_before, commits_after;
        unsigned int n_inst = 0;
        cfg_handle *insts = NULL;

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &commits_before,
                 "/agent:%s/ta_conf_demo:/batch_commits:", pco_iut->ta));

        /*
         * A plain (non-local) add would reach the agent as a lone
         * RPC and auto-commit right there (same rule as every other
         * commit-bearing node, see conf_key.c's key_add/key_commit
         * comment): the counter would already move before the
         * explicit commit below runs. Stage the add locally instead,
         * so the only RPC the agent sees is the one this commit
         * triggers, and the delta below is entirely attributable to
         * it.
         */
        CHECK_RC(cfg_add_instance_local_fmt(NULL, CVT_NONE, NULL,
                 "/agent:%s/ta_conf_demo:/batch:b1", pco_iut->ta));
        CHECK_RC(cfg_commit_fmt("/agent:%s/ta_conf_demo:/batch:b1",
                                pco_iut->ta));

        CHECK_RC(cfg_find_pattern_fmt(&n_inst, &insts,
                 "/agent:%s/ta_conf_demo:/batch:*", pco_iut->ta));
        if (n_inst != 1)
            TEST_VERDICT("batch list returned %u instances", n_inst);
        free(insts);

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &commits_after,
                 "/agent:%s/ta_conf_demo:/batch_commits:", pco_iut->ta));
        if (commits_after != commits_before + 1)
        {
            TEST_VERDICT("batch_commits went from %" PRIu32
                         " to %" PRIu32, commits_before, commits_after);
        }

        CHECK_RC(cfg_del_instance_fmt(false,
                 "/agent:%s/ta_conf_demo:/batch:b1", pco_iut->ta));

        CHECK_RC(cfg_find_pattern_fmt(&n_inst, &insts,
                 "/agent:%s/ta_conf_demo:/batch:*", pco_iut->ta));
        if (n_inst != 0)
            TEST_VERDICT("batch list after del returned %u instances",
                         n_inst);
        free(insts);
    }

    TEST_STEP("Commit grouping applies staged values once");
    CHECK_RC(cfg_set_instance_local_fmt(CFG_VAL(UINT32, 5),
             "/agent:%s/ta_conf_demo:/grp:/a:", pco_iut->ta));
    CHECK_RC(cfg_set_instance_local_fmt(CFG_VAL(UINT32, 6),
             "/agent:%s/ta_conf_demo:/grp:/b:", pco_iut->ta));
    CHECK_RC(cfg_commit_fmt("/agent:%s/ta_conf_demo:/grp:",
                            pco_iut->ta));
    CHECK_RC(cfg_synchronize_fmt(true, "/agent:%s/ta_conf_demo:/grp:",
                                 pco_iut->ta));

    {
        uint32_t a_val, b_val, commits_val;

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &a_val,
                 "/agent:%s/ta_conf_demo:/grp:/a:", pco_iut->ta));
        if (a_val != 5)
            TEST_VERDICT("grp/a reads back '%" PRIu32 "'", a_val);

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &b_val,
                 "/agent:%s/ta_conf_demo:/grp:/b:", pco_iut->ta));
        if (b_val != 6)
            TEST_VERDICT("grp/b reads back '%" PRIu32 "'", b_val);

        vt = CVT_UINT32;
        CHECK_RC(cfg_get_instance_fmt(&vt, &commits_val,
                 "/agent:%s/ta_conf_demo:/grp:/commits:", pco_iut->ta));
        if (commits_val != 1)
            TEST_VERDICT("grp/commits reads back '%" PRIu32 "'",
                         commits_val);
    }

    TEST_SUCCESS;

cleanup:
    free(str);
    TEST_END;
}
