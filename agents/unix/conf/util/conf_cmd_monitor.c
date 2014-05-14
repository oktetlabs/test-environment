/** @file
 * @brief Unix Test Agent command output monitor support.
 *
 * Implementation of unix TA command output monitor configuring support.
 *
 * Copyright (C) 2004-2014 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef TE_LGR_USER
#define TE_LGR_USER      "Unix Conf Command Monitor"
#endif

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "te_defs.h"
#include "te_queue.h"
#include "te_errno.h"
#include "rcf_ch_api.h"
#include "logger_api.h"
#include "rcf_pch.h"

#include "te_cmd_monitor.h"

TAILQ_HEAD(, cmd_monitor_t) cmd_monitors_h;

/**
 * Searching for the command monitor by name.
 * 
 * @param name        Command monitor name.
 * 
 * @return The command monitor structure pointer or @c NULL.
 */
static cmd_monitor_t *
monitor_find_by_name(const char *name)
{
    cmd_monitor_t *monitor = NULL;

    TAILQ_FOREACH(monitor, &cmd_monitors_h, links)
        if (strcmp(monitor->name, name) == 0)
            break;

    return monitor;
}

/**
 * Add the command monitor object
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     Node value (unused)
 * @param pname     The command monitor name name
 *
 * @return 0 on success or error code on failure
 */
static te_errno
cmd_monitor_add(unsigned int gid, const char *oid, char *value, char *name)
{
    cmd_monitor_t   *monitor;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    monitor = monitor_find_by_name(name);
    if (monitor != NULL)
    {
        ERROR("Cannot add another command monitor with "
              "the same name '%s'",
              name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    monitor = calloc(1, sizeof(*monitor));

    monitor->enable = FALSE;
    monitor->name = strdup(name);
    monitor->command = strdup("");
    monitor->time_to_wait = strdup("");

    TAILQ_INSERT_TAIL(&cmd_monitors_h, monitor, links);

    return 0;
}

/**
 * Auxiliary function to enable/disable a command monitor
 *
 * @param monitor   Command monitor structure pointer
 * @param enable    Whether we should enable or disable it
 *
 * @return 0 on success or error code on failure
 */
static te_errno
_monitor_set_enable(cmd_monitor_t *monitor, te_bool enable)
{
    int rc = 0;

    if (enable == monitor->enable)
        return 0;

    if (enable)
    {
        rc = pthread_create(&monitor->thread, NULL,
                            (void *)&te_command_monitor, monitor);
        if (rc != 0)
        {
            ERROR("Cannot start the monitor thread for command '%s'",
                  monitor->command);
            return TE_RC(TE_TA_UNIX, te_rc_os2te(rc));
        }
    }
    else
    {
        rc = pthread_cancel(monitor->thread); 
        if (rc != 0 && rc != ESRCH)
        {
            ERROR("Cannot cancel the monitor thread for command '%s'",
                  monitor->command);
            return TE_RC(TE_TA_UNIX, te_rc_os2te(rc));
        }
        rc = pthread_join(monitor->thread, NULL);
        if (rc != 0)
        {
            ERROR("Cannot join the monitor thread for command '%s'",
                  monitor->command);
            return TE_RC(TE_TA_UNIX, te_rc_os2te(rc));
        }
    }

    monitor->enable = enable;
    return 0;
}

/**
 * Auxiliary function to delete a command monitor object
 *
 * @param monitor   Command monitor structure pointer
 *
 * @return 0 on success or error code on failure
 */
static te_errno
_cmd_monitor_del(cmd_monitor_t *monitor)
{
    te_errno rc = 0;

    if ((rc = _monitor_set_enable(monitor, FALSE)) != 0)
        return rc;

    TAILQ_REMOVE(&cmd_monitors_h, monitor, links);

    free(monitor->name);
    free(monitor->command);
    free(monitor->time_to_wait);
    free(monitor);

    return 0;
}

/**
 * Delete a command monitor object
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param name      Command monitor object name
 *
 * @return 0 on success or error code on failure
 */
static te_errno
cmd_monitor_del(unsigned int gid, const char *oid, const char *name)
{
    cmd_monitor_t   *monitor = monitor_find_by_name(name);

    UNUSED(gid);
    UNUSED(oid);

    if (monitor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return _cmd_monitor_del(monitor);
}

/**
 * Get list of names of all command monitor objects
 *
 * @param gid         Group identifier (unused)
 * @param oid         Full object instance identifier (unused)
 * @param list [out]  Location for the list
 *
 * @return 0 on success or error code on failure
 */
static te_errno
cmd_monitors_list(unsigned int gid, const char *oid, char **list)
{
    cmd_monitor_t    *monitor;
    size_t            list_size = 0;
    size_t            list_len  = 0;

    UNUSED(gid);
    UNUSED(oid);

    *list = NULL;

    list_len = 0;
    TAILQ_FOREACH(monitor, &cmd_monitors_h, links)
    {
        if (list_len + strlen(monitor->name) + 2 >= list_size)
        {
            list_size = (list_size + strlen(monitor->name) + 2) * 2;
            if ((*list = realloc(*list, list_size)) == NULL)
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }

        list_len += sprintf(*list + list_len, "%s ", monitor->name);
    }

    return 0;
}

/**
 * Common function to get command monitor properties values.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier.
 * @param value  [out] Property value.
 * @param name         The monitor name.
 *
 * @return 0 on success or error code on failure
 */
static te_errno
monitor_common_get(unsigned int gid, const char *oid, char *value,
                   const char *name)
{
    cmd_monitor_t *monitor = monitor_find_by_name(name);

    UNUSED(gid);

    if (monitor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (value == NULL)
    {
       ERROR("A buffer to get a variable value is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strstr(oid, "/enable:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%d", monitor->enable);
    else if (strstr(oid, "/command:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%s", monitor->command);
    else if (strstr(oid, "/time_to_wait:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%s", monitor->time_to_wait);
    else
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return 0;
}

/**
 * Enable or disable a command monitor.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param value        Enabled (1) or disabled (0).
 * @param name         The monitor name.
 *
 * @return 0 on success or error code on failure
 */
static te_errno
monitor_set_enable(unsigned int gid, const char *oid, char *value,
                   const char *name)
{
    cmd_monitor_t *monitor = monitor_find_by_name(name);
    te_bool        enable = FALSE;

    UNUSED(gid);
    UNUSED(oid);

    if (monitor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    enable = (atoi(value) == 0) ? FALSE : TRUE;

    return _monitor_set_enable(monitor, enable);
}

/**
 * Common function to set command monitor properties values.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier.
 * @param value        Property value.
 * @param name         The monitor name.
 *
 * @return 0 on success or error code on failure
 */
static te_errno
monitor_common_set(unsigned int gid, const char *oid, char *value,
                   const char *name)
{
    cmd_monitor_t *monitor = monitor_find_by_name(name);

    UNUSED(gid);

    if (monitor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (value == NULL)
    {
       ERROR("A buffer to set a variable value is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    if (monitor->enable)
    {
       ERROR("Cannot change monitor properties while it is enabled");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strstr(oid, "/command:") != NULL)
    {
        free(monitor->command);
        monitor->command = strdup(value);
    }
    else if (strstr(oid, "/time_to_wait:") != NULL)
    {
        free(monitor->time_to_wait);
        monitor->time_to_wait = strdup(value);
    }
    else
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return 0;
}

RCF_PCH_CFG_NODE_RW(monitor_enable, "enable", NULL,
                    NULL, monitor_common_get, monitor_set_enable);
RCF_PCH_CFG_NODE_RW(monitor_command, "command", NULL,
                    &monitor_enable,
                    monitor_common_get, monitor_common_set);
RCF_PCH_CFG_NODE_RW(monitor_time, "time_to_wait", NULL,
                    &monitor_command,
                    monitor_common_get, monitor_common_set);

static rcf_pch_cfg_object node_monitor_inst =
    { "monitor", 0, &monitor_time, NULL,
      NULL, NULL,
      (rcf_ch_cfg_add)cmd_monitor_add, (rcf_ch_cfg_del)cmd_monitor_del,
      (rcf_ch_cfg_list)cmd_monitors_list, NULL, NULL };

/*
 * Initializing command monitor subtree.
 *
 * @return 0 on success or error code on failure
 */
te_errno
ta_unix_conf_cmd_monitor_init(void)
{
    TAILQ_INIT(&cmd_monitors_h);

    return rcf_pch_add_node("/agent", &node_monitor_inst);
}

/*
 * Cleaning things up on termination.
 *
 * @return 0 on success or error code on failure
 */
te_errno
ta_unix_conf_cmd_monitor_cleanup(void)
{
    cmd_monitor_t     *monitor;
    int                rc;

    while(!TAILQ_EMPTY(&cmd_monitors_h))
    {
        monitor = TAILQ_FIRST(&cmd_monitors_h);
        if (monitor == NULL)
            break;

        if ((rc = _cmd_monitor_del(monitor)) != 0)
            return rc;
    }

    return 0;
}
