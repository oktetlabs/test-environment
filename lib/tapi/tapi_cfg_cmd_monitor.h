/** @file
 * @brief Test API to configure command monitor.
 *
 * @defgroup tapi_cmd_monitor_def Command monitor TAPI
 * @ingroup tapi_conf
 * @{
 *
 * Definition of API to configure command monitor.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_CMD_MONITOR_H__
#define __TE_TAPI_CFG_CMD_MONITOR_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start a command monitor. It will periodically run specified
 * shell command and log its output.
 *
 * @param ta            Test Agent name.
 * @param name          Name for a node in configuration tree.
 * @param command       Command to run.
 * @param time_to_wait  How long to wait before running a command
 *                      again, in milliseconds.
 *
 * @return Status code.
 */
static inline te_errno
tapi_cfg_cmd_monitor_begin(char const *ta,
                           char const *name,
                           char const *command,
                           int time_to_wait)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                              "/agent:%s/command_monitor:%s",
                              ta, name);
    if (rc != 0)
        return rc;

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, time_to_wait),
                                   "/agent:%s/command_monitor:%s/time_to_wait:",
                                   ta, name)) != 0 ||
        (rc = cfg_set_instance_fmt(CFG_VAL(STRING, command),
                                   "/agent:%s/command_monitor:%s/command:",
                                   ta, name)) != 0 ||
        (rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                   "/agent:%s/command_monitor:%s/enable:",
                                   ta, name)) != 0)
    {
        cfg_del_instance_fmt(FALSE,
                             "/agent:%s/command_monitor:%s",
                             ta, name);
        return rc;
    }

    return 0;
}

/**
 * Stop a command monitor (removing its node from configuration tree).
 *
 * @param ta          Test Agent name.
 * @param name        Name of the command monitor node in configuration
 *                    tree.
 *
 * @return Status code.
 */
static inline te_errno
tapi_cfg_cmd_monitor_end(char const *ta,
                         char const *name)
{
    return cfg_del_instance_fmt(FALSE,
                                "/agent:%s/monitor:%s",
                                ta, name);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

/**@} <!-- END tapi_cmd_monitor_def --> */

#endif /* !__TE_TAPI_CFG_CMD_MONITOR_H__ */
