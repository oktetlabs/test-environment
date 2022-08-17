/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Remote applications logs handling
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER      "Log Remote"

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
#include "te_str.h"
#include "comm_agent.h"
#include "te_raw_log.h"
#include "logger_api.h"
#include "rcf_common.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "unix_internal.h"


/** Maximum length of accumulated log */
#define LOG_REMOTE_MAX_LEN          2047

static te_log_level
map_name_to_level(const char *name)
{
    static const struct {
        char           *name;
        te_log_level    level;
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
 * Wrapper for close() to be called in thread cleanup.
 *
 * @param p     Pointer to socket FD.
 */
static void
close_cleanup_wrapper(void *p)
{
    int *s = (int *)p;

    close(*s);
}

/**
 * Remote logs output via Logger
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
log_remote(void *ready, int argc, char *argv[])
{
    char               user[64];
    int                s;
    in_addr_t          addr = 0; /* any address */
    struct sockaddr_in saddr;
    int                log_level;
    int                interval;
    int                rc;
    int                port;
    fd_set             r_set;
    struct timeval     timeout;
    uint8_t           *buffer;

    /* reading arguments */
    if (argc < 5)
    {
        ERROR("%s: few arguments",
              __FUNCTION__);
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* address */
    if (0 != strcmp(argv[0], "any"))
        addr = inet_addr(argv[0]);

    /* log_level */
    log_level = map_name_to_level(argv[1]);
    if (log_level == 0)
    {
        ERROR("%s: log level '%s' is unknown", argv[1]);
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* polling interval */
    interval = strtol(argv[2], NULL, 10);
    if (interval <= 0)
    {
        ERROR("%s: invalid interval value: '%s'", argv[2]);
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#define TE_LOG_REMOTE_DEFAULT_PORT_NUMBER 10239
    /* port number to listen on, default it 10239 */
    port = strtol(argv[3], NULL, 10);
    if (port < 0)
    {
        ERROR("%s: invalid port value: '%s'", argv[3]);
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* user */
    TE_STRLCPY(user, argv[4], sizeof(user));

    /* create socket and bind socket */
    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        int rc = errno;

        ERROR("%s: failed to open socket for incomming logs: %s",
              __FUNCTION__, strerror(rc));
        sem_post(ready);

        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    /* set reuseaddr */
    {
        int optval = 1;

        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                       &optval, sizeof(optval)) != 0)
        {
            rc = TE_OS_RC(TE_COMM, errno);
            ERROR("setsockopt(SOL_SOCKET, SO_REUSEADDR, enabled): errno=%d\n",
                  errno);
            close(s);
            return rc;
        }

        optval = 1000000;
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF,
                       &optval, sizeof(optval)) != 0)
        {
            rc = TE_OS_RC(TE_COMM, errno);
            ERROR("setsockopt(SOL_SOCKET, SO_REUSEADDR, enabled): errno=%d\n",
                  errno);
            close(s);
            return rc;
        }

    }



    /* fill address */
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = addr;
    saddr.sin_family = AF_INET;

    rc = bind(s, (struct sockaddr *)&saddr, sizeof(saddr));
    if (rc != 0)
    {
        int rc = errno;

        ERROR("%s: failed to bind socket: %s",
              __FUNCTION__, strerror(rc));
        close(s);
        sem_post(ready);

        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    if ((buffer = malloc(LOG_REMOTE_MAX_LEN + 1)) == NULL)
    {
        int rc = errno;

        ERROR("%s: malloc failed at line %d: %d", __FUNCTION__,
              __LINE__, rc);
        sem_post(ready);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    sem_post(ready);

    pthread_cleanup_push(&close_cleanup_wrapper,
                         (void *)&s);
    pthread_cleanup_push(free, buffer);

    for (;;)
    {
        pthread_testcancel();

        FD_ZERO(&r_set);
        FD_SET(s, &r_set);

        timeout.tv_sec = 0;
        timeout.tv_usec = interval;


        rc = select(s + 1, &r_set, NULL, NULL, &timeout);
        if (rc < 1)
            continue;

        if (FD_ISSET(s, &r_set))
        {
            /* read logs and log them */
            rc = read(s, buffer, LOG_REMOTE_MAX_LEN);
            if (rc < 0)
                continue;

            buffer[rc] = '\0';

            LGR_MESSAGE(log_level, user, "%s",
                        buffer);
        }
    }
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);

    return 0;
}
