/** @file
 * @brief Test API to register verdicts
 *
 * Implementation of test API to register verdicts at Tester.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id: logger_ten.c 30831 2006-08-22 06:48:27Z arybchik $
 */

#define TE_LGR_USER     "Verdict TAPI"

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
#include "tester_internal.h"


/** Maximum length of the test verdict */
#define TEST_VERDICT_LEN_MAX    500


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

/** 
 * Storage for test verdict message.
 *
 * @note It should be used under lock only.
 */
static struct tester_test_verdict {
    tester_test_verdict_hdr hdr;
    char                    str[TEST_VERDICT_LEN_MAX];
} msg;

/**
 * Parameters of the converter to single string.
 */
static struct te_log_out_params cm =
    { NULL, (uint8_t *)msg.str, sizeof(msg.str), 0 };


/**
 * atexit() callback to deallocate resources used by test verdicts
 * interface.
 */
static void
te_test_verdict_close(void)
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
 * Compose test verdict message and send it to Tester.
 *
 * @param fmt           printf()-like format string with TE extensions
 *
 * @note The function uses @e te_test_id global variable.
 */
void
te_test_verdict(const char *fmt, ...)
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
        atexit(te_test_verdict_close);

        msg.hdr.id = te_test_id;
    }

    cm.offset = 0;

    va_start(ap, fmt);
    rc = te_log_vprintf(&cm, fmt, ap);
    va_end(ap);

    if (rc == 0)
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
