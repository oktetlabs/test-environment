/** @file
 * @brief Routines to call shell command
 *
 * Auxiluary routines to call shell command
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Alexandra Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TE Shell Cmd"

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "logger_api.h"

#include "te_shell_cmd.h"


/*
 * The function implementation is moved to the header in order to
 * avoid issues with rcfunix dynamic linkage.
 */
pid_t
te_shell_cmd(const char *cmd, uid_t uid,
             int *in_fd, int *out_fd, int *err_fd)
{
    return te_shell_cmd_inline(cmd, uid, in_fd, out_fd, err_fd);
}
