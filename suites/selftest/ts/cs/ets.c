/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Test Environment
 *
 * Check ETS support in Configurator
 */

/** @page ets Check ETS configuration
 *
 * @objective Check that ETS configuration can be retrieved,
 *            updated with a single commit, and restored.
 *
 * The test changes the Traffic Class bandwidths, the TSA and the
 * Priority to Traffic Class mapping, so that they always differ from
 * the original ones, and turns willing off. The other fields (receive
 * bandwidths and recommended values) are applied as read: drivers may
 * ignore them.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "cs/ets"

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

#include "tapi_cfg_ets.h"
#include "tapi_test.h"
#include "tapi_env.h"

/** First priority mapped to Traffic Class 1 by the test. */
#define ETS_TEST_TC1_PRIO 4

/** Bandwidth of each of the two Traffic Classes, percent. */
#define ETS_TEST_TC_BW_EQUAL 50

/** Bandwidth of TC 0 if the equal split is the original configuration. */
#define ETS_TEST_TC0_BW_ALT 60

/** Bandwidth of TC 1 if the equal split is the original configuration. */
#define ETS_TEST_TC1_BW_ALT 40

/** Bandwidth of the only Traffic Class, percent. */
#define ETS_TEST_TC_BW_ALL 100

/**
 * Make an ETS configuration which differs from the original one.
 *
 * With at least two Traffic Classes, priorities 0-3 go to TC 0 and 4-7 to
 * TC 1, both scheduled by ETS with 50% each (60% and 40% if that is the
 * original configuration). With one Traffic Class, all priorities go to
 * TC 0, and its TSA switches between ETS (100%) and strict priority.
 * Other Traffic Classes are strict priority with no bandwidth.
 *
 * @param origin    Original ETS state.
 * @param target    Where to store the new configuration.
 */
static void
ets_target_make(const tapi_dcb_ets_state *origin, tapi_dcb_ets_config *target)
{
    unsigned int i;

    *target = origin->config;

    memset(target->tc_bw, 0, sizeof(target->tc_bw));
    memset(target->tc_tsa, TAPI_DCB_TSA_STRICT, sizeof(target->tc_tsa));
    memset(target->prio_tc, 0, sizeof(target->prio_tc));

    if (origin->ets_cap >= 2)
    {
        for (i = ETS_TEST_TC1_PRIO; i < TAPI_DCB_NUM_PRIORITIES; i++)
            target->prio_tc[i] = 1;

        target->tc_tsa[0] = TAPI_DCB_TSA_ETS;
        target->tc_tsa[1] = TAPI_DCB_TSA_ETS;
        target->tc_bw[0] = ETS_TEST_TC_BW_EQUAL;
        target->tc_bw[1] = ETS_TEST_TC_BW_EQUAL;

        if (tapi_dcb_ets_config_equal(target, &origin->config))
        {
            target->tc_bw[0] = ETS_TEST_TC0_BW_ALT;
            target->tc_bw[1] = ETS_TEST_TC1_BW_ALT;
        }
    }
    else if (origin->config.tc_tsa[0] != TAPI_DCB_TSA_ETS)
    {
        target->tc_tsa[0] = TAPI_DCB_TSA_ETS;
        target->tc_bw[0] = ETS_TEST_TC_BW_ALL;
    }
}

/**
 * Check that the fields the test changes, and willing, are as expected.
 *
 * @param expected  Expected ETS configuration.
 * @param current   Current ETS configuration.
 * @param what      Configuration name for verdicts.
 */
static void
ets_check(const tapi_dcb_ets_config *expected,
          const tapi_dcb_ets_config *current, const char *what)
{
    if (current->willing != expected->willing)
        TEST_VERDICT("Unexpected willing state of the %s configuration",
                     what);

    if (memcmp(current->tc_bw, expected->tc_bw,
               sizeof(expected->tc_bw)) != 0)
        TEST_VERDICT("Unexpected Traffic Class bandwidths of the %s "
                     "configuration", what);

    if (memcmp(current->tc_tsa, expected->tc_tsa,
               sizeof(expected->tc_tsa)) != 0)
        TEST_VERDICT("Unexpected Traffic Class <-> TSA mapping of the %s "
                     "configuration", what);

    if (memcmp(current->prio_tc, expected->prio_tc,
               sizeof(expected->prio_tc)) != 0)
        TEST_VERDICT("Unexpected Priority <-> Traffic Class mapping of the "
                     "%s configuration", what);
}

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;
    const struct if_nameindex *iut_if = NULL;
    tapi_dcb_ets_state origin_ets;
    tapi_dcb_ets_state current_ets;
    tapi_dcb_ets_state target_ets;
    bool restore_origin_ets = false;
    bool present;

    TEST_START;

    TEST_PARAM_DOC(env, "IUT host and its interface");
    TEST_GET_PCO(pco_iut);
    TEST_GET_IF(iut_if);

    TEST_STEP("Check that the interface provides ETS");

    CHECK_RC(tapi_cfg_ets_is_present(pco_iut->ta, iut_if->if_name,
                                     &present));
    if (!present)
        TEST_SKIP("ETS is not supported by the interface");

    TEST_STEP("Retrieve the current ETS configuration");

    rc = tapi_cfg_ets_get(pco_iut->ta, iut_if->if_name, &current_ets);
    if (rc != 0)
        TEST_VERDICT("Failed to receive ETS configuration: %r", rc);

    origin_ets = current_ets;

    target_ets = current_ets;
    if (target_ets.ets_cap == 0)
        TEST_SKIP("ETS reports no supported Traffic Classes");

    TEST_STEP("Change ETS configuration");

    ets_target_make(&origin_ets, &target_ets.config);
    target_ets.config.willing = false;

    restore_origin_ets = true;

    rc = tapi_cfg_ets_set(pco_iut->ta, iut_if->if_name, &target_ets.config);
    if (rc != 0)
        TEST_VERDICT("Failed to set ETS configuration: %r", rc);

    TEST_STEP("Verify ETS configuration");

    rc = tapi_cfg_ets_get(pco_iut->ta, iut_if->if_name, &current_ets);
    if (rc != 0)
        TEST_VERDICT("Failed to receive ETS configuration: %r", rc);

    /*
     * The capabilities are read-only, so this only checks that applying
     * the configuration did not change them.
     */
    if (current_ets.ets_cap != origin_ets.ets_cap)
        TEST_VERDICT("Unexpected number of supported Traffic Classes");

    if (current_ets.cbs != origin_ets.cbs)
        TEST_VERDICT("Unexpected Credit-Based Shaper capability");

    ets_check(&target_ets.config, &current_ets.config, "applied");

    TEST_STEP("Restore original ETS configuration");

    restore_origin_ets = false;

    rc = tapi_cfg_ets_set(pco_iut->ta, iut_if->if_name, &origin_ets.config);
    if (rc != 0)
        TEST_VERDICT("Failed to restore ETS configuration: %r", rc);

    rc = tapi_cfg_ets_get(pco_iut->ta, iut_if->if_name, &current_ets);
    if (rc != 0)
        TEST_VERDICT("Failed to receive ETS configuration: %r", rc);

    ets_check(&origin_ets.config, &current_ets.config, "restored");

    TEST_SUCCESS;

cleanup:
    if (restore_origin_ets)
        CLEANUP_CHECK_RC(tapi_cfg_ets_set(pco_iut->ta, iut_if->if_name,
                                          &origin_ets.config));

    TEST_END;
}
