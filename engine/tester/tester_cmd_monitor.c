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

#include "te_defs.h"
#include "te_queue.h"
#include "te_errno.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_cfg_cmd_monitor.h"
#include "tester_cmd_monitor.h"

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
