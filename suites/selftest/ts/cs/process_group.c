/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Check that stopping a process takes its process group down
 *
 * Copyright (C) 2026 OKTET Labs Ltd. All rights reserved.
 */

/** @page process_group Stop a process together with its children
 *
 * @objective Check that stopping an /agent/process entry kills what the
 *            process forked, and that stop_group turns that off.
 *
 * A program run through a wrapper script is the usual shape: the
 * agent knows the wrapper's pid, the wrapper forks the real program
 * and waits for it. Killing the wrapper alone leaves the program
 * running, which is what the next test, or the next run, then
 * stumbles over.
 *
 * @par Scenario:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "process_group"

#include "te_config.h"

#include "tapi_test.h"
#include "conf_api.h"
#include "te_sleep.h"
#include "tapi_cfg_process.h"

/** The command line of the child the shell forks, as pgrep sees it */
#define CHILD_CMD       "sleep 65432"

/** Time the shell is given to fork its child */
#define START_TIMEOUT_MS    2000

/** Time pgrep/pkill are given to finish */
#define CHECK_TIMEOUT_MS    5000

/**
 * Run a one-shot tool as a process entry and return its exit status.
 */
static void
run_tool(const char *ta, const char *ps_name, const char *exe,
         const char *arg1, const char *arg2, int *exit_value)
{
    tapi_cfg_ps_exit_status_t exit_status;

    CHECK_RC(tapi_cfg_ps_add(ta, ps_name, exe, false));
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps_name, 1, arg1));
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps_name, 2, arg2));
    CHECK_RC(tapi_cfg_ps_start(ta, ps_name));
    CHECK_RC(tapi_cfg_ps_wait(ta, ps_name, CHECK_TIMEOUT_MS, &exit_status));
    CHECK_RC(tapi_cfg_ps_del(ta, ps_name));

    if (exit_status.type != TAPI_CFG_PS_EXIT_STATUS_EXITED)
        TEST_FAIL("%s did not exit normally", exe);

    *exit_value = exit_status.value;
}

/**
 * Whether a child with the known command line is still running.
 *
 * pgrep exits with 0 when it matched something and 1 when it did not.
 */
static bool
child_is_running(const char *ta, const char *ps_name)
{
    int value;

    run_tool(ta, ps_name, "pgrep", "-fx", CHILD_CMD, &value);
    if (value != 0 && value != 1)
        TEST_FAIL("pgrep exited with unexpected status %d", value);

    return value == 0;
}

static void
start_and_stop(const char *ta, const char *ps_name)
{
    te_errno rc;

    CHECK_RC(tapi_cfg_ps_start(ta, ps_name));

    rc = tapi_cfg_ps_wait(ta, ps_name, START_TIMEOUT_MS, NULL);
    if (TE_RC_GET_ERROR(rc) != TE_EINPROGRESS)
        TEST_FAIL("The shell is not running: %r", rc);

    CHECK_RC(tapi_cfg_ps_stop(ta, ps_name));

    /* Let a child that was signalled finish exiting before looking */
    te_motivated_msleep(200, "Let the signalled child exit");
}

int
main(int argc, char *argv[])
{
    const char *ta = "Agt_A";
    const char *ps_name = "pgroup_shell";
    bool stop_group;
    int value;

    TEST_START;

    TEST_STEP("Add a shell process that forks a child and waits for it");
    CHECK_RC(tapi_cfg_ps_add(ta, ps_name, "sh", false));
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps_name, 1, "-c"));
    /* Two commands, so the shell forks the first rather than exec it */
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ps_name, 2,
                                 CHILD_CMD "; " CHILD_CMD));

    TEST_STEP("Check that the process group is stopped by default");
    CHECK_RC(tapi_cfg_ps_get_stop_group(ta, ps_name, &stop_group));
    if (!stop_group)
        TEST_FAIL("stop_group is expected to be on by default");

    TEST_STEP("Start and stop the shell: the child must be gone too");
    start_and_stop(ta, ps_name);
    if (child_is_running(ta, "pgroup_check1"))
        TEST_VERDICT("The child survived a stop with stop_group on");

    TEST_STEP("Turn stop_group off, start and stop the shell again: "
              "the child must survive");
    CHECK_RC(tapi_cfg_ps_set_stop_group(ta, ps_name, false));
    CHECK_RC(tapi_cfg_ps_get_stop_group(ta, ps_name, &stop_group));
    if (stop_group)
        TEST_FAIL("stop_group did not read back as off");

    start_and_stop(ta, ps_name);
    if (!child_is_running(ta, "pgroup_check2"))
        TEST_VERDICT("The child did not survive a stop with stop_group off");

    TEST_STEP("Kill the surviving child");
    run_tool(ta, "pgroup_pkill", "pkill", "-fx", CHILD_CMD, &value);
    if (value != 0)
        TEST_FAIL("pkill did not find the child it was supposed to kill");

    TEST_STEP("Delete the shell process with its subtree");
    /*
     * With children, the way a generic cleanup removes an entry it
     * knows nothing about: the read_write leaves below it cannot be
     * deleted on their own and must be skipped, not reported.
     */
    CHECK_RC(cfg_del_instance_fmt(true, "/agent:%s/process:%s",
                                  ta, ps_name));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
