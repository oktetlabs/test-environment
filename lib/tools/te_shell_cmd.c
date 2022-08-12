/** @file
 * @brief Routines to call shell command
 *
 * Auxiluary routines to call shell command
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Alexandra Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TE Shell Cmd"

#include "te_config.h"

#include "logger_api.h"

#include "te_shell_cmd.h"

pid_t
te_shell_cmd(const char *cmd, uid_t uid,
             int *in_fd, int *out_fd, int *err_fd)
{
    return te_exec_child("/bin/sh",
            (char *const[]){"sh", "-c", (char *)cmd, (char *)NULL},
            NULL, uid, in_fd, out_fd, err_fd, NULL);
}
