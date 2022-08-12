/** @file
 * @brief Test Environment
 *
 * Check process autorestart support in Configurator
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 */

/** @page process-autorestart Create process with autorestart feature
 *
 * @objective Check that a process will be started with specified periodicity,
 *            check that a running process will not be harmed by autorestart
 *            subsystem
 *
 * Execute `date +%T` process with autorestart value set to 3 seconds so that
 * the process is restarted every 3 seconds and prints its output to log.
 * Then stop the process, change the autorestart value to 2 seconds and start
 * the process over.
 * The output will show the time when the process was executed, and therefore
 * it will be possible to check that the execution period is as desired.
 *
 * Execute `ping -w 5 -i 5 localhost` (ping will exit in 5 seconds,
 * interval between sending packets is also 5 seconds) process which will run
 * for 5 seconds. Set autorestart value to 1 second and stop the process after
 * 3 seconds. Logs will show that the process was started only once.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "process_autorestart"

#include "te_config.h"
#include "tapi_test.h"
#include "tapi_cfg_process.h"

int
main(int argc, char *argv[])
{
    const char *ta = "Agt_A";
    const char *date_ps = "date_ps";
    const char *ping_ps = "ping_ps";

    TEST_START;

    TEST_STEP("Create the processes with arguments and options");
    CHECK_RC(tapi_cfg_ps_add(ta, date_ps, "date", FALSE));
    CHECK_RC(tapi_cfg_ps_add_arg(ta, date_ps, 1, "+%T"));

    CHECK_RC(tapi_cfg_ps_add(ta, ping_ps, "ping", FALSE));
    CHECK_RC(tapi_cfg_ps_add_opt(ta, ping_ps, "w", "5"));
    CHECK_RC(tapi_cfg_ps_add_opt(ta, ping_ps, "i", "5"));
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ping_ps, 1, "localhost"));

    TEST_STEP("Set autorestart value for the date process to 3 seconds");
    CHECK_RC(tapi_cfg_ps_set_autorestart(ta, date_ps, 3));

    TEST_STEP("Start the date process");
    CHECK_RC(tapi_cfg_ps_start(ta, date_ps));

    VSLEEP(10, "Wait for the date process to be executed for several times by "
               "the autorestart subsystem");

    TEST_STEP("Stop the date process");
    CHECK_RC(tapi_cfg_ps_stop(ta, date_ps));

    TEST_STEP("Change autorestart value for the date process to 2 seconds");
    CHECK_RC(tapi_cfg_ps_set_autorestart(ta, date_ps, 2));

    TEST_STEP("Start the date process again");
    CHECK_RC(tapi_cfg_ps_start(ta, date_ps));

    VSLEEP(5, "Wait for the date process to be executed for several times by "
              "the autorestart subsystem");

    TEST_STEP("Delete the date process");
    CHECK_RC(tapi_cfg_ps_del(ta, date_ps));

    TEST_STEP("Set autorestart value for the ping process to 1 second");
    CHECK_RC(tapi_cfg_ps_set_autorestart(ta, ping_ps, 1));

    TEST_STEP("Start the ping process");
    CHECK_RC(tapi_cfg_ps_start(ta, ping_ps));

    VSLEEP(3, "Wait for the ping process to run for some time");

    TEST_STEP("Delete the ping process");
    CHECK_RC(tapi_cfg_ps_del(ta, ping_ps));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
