/** @file
 * @brief Logging of input through TE Logger
 *
 * Redirection of stdout/stderr in two files: stdout merged with stderr,
 * stderr only.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#include "config.h"

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
#include "logger_api.h"
#include "logger_ten.h"
#include "te_raw_log.h"


DEFINE_LGR_ENTITY("Tee");


int
main (int argc, char *argv[])
{
    char          *buffer; 
    char          *current;
    char          *fence;
    int            interval;
    int            current_timeout = -1;
    int            len;
    struct pollfd  poller;

#define MAYBE_DO_LOG \
    do { \
        if (current != buffer) \
        { \
            *current = '\0'; \
            LGR_MESSAGE(TE_LL_WARN, argv[1], "%s", buffer); \
            current_timeout = -1; \
            current = buffer; \
        } \
    } while (0)

    if (argc != 3)
    {
        ERROR("Usage: te_tee agent-name msg-interval");
        return EXIT_FAILURE;
    }

    interval = strtol(argv[2], NULL, 10);
    if (interval <= 0)
    {
        ERROR("Invalid interval value: %s", argv[2]);
        return EXIT_FAILURE;
    }

    if ((buffer = malloc(TE_LOG_FIELD_MAX + 1)) == NULL)
    {
        ERROR("%s(): malloc failed: %d", __FUNCTION__, errno);
        return EXIT_FAILURE;
    }
    current = buffer;
    fence   = buffer + TE_LOG_FIELD_MAX;
    *fence  = '\0';
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK | fcntl(STDIN_FILENO, F_GETFL));
    
    for (;;)
    {
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
                int rc = errno;
                
                MAYBE_DO_LOG;
                if (len < 0)
                    ERROR("Error reading from stdin: %d", rc);
                break;
            }
            write(STDOUT_FILENO, current, len);
            current += len;
            if (current == fence)
            {
                LGR_MESSAGE(TE_LL_WARN, argv[1], "%s", buffer);
                current_timeout = -1;
                current = buffer;
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
