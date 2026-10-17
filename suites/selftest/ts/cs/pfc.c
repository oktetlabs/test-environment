/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Test Environment
 *
 * Check PFC support in Configurator
 */

/** @page pfc Check PFC configuration
 *
 * @objective Check that PFC configuration can be retrieved,
 *            updated with a single commit, and restored.
 *
 * The test disables PFC on the first priority on which it is enabled, or
 * enables it on one priority if it is disabled on all, so that the
 * configuration always changes. MACsec bypass and delay are applied as
 * read: drivers may ignore them.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "cs/pfc"

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

#include "tapi_cfg_pfc.h"
#include "tapi_test.h"
#include "tapi_env.h"

/** Priority whose PFC is enabled if it is disabled on all priorities. */
#define PFC_TEST_PRIO 3

/**
 * Check that a PFC configuration is as expected.
 *
 * @param expected  Expected PFC configuration.
 * @param current   Current PFC configuration.
 * @param what      Configuration name for verdicts.
 */
static void
pfc_check(const tapi_dcb_pfc_config *expected,
          const tapi_dcb_pfc_config *current, const char *what)
{
    if (memcmp(current->pfc_en, expected->pfc_en,
               sizeof(expected->pfc_en)) != 0)
        TEST_VERDICT("Unexpected PFC TCs state of the %s configuration",
                     what);

    if (current->mbc != expected->mbc)
        TEST_VERDICT("Unexpected MACsec Bypass state of the %s "
                     "configuration", what);

    if (current->delay != expected->delay)
        TEST_VERDICT("Unexpected PFC delay of the %s configuration", what);
}

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;
    const struct if_nameindex *iut_if = NULL;
    tapi_dcb_pfc_state origin_pfc;
    tapi_dcb_pfc_state current_pfc;
    tapi_dcb_pfc_state target_pfc;
    bool restore_origin_pfc = false;
    unsigned int prio;
    bool present;

    TEST_START;

    TEST_PARAM_DOC(env, "IUT host and its interface");
    TEST_GET_PCO(pco_iut);
    TEST_GET_IF(iut_if);

    TEST_STEP("Check that the interface provides PFC");

    CHECK_RC(tapi_cfg_pfc_is_present(pco_iut->ta, iut_if->if_name,
                                     &present));
    if (!present)
        TEST_SKIP("PFC is not supported by the interface");

    TEST_STEP("Retrieve the current PFC configuration");

    rc = tapi_cfg_pfc_get(pco_iut->ta, iut_if->if_name, &current_pfc);
    if (rc != 0)
        TEST_VERDICT("Failed to receive PFC configuration: %r", rc);

    origin_pfc = target_pfc = current_pfc;
    if (origin_pfc.pfc_cap == 0)
        TEST_SKIP("PFC reports no supported Traffic Classes");

    TEST_STEP("Change PFC configuration");

    for (prio = 0; prio < TAPI_DCB_NUM_PRIORITIES; prio++)
    {
        if (target_pfc.config.pfc_en[prio])
            break;
    }
    if (prio < TAPI_DCB_NUM_PRIORITIES)
        target_pfc.config.pfc_en[prio] = false;
    else
        target_pfc.config.pfc_en[PFC_TEST_PRIO] = true;

    restore_origin_pfc = true;

    rc = tapi_cfg_pfc_set(pco_iut->ta, iut_if->if_name, &target_pfc.config);
    if (rc != 0)
        TEST_VERDICT("Failed to set PFC configuration: %r", rc);

    TEST_STEP("Verify PFC configuration");

    rc = tapi_cfg_pfc_get(pco_iut->ta, iut_if->if_name, &current_pfc);
    if (rc != 0)
        TEST_VERDICT("Failed to receive PFC configuration: %r", rc);

    pfc_check(&target_pfc.config, &current_pfc.config, "applied");

    TEST_STEP("Restore original PFC configuration");

    restore_origin_pfc = false;

    rc = tapi_cfg_pfc_set(pco_iut->ta, iut_if->if_name, &origin_pfc.config);
    if (rc != 0)
        TEST_VERDICT("Failed to restore PFC configuration: %r", rc);

    rc = tapi_cfg_pfc_get(pco_iut->ta, iut_if->if_name, &current_pfc);
    if (rc != 0)
        TEST_VERDICT("Failed to receive PFC configuration: %r", rc);

    pfc_check(&origin_pfc.config, &current_pfc.config, "restored");

    TEST_SUCCESS;

cleanup:
    if (restore_origin_pfc)
        CLEANUP_CHECK_RC(tapi_cfg_pfc_set(pco_iut->ta, iut_if->if_name,
                                          &origin_pfc.config));

    TEST_END;
}
