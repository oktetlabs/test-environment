/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Testing Block Device Configuration TAPI
 *
 * Testing Block Device Configuration TAPI.
 */

/** @page cs-loop Testing Loop Block Device Configuration TAPI
 *
 * @objective Check that Loop Block Device Configuration TAPI works properly.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "cs/loop"

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
#include "tapi_cfg_block.h"
#include "tapi_test.h"
#include "tapi_env.h"

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    const char *blockdev;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_STRING_PARAM(blockdev);

    TEST_STEP("Initialize loop devices");
    CHECK_RC(tapi_cfg_block_initialize_loop(pco_iut->ta));

    CHECK_RC(tapi_cfg_block_grab(pco_iut->ta, blockdev));

    CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, "/agent:%s", pco_iut->ta));

    if (!tapi_cfg_block_is_loop(pco_iut->ta, blockdev))
        TEST_VERDICT("%s is not a loop device on %s", blockdev, pco_iut->ta);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
