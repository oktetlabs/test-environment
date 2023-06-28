/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Logging of input through TE Logger
 *
 * Redirection of stdout/stderr in two files: stdout merged with stderr,
 * stderr only.
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
#include "te_str.h"

#define DEFAULT_LL TE_LL_WARN
#define DEFAULT_LOG_USER default_log_user

#define LOG_ENTITY_BUF_LEN 20
#define LOG_USER_BUF_LEN 15

static const char *default_log_user;

typedef struct log_msg {
    te_log_level level;
    char entity[LOG_ENTITY_BUF_LEN];
    char user[LOG_USER_BUF_LEN];
    te_string msg;
} log_msg;

#define LOG_MESSAGE_INIT \
    {.level = 0, .entity = {0,}, .user = {0,}, .msg = TE_STRING_INIT}

static inline void
flush_msg_buffer(log_msg* msg)
{
    if (msg->msg.len > 0)
    {
        TE_LOG(msg->level, msg->entity, msg->user, "%s", msg->msg.ptr);

        te_string_reset(&msg->msg);
    }
}

static te_errno
line_handler(char *line, void *user_data)
{
    log_msg *cur_msg = (log_msg *)user_data;

    te_string_append(&cur_msg->msg, "%s\n", line);
    flush_msg_buffer(cur_msg);

    return 0;
}

int
main (int argc, char *argv[])
{
    static char buffer[TE_LOG_FIELD_MAX + 1];
    int interval;
    int current_timeout = -1;
    int len;
    struct pollfd  poller;
    te_string buffer_str = TE_STRING_INIT;
    log_msg cur_msg = LOG_MESSAGE_INIT;


    te_log_init("(Tee)", ten_log_message);

    if (argc != 4)
    {
        ERROR("Usage: te_tee lgr-entity lgr-user msg-interval");
        return EXIT_FAILURE;
    }
    te_log_init(argv[1], NULL);
    default_log_user = argv[2];

    interval = strtol(argv[3], NULL, 10);
    if (interval <= 0)
    {
        ERROR("Invalid interval value: %s", argv[3]);
        return EXIT_FAILURE;
    }

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK | fcntl(STDIN_FILENO, F_GETFL));

    te_strlcpy(cur_msg.entity, TE_LGR_ENTITY, sizeof(cur_msg.entity));
    te_strlcpy(cur_msg.user, DEFAULT_LOG_USER, sizeof(cur_msg.user));
    cur_msg.level = DEFAULT_LL;

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
            VERB("trying to read %d bytes", sizeof(buffer));
            len = read(poller.fd, buffer, sizeof(buffer));
            if (len <= 0)
            {
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
            rc = write(STDOUT_FILENO, buffer, len);
            if (rc < 0)
            {
                ERROR("Failed to write date stdout: %s", strerror(errno));
                break;
            }

            te_string_append(&buffer_str, "%s", buffer);
            te_string_process_lines(&buffer_str, TRUE, line_handler, &cur_msg);

            if (current_timeout < 0)
                current_timeout = interval;
        }
        else if (poller.revents & POLLERR)
        {
            flush_msg_buffer(&cur_msg);
            ERROR("Error condition signaled on stdin");
            break;
        }
        else if (poller.revents & POLLHUP)
        {
            flush_msg_buffer(&cur_msg);
            break;
        }
        else
        {
            VERB("timeout");
            flush_msg_buffer(&cur_msg);
        }
    }

    te_string_process_lines(&buffer_str, FALSE, line_handler, &cur_msg);

    return EXIT_SUCCESS;
}
