/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Code dealing with running command monitors
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "Tester Command Monitors"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#include <sys/wait.h>

#include "te_defs.h"
#include "te_queue.h"
#include "te_errno.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_cfg_cmd_monitor.h"
#include "tester_cmd_monitor.h"
#include "tester_defs.h"

int tester_monitor_id = -1;

/* See description in tester_cmd_monitor.h */
const char *
cmd_monitor_type2str(cmd_monitor_type type)
{
    switch (type)
    {
        case TESTER_CMD_MONITOR_NONE:
            return "Dummy";
        case TESTER_CMD_MONITOR_TA:
            return "TA";
        case TESTER_CMD_MONITOR_TEST:
            return "TEST";
    }

    return "(UNKNOWN)";
}

/* See description in tester_cmd_monitor.h */
void
free_cmd_monitor(cmd_monitor_descr *monitor)
{
    free(monitor->command);
    free(monitor->ta);
    free(monitor);
}

/* See description in tester_cmd_monitor.h */
void
free_cmd_monitors(cmd_monitor_descrs *monitors)
{
    cmd_monitor_descr  *monitor;

    while ((monitor = TAILQ_FIRST(monitors)) != NULL)
    {
        TAILQ_REMOVE(monitors, monitor, links);
        free_cmd_monitor(monitor);
    }
}

/**
 * Start command monitor as generic test.
 *
 * @param monitor   Monitor description structure pointer.
 *
 * @return Status code.
 */
static te_errno
test_cmd_monitor_begin(cmd_monitor_descr *monitor)
{
#define TEST_MAX_PARAMS 16
    te_string params[TEST_MAX_PARAMS];
    char     *argv[TEST_MAX_PARAMS];
    int       next_id = 0;

    int     i;
    int     ret;
    pid_t   pid;
    test_id exec_id = TE_LOG_ID_UNDEFINED;
    int     rand_seed = rand();

    ENTRY("name=%s cmd=%s", monitor->name, monitor->command);

    for (i = 0; i < TE_ARRAY_LEN(params); i++)
        params[i] = (te_string)TE_STRING_INIT;

    te_string_append(&params[next_id++], "%s", monitor->command);
    te_string_append(&params[next_id++], "te_test_id=%u", exec_id);
    te_string_append(&params[next_id++], "te_test_name=%s", monitor->name);
    te_string_append(&params[next_id++], "te_rand_seed=%d", rand_seed);

    for (i = 0; i < next_id; i++)
        argv[i] = params[i].ptr;
    argv[i] = NULL;

    VERB("ID=%d execvp(%s, ...)", exec_id, monitor->command);
    pid = fork();

    if (pid == 0)
    {
        ret = execvp(monitor->command, argv);
        if (ret < 0)
        {
            ERROR("ID=%d execvp(%s, ...) failed: %s", exec_id,
                  monitor->command, strerror(errno));
            exit(1);
        }
        /* unreachable */
    }

    for (i = 0; i < next_id; i++)
        te_string_free(&params[i]);

    if (pid < 0)
        return TE_OS_RC(TE_TESTER, errno);

    monitor->test_pid = pid;

    EXIT("pid:%d", pid);
    return 0;
}

/**
 * Stop test based command monitor.
 *
 * @param monitor   Monitor description structure pointer.
 *
 * @return Status code.
 */
static te_errno
test_cmd_monitor_end(cmd_monitor_descr *monitor)
{
    int   rc;
    pid_t pid;

    ENTRY("name=%s cmd=%s pid:%d", monitor->name, monitor->command,
          monitor->test_pid);

    if (waitpid(monitor->test_pid, NULL, WNOHANG) == 0)
    {
        rc = kill(monitor->test_pid, SIGUSR1);
        if (rc < 0)
        {
            ERROR("Failed to kill test command monitor (%s)",
                  monitor->command);
            return TE_OS_RC(TE_TESTER, errno);
        }
    }

    pid = waitpid(monitor->test_pid, &rc, 0);
    if (pid < 0)
    {
        ERROR("Failed to wait test command monitor (%s) to end",
              monitor->command);
        return TE_OS_RC(TE_TESTER, errno);
    }

    if (rc < 0)
        return TE_OS_RC(TE_TESTER, rc);

    EXIT();
    return 0;
}

/* See description in tester_cmd_monitor.h */
int
start_cmd_monitors(cmd_monitor_descrs *monitors)
{
    cmd_monitor_descr *monitor;
    int                rc = 0;

    TAILQ_FOREACH(monitor, monitors, links)
    {
        te_errno status;

        switch (monitor->type)
        {
            case TESTER_CMD_MONITOR_NONE:
                continue;
            case TESTER_CMD_MONITOR_TA:
                assert(monitor->ta != NULL);
                if (!monitor->run_monitor)
                    continue;
                status = tapi_cfg_cmd_monitor_begin(
                    monitor->ta, monitor->name, monitor->command,
                    monitor->time_to_wait);
                break;
            case TESTER_CMD_MONITOR_TEST:
                status = test_cmd_monitor_begin(monitor);
                break;
        }

        if (status == 0)
            monitor->enabled = true;
        else
        {
            ERROR("Failed to enable TA command monitor for '%s': %r",
                  cmd_monitor_type2str(monitor->type), monitor->command,
                  status);
            rc = -1;
        }
    }

    return rc;
}

/* See description in tester_cmd_monitor.h */
int
stop_cmd_monitors(cmd_monitor_descrs *monitors)
{
    int rc = 0;
    cmd_monitor_descr *monitor;

    TAILQ_FOREACH(monitor, monitors, links)
    {
        te_errno status;

        if (!monitor->enabled)
            continue;

        switch (monitor->type)
        {
            case TESTER_CMD_MONITOR_NONE:
                continue;
            case TESTER_CMD_MONITOR_TA:
                status = tapi_cfg_cmd_monitor_end(monitor->ta, monitor->name);
                break;
            case TESTER_CMD_MONITOR_TEST:
                status = test_cmd_monitor_end(monitor);
                break;
        }

        if (status == 0)
            monitor->enabled = false;
        else
        {
            ERROR("Failed to disable %s command monitor for '%s': %r",
                  cmd_monitor_type2str(monitor->type), monitor->command,
                  status);
            rc = -1;
        }
    }

    return rc;
}

/* See description in tester_cmd_monitor.h */
te_errno
cmd_monitor_set_type(cmd_monitor_descr *monitor, cmd_monitor_type type,
                     const char *reason)
{
    if (monitor->type != type && monitor->type != TESTER_CMD_MONITOR_NONE)
    {
        ERROR("Failed to change command monitor type "
              "from %s to %s during processing %s",
              cmd_monitor_type2str(monitor->type), cmd_monitor_type2str(type),
              reason);
        return TE_OS_RC(TE_TESTER, EINVAL);
    }

    monitor->type = type;
    return 0;
}
