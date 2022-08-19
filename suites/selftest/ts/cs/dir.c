/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Access TA work directories through configurator
 *
 * Access TA work directories through the Cconfigurator
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page cs-dir Access TA work directories through configurator
 *
 * @objective Check that TA reports its directories sanely
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "cs/dir"


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
#include "te_str.h"
#include "tapi_cfg_base.h"
#include "tapi_test.h"
#include "tapi_env.h"

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    char *agent_dir = NULL;
    char *tmp_dir = NULL;
    char *kmod_dir = NULL;
    char *lib_dir = NULL;

    TEST_START;

    TEST_GET_PCO(pco_iut);

#define GET_DIR(_label, _var, _kind) \
    do {                                                                \
        (_var) = tapi_cfg_base_get_ta_dir(pco_iut->ta, (_kind));        \
        if (te_str_is_null_or_empty(_var))                              \
            TEST_VERDICT("Cannot get the " _label " directory");        \
        RING("The " _label " directory on %s is %s",                    \
             pco_iut->ta, _var);                                        \
    } while (0)

    TEST_STEP("Get the agent directory");
    GET_DIR("agent", agent_dir, TAPI_CFG_BASE_TA_DIR_AGENT);

    TEST_STEP("Get the temporary directory");
    GET_DIR("temporary", tmp_dir, TAPI_CFG_BASE_TA_DIR_TMP);

    TEST_STEP("Get the kernel modules directory");
    GET_DIR("kernel module", kmod_dir, TAPI_CFG_BASE_TA_DIR_KMOD);

    TEST_STEP("Get the library directory");
    GET_DIR("library", lib_dir, TAPI_CFG_BASE_TA_DIR_BIN);

    TEST_SUCCESS;

cleanup:
    free(agent_dir);
    free(tmp_dir);
    free(kmod_dir);
    free(lib_dir);

    TEST_END;
}
