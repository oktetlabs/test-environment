/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Logger subsystem API - TEN side
 *
 * TEN side Logger library.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Logger TEN"

#include "te_config.h"

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
#include "te_errno.h"
#include "te_raw_log.h"
#include "ipc_client.h"
#include "logger_api.h"
#include "logger_int.h"
#include "logger_ten.h"

#define TE_LOGGER_TEN_INTERNALS
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
 * Logging output interface.
 *
 * @note It should be used under lgr_lock only.
 */
static te_log_msg_raw_data lgr_out;


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
        fprintf(stderr, "Failed to send message to IPC server '%s': %s\n",
                LGR_SRV_NAME, strerror(errno));
    }
}


/**
 * Compose log message and send it to TE Logger.
 *
 * This function complies with te_log_message_f prototype.
 */
/* See the description in logger_ten.h */
void
ten_log_message(const char *file, unsigned int line,
                te_log_ts_sec sec, te_log_ts_usec usec,
                unsigned int level,
                const char *entity, const char *user,
                const char *fmt, va_list ap)
{
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&lgr_lock);
#endif

    if (lgr_client == NULL)
    {
        int  rc;
        char name[32];

        if (snprintf(name, sizeof(name), "lgr_client_%u",
                     (unsigned int)getpid()) >= (int)sizeof(name))
        {
            fprintf(stderr, "ERROR: Logger client name truncated");
        }
        rc = ipc_init_client(name, LOGGER_IPC, &lgr_client);
        if (rc != 0)
        {
#ifdef HAVE_PTHREAD_H
            pthread_mutex_unlock(&lgr_lock);
#endif
            return;
        }
        assert(lgr_client != NULL);
        te_log_message_tx = log_message_ipc;
        atexit(log_client_close);

        /* Initialize backend */
        lgr_out.common = te_log_msg_out_raw;
        lgr_out.buf = lgr_out.end = NULL;
        lgr_out.args_max = 0;
        lgr_out.args = NULL;
    }

    log_message_va(&lgr_out, file, line, sec, usec, level, entity, user,
                   fmt, ap);

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
    if ((res = pthread_mutex_trylock(&lgr_lock)) != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_trylock() failed: %s\n",
                __FUNCTION__, strerror(res));
        return;
    }
#endif
    res = ipc_close_client(lgr_client);
    if (res != 0)
    {
        fprintf(stderr, "%s(): ipc_close_client() failed\n",
                __FUNCTION__);
    }
    else
    {
        lgr_client = NULL;
    }
    free(lgr_out.buf);
    lgr_out.buf = lgr_out.end = NULL;
    free(lgr_out.args);
    lgr_out.args = NULL;
    lgr_out.args_max = 0;
#ifdef HAVE_PTHREAD_H
    if ((res = pthread_mutex_unlock(&lgr_lock)) != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_unlock() failed: %s\n",
                __FUNCTION__, strerror(res));
    }
#endif
}


/* See description in logger_ten.h */
int
log_flush_ten(const char *ta_name)
{
    const char *const msg = LGR_FLUSH;

    struct ipc_client *log_client;

    int     rc;
    char    ta_srv[strlen(LGR_SRV_FOR_TA_PREFIX) + 1 +
                   ((ta_name != NULL) ? strlen(ta_name) : 0)];
    char    answer[strlen(msg)];
    size_t  answer_len = sizeof(answer);
    char    clnt_name[64];

    if (ta_name == NULL)
    {
        ERROR("Invalid TA name");
        return TE_EINVAL;
    }

    snprintf(clnt_name, sizeof(clnt_name), "LOGGER_FLUSH_%s", ta_name);

    rc = ipc_init_client(clnt_name, LOGGER_IPC, &log_client);
    if (rc != 0)
    {
        ERROR("Failed to initialize log flush client: %r", rc);
        return rc;
    }
    assert(log_client != NULL);

    sprintf(ta_srv, "%s%s", LGR_SRV_FOR_TA_PREFIX, ta_name);
    rc = ipc_send_message_with_answer(log_client, ta_srv, msg, strlen(msg),
                                      answer, &answer_len);
    if (rc != 0)
    {
        ipc_close_client(log_client);
        ERROR("Failed to flush log on TA '%s': rc=%r", ta_name, rc);
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
