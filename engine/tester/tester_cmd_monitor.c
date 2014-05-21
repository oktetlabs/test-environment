/** @file
 * @brief Tester Subsystem
 *
 * Code dealing with running command monitors
 *
 *
 * Copyright (C) 2004-2014 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
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
        if (monitor->ta != NULL && monitor->run_monitor)
        {
            if (tapi_cfg_cmd_monitor_begin(monitor->ta,
                                           monitor->name,
                                           monitor->command,
                                           monitor->time_to_wait) == 0)
                monitor->enabled = TRUE;
            else
            {
                ERROR("Failed to enable command monitor for '%s'",
                      monitor->command);
                rc = -1;
            }
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
        if (monitor->ta != NULL && monitor->enabled)
        {
            if (tapi_cfg_cmd_monitor_end(monitor->ta,
                                         monitor->name) == 0)
                monitor->enabled = FALSE;
            else
            {
                ERROR("Failed to enable command monitor for '%s'",
                      monitor->command);
                rc = -1;
            }
        }
    }

    return rc;
}
