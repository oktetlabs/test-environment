/** @file
 * @brief Power Distribution Unit Proxy Test Agent
 *
 * Cold reboot host via command line
 *
 * Copyright (C) 2003-2021 OKTET Labs. All rights reserved.
 *
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#include <stdlib.h>
#include <stddef.h>
#if HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include "te_config.h"

#include "te_stdint.h"
#include "te_defs.h"

#include "logger_api.h"
#include "logger_ta.h"

#include "agentlib.h"
#include "rcf_common.h"

static char *cmd_to_cold_reboot = NULL;

/* See description in power_ctl_internal.h */
te_errno
ta_shell_cold_reboot(char *id)
{
    char buf[RCF_MAX_PATH];
    int rc;

    snprintf(buf, RCF_MAX_PATH, cmd_to_cold_reboot, id);

    RING("Reboot '%s' with '%s'", id, buf);
    rc = ta_system(buf);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
    {
        ERROR("Failed to cold reboot '%s', rc=%r", id, rc);
        return TE_EFAIL;
    }

    return 0;
}

/* See description in power_ctl_internal.h */
int
ta_shell_init_cold_reboot(char *param)
{
    char *cmd;

    cmd = strchr(param, '=');
    if (cmd == NULL)
    {
        fprintf(stderr, "Failed to get command to cold reboot\n");
        return -1;
    }

    cmd++;
    cmd_to_cold_reboot = strdup(cmd);
    if (cmd_to_cold_reboot == NULL)
    {
        fprintf(stderr, "Failed to allocate memory\n");
        return -1;
    }

    return 0;
}