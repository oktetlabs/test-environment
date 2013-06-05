/** @file
 * @brief Serial console parser thread
 *
 * Implementation of the serial console parser thread.
 *
 * Copyright (C) 2004-2012 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef TE_LGR_USER
#define TE_LGR_USER      "Serial console parser thread"
#endif

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#else
#include <poll.h>
#endif

#include "rcf_common.h"
#include "logger_api.h"
#include "conf_serial_parser.h"
#include "unix_internal.h"

/** Time interval for Log Serial Alive messages */
#define LOG_SERIAL_ALIVE_TIMEOUT    60000

/** Maximum length of accumulated log */
#define LOG_SERIAL_MAX_LEN          2047

/** Conserver escape sequences */
#define CONSERVER_ESCAPE    "\05c"
#define CONSERVER_CMDLEN    3
#define CONSERVER_START     CONSERVER_ESCAPE ";"
#define CONSERVER_SPY       CONSERVER_ESCAPE "s"
#define CONSERVER_STOP      CONSERVER_ESCAPE "."

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

#if HAVE_FCNTL_H
    /*
     * Try to set close-on-exec flag, but ignore failures,
     * since it's not critical.
     */
    (void)fcntl(sock, F_SETFD, FD_CLOEXEC);
#endif

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
            char err_msg[64] = "";

            err_msg[0] = *buf;
            read(sock, err_msg + 1, sizeof(err_msg) - 2);
            ERROR("Conserver said: \"%s\", quitting", err_msg);
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
    write(sock, CONSERVER_START, CONSERVER_CMDLEN);
    SKIP_LINE;
    write(sock, CONSERVER_SPY, CONSERVER_CMDLEN);
    SKIP_LINE;
    fcntl(sock, F_SETFL, O_NONBLOCK | fcntl(sock, F_GETFL));
    return sock;
#undef SKIP_LINE
}

/**
 * Processing of the serial console output data
 * 
 * @param parser    Pointer to the parser config
 * @param buffer    Buffer with a data
 */
static void
parser_data_processing(serial_parser_t *parser, char *buffer)
{
    serial_event_t   *event;
    serial_pattern_t *pat;

    if (pthread_mutex_lock(&parser->mutex) != 0)
    {
        ERROR("Couldn't to lock the mutex");
        return;
    }

    SLIST_FOREACH(event, &parser->events, ent_ev_l)
    {
        SLIST_FOREACH(pat, &event->patterns, ent_pat_l)
        {
            if (strstr(buffer, pat->v) != NULL)
            {
                event->status = TRUE;
                event->count++;
                break;
            }
        }
    }

    if (parser->logging == TRUE)
        LGR_MESSAGE(parser->level, parser->c_name, "%s", buffer);

    if (pthread_mutex_unlock(&parser->mutex) != 0)
    {
        ERROR("Couldn't to unlock the mutex");
        return;
    }
}

/* See description in the conf_serial_parser.h */
int 
te_serial_parser(serial_parser_t *parser)
{
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
    char * volatile current;
    char * volatile fence;
    volatile int    current_timeout = LOG_SERIAL_ALIVE_TIMEOUT;

    char           *newline;
    int             interval;
    int             len;
    int             rest_len;
    struct pollfd   poller;

    time_t now;
    time_t last_alive = 0;

#define MAYBE_DO_LOG \
    {                                                               \
        if (current != buffer)                                      \
        {                                                           \
            *current = '\0';                                        \
            newline = strrchr(buffer, '\n');                        \
            if (newline == NULL)                                    \
            {                                                       \
                rest_len = 0;                                       \
            }                                                       \
            else                                                    \
            {                                                       \
                *newline++ = '\0';                                  \
                if (*newline == '\r')                               \
                   newline++;                                       \
                rest_len = current - newline;                       \
            }                                                       \
                                                                    \
            if (*buffer != '\0')                                    \
                parser_data_processing(parser, buffer);             \
            if (rest_len > 0)                                       \
                memmove(buffer, newline, rest_len);                 \
                                                                    \
            current = buffer + rest_len;                            \
            *current = '\0';                                        \
            current_timeout = LOG_SERIAL_ALIVE_TIMEOUT;             \
        }                                                           \
    }

    TE_SERIAL_MALLOC(buffer, LOG_SERIAL_MAX_LEN + 1);

    if (pthread_mutex_lock(&parser->mutex) != 0)
    {
        ERROR("Couldn't to lock the mutex");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    interval = parser->interval;

    if (*parser->c_name != '/')
    {
        if (parser->port >= 0)
            snprintf(tmp, RCF_MAX_PATH, "%d:%s:%s", parser->port,
                     parser->user, parser->c_name);
        else
            strncpy(tmp, parser->c_name, RCF_MAX_PATH);
        pthread_mutex_unlock(&parser->mutex);
        poller.fd = open_conserver(tmp);
        if (poller.fd < 0)
        {
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
    }
    else
    {
        if (strlen(parser->mode) == 0 ||
            strcmp(parser->mode, "exclusive") == 0)
        {
            sprintf(tmp, "fuser -s %s", parser->mode);
            if (ta_system(tmp) == 0)
            {
                pthread_mutex_unlock(&parser->mutex);
                ERROR("%s is already is use, won't log", parser->c_name);
                return TE_RC(TE_TA_UNIX, TE_EBUSY);
            }
        }
        else if (strcmp(parser->mode, "force") == 0)
        {
            sprintf(tmp, "fuser -s -k %s", parser->c_name);
            if (ta_system(tmp) == 0)
                WARN("%s was in use, killing the process", parser->c_name);
        }
        else if (strcmp(parser->mode, "shared") == 0)
        {
            sprintf(tmp, "fuser -s %s", parser->c_name);
            if (ta_system(tmp) == 0)
                WARN("%s is in use, logging anyway", parser->c_name);
        }
        else
        {
            pthread_mutex_unlock(&parser->mutex);
            ERROR("Invalid sharing mode '%s'", parser->mode);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        poller.fd = open(parser->c_name, O_RDONLY | O_NOCTTY | O_NONBLOCK);
        if (poller.fd < 0)
        {
            int rc = errno;

            pthread_mutex_unlock(&parser->mutex);
            ERROR("Cannot open %s: %d", parser->c_name, rc);
            return TE_OS_RC(TE_TA_UNIX, rc);
        }
        pthread_mutex_unlock(&parser->mutex);
    }

    current = buffer;
    *current = '\0';

    fence   = buffer + LOG_SERIAL_MAX_LEN;
    *fence  = '\0';

    pthread_cleanup_push(free, buffer);
    for (;;)
    {
        poller.revents = 0;
        poller.events = POLLIN;
        poll(&poller, 1, current_timeout);

        VERB("something is available");
        pthread_testcancel();

        now = time(NULL);
        if (now - last_alive >= LOG_SERIAL_ALIVE_TIMEOUT / 1000)
        {
            INFO("%s() thread is alive", __FUNCTION__);
            last_alive = now;
        }

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
            else if (len == 0)
            {
                MAYBE_DO_LOG;
                ERROR("Terminal is closed");
                break;
            }
            current += len;
            *current = '\0';
            VERB("%d bytes actually read: %s", len, current - len);

            if (current == fence)
            {
                MAYBE_DO_LOG;
            }
            else
            {
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
    pthread_cleanup_pop(1); /* free buffer */
    return 0;
#undef MAYBE_DO_LOG
}
