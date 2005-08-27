/** @file
 * @brief Routines to call shell command
 *
 * Auxiluary routines to call shell command
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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


//#include "te_defs.h"
//#include "logger_api.h"


/* See description in te_shell_cmd.h */
pid_t
te_shell_cmd(const char *cmd, uid_t uid, int *in_fd, int *out_fd)
{
    int   pid;
    int   in_pipe[2], out_pipe[2];

    if (cmd == NULL)
    {
        errno = EINVAL;
        return -1;
    }
    if (in_fd != NULL && pipe(in_pipe) != 0)
        return -1;
    if (out_fd != NULL && pipe(out_pipe) != 0)
    {
        if (in_fd != NULL)
        {
            close(in_pipe[0]);
            close(in_pipe[1]);
        }
        return -1;
    }

    pid = fork();
    if (pid == 0)
    {
        if (in_fd != 0)
        {
            close(in_pipe[1]);
            if (in_pipe[0] != STDIN_FILENO)
            {
                close(STDIN_FILENO);
                dup2(in_pipe[0], STDIN_FILENO);
            }
        }
        if (out_fd != 0)
        {
            close(out_pipe[0]);
            if (out_pipe[1] != STDOUT_FILENO)
            {
                close(STDOUT_FILENO);
                dup2(out_pipe[1], STDOUT_FILENO);
            }
        }
        if (uid != (uid_t)(-1) && setuid(uid) != 0)
        {
            ERROR("Failed to set user %d before runing command \"%s\"",
                  uid, cmd);
        }
        execl("/bin/sh", "sh", "-c", cmd, (char *) 0);
        return 0;
    }

    if (in_fd != NULL)
        close(in_pipe[0]);
    if (out_fd != NULL)
        close(out_pipe[1]);
    if (pid < 0)
    {
        if (in_fd != NULL)
        {
            close(in_pipe[1]);
            *in_fd = -1;
        }
        if (out_fd != NULL)
        {
            close(out_pipe[0]);
            *out_fd = -1;
        }
    }
    else
    {
        if (in_fd != NULL)
            *in_fd = in_pipe[1];
        if (out_fd != NULL)
            *out_fd = out_pipe[0];
    }
    return pid;
}

