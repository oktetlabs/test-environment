/** @file
 * @brief API to call shell commands
 *
 * Routines to call shell commands
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

#ifndef __TE_SHELL_CMD_H__
#define __TE_SHELL_CMD_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "te_defs.h"
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function to base system()-like and popen()-like functions on it.
 * You MUST use uid parameter instead of "su - user -c", because su makes
 * one more fork, and you do not know how to kill this grandchild process.
 *
 * If you use this fuction from agent, this process SHOULD be catched by 
 * ta_waitpid() or killed by ta_kill_death().
 *
 * @param cmd    command to run in the shell
 * @param uid    user id to run the shell; use -1 for default
 * @param in_fd  location to store file descriptor of pipe to stdin of 
 *               the shell command; use NULL for standard stdin
 * @param out_fd location to store file descriptor of pipe to stdout of
 *               the shell command; use NULL for standard stdout
 * 
 * @return pid value, positive on success, negative on failure
 */
static inline pid_t
te_shell_cmd_inline(const char *cmd, uid_t uid, 
                    int *in_fd, int *out_fd, int *err_fd)
{
    int   pid;
    int   in_pipe[2], out_pipe[2], err_pipe[2];

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
    if (err_fd != NULL && pipe(err_pipe) != 0)
    {
        if (in_fd != NULL)
        {
            close(in_pipe[0]);
            close(in_pipe[1]);
        }
        if (out_fd != NULL)
        {
            close(out_pipe[0]);
            close(out_pipe[1]);
        }
        return -1;
    }

    pid = fork();
    if (pid == 0)
    {
        int pipe_fd[3] = { 0, 1, 2};
        int busy_fd[6] = { -1, -1, -1, -1, -1, -1};
        int i;

        /* Set us to be the process leader */
        setpgid(getpid(), getpid());
        if (uid != (uid_t)(-1) && setuid(uid) != 0)
        {
            ERROR("Failed to set user %d before runing command \"%s\"",
                  uid, cmd);
        }

        /* Find file descriptors */
        if (in_fd != NULL )
        {
            close(in_pipe[1]);
            pipe_fd[0] = in_pipe[0];
        }
        if (out_fd != NULL )
        {
            close(out_pipe[0]);
            pipe_fd[1] = out_pipe[1];
        }
        if (err_fd != NULL )
        {
            close(err_pipe[0]);
            pipe_fd[2] = err_pipe[1];
        }
        for (i = 0; i < 3; i++)
        {
            if (pipe_fd[i] >= 0 && pipe_fd[i] < 6)
                busy_fd[pipe_fd[i]] = i;
        }

        /* Dup file descriptors where necessary */
        for (i = 0; i < 3; i++)
        {
            if (busy_fd[i] == -1)
            {
                dup2(pipe_fd[i], i);
                close(pipe_fd[i]);
                pipe_fd[i] = i;
            }
        }

        /* Most likely, all pipe fds are duped correctly at this point */
        if (pipe_fd[0] != 0 || pipe_fd[1] != 1 || pipe_fd[2] != 2)
        {
            WARN("This part of code in %s is not debugged properly. "
                 "Tell sasha@oktetlabs.ru if it is really triggered",
                 __FUNCTION__);
            for (i = 0; i < 3; i++)
            {
                if (pipe_fd[i] != i && pipe_fd[i] < 3)
                {
                    /* Free 0,1,2 fds by duping them into 4,5,6
                     * busy_fd is now correct in 3-5 interval only */
                    int j;

                    for (j = 3; j < 6; j++)
                    {
                        if (busy_fd[j] == -1)
                        {
                            dup2(pipe_fd[i], j);
                            close(pipe_fd[i]);
                            busy_fd[j] = i;
                            pipe_fd[i] = j;
                            break;
                        }
                    }
                }
            }
            for (i = 0; i < 3; i++)
            {
                if (pipe_fd[i] != i)
                {
                    dup2(pipe_fd[i], i);
                    close(pipe_fd[i]);
                }
            }
        }

        execl("/bin/sh", "sh", "-c", cmd, (char *) 0);
        return 0;
    }

    if (in_fd != NULL)
        close(in_pipe[0]);
    if (out_fd != NULL)
        close(out_pipe[1]);
    if (err_fd != NULL)
        close(err_pipe[1]);
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
        if (err_fd != NULL)
        {
            close(err_pipe[0]);
            *err_fd = -1;
        }
    }
    else
    {
        if (in_fd != NULL)
            *in_fd = in_pipe[1];
        if (out_fd != NULL)
            *out_fd = out_pipe[0];
        if (err_fd != NULL)
            *err_fd = err_pipe[0];
    }
    return pid;
}

extern pid_t te_shell_cmd(const char *cmd, uid_t uid,
                          int *in_fd, int *out_fd, int *err_fd);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_SHELL_CMD_H__ */
