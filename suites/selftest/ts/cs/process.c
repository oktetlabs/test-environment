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
    char           *ps_name = "testps";

    TEST_START;

    TEST_STEP("Add a process");
    CHECK_RC(tapi_cfg_ps_add(ta, ps_name, "yes", FALSE));

    CHECK_RC(rc = cfg_synchronize_fmt(TRUE, "/agent:%s/process:%s",
                                      ta, ps_name));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/process:%s",
                                 ta, ps_name));

    TEST_STEP("Start the process");
    CHECK_RC(tapi_cfg_ps_start(ta, ps_name));

    TEST_STEP("Delete the process");
    CHECK_RC(tapi_cfg_ps_del(ta, ps_name));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
