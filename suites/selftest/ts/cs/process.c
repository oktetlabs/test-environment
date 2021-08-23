/** @file
 * @brief Test Environment
 *
 * Check processes support in Configurator
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 */

/** @page process Create process
 *
 * @objective Check that process may be created on TA
 *
 * @par Scenario:
 *
 * @author Dilshod Urazov <Dilshod.Urazov@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "process"

#include "te_config.h"

#include "tapi_test.h"
#include "conf_api.h"
#include "rcf_api.h"
#include "tapi_cfg_process.h"

int
main(int argc, char *argv[])
{
    const char     *ta = "Agt_A";
    char           *ps1_name = "testps1";
    char           *ps2_name = "testps2";
    te_bool         is_running;

    TEST_START;

    TEST_STEP("Add processes");
    CHECK_RC(tapi_cfg_ps_add(ta, ps1_name, "echo", FALSE));

    CHECK_RC(rc = cfg_synchronize_fmt(TRUE, "/agent:%s/process:%s",
                                      ta, ps1_name));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/process:%s",
                                 ta, ps1_name));

    CHECK_RC(tapi_cfg_ps_add(ta, ps2_name, "printenv", FALSE));

    CHECK_RC(rc = cfg_synchronize_fmt(TRUE, "/agent:%s/process:%s",
                                      ta, ps2_name));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/process:%s",
                                 ta, ps2_name));

    TEST_STEP("Add arguments for the first process");
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps1_name, 3, "TESTARG1"));
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps1_name, 1, "TESTARG2"));

    CHECK_RC(rc = cfg_synchronize_fmt(TRUE, "/agent:%s/process:%s",
                                      ta, ps1_name));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/process:%s",
                                 ta, ps1_name));

    TEST_STEP("Start the first process");
    CHECK_RC(tapi_cfg_ps_start(ta, ps1_name));

    SLEEP(1);

    TEST_STEP("Stop the first process");
    CHECK_RC(tapi_cfg_ps_stop(ta, ps1_name));

    TEST_STEP("Check that the first process is not running");
    CHECK_RC(tapi_cfg_ps_get_status(ta, ps1_name, &is_running));
    if (is_running)
        TEST_FAIL("The first process is running, but it has been stopped");
    else
        RING("The first process is expectedly not running");

    TEST_STEP("Add more arguments for the first process");
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps1_name, 2, "TESTARG3"));
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps1_name, 4, "TESTARG4"));

    TEST_STEP("Add options for the first process");
    CHECK_RC(tapi_cfg_ps_add_opt(ta, ps1_name, "s", "optval1"));
    CHECK_RC(tapi_cfg_ps_add_opt(ta, ps1_name, "o", NULL));
    CHECK_RC(tapi_cfg_ps_add_opt(ta, ps1_name, "long", "optval2"));
    CHECK_RC(tapi_cfg_ps_add_opt(ta, ps1_name, "without_val", NULL));

    CHECK_RC(rc = cfg_synchronize_fmt(TRUE, "/agent:%s/process:%s",
                                      ta, ps1_name));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/process:%s",
                                 ta, ps1_name));

    TEST_STEP("Start the first process again");
    CHECK_RC(tapi_cfg_ps_start(ta, ps1_name));

    SLEEP(1);

    TEST_STEP("Stop the first process");
    CHECK_RC(tapi_cfg_ps_stop(ta, ps1_name));

    TEST_STEP("Set long option value separator for the first process");
    CHECK_RC(tapi_cfg_ps_set_long_opt_sep(ta, ps1_name, "="));

    CHECK_RC(rc = cfg_synchronize_fmt(TRUE, "/agent:%s/process:%s",
                                      ta, ps1_name));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/process:%s",
                                 ta, ps1_name));

    TEST_STEP("Start the first process for the third time");
    CHECK_RC(tapi_cfg_ps_start(ta, ps1_name));

    SLEEP(1);

    TEST_STEP("Stop the first process");
    CHECK_RC(tapi_cfg_ps_stop(ta, ps1_name));

    TEST_STEP("Add environment variables for the second process");
    CHECK_RC(tapi_cfg_ps_add_env(ta, ps2_name, "TESTENVVAR1", "TESTENVVAL1"));
    CHECK_RC(tapi_cfg_ps_add_env(ta, ps2_name, "TESTENVVAR2", "TESTENVVAL2"));

    TEST_STEP("Add corresponding arguments for the second process");
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps2_name, 1, "TESTENVVAR1"));
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps2_name, 2, "TESTENVVAR2"));

    CHECK_RC(rc = cfg_synchronize_fmt(TRUE, "/agent:%s/process:%s",
                                      ta, ps2_name));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/process:%s",
                                 ta, ps2_name));

    TEST_STEP("Start the second process");
    CHECK_RC(tapi_cfg_ps_start(ta, ps2_name));

    SLEEP(1);

    TEST_STEP("Delete the processes");
    CHECK_RC(tapi_cfg_ps_del(ta, ps1_name));
    CHECK_RC(tapi_cfg_ps_del(ta, ps2_name));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
