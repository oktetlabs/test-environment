/** @file
 * @brief Unix Kernel Logger
 *
 * Unix Kernel Logger implementation
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER      "Log Kernel"

#include "te_config.h"
#include "package.h"

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
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
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
#include "te_raw_log.h"
#include "logger_api.h"
#include "rcf_common.h"
#include "te_serial_parser.h"

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

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

#define NETCONSOLE_PREF     "netconsole:"

static int (*func_system)(const char *cmd) = &system;

void
te_kernel_log_set_system_func(void *p)
{
    func_system = p;
}

/**
 * Described in te_kernel_log.h
 */
te_errno 
te_get_host_addrs(const char *host_name, struct sockaddr_in *host_ipv4,
                  te_bool *ipv4_found, struct sockaddr_in6 *host_ipv6,
                  te_bool *ipv6_found)
{
#if defined(HAVE_GETADDRINFO) && defined(HAVE_NETDB_H)
    struct addrinfo    *addrs;
    struct addrinfo    *p;
    int                 rc = 0;
    int                 tmp_err;

    rc = getaddrinfo(host_name, NULL, NULL,
                     &addrs);
    if (rc < 0)
    {
        tmp_err = errno;
        ERROR("%s(): failed to get info about the host %s, errno '%s'",
              __FUNCTION__, host_name, strerror(tmp_err));
        return te_rc_os2te(tmp_err);
    }

    for (p = addrs; p != NULL; p = p->ai_next)
    {
        if (p->ai_family == AF_INET && host_ipv4 != NULL &&
            ipv4_found != NULL)
        {
            memcpy(host_ipv4, p->ai_addr, p->ai_addrlen);
            *ipv4_found = TRUE;
        }
        else if (p->ai_family == AF_INET6 && host_ipv6 != NULL &&
                 ipv6_found != NULL)
        {
            memcpy(host_ipv6, p->ai_addr, p->ai_addrlen);
            *ipv6_found = TRUE;
        }
    }

    freeaddrinfo(addrs);
    return 0;
#else
    UNUSED(host_name);
    UNUSED(host_ipv4);
    UNUSED(ipv4_found);
    UNUSED(host_ipv6);
    UNUSED(ipv6_found);

    ERROR("%s(): was not compiled due to lack of system features",
          __FUNCTION__);
    return TE_ENOSYS;
#endif
}


/* See description in the te_kernel_log.h */
int
map_name_to_level(const char *name)
{
    static const struct {
        char           *name;
        te_log_level    level;
    } levels[] = {{"ERROR", TE_LL_ERROR},
                  {"WARN",  TE_LL_WARN},
                  {"RING",  TE_LL_RING},
                  {"INFO",  TE_LL_INFO},
                  {"VERB",  TE_LL_VERB},
                  {"PACKET", TE_LL_PACKET}};
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
 * @param srv_address   An address to connect
 * @param user          A conserver user name
 * @param console       A console name
 *
 * @sa open_conserver
 */
static int
connect_conserver(struct sockaddr_storage *srv_addr, const char *user,
                  const char *console)
{
    int                sock = -1;

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

    sock = socket(SA(srv_addr)->sa_family == AF_INET ? PF_INET : PF_INET6,
                  SOCK_STREAM, 0);
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

    VERB("Connecting to conserver");
    if (connect(sock, (struct sockaddr *)srv_addr, sizeof(*srv_addr)) < 0)
    {
        ERROR("Unable to connect to conserver: errno=%d",
              errno);
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
 *                   [(IP address or host name):]port:user:console
 *                   (parenthesises are necessary only for IPv6 address)
 */
static int
open_conserver(const char *conserver)
{
    in_port_t   port;
    int         sock;
    char        buf[1];
    char       *tmp = NULL;
    char       *dup_arg = NULL;
    char        user[64];
    char       *console = NULL;
    char       *parenthesis = NULL;
    char       *point = NULL;
    char       *colon = NULL;
    char       *endptr = NULL;
    te_bool     is_ipv4 = TRUE;

    struct sockaddr_storage srv_addr;

    if (strlen(conserver) == 0)
    {
        ERROR("Conserver configuration is void string");
        return -1;
    }
    dup_arg = strdup(conserver);
    tmp = dup_arg;
    point = strchr(tmp, '.');
    colon = strchr(tmp, ':');
    strtol(tmp, &endptr, 10);

    memset(&srv_addr, 0, sizeof(srv_addr));

    if (*tmp == '(' || (point != NULL && point < colon) ||
        (endptr != NULL && *endptr != ':'))
    {
        if (*tmp == '(')
        {
            tmp++;
            parenthesis = strchr(tmp, ')');
            if (parenthesis == NULL)
            {
                ERROR("Wrong conserver configuration string: \"%s\"",
                      conserver);
                free(dup_arg);
                return -1;
            }
            *parenthesis = '\0';
            colon = parenthesis + 1;
            if (*colon != ':')
            {
                ERROR("Bad conserver configuration string: \"%s\"",
                      conserver);
                free(dup_arg);
                return -1;
            }
        }
        else
            *colon = '\0';

        point = strchr(tmp, '.');
        if (point == NULL)
            is_ipv4 = FALSE;
        
        if (is_ipv4)
            SIN(&srv_addr)->sin_family = AF_INET;
        else
            SIN6(&srv_addr)->sin6_family = AF_INET6;

        if (inet_pton(is_ipv4 ? AF_INET : AF_INET6,
                      tmp,
                      is_ipv4 ? (void *)&SIN(&srv_addr)->sin_addr :
                                (void *)&SIN6(&srv_addr)->sin6_addr) != 1)
        {
            struct sockaddr_in  ipv4_addr;
            struct sockaddr_in6 ipv6_addr;
            te_bool             ipv4_found;
            te_bool             ipv6_found;
            int                 rc;

            rc = te_get_host_addrs(tmp, &ipv4_addr, &ipv4_found,
                                   &ipv6_addr, &ipv6_found);
            if (rc == 0)
            {
                if (ipv4_found)
                    memcpy(&srv_addr, &ipv4_addr, sizeof(ipv4_addr));
                else if (ipv6_found)
                    memcpy(&srv_addr, &ipv6_addr, sizeof(ipv6_addr));
                else rc = -1;
            }

            if (rc != 0)
            {
                ERROR("Bad address or host name: \"%s\"", conserver);
                free(dup_arg);
                return -1;
            }
        }

        tmp = colon + 1;
    }
    else
    {
        SIN(&srv_addr)->sin_addr.s_addr = htonl(INADDR_ANY);
        SIN(&srv_addr)->sin_family = AF_INET;
        /* If address is omitted but colon remained */
        if (*tmp == ':')
            tmp++;
    }

    port = strtoul(tmp, &colon, 10);

    if (port <= 0 || *colon != ':')
    {
        ERROR ("Bad port: \"%s\"", conserver);
        free(dup_arg);
        return -1;
    }

    if (is_ipv4)
        SIN(&srv_addr)->sin_port = htons(port);
    else
        SIN6(&srv_addr)->sin6_port = htons(port);

    strcpy(user, colon + 1);
    console = strchr(user, ':');
    if (console == NULL)
    {
        ERROR ("No console specified: \"%s\"", conserver);
        free(dup_arg);
        return -1;
    }
    *console++ = '\0';

    free(dup_arg);

    sock = connect_conserver(&srv_addr, user, console);
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

    if (is_ipv4)
        SIN(&srv_addr)->sin_port = htons(port);
    else
        SIN6(&srv_addr)->sin6_port = htons(port);

    close(sock);
    sock = connect_conserver(&srv_addr, user, console);
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
                WARN("Parser %s has detected overlap with pattern '%s'. "
                     "Tester event %s is activated.", parser->name, pat->v,
                     event->t_name);
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

/* See description in te_kernel_log.h */
int
log_serial(void *ready, int argc, char *argv[])
{
    serial_parser_t parser;
    int             rc;

    if (argc < 4)
    {
        ERROR("Too few parameters to serial_console_log");
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    memset(&parser, 0, sizeof(parser));

    strncpy(parser.user, argv[0], TE_SERIAL_MAX_NAME);
    parser.level = map_name_to_level(argv[1]);
    if (parser.level == 0)
    {
        ERROR("Error level %s is unknown", argv[1]);
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    parser.interval = strtol(argv[2], NULL, 10);
    if (parser.interval <= 0)
    {
        ERROR("Invalid interval value: %s", argv[2]);
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    strncpy(parser.c_name, argv[3], TE_SERIAL_MAX_NAME);
    if (argc == 5)
        strncpy(parser.mode, argv[4], TE_SERIAL_MAX_NAME);
    sem_post(ready);

    parser.rcf = TRUE;
    parser.logging = TRUE;
    parser.port = -1;
    rc = pthread_mutex_init(&parser.mutex, NULL);
    if (rc != 0)
    {
        ERROR("Couldn't init mutex of the %s parser, error: %s",
              parser.name, strerror(rc));
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    te_serial_parser(&parser);

    return 0;
}

/* See description in the te_serial_parser.h */
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

    time_t  now;
    time_t  last_alive = 0;
    int     incomp_str_count = 0;
    te_bool rcf;
    char    user[TE_SERIAL_MAX_NAME + 1];

#define MAYBE_DO_LOG \
do {                                                            \
    if (current != buffer)                                      \
    {                                                           \
        *current = '\0';                                        \
        newline = strrchr(buffer, '\n');                        \
        if (newline == NULL)                                    \
        {                                                       \
            incomp_str_count++;                                 \
            rest_len = strlen(buffer);                          \
            if (rest_len == LOG_SERIAL_MAX_LEN)                 \
                incomp_str_count = 10;                          \
        }                                                       \
        else                                                    \
        {                                                       \
            incomp_str_count = 10;                              \
            *newline++ = '\0';                                  \
            if (*newline == '\r')                               \
               newline++;                                       \
            rest_len = current - newline;                       \
        }                                                       \
                                                                \
        if ((*buffer != '\0' && incomp_str_count >= 10) || rest_len == 0) \
        {                                                       \
            if (rcf)                                            \
                LGR_MESSAGE(TE_LL_WARN, user, "%s", buffer);    \
            else                                                \
                parser_data_processing(parser, buffer);         \
            incomp_str_count = 0;                               \
            if (rest_len > 0 && newline != NULL)                \
            {                                                   \
                current = buffer + rest_len;                    \
                memmove(buffer, newline, rest_len);             \
            }                                                   \
            else                                                \
                current = buffer;                               \
            *current = '\0';                                    \
            current_timeout = LOG_SERIAL_ALIVE_TIMEOUT;         \
        }                                                       \
    }                                                           \
} while (0)

    TE_SERIAL_MALLOC(buffer, LOG_SERIAL_MAX_LEN + 1);

    if (pthread_mutex_lock(&parser->mutex) != 0)
    {
        ERROR("Couldn't to lock the mutex");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    interval = parser->interval;
    rcf = parser->rcf;
    strncpy(user, parser->c_name, sizeof(user));

    if (strncmp(parser->c_name, NETCONSOLE_PREF,
                strlen(NETCONSOLE_PREF)) == 0)
    {
        struct sockaddr_in local_addr;

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (parser->port < 0)
            parser->port =
                strtol(parser->c_name + strlen(NETCONSOLE_PREF), NULL, 10);

        local_addr.sin_port = htons(parser->port);

        pthread_mutex_unlock(&parser->mutex);
        poller.fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (poller.fd < 0 || 
            bind(poller.fd, (struct sockaddr *)&local_addr,
                 sizeof(local_addr)) < 0)
        {
            ERROR("netconsole init socket() or bind() failed: %s",
                  strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
    }
    else if (*parser->c_name != '/')
    {
        if (parser->port >= 0)
            snprintf(tmp, RCF_MAX_PATH, "%d:%s:%s", parser->port,
                     parser->user, parser->c_name);
        else
            strncpy(tmp, parser->c_name, RCF_MAX_PATH);
        pthread_mutex_unlock(&parser->mutex);
        if ((poller.fd = open_conserver(tmp)) < 0)
            return TE_OS_RC(TE_TA_UNIX, errno);
    }
    else
    {
        if (strlen(parser->mode) == 0 ||
            strcmp(parser->mode, "exclusive") == 0)
        {
            sprintf(tmp, "fuser -s %s", parser->mode);
            if (func_system(tmp) == 0)
            {
                pthread_mutex_unlock(&parser->mutex);
                ERROR("%s is already is use, won't log", parser->c_name);
                return TE_RC(TE_TA_UNIX, TE_EBUSY);
            }
        }
        else if (strcmp(parser->mode, "force") == 0)
        {
            sprintf(tmp, "fuser -s -k %s", parser->c_name);
            if (func_system(tmp) == 0)
                WARN("%s was in use, killing the process", parser->c_name);
        }
        else if (strcmp(parser->mode, "shared") == 0)
        {
            sprintf(tmp, "fuser -s %s", parser->c_name);
            if (func_system(tmp) == 0)
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
