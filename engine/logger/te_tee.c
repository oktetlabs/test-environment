/** @file
 * @brief Logging of input through TE Logger
 *
 * Redirection of stdout/stderr in two files: stdout merged with stderr,
 * stderr only.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#else
#include <poll.h>
#endif

#define TE_LGR_USER     "Self"
#include "te_defs.h"
#include "logger_api.h"
#include "logger_ten.h"
#include "te_raw_log.h"


int
main (int argc, char *argv[])
{
    char          *buffer;
    char          *current;
    char          *fence;
    char          *newline;
    char          *rest = NULL;
    int            interval;
    int            current_timeout = -1;
    int            len;
    struct pollfd  poller;
    static char    buffer1[TE_LOG_FIELD_MAX + 1];
    static char    buffer2[TE_LOG_FIELD_MAX + 1];

    te_log_init("(Tee)");

#define MAYBE_DO_LOG \
    do {                                                       \
        if (current != buffer)                                 \
        {                                                      \
            *current = '\0';                                   \
            newline = strrchr(buffer, '\n');                   \
            if (newline)                                       \
                *newline = '\0';                               \
            LGR_MESSAGE(TE_LL_WARN, argv[2], "%s%s",           \
                        rest ? rest : "",                      \
                        buffer);                               \
            if (!newline)                                      \
            {                                                  \
                fence = buffer + TE_LOG_FIELD_MAX;             \
                rest = NULL;                                   \
            }                                                  \
            else                                               \
            {                                                  \
                rest = newline + 1;                            \
                buffer = (buffer == buffer1 ? buffer2 : buffer1); \
                fence = buffer + TE_LOG_FIELD_MAX - (current - rest);   \
            }                                                  \
            current_timeout = -1;                              \
            current = buffer;                                  \
        }                                                      \
    } while (0)

    if (argc != 4)
    {
        ERROR("Usage: te_tee lgr-entity lgr-user msg-interval");
        return EXIT_FAILURE;
    }
    te_log_init(argv[1]);

    interval = strtol(argv[3], NULL, 10);
    if (interval <= 0)
    {
        ERROR("Invalid interval value: %s", argv[3]);
        return EXIT_FAILURE;
    }

    buffer  = buffer1;
    current = buffer;
    fence   = buffer + TE_LOG_FIELD_MAX;
    *fence  = '\0';
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK | fcntl(STDIN_FILENO, F_GETFL));
    
    for (;;)
    {
        int rc;

        poller.fd = STDIN_FILENO;
        poller.revents = 0;
        poller.events = POLLIN;
        poll(&poller, 1, current_timeout);
        VERB("something is available");
        if (poller.revents & POLLIN)
        {
            VERB("trying to read %d bytes", fence - current);
            len = read(poller.fd, current, fence - current);
            if (len <= 0)
            {
                MAYBE_DO_LOG;
                if (len < 0)
                    ERROR("Error reading from stdin: %s", strerror(errno));
                break;
            }

            /*
             * if the output tells us that it can't take that much data - ok,
             * it can happen, what else can we realistically do?
             *
             * if we actually see an error - something is wrong and we should
             * try to stop doing whatever we're doing.
             */
            rc = write(STDOUT_FILENO, current, len);
            if (rc < 0)
            {
                ERROR("Failed to write date stdout: %s", strerror(errno));
                break;
            }

            current += len;
            if (current == fence)
            {
                MAYBE_DO_LOG;
            }
            else
            {
                if (current_timeout < 0)
                    current_timeout = interval;
            }
        }
        else if (poller.revents & POLLERR)
        {
            MAYBE_DO_LOG;
            ERROR("Error condition signaled on stdin");
            break;
        }
        else if (poller.revents & POLLHUP)
        {
            MAYBE_DO_LOG;
            break;
        }
        else
        {
            VERB("timeout");
            MAYBE_DO_LOG;
        }
    }

    return EXIT_SUCCESS;
}
