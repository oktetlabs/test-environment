/** @file
 * @brief Logger subsystem API - TA side
 *
 * TA side Logger functionality for
 * forked TA processes and newly created threads
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Mamadou Ngom <Mamadou.Ngom@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "LogFork Client"

#include "te_config.h"

#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "te_tools.h"
#include "te_str.h"
#include "ta_common.h"

#include "logfork.h"
#include "logfork_int.h"


/** Socket used by all client to register */
static int logfork_clnt_sockd = -1;

static void *logfork_clnt_sockd_lock = NULL;


/**
 * Open client socket.
 *
 * @retval 0    Success
 * @retval -1   Failure
 *
 * @note should be called under lock
 */
static inline int
open_sock(void)
{
    char *port;
    int   sock;

    struct sockaddr_in addr;

    if ((port = getenv("TE_LOG_PORT")) == NULL)
    {
        fprintf(stderr, "Failed to register logfork user: "
                        "TE_LOG_PORT is not exported\n");
        fflush(stderr);
        return -1;
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        fprintf(stderr,
                "Failed to register logfork user: socket() failed\n");
        fflush(stderr);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(atoi(port));

#ifndef WINDOWS
    fcntl(sock, F_SETFD, FD_CLOEXEC);
#endif

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        fprintf(stderr,
                "Failed to register logfork user: - connect() failed\n");
        fflush(stderr);
        close(sock);
        return -1;
    }

    if (logfork_clnt_sockd_lock == NULL)
        logfork_clnt_sockd_lock = thread_mutex_create();
    thread_mutex_lock(logfork_clnt_sockd_lock);

    if (logfork_clnt_sockd < 0)
    {
        logfork_clnt_sockd = sock;
        thread_mutex_unlock(logfork_clnt_sockd_lock);
    }
    else
    {
        thread_mutex_unlock(&logfork_clnt_sockd_lock);
        close(sock);
    }

    return 0;
}

/* See description in logfork.h */
int
logfork_register_user(const char *name)
{
    logfork_msg msg;

    memset(&msg, 0, sizeof(msg));
    te_strlcpy(msg.__add_name, name, sizeof(msg.__add_name));
    msg.pid = getpid();
    msg.tid = thread_self();
    msg.type = LOGFORK_MSG_ADD_USER;

    if (logfork_clnt_sockd_lock == NULL)
        logfork_clnt_sockd_lock = thread_mutex_create();

    if (logfork_clnt_sockd == -1 && open_sock() != 0)
    {
        return -1;
    }

    if (send(logfork_clnt_sockd, (char *)&msg, sizeof(msg), 0) !=
            (ssize_t)sizeof(msg))
    {
        fprintf(stderr, "logfork_register_user() - cannot send "
                "notification: %s\n", strerror(errno));
        fflush(stderr);
        return -1;
    }

    return 0;
}

int
logfork_set_id_logging(te_bool enabled)
{
    logfork_msg msg;

    memset(&msg, 0, sizeof(msg));
    msg.pid = getpid();
    msg.tid = thread_self();
    msg.type = LOGFORK_MSG_SET_ID_LOGGING;
    msg.msg.set_id_logging.enabled = enabled;

    if (logfork_clnt_sockd_lock == NULL)
        logfork_clnt_sockd_lock = thread_mutex_create();

    if (logfork_clnt_sockd == -1 && open_sock() != 0)
    {
        return -1;
    }

    if (send(logfork_clnt_sockd, (char *)&msg, sizeof(msg), 0) !=
            (ssize_t)sizeof(msg))
    {
        fprintf(stderr, "%s() - cannot send update message: %s\n",
                __FUNCTION__, strerror(errno));
        fflush(stderr);
        return -1;
    }

    return 0;
}

/* See description in logfork.h */
int
logfork_delete_user(pid_t pid, uint32_t tid)
{
    logfork_msg msg;

    memset(&msg, 0, sizeof(msg));
    msg.pid = pid;
    msg.tid = tid;
    msg.type = LOGFORK_MSG_DEL_USER;

    if (logfork_clnt_sockd_lock == NULL)
        logfork_clnt_sockd_lock = thread_mutex_create();

    if (logfork_clnt_sockd == -1 && open_sock() != 0)
    {
        return -1;
    }

    if (send(logfork_clnt_sockd, (char *)&msg, sizeof(msg), 0) !=
            (ssize_t)sizeof(msg))
    {
        fprintf(stderr, "logfork_delete_user() - cannot send "
                "user delete request: %s\n", strerror(errno));
        fflush(stderr);
        return -1;
    }

    return 0;
}
/**
 * Function for logging to be used by forked processes.
 *
 * This function complies with te_log_message_f prototype.
 */
void
logfork_log_message(const char *file, unsigned int line,
                    te_log_ts_sec sec, te_log_ts_usec usec,
                    unsigned int level, const char *entity,
                    const char *user, const char *fmt, va_list ap)
{
    logfork_msg msg;

    static te_bool init = FALSE;

    struct te_log_out_params cm =
        { NULL, (uint8_t *)msg.__log_msg, sizeof(msg.__log_msg), 0 };

    UNUSED(file);
    UNUSED(line);
    UNUSED(entity);

    memset(&msg, 0, sizeof(msg));

    (void)te_log_vprintf_old(&cm, fmt, ap);

    msg.pid = getpid();
    msg.tid = thread_self();
    msg.type = LOGFORK_MSG_LOG;
    te_strlcpy(msg.__lgr_user, user, sizeof(msg.__lgr_user));
    msg.__log_sec = sec;
    msg.__log_usec = usec;
    msg.__log_level = level;

    if (!init && logfork_clnt_sockd == -1)
        open_sock();

    init = TRUE;

    if (logfork_clnt_sockd == -1)
    {
        fprintf(stdout, "%s %s\n", user, msg.__log_msg);
        fflush(stdout);
        return;
    }

    if (send(logfork_clnt_sockd, (char *)&msg, sizeof(msg), 0) !=
            (ssize_t)sizeof(msg))
    {
        fprintf(stderr, "%s(): sendto() failed: %s\n",
                __FUNCTION__, strerror(errno));
    }
}
