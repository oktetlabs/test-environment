/** @file
 * @brief Logger subsystem API - TEN side
 *
 * TEN side Logger library.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Logger TEN"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#else
/* Temporary for debugging */
#error We have no pthread.h!
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_raw_log.h"
#include "ipc_client.h"
#include "logger_api.h"
#include "logger_int.h"
#include "logger_ten.h"
#include "logger_ten_int.h"


/** Maximum logger message length */
#define LGR_TEN_MSG_BUF_INIT    0x1000


#ifdef HAVE_PTHREAD_H
/** Mutual exclusion execution lock */
static pthread_mutex_t  lgr_lock = PTHREAD_MUTEX_INITIALIZER;
#endif
/**
 * Handle of Logger IPC client.
 *
 * @note It should be used under lgr_lock only.
 */
static struct ipc_client *lgr_client = NULL;
/**
 * Buffer for log message.
 *
 * @note It should be used under lgr_lock only.
 */
static uint8_t *lgr_msg_buf = NULL;
/**
 * Length of the buffer for log message.
 *
 * @note It should be used under lgr_lock only.
 */
static size_t lgr_msg_buf_len = 0;

/** Path to the directory with TE logs */
static const char *te_log_dir = NULL;
/** Transport to log messages */
static te_log_message_tx_f te_log_message_tx = NULL;


/**
 * Log message via IPC.
 *
 * @param msg       Message to be logged
 * @param len       Length of the message to be logged
 */
static void
log_message_ipc(const void *msg, size_t len)
{
    if (ipc_send_message(lgr_client, LGR_SRV_NAME, msg, len) != 0)
    {
        fprintf(stderr, "Failed to send message to IPC server '%s': ",
                LGR_SRV_NAME);
        perror("");
    }
}


/* See description in logger_api.h */
void
log_message(uint16_t level, const char *entity_name,
            const char *user_name, const char *form_str, ...)
{
    va_list ap;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&lgr_lock);
#endif
    
    if (lgr_client == NULL)
    {
        char name[32];

        if (snprintf(name, sizeof(name), "lgr_client_%u",
                     (unsigned int)getpid()) >= (int)sizeof(name))
        {
            fprintf(stderr, "ERROR: Logger client name truncated");
        }
        lgr_client = ipc_init_client(name);
        if (lgr_client == NULL)
        {
#ifdef HAVE_PTHREAD_H
            pthread_mutex_unlock(&lgr_lock);
#endif
            return;
        }
        te_log_message_tx = log_message_ipc;
    }
    if (lgr_msg_buf == NULL)
    {
        lgr_msg_buf = malloc(LGR_TEN_MSG_BUF_INIT);
        if (lgr_msg_buf == NULL)
        {
            perror("malloc() failed");
            return;
        }
        lgr_msg_buf_len = LGR_TEN_MSG_BUF_INIT;
    }

    va_start(ap, form_str);
    log_message_va(&lgr_msg_buf, &lgr_msg_buf_len,
                   level, entity_name, user_name, form_str, ap);
    va_end(ap);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&lgr_lock);
#endif
}


/* See description in logger_ten.h */
void
log_client_close(void)
{
    int res;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&lgr_lock);
#endif
    res = ipc_close_client(lgr_client);
    if (res != 0)
    {
        fprintf(stderr, "log_client_close(): ipc_close_client failed\n");
    }
    else
    {
        lgr_client = NULL;
    }
    free(lgr_msg_buf);
    lgr_msg_buf = NULL;
    lgr_msg_buf_len = 0;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&lgr_lock);
#endif
}


/* See description in logger_ten.h */
int
log_flush_ten(const char *ta_name)
{
    const char * const msg = "FLUSH";

    struct ipc_client *log_client;

    int     rc;
    char    ta_srv[strlen(LGR_SRV_FOR_TA_PREFIX) + 1 +
                   (ta_name != NULL) ? strlen(ta_name) : 0];
    char    answer[strlen(msg)];
    size_t  answer_len = sizeof(answer);

    if (ta_name == NULL)
    {
        ERROR("Invalid TA name");
        return EINVAL;
    }

    log_client = ipc_init_client("LOGGER_FLUSH_CLIENT");
    if (log_client == NULL)
    {
        ERROR("Failed to initialize log flush client");
        return errno;
    }

    strcpy(ta_srv, LGR_SRV_FOR_TA_PREFIX);
    strcat(ta_srv, ta_name);
    rc = ipc_send_message_with_answer(log_client, ta_srv, msg, strlen(msg),
                                      answer, &answer_len);
    if (rc != 0)
    {
        ipc_close_client(log_client);
        ERROR("Failed to flush log on TA '%s'", ta_name);
        return rc;
    }

    rc = ipc_close_client(log_client);
    if (rc != 0)
    {
        ERROR("Failed to close log flush client");
        return rc;
    }

    return 0;
}
