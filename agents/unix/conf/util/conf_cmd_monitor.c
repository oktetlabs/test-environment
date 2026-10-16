/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent command output monitor support.
 *
 * Implementation of unix TA command output monitor configuring support.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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

#include "te_alloc.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_errno.h"
#include "te_str.h"
#include "te_string.h"
#include "te_vector.h"
#include "rcf_ch_api.h"
#include "logger_api.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"

#include "te_cmd_monitor.h"

static TAILQ_HEAD(, cmd_monitor_t) cmd_monitors_h;

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
 * Find the command monitor a request is addressed to.
 *
 * @param ctx       Request context
 *
 * @return The command monitor structure pointer or @c NULL.
 */
static cmd_monitor_t *
monitor_find(const ta_conf_ctx *ctx)
{
    return monitor_find_by_name(ta_conf_ctx_inst(ctx, "command_monitor"));
}

/**
 * Add the command monitor object
 *
 * @param ctx       Request context
 *
 * @return 0 on success or error code on failure
 */
static te_errno
cmd_monitor_add(const ta_conf_ctx *ctx)
{
    const char      *name = ta_conf_ctx_inst(ctx, "command_monitor");
    cmd_monitor_t   *monitor;

    monitor = monitor_find_by_name(name);
    if (monitor != NULL)
    {
        ERROR("Cannot add another command monitor with "
              "the same name '%s'",
              name);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }
    monitor = TE_ALLOC(sizeof(*monitor));

    monitor->enable = false;
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
_monitor_set_enable(cmd_monitor_t *monitor, bool enable)
{
    int rc = 0;

    if (enable == monitor->enable)
        return 0;

    if (enable)
    {
        rc = pthread_create(&monitor->thread, NULL,
                            te_command_monitor, monitor);
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

    if ((rc = _monitor_set_enable(monitor, false)) != 0)
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
 * @param ctx       Request context
 *
 * @return 0 on success or error code on failure
 */
static te_errno
cmd_monitor_del(const ta_conf_ctx *ctx)
{
    cmd_monitor_t   *monitor = monitor_find(ctx);

    if (monitor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return _cmd_monitor_del(monitor);
}

/**
 * Get list of names of all command monitor objects
 *
 * @param ctx       Request context (unused)
 * @param names     Vector of heap-allocated names to append to
 *
 * @return 0 on success or error code on failure
 */
static te_errno
cmd_monitors_list(const ta_conf_ctx *ctx, te_vec *names)
{
    cmd_monitor_t    *monitor;

    UNUSED(ctx);

    TAILQ_FOREACH(monitor, &cmd_monitors_h, links)
    {
        char *name = TE_STRDUP(monitor->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

/**
 * Find a command monitor whose properties may be changed.
 *
 * @param ctx           Request context
 * @param monitor [out] Command monitor structure pointer
 *
 * @return 0 on success or error code on failure
 */
static te_errno
monitor_find_disabled(const ta_conf_ctx *ctx, cmd_monitor_t **monitor)
{
    *monitor = monitor_find(ctx);
    if (*monitor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if ((*monitor)->enable)
    {
       ERROR("Cannot change monitor properties while it is enabled");
       return TE_RC(TE_TA_UNIX, TE_EBUSY);
    }

    return 0;
}

/**
 * Get the time to wait between command runs.
 *
 * @param ctx          Request context
 * @param val    [out] Time to wait, in milliseconds
 *
 * @return 0 on success or error code on failure
 */
static te_errno
monitor_time_to_wait_get(const ta_conf_ctx *ctx, int32_t *val)
{
    cmd_monitor_t *monitor = monitor_find(ctx);
    int            v;
    te_errno       rc;

    if (monitor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    /* Not set yet; the monitor thread takes "" as 0 */
    if (*monitor->time_to_wait == '\0')
    {
        *val = 0;
        return 0;
    }

    rc = te_strtoi(monitor->time_to_wait, 10, &v);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    *val = v;
    return 0;
}

/**
 * Set the time to wait between command runs.
 *
 * @param ctx          Request context
 * @param val          Time to wait, in milliseconds
 *
 * @return 0 on success or error code on failure
 */
static te_errno
monitor_time_to_wait_set(const ta_conf_ctx *ctx, int32_t val)
{
    cmd_monitor_t *monitor;
    te_errno       rc;

    rc = monitor_find_disabled(ctx, &monitor);
    if (rc != 0)
        return rc;

    free(monitor->time_to_wait);
    monitor->time_to_wait = te_string_fmt("%" PRId32, val);

    return 0;
}

/**
 * Get the monitored command.
 *
 * @param ctx          Request context
 * @param val    [out] Shell command
 *
 * @return 0 on success or error code on failure
 */
static te_errno
monitor_command_get(const ta_conf_ctx *ctx, te_string *val)
{
    cmd_monitor_t *monitor = monitor_find(ctx);

    if (monitor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(val, "%.*s", RCF_MAX_VAL - 1, monitor->command);
    return 0;
}

/**
 * Set the monitored command.
 *
 * @param ctx          Request context
 * @param val          Shell command
 *
 * @return 0 on success or error code on failure
 */
static te_errno
monitor_command_set(const ta_conf_ctx *ctx, const char *val)
{
    cmd_monitor_t *monitor;
    te_errno       rc;

    rc = monitor_find_disabled(ctx, &monitor);
    if (rc != 0)
        return rc;

    free(monitor->command);
    monitor->command = strdup(val);

    return 0;
}

/**
 * Get the enable state of a command monitor.
 *
 * @param ctx          Request context
 * @param val    [out] Enabled (1) or disabled (0)
 *
 * @return 0 on success or error code on failure
 */
static te_errno
monitor_enable_get(const ta_conf_ctx *ctx, int32_t *val)
{
    cmd_monitor_t *monitor = monitor_find(ctx);

    if (monitor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = monitor->enable;
    return 0;
}

/**
 * Enable or disable a command monitor.
 *
 * @param ctx          Request context
 * @param val          Non-zero to enable, 0 to disable
 *
 * @return 0 on success or error code on failure
 */
static te_errno
monitor_enable_set(const ta_conf_ctx *ctx, int32_t val)
{
    cmd_monitor_t *monitor = monitor_find(ctx);

    if (monitor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return _monitor_set_enable(monitor, val != 0);
}

static const ta_conf_node *const node_cmd_monitor =
    TA_CONF_COLL("command_monitor", cmd_monitor_add, cmd_monitor_del,
                 cmd_monitors_list,
        TA_CONF_RW_INT32("time_to_wait", monitor_time_to_wait_get,
                         monitor_time_to_wait_set),
        TA_CONF_RW_STR("command", monitor_command_get, monitor_command_set),
        TA_CONF_RW_INT32("enable", monitor_enable_get, monitor_enable_set));

/*
 * Initializing command monitor subtree.
 *
 * @return 0 on success or error code on failure
 */
te_errno
ta_unix_conf_cmd_monitor_init(void)
{
    TAILQ_INIT(&cmd_monitors_h);

    return ta_conf_register("/agent", node_cmd_monitor);
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
