/** @file
 * @brief Test Environment
 *
 * Check support for killing CS controlled processes and waiting for them
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 */

/** @page process-ping Run ping process, kill it and wait for it
 *
 * @objective Check support for killing CS controlled processes and waiting for
 *            them
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "process_ping"

#include "te_config.h"

#include "tapi_test.h"
#include "tapi_cfg_process.h"

int
main(int argc, char *argv[])
{
    te_errno res;

    const char *ta = "Agt_A";
    char *ps_name = "ping_ps";

    int timeout_s = 3;
    tapi_cfg_ps_exit_status_t exit_status;

    TEST_START;

    TEST_STEP("Add the process");
    CHECK_RC(tapi_cfg_ps_add(ta, ps_name, "ping", FALSE));

    TEST_STEP("Add options");
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps_name, 1, "localhost"));

    TEST_STEP("Start the ping process");
    CHECK_RC(tapi_cfg_ps_start(ta, ps_name));

    TEST_STEP("Wait for the process to run for %d seconds", timeout_s);
    res = tapi_cfg_ps_wait(ta, ps_name, TE_SEC2MS(timeout_s), NULL);
    if (TE_RC_GET_ERROR(res) != TE_EINPROGRESS)
        TEST_FAIL("Ping is not running");

    TEST_STEP("Send SIGTERM to the process");
    CHECK_RC(tapi_cfg_ps_kill(ta, ps_name, SIGTERM));

    TEST_STEP("Check exit status of the process");
    CHECK_RC(tapi_cfg_ps_wait(ta, ps_name, 0, &exit_status));
    if (exit_status.type != TAPI_CFG_PS_EXIT_STATUS_SIGNALED)
    {
        TEST_FAIL("Exit status of the process does not represent that it was "
                  "killed by signal");
    }
    else if (exit_status.value != SIGTERM)
    {
        TEST_FAIL("Exit status value is supposed to be equal to %d (SIGTERM "
                  "signal number), but the real value is %d",
                  SIGTERM, exit_status.value);
    }
    else
    {
        RING("The process was signaled via SIGTERM as expected");
    }

    TEST_STEP("Add an option to the process to send only one packet");
    CHECK_RC(tapi_cfg_ps_add_opt(ta, ps_name, "c", "1"));

    TEST_STEP("Start the process again");
    CHECK_RC(tapi_cfg_ps_start(ta, ps_name));

    TEST_STEP("Wait for the process to ensure it finishes running in %d "
              "seconds", timeout_s);
    CHECK_RC(tapi_cfg_ps_wait(ta, ps_name, TE_SEC2MS(timeout_s), NULL));

    TEST_STEP("Delete the process");
    CHECK_RC(tapi_cfg_ps_del(ta, ps_name));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
