/** @file
 * @brief Linux Test Agent
 *
 * Linux Serial Output Logger
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#else
#include <poll.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "linux_internal.h"

#define TE_LGR_USER      "Main"
#include "logger_ta.h"



static te_log_level_t
map_name_to_level(const char *name)
{
    static const struct {
        char            *name;
        te_log_level_t   level;
    } levels[] = {{"ERROR", TE_LL_ERROR},
                  {"WARN",  TE_LL_WARN},
                  {"RING",  TE_LL_RING},
                  {"INFO",  TE_LL_INFO},
                  {"VERB",  TE_LL_VERB}};
    unsigned i;
    
    for (i = 0; i < sizeof(levels) / sizeof(*levels); i++)
    {
        if (!strcmp(levels[i].name, name))
            return levels[i].level;
    }
    return 0;
}

/**
 * Log host serial output via Logger component
 *
 * @param ready       Parameter release semaphore
 * @param argc        Number of string arguments
 * @param argv        String arguments:
 *                    - log user
 *                    - log level
 *                    - message interval
 *                    - tty name
 *                    - sharing mode (opt)
 */
int 
log_serial(void *ready, int argc, char *argv[])
{
    char           user[64] = "";
    char           tmp[PATH_MAX + 16];
    char          *buffer; 
    char          *current;
    char          *fence;
    te_log_level_t level;
    int            interval;
    int            current_timeout = -1;
    int            len;
    struct pollfd  poller;

#define MAYBE_DO_LOG \
    do { \
        if (current != buffer) \
        { \
            *current = '\0'; \
            LGR_MESSAGE(level, user, "%s", buffer); \
            current_timeout = -1; \
            current = buffer; \
        } \
    } while (0)
    
    if (argc < 4)
    {
        ERROR("Too few parameters to log_serial");
        sem_post(ready);
        return TE_RC(TE_TA_LINUX, EINVAL);
    }
    strncpy(user, argv[0], sizeof(user) - 1);
    level = map_name_to_level(argv[1]);
    if (level == 0)
    {
        ERROR("Error level %s is unknown", argv[1]);
        sem_post(ready);
        return TE_RC(TE_TA_LINUX, EINVAL);
    }


    if (argc < 5 || strcmp(argv[4], "exclusive") == 0)
    {
        sprintf(tmp, "fuser -s %s", argv[3]);
        if (ta_system(tmp) == 0)
        {
            ERROR("%s is already is use, won't log", argv[3]);
            sem_post(ready);
            return TE_RC(TE_TA_LINUX, EBUSY);
        }
    }
    else if (strcmp(argv[4], "force") == 0)
    {
        sprintf(tmp, "fuser -s -k %s", argv[3]);
        if (ta_system(tmp) == 0)
            WARN("%s was in use, killing the process", argv[3]);
    }
    else if (strcmp(argv[4], "shared") == 0)
    {
        sprintf(tmp, "fuser -s %s", argv[3]);
        if (ta_system(tmp) == 0)
            WARN("%s is in use, logging anyway", argv[3]);
    }
    else
    {
        ERROR("Invalid sharing mode '%s'", argv[4]);
        sem_post(ready);
        return TE_RC(TE_TA_LINUX, EINVAL);
    }
    
    interval = strtol(argv[2], NULL, 10);
    if (interval <= 0)
    {
        ERROR("Invalid interval value: %s", argv[2]);
        sem_post(ready);
        return TE_RC(TE_TA_LINUX, EINVAL);
    }
    poller.fd = open(argv[3], O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (poller.fd < 0)
    {
        int rc = errno;

        ERROR("Cannot open %s: %d", argv[3], rc);
        sem_post(ready);
        return TE_RC(TE_TA_LINUX, rc);
    }
    if ((buffer = malloc(TE_LOG_FIELD_MAX + 1)) == NULL)
    {
        int rc = errno;

        ERROR("%s(): malloc failed: %d", __FUNCTION__, rc);
        sem_post(ready);
        return TE_RC(TE_TA_LINUX, rc);
    }
    current = buffer;
    fence   = buffer + TE_LOG_FIELD_MAX;
    *fence  = '\0';
    
    sem_post(ready);
    pthread_cleanup_push(close, (void *)poller.fd);
    pthread_cleanup_push(free, buffer);
    for (;;)
    {
        poller.revents = 0;
        poller.events = POLLIN;
        poll(&poller, 1, current_timeout);
        VERB("something is available");
        pthread_testcancel(); 
        if (poller.revents & POLLIN)
        {
            VERB("trying to read %d bytes", fence - current);
            len = read(poller.fd, current, fence - current);
            if (len < 0)
            {
                MAYBE_DO_LOG;
                ERROR("Error reading from terminal: %d", errno);
                break;
            }
            current += len;
            if (current == fence)
            {
                LGR_MESSAGE(level, user, "%s", buffer);
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
            ERROR("Error condition signaled on terminal");
            break;
        }
        else if (poller.revents & POLLHUP)
        {
            MAYBE_DO_LOG;
            RING("Terminal hung up");
            break;
        }
        else
        {
            VERB("timeout");
            MAYBE_DO_LOG;
        }
    }
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    return 0;
#undef MAYBE_DO_LOG
}
