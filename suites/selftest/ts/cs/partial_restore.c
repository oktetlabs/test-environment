/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test Environment
 *
 * Check partial backup restoring in Configurator
 */

/** @page partial_restore TA's subtree configurator
 *
 * @objective Check configurator TA's subtree restoring.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "cs/partial_restore"

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

/**
 * Check current and expected values.
 * If @p current_val != @p expected_val then @p TEST_FAIL.
 *
 * @param[in] current_val   Current @p ta's @p oid value.
 * @param[in] expected_val  Expected @p ta's @p oid value.
 * @param[in] ta            Test Agent name.
 * @param[in] oid           String OID.
 */
#define CHECK_VALUE(current_val, expected_val, ta, oid) \
    do {                                                    \
        uint32_t cur_val = (current_val);                   \
        uint32_t exp_val = (expected_val);                  \
                                                            \
        if (cur_val != exp_val)                             \
        {                                                   \
            TEST_FAIL("Value of '/agent:%s%s:' is '%u', "   \
                      "but should be '%u'",                 \
                      (ta), (oid), cur_val, exp_val);       \
        }                                                   \
    } while(0)

/**
 * Save @p old_value of @p ta's @p oid_name, generate and set @p new_value and
 * read to @p cur_value current @p ta's @p oid_name value.
 *
 * @param[in]  ta           Test Agent name.
 * @param[in]  oid_name     String OID
 * @param[out] old_value    Old value to save.
 * @param[out] new_value    New value to set (@p old_value + @c 1).
 * @param[out] cur_value    Save the current value after the change.
 */
static void
cfg_change_value(const char *ta, const char *oid_name,
                 uint32_t *old_value, uint32_t *new_value, uint32_t *cur_value)
{
    CHECK_RC(cfg_get_uint32(old_value, "/agent:%s%s:", ta, oid_name));

    *new_value = *(old_value) + 1;
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT32, *new_value),
                                  "/agent:%s%s:", ta, oid_name));

    CHECK_RC(cfg_get_uint32(cur_value, "/agent:%s%s:", ta, oid_name));

    if (*new_value != *cur_value)
    {
        TEST_VERDICT("Incorrect '/agent:%s%s:' value. "
                     "It should be '%u', but it's '%u'",
                     ta, oid_name, *new_value, *cur_value);
    }

    RING("Old value of '/agent:%s%s:' is '%u'", ta, oid_name, *old_value);
    RING("New value of '/agent:%s%s:' is '%u'", ta, oid_name, *new_value);
    RING("Cur value of '/agent:%s%s:' is '%u'", ta, oid_name, *cur_value);
}

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;
    rcf_rpc_server *pco_tst = NULL;

    char *backup = NULL;

    const char *oid_name = NULL;

    uint32_t iut_old_value = -1;
    uint32_t iut_new_value = -1;
    uint32_t iut_cur_value = -1;

    uint32_t tst_old_value = -1;
    uint32_t tst_new_value = -1;
    uint32_t tst_cur_value = -1;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_tst);

    TEST_GET_STRING_PARAM(oid_name);

    TEST_STEP("Create and verify configuration backup");

    CHECK_RC(cfg_create_backup(&backup));
    CHECK_RC(cfg_verify_backup(backup));

    TEST_STEP("Change existing configuration");

    TEST_SUBSTEP("Set to OID '/agent:%s%s:' new value", pco_iut->ta, oid_name);
    cfg_change_value(pco_iut->ta, oid_name,
                     &iut_old_value, &iut_new_value, &iut_cur_value);

    TEST_SUBSTEP("Set to OID '/agent:%s%s:' new value", pco_tst->ta, oid_name);
    cfg_change_value(pco_tst->ta, oid_name,
                     &tst_old_value, &tst_new_value, &tst_cur_value);

    TEST_STEP("Restore configuration subtree");

    TEST_SUBSTEP("Restore IUT configuration subtree");
    CHECK_RC(cfg_restore_backup_ta(pco_iut->ta, backup));

    TEST_STEP("Check configuration subtree after restoring");

    TEST_SUBSTEP("Get current OID '/agent:%s%s:' value", pco_iut->ta, oid_name);
    CHECK_RC(cfg_get_uint32(&iut_cur_value,
                            "/agent:%s%s:", pco_iut->ta, oid_name));

    TEST_SUBSTEP("Get current OID '/agent:%s%s:' value", pco_tst->ta, oid_name);
    CHECK_RC(cfg_get_uint32(&tst_cur_value,
                            "/agent:%s%s:", pco_tst->ta, oid_name));

    TEST_SUBSTEP("Check that configuration subtree was successfully restored");
    CHECK_VALUE(iut_cur_value, iut_old_value, pco_iut->ta, oid_name);
    CHECK_VALUE(tst_cur_value, tst_new_value, pco_tst->ta, oid_name);

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(cfg_restore_backup(backup));
    CLEANUP_CHECK_RC(cfg_release_backup(&backup));
    free(backup);

    TEST_END;
}
