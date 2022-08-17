/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to send messages to Tester
 *
 * Implementation of test API to send messages to Tester.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Test-Tester Messages TAPI"

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
#include "te_format.h"
#include "ipc_client.h"
#include "tapi_test.h"
#include "tester_msg.h"


/** Maximum length of the test message */
#define TEST_MSG_LEN_MAX    1000


#ifdef HAVE_PTHREAD_H
/** Mutual exclusion execution lock */
static pthread_mutex_t  lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/**
 * Handle of Tester IPC client.
 *
 * @note It should be used under lock only.
 */
static struct ipc_client *ipcc = NULL;

/** Name of the Tester IPC server */
static const char *ipcs_name;

struct tester_test_message {
    tester_test_msg_hdr     hdr;
    char                    str[TEST_MSG_LEN_MAX];
};

/**
 * Storage for test message.
 *
 * @note It should be used under lock only.
 */
static struct tester_test_message msg;

/**
 * Parameters of the converter to single string.
 */
static struct te_log_out_params cm =
    { NULL, (uint8_t *)msg.str, sizeof(msg.str), 0 };

/**
 * atexit() callback to deallocate resources used by test message
 * interface.
 */
static void
te_test_tester_message_close(void)
{
    int res;

#ifdef HAVE_PTHREAD_H
    if ((res = pthread_mutex_trylock(&lock)) != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_trylock() failed: %s\n",
                __FUNCTION__, strerror(res));
        return;
    }
#endif
    res = ipc_close_client(ipcc);
    if (res != 0)
    {
        fprintf(stderr, "%s(): ipc_close_client() failed\n",
                __FUNCTION__);
    }
    else
    {
        ipcc = NULL;
    }
#ifdef HAVE_PTHREAD_H
    if ((res = pthread_mutex_unlock(&lock)) != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_unlock() failed: %s\n",
                __FUNCTION__, strerror(res));
    }
#endif
}

/**
 * Compose test message and send it to Tester.
 *
 * @param fmt           printf()-like format string with TE extensions
 *
 * @note The function uses @e te_test_id global variable.
 */
void
te_test_tester_message(te_test_msg_type type, const char *fmt, ...)
{
    te_errno    rc;
    va_list     ap;
    size_t      len = 0;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&lock);
#endif

    if (ipcc == NULL)
    {
        char    name[32];

        ipcs_name = getenv(TESTER_IPC_SERVER_ENV);
        if (ipcs_name == NULL)
        {
#ifdef HAVE_PTHREAD_H
            pthread_mutex_unlock(&lock);
#endif
            ERROR("%s(): Tester IPC server name is unknown",
                  __FUNCTION__);
            return;
        }

        if (snprintf(name, sizeof(name), "test_%u_%u", te_test_id,
                     (unsigned int)getpid()) >= (int)sizeof(name))
        {
            WARN("%s(): Tester IPC client name truncated",
                 __FUNCTION__);
        }
        rc = ipc_init_client(name, TESTER_IPC, &ipcc);
        if (rc != 0)
        {
#ifdef HAVE_PTHREAD_H
            pthread_mutex_unlock(&lock);
#endif
            ERROR("%s(): Failed to initialize IPC client: %r",
                  __FUNCTION__, rc);
            return;
        }
        assert(ipcc != NULL);
        atexit(te_test_tester_message_close);

        msg.hdr.id = te_test_id;
    }

    msg.hdr.type = type;
    cm.offset = 0;

    va_start(ap, fmt);
    rc = te_log_vprintf_old(&cm, fmt, ap);
    va_end(ap);

    if (rc == 0 || rc == TE_ESMALLBUF)
    {
        rc = ipc_send_message(ipcc, ipcs_name, &msg,
                              sizeof(msg.hdr) + cm.offset + 1);
        if (rc != 0)
        {
            ERROR("%s(): ipc_send_message() failed: %r",
                  __FUNCTION__, rc);
        }
        else if ((rc = ipc_receive_answer(ipcc, ipcs_name,
                                          NULL, &len)) != 0)
        {
            ERROR("%s(): ipc_receive_answer() failed: %r",
                  __FUNCTION__, rc);
        }
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&lock);
#endif
}
