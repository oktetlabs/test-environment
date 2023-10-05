/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief User management management testing
 *
 * User management testing
 *
 */

/** @page cs-user A sample of user management TAPI
 *
 * @objective Check that user management routines work correctly
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "cs/user"


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
#include "tapi_cfg.h"
#include "tapi_test.h"
#include "tapi_env.h"

int
main(int argc, char **argv)
{

    rcf_rpc_server *pco_iut = NULL;
    int uid;
    te_bool added = FALSE;
    te_errno status = 0;

    TEST_START;
    TEST_GET_INT_PARAM(uid);

    TEST_GET_PCO(pco_iut);

    CHECK_RC(tapi_cfg_add_new_user(pco_iut->ta, uid));
    status = tapi_cfg_add_new_user(pco_iut->ta, uid);
    if (TE_RC_GET_ERROR(status) != TE_EEXIST)
    {
        TEST_VERDICT("Unexpected result attempting "
                     "to add a user twice: %r", status);
    }

    CHECK_RC(tapi_cfg_add_user_if_needed(pco_iut->ta, uid, &added));
    if (added)
        TEST_VERDICT("Existing user reported as newly added");
    CHECK_RC(tapi_cfg_del_user(pco_iut->ta, uid));
    CHECK_RC(tapi_cfg_add_user_if_needed(pco_iut->ta, uid, &added));
    if (!added)
        TEST_VERDICT("Newly added user reported as existing");

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(tapi_cfg_del_user(pco_iut->ta, uid));

    TEST_END;
}
