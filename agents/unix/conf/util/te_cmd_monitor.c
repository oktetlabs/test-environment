/** @file
 * @brief Command monitor
 *
 * Command monitor process implementation.
 *
 * Copyright (C) 2014 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */
#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "te_rpc_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "logger_ta_lock.h"
#include "logfork.h"
#include "te_cmd_monitor.h"
#include "unix_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <string.h>
#include <pthread.h>

/** User environment */
extern char **environ;

/**
 * Clean things up on thread termination.
 *
 * @param arg   Pointer to pipe fds to be closed
 */
static void
cleanup_handler(void *arg)
{
    int *pipefds = arg;
    if (pipefds[0] >= 0)
        close(pipefds[0]);
    if (pipefds[1] >= 0)
        close(pipefds[1]);
}

/* See description in te_cmd_monitor.h */
void * 
te_command_monitor(void *arg)
{
#define STR_LEN 2048
#define MONITOR_OUT \
    RING("[Command monitor '%s'] Output from command '%s':\n%s", \
         monitor->name, command, buf);

    cmd_monitor_t *monitor;
    unsigned long int time_to_wait;
    char *endptr = NULL;
    char *command;
    pid_t command_pid;
    int pipefds[2] = {-1, -1};

    monitor = (cmd_monitor_t *)arg;

    command = monitor->command;

    errno = 0;
    time_to_wait = strtoul(monitor->time_to_wait, &endptr, 0);
    if (errno != 0 || (endptr != NULL && *endptr != '\0'))
    {
        ERROR("%s(): failed to parse time_to_wait value in '%s'",
              __FUNCTION__, monitor->time_to_wait);
        pthread_exit(NULL);
    }

    pthread_cleanup_push(cleanup_handler, pipefds);
    if (pipe(pipefds) != 0)
    {
        ERROR("%s(): failed to create a pipe, errno %d (%s)",
              __FUNCTION__, errno, errno_rpc2str(errno_h2rpc(errno)));
        pthread_exit(NULL);
    }

    while (1)
    {
        struct timespec req;

        fflush(stdin);
        fflush(stdout);

        command_pid = fork();
        if (command_pid == 0)
        {
            const char *sh_argv[4];

            if (dup2(pipefds[1], STDOUT_FILENO) < 0 ||
                dup2(pipefds[1], STDERR_FILENO) < 0)
            {
                ERROR("%s(): failed to redirect command output, "
                      "errno %d (%s)",
                      __FUNCTION__, errno,
                      errno_rpc2str(errno_h2rpc(errno)));
                pthread_exit(NULL);
            }

            sh_argv[0] = "/bin/sh";
            sh_argv[1] = "-c";
            sh_argv[2] = command;
            sh_argv[3] = NULL;

            execve("/bin/sh", (char * const *)sh_argv,
                   (char * const *)environ);

            assert(errno != 0);
            ERROR("%s(): failed to call execve(), errno %d (%s)",
                  __FUNCTION__, errno,
                  errno_rpc2str(errno_h2rpc(errno)));
            pthread_exit(NULL);
        }
        else
        {
            int status;
            int rc;
            int rc2;
            struct pollfd poll_fd;
            char buf[STR_LEN + 1];
            int buf_pos = 0;

            buf[0] = '\0';            
            do {
                poll_fd.fd = pipefds[0];
                poll_fd.events = POLLIN;
                poll_fd.revents = 0;
                if (poll(&poll_fd, 1, 10) == 1 &&
                    (poll_fd.revents & POLLIN))
                {
                    rc2 = read(pipefds[0], buf + buf_pos,
                               STR_LEN - buf_pos);
                    if (rc2 > 0)
                    {
                        buf[buf_pos + rc2] = '\0';
                        if (rc2 == STR_LEN - buf_pos)
                        {
                            char *p = NULL;

                            p = strrchr(buf, '\n');
                            if (p == NULL)
                                p = buf + STR_LEN;

                            *p = '\0';
                            MONITOR_OUT;
                            if (p < buf + STR_LEN)
                                memmove(buf, p + 1,
                                        STR_LEN - (p + 1 - buf) + 1);
                            else
                                buf[0] = '\0';
                        }
                        buf_pos = strlen(buf);
                    }
                }
            } while ((rc = ta_waitpid(command_pid, &status, WNOHANG)) == 0);

            if (buf[0] != '\0')
                MONITOR_OUT;

            if (rc < 0)
                ERROR("%s(): failed to wait for a command termination, "
                      "errno %d (%s)",
                      __FUNCTION__, errno,
                      errno_rpc2str(errno_h2rpc(errno)));
            else
            {
                if (WIFEXITED(status)) 
                    RING("Command '%s' exited with status %d",
                         command, WEXITSTATUS(status));
                else if (WIFSIGNALED(status))
                    RING("Command '%s' was terminated by signal %d",
                         command, WTERMSIG(status));
                else
                    ERROR("waitpid() returned unexpected status");
            }
        }

        req.tv_sec = time_to_wait / 1000;
        req.tv_nsec = (time_to_wait % 1000) * 1000000L;
        nanosleep(&req, NULL);
    }

    pthread_cleanup_pop(1);
    return NULL;
}
