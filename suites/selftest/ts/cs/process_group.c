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
 * stumbles over. The wrapper dies on the first signal at once, so the
 * stop must keep watching the group after that: a program that
 * ignores SIGTERM only goes with the SIGKILL the stop falls back to.
 *
 * @par Scenario:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "process_group"

#include "te_config.h"

#include <stdarg.h>

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
 * The argument list is terminated by @c NULL.
 */
static void
run_tool(const char *ta, const char *ps_name, const char *exe,
         int *exit_value, ...)
{
    tapi_cfg_ps_exit_status_t exit_status;
    const char *arg;
    unsigned int i;
    va_list ap;

    CHECK_RC(tapi_cfg_ps_add(ta, ps_name, exe, false));
    va_start(ap, exit_value);
    for (i = 1; (arg = va_arg(ap, const char *)) != NULL; i++)
        CHECK_RC(tapi_cfg_ps_add_arg(ta, ps_name, i, arg));
    va_end(ap);
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

    run_tool(ta, ps_name, "pgrep", &value, "-fx", CHILD_CMD, NULL);
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
    const char *ign_name = "pgroup_shell_ign";
    bool stop_group;
    int value;

    TEST_START;

    TEST_STEP("Kill a child an earlier failed run may have left behind");
    /* SIGKILL: one of the children below ignores SIGTERM */
    run_tool(ta, "pgroup_pkill0", "pkill", &value, "-9", "-fx", CHILD_CMD,
             NULL);

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

    TEST_STEP("Add a shell whose child ignores SIGTERM, start and stop "
              "it: the stop must fall back to SIGKILL for the child");
    CHECK_RC(tapi_cfg_ps_add(ta, ign_name, "sh", false));
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ign_name, 1, "-c"));
    /*
     * The outer shell dies on SIGTERM at once, like a wrapper script.
     * The inner one ignores it and so does the sleep it execs: the
     * ignored disposition is inherited.
     */
    CHECK_RC(tapi_cfg_ps_add_arg(ta, ign_name, 2,
                                 "sh -c 'trap \"\" TERM; " CHILD_CMD
                                 "'; true"));
    start_and_stop(ta, ign_name);
    if (child_is_running(ta, "pgroup_check_ign"))
        TEST_VERDICT("A child ignoring SIGTERM survived a stop "
                     "with stop_group on");
    CHECK_RC(tapi_cfg_ps_del(ta, ign_name));

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
    run_tool(ta, "pgroup_pkill", "pkill", &value, "-fx", CHILD_CMD, NULL);
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
