/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Test Environment
 *
 * Check DCBX support in Configurator
 */

/** @page dcbx Check DCBX configuration
 *
 * @objective Check that DCBX configuration can be retrieved,
 *            updated with a single commit, and restored.
 *
 * The test sets host-managed IEEE mode, or host-managed CEE mode if IEEE
 * is already set, so that the mode always changes.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "cs/dcbx"

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

#include "tapi_cfg_dcbx.h"
#include "tapi_test.h"
#include "tapi_env.h"

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;
    const struct if_nameindex *iut_if = NULL;
    tapi_dcb_dcbx_state origin_dcbx;
    tapi_dcb_dcbx_state current_dcbx;
    tapi_dcb_dcbx_state target_dcbx;
    bool restore_origin_dcbx = false;
    bool present;

    TEST_START;

    TEST_PARAM_DOC(env, "IUT host and its interface");
    TEST_GET_PCO(pco_iut);
    TEST_GET_IF(iut_if);

    TEST_STEP("Check that the interface provides DCBX");

    CHECK_RC(tapi_cfg_dcbx_is_present(pco_iut->ta, iut_if->if_name,
                                      &present));
    if (!present)
        TEST_SKIP("DCBX is not supported by the interface");

    TEST_STEP("Retrieve the current DCBX configuration");

    rc = tapi_cfg_dcbx_get(pco_iut->ta, iut_if->if_name, &current_dcbx);
    if (rc != 0)
        TEST_VERDICT("Failed to receive DCBX configuration: %r", rc);

    origin_dcbx = current_dcbx;

    TEST_STEP("Change DCBX configuration");

    target_dcbx = TAPI_DCB_DCBX_HOST | TAPI_DCB_DCBX_VER_IEEE;
    if (target_dcbx == origin_dcbx)
        target_dcbx = TAPI_DCB_DCBX_HOST | TAPI_DCB_DCBX_VER_CEE;

    restore_origin_dcbx = true;

    rc = tapi_cfg_dcbx_set(pco_iut->ta, iut_if->if_name, target_dcbx);
    if (rc != 0)
        TEST_VERDICT("Failed to set DCBX configuration: %r", rc);

    TEST_STEP("Verify DCBX configuration");

    rc = tapi_cfg_dcbx_get(pco_iut->ta, iut_if->if_name, &current_dcbx);
    if (rc != 0)
        TEST_VERDICT("Failed to receive DCBX configuration: %r", rc);

    if (current_dcbx != target_dcbx)
        TEST_VERDICT("Unexpected DCBX mode state");

    TEST_STEP("Restore original DCBX configuration");

    restore_origin_dcbx = false;

    rc = tapi_cfg_dcbx_set(pco_iut->ta, iut_if->if_name, origin_dcbx);
    if (rc != 0)
        TEST_VERDICT("Failed to restore DCBX configuration: %r", rc);

    rc = tapi_cfg_dcbx_get(pco_iut->ta, iut_if->if_name, &current_dcbx);
    if (rc != 0)
        TEST_VERDICT("Failed to receive DCBX configuration: %r", rc);

    if (current_dcbx != origin_dcbx)
        TEST_VERDICT("DCBX configuration is not restored");

    TEST_SUCCESS;

cleanup:
    if (restore_origin_dcbx)
        CLEANUP_CHECK_RC(tapi_cfg_dcbx_set(pco_iut->ta, iut_if->if_name,
                                           origin_dcbx));

    TEST_END;
}
