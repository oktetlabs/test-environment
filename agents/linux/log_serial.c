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

#include "te_config.h"
#include "config.h"

#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#else
#include <poll.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
#include "rcf_common.h"
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
 * Auxiliary procedure to connect to conserver.
 *
 * @return connected socket or -1 if failed
 *
 * @param port     A port to connect
 * @param user     A conserver user name
 * @param console  A console name
 *
 * @sa open_conserver
 */
static int
connect_conserver(int port, const char *user, const char *console)
{
    int                sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in inaddr;

    char               buf[32];
    int                len;

#define EXPECT_OK \
    do { \
        if (read(sock, buf, 4) < 4 || memcmp(buf, "ok\r\n", 4) != 0) \
        { \
            ERROR("Conserver sent us non-ok, errno=%d", errno); \
            close(sock); \
            return -1; \
        } \
    } while(0);

    if (sock < 0)
    {
        ERROR("Cannot create socket: errno=%d", errno);
        return -1;
    }
    inaddr.sin_family      = AF_INET;
    inaddr.sin_port        = htons(port);
    inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    VERB("Connecting to conserver at localhost:%d", port);
    if (connect(sock, (struct sockaddr *)&inaddr, sizeof(inaddr)) < 0)
    {
        ERROR("Unable to connect to conserver on port %d: errno=%d", 
              port, errno);
        close(sock);
        return -1;
    }
    EXPECT_OK;
    VERB("Connected");
    strcpy(buf, "login ");
    strncat(buf, user, sizeof(buf) - 8);
    strcat(buf, "\n");
    len = strlen(buf);
    if (write(sock, buf, len) < len)
    {
        ERROR("Error writing to conserver socket, errno=%d", errno);
        close(sock);
        return -1;
    }
    EXPECT_OK;
    VERB("Logged in");

    strcpy(buf, "call ");
    strncat(buf, console, sizeof(buf) - 7);
    strcat(buf, "\n");
    len = strlen(buf);
    if (write(sock, buf, len) < len)
    {
        ERROR("Error writing to conserver socket, errno=%d", errno);
        close(sock);
        return -1;
    }
    
    return sock;

#undef EXPECT_OK
}


/**
 * Connects to conserver listening on a given port at localhost and
 * authenticates to it.
 *
 * @return connected socket (or -1 if failed)
 *
 * @param conserver  A colon-separated string of the form:
 *                   port:user:console
 */
static int
open_conserver(const char *conserver)
{
    int   port;
    int   sock;
    char  buf[1];
    char *tmp;
    char  user[64];
    char *console;
    

    port = strtoul(conserver, &tmp, 10);
    
    if (port <= 0 || *tmp != ':')
    {
        ERROR ("Bad port: \"%s\"", conserver);
        return -1;
    }
    strcpy(user, tmp + 1);
    console = strchr(user, ':');
    if (console == NULL)
    {
        ERROR ("No console specified: \"%s\"", conserver);
        return -1;
    }
    *console++ = '\0';

    sock = connect_conserver(port, user, console);
    if (sock < 0)
        return -1;
    port = 0;
    for(;;)
    {
        if (read(sock, buf, 1) < 1)
        {
            ERROR("Error getting console port, errno=%d", errno);
            close(sock);
            return -1;
        }
        if (*buf == '\r')
            continue;
        if (*buf == '\n')
            break;
        if (!isdigit(*buf))
        {
            ERROR("Non-numeric response from conserver: %c", *buf);
            close(sock);
            return -1;
            
        }
        port = (port * 10) + *buf - '0';
    }

    close(sock);
    sock = connect_conserver(port, user, console);
    if (sock < 0)
        return -1;

#define SKIP_LINE \
    do { \
        for(*buf = '\0'; *buf != '\n'; ) \
        { \
            if (read(sock, buf, 1) < 1) \
            { \
                ERROR("Error reading from conserver, errno=%d", errno); \
                close(sock); \
                return -1; \
            } \
       } \
    } while(0)
    
    SKIP_LINE;
    write(sock, "\xFF\05c;", 4); /* this magic string causes conserver 
                                   * to actually send us data
                                   */
    SKIP_LINE;
    fcntl(sock, F_SETFL, O_NONBLOCK | fcntl(sock, F_GETFL));
    return sock;
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
 *                    (if it does not start with "/", it is interpreted
 *                     as a conserver connection designator)
 *                    - sharing mode (opt)
 */
int 
log_serial(void *ready, int argc, char *argv[])
{
    char           user[64] = "";
    char           tmp[RCF_MAX_PATH + 16];

    /*
     * 'volatile' is used below to avoid warnings in AMD-64 build.
     * If 'volatile' is not no specified, the following warning is
     * generated by compiler:
     *      variable `current_timeout' might be clobbered by `longjmp'
     *      or `vfork'
     * This has something to do with pthread_cleanup_* calls
     * (in the sense that when these are commented out, no warnings
     * are issued). However, IMHO they have nothing in common with
     * non-local control transfers, so I would say it's just a compiler
     * attempting to be over-smart. -- artem
     */
    char * volatile buffer; 
    char * volatile other_buffer;
    char * volatile rest = NULL;
    char * volatile current;
    char * volatile fence;
    volatile int    current_timeout = -1;

    te_log_level_t  level;
    char           *newline;
    int             interval;
    int             len;
    struct pollfd   poller;

#define MAYBE_DO_LOG \
    do {                                                       \
        if (current != buffer)                                 \
        {                                                      \
            *current = '\0';                                   \
            newline = strrchr(buffer, '\n');                   \
            if (newline)                                       \
            {                                                  \
                *newline = '\0';                               \
                if (newline[1] == '\r')                        \
                   newline++;                                  \
            }                                                  \
            LGR_MESSAGE(TE_LL_WARN, user, "%s%s",              \
                        rest ? rest : "",                      \
                        buffer);                               \
            if (!newline)                                      \
            {                                                  \
                fence = buffer + TE_LOG_FIELD_MAX;             \
                rest = NULL;                                   \
            }                                                  \
            else                                               \
            {                                                  \
                char *tmp;                                     \
                                                               \
                rest = newline + 1;                            \
                tmp = buffer;                                  \
                buffer = other_buffer;                         \
                other_buffer = tmp;                            \
                fence = buffer + TE_LOG_FIELD_MAX - (current - rest);   \
            }                                                  \
            current_timeout = -1;                              \
            current = buffer;                                  \
        }                                                      \
    } while (0)
    
    if (argc < 4)
    {
        ERROR("Too few parameters to log_serial");
        sem_post(ready);
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
    }
    strncpy(user, argv[0], sizeof(user) - 1);
    level = map_name_to_level(argv[1]);
    if (level == 0)
    {
        ERROR("Error level %s is unknown", argv[1]);
        sem_post(ready);
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
    }

    interval = strtol(argv[2], NULL, 10);
    if (interval <= 0)
    {
        ERROR("Invalid interval value: %s", argv[2]);
        sem_post(ready);
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
    }

    if ((buffer = malloc(TE_LOG_FIELD_MAX + 1)) == NULL)
    {
        int rc = errno;

        ERROR("%s(): malloc failed at line %d: %d", __FUNCTION__, 
              __LINE__, rc);
        sem_post(ready);
        return TE_OS_RC(TE_TA_LINUX, rc);
    }
    if ((other_buffer = malloc(TE_LOG_FIELD_MAX + 1)) == NULL)
    {
        int rc = errno;

        ERROR("%s(): malloc failed at line %d: %d", __FUNCTION__, 
              __LINE__, rc);
        sem_post(ready);
        return TE_OS_RC(TE_TA_LINUX, rc);
    }

    if (*argv[3] != '/')
    {
        static char tmp[128];
        strncpy(tmp, argv[3], sizeof(tmp) - 1);
        sem_post(ready);
        poller.fd = open_conserver(tmp);
        if (poller.fd < 0)
        {
            return TE_OS_RC(TE_TA_LINUX, errno);
        }
    }
    else
    {
        if (argc < 5 || strcmp(argv[4], "exclusive") == 0)
        {
            sprintf(tmp, "fuser -s %s", argv[3]);
            if (ta_system(tmp) == 0)
            {
                sem_post(ready);
                ERROR("%s is already is use, won't log", argv[3]);
                return TE_RC(TE_TA_LINUX, TE_EBUSY);
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
            sem_post(ready);
            ERROR("Invalid sharing mode '%s'", argv[4]);
            return TE_RC(TE_TA_LINUX, TE_EINVAL);
        }
        
        poller.fd = open(argv[3], O_RDONLY | O_NOCTTY | O_NONBLOCK);
        if (poller.fd < 0)
        {
            int rc = errno;
            
            sem_post(ready);
            ERROR("Cannot open %s: %d", argv[3], rc);
            return TE_OS_RC(TE_TA_LINUX, rc);
        }
        sem_post(ready);
    }


    current = buffer;
    fence   = buffer + TE_LOG_FIELD_MAX;
    *fence  = '\0';
    
    pthread_cleanup_push((void (*)(void *))close, (void *)(long)poller.fd);
    pthread_cleanup_push(free, buffer);
    pthread_cleanup_push(free, other_buffer);
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
            VERB("%d bytes actually read", len);
            current += len;
            if (current == fence)
            {
                MAYBE_DO_LOG;
            }
            else
            {
                if (current_timeout < 0)
                    current_timeout = interval;
                VERB("timeout will be %d", current_timeout);
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
    pthread_cleanup_pop(1); /* free other buffer */
    pthread_cleanup_pop(1); /* free buffer */
    pthread_cleanup_pop(1); /* close fd */
    return 0;
#undef MAYBE_DO_LOG
}
