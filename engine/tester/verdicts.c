/** @file
 * @brief Tester Subsystem
 *
 * Verdicts listener implementation.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

/** Logging user name to be used here */
#define TE_LGR_USER     "Verdicts"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_errno.h"
#include "te_alloc.h"
#include "ipc_server.h"
#include "tester_internal.h"
#include "tester_result.h"


struct tester_verdicts_listener {
    struct ipc_server      *ipcs;       /**< IPC server data */
    pthread_t               thread;     /**< Thread control data */
    tester_test_results    *results;    /**< List with tests which
                                             are in progress to store
                                             received verdicts */
    volatile te_bool        stop;       /**< Control to stop listener */
};


/**
 * Register received verdict message.
 *
 * @param results       List with tests which are in progress
 * @param id            ID of the test which generates verdict
 * @param verdict       Verdict (owned by the routine)
 */
static void
register_verdict(tester_test_results *results,
                 test_id id, char *verdict)
{
    int                 ret;
    te_errno            rc;
    tester_test_result *test;

    assert(results != NULL);
    ret = pthread_mutex_lock(&results->lock);
    if (ret != 0)
    {
        rc = TE_OS_RC(TE_TESTER, ret);
        ERROR("Failed to lock list with status of tests which are "
              "in progress: %r - drop verdict for test test %u",
              rc, (unsigned)id);
        free(verdict);
    }
    else
    {
        for (test = results->list.lh_first;
             test != NULL && test->id != id;
             test = test->links.le_next);
    
        if (test == NULL)
        {
            ERROR("Verdict message from the test which is not running!");
            free(verdict);
        }
        else
        {
            /* Add verdict to the list of verdicts */
            RING("Verdict: %s", verdict);
            free(verdict);
            /* TODO */
        }

        ret = pthread_mutex_unlock(&results->lock);
        if (ret != 0)
        {
            rc = TE_OS_RC(TE_TESTER, ret);
            ERROR("Failed to unlock list with status of tests which "
                  "are in progress: %r - deadlock is possible", rc);
        }
    }
}

/**
 * Receive and process IPC message with test verdict.
 *
 * @param ipcs          IPC server to receive message
 * @param results       List with tests which are in progress
 */
static void
receive_and_process_message(struct ipc_server *ipcs,
                            tester_test_results *results)
{
    te_errno                    rc;
    tester_test_verdict_hdr     hdr;
    size_t                      len = sizeof(hdr);
    struct ipc_server_client   *client = NULL;
    char                       *str;

    rc = ipc_receive_message(ipcs, &hdr, &len, &client);
    if (rc == 0)
    {
        if (len == sizeof(hdr))
        {
            WARN("Empty verdict message is received from the test "
                 "with ID %u - ignore", (unsigned)(hdr.id));
        }
        else
        {
            ERROR("Too small IPC message is received - ignore");
        }
    }
    else if (TE_RC_GET_ERROR(rc) != TE_ESMALLBUF)
    {
        ERROR("Failed to receive verdict message: %r - "
              "try to continue", rc);
    }
    else
    {
        /* TODO - Correct allocation and usage below */
        str = malloc(len);
        if (str == NULL)
        {
            ERROR("Failed to allocate memory for verdict string - skip");
        }
        else
        {
            rc = ipc_receive_message(ipcs, str, &len, &client);
            if (rc != 0)
            {
                ERROR("Failed to receive verdict message "
                      "string: %r - skip", rc);
            }
            else
            {
                register_verdict(results, hdr.id, str);

                /* 
                 * Send confirmation that verdict message
                 * has been processed
                 */
                rc = ipc_send_answer(ipcs, client, NULL, 0);
                if (rc != 0)
                {
                    ERROR("Failed to send verdict message processing "
                          "confirmation: %r - test %u will hang on",
                          rc, (unsigned)(hdr.id));
                }
            }
        }
    }
}

/**
 * Entry point of the verdicts listener thread.
 */
static void *
tester_verdicts_listener_thread(void *opaque)
{
    tester_verdicts_listener   *ctx = opaque;
    fd_set                      fds;
    int                         max_fd;
    struct timeval              timeout;
    int                         ret;
    te_errno                    rc;

    RING("EEEE");

    while (!ctx->stop)
    {
        FD_ZERO(&fds);
        max_fd = ipc_get_server_fds(ctx->ipcs, &fds);
        
        timeout.tv_sec = 0;
        timeout.tv_usec = TE_MS2US(100);

        ret = select(max_fd + 1, &fds, NULL, NULL, &timeout);
        if (ret < 0)
        {
            rc = TE_OS_RC(TE_TESTER, errno);
            ERROR("%s(): select() failed unexpectedly: %r",
                  __FUNCTION__, rc);
        }
        else if (ret > 0)
        {
            if (ipc_is_server_ready(ctx->ipcs, &fds, max_fd))
            {
    RING("RECEIVED");
                receive_and_process_message(ctx->ipcs, ctx->results);
            }
        }
    }

    return NULL;
}

/* See the description in tester_result.h */
te_errno
tester_verdicts_listener_start(tester_verdicts_listener **ctx,
                               tester_test_results       *results)
{
    te_errno  rc;
    char     *name = getenv(TESTER_IPC_SERVER_ENV);
    char      name_buf[32];

    if (name == NULL)
    {
        if (snprintf(name_buf, sizeof(name_buf), "TE_TESTER_%u",
                     (unsigned)getpid()) >= (int)sizeof(name_buf))
        {
            ERROR("%s(): Too short buffer for Tester IPC server name",
                  __FUNCTION__);
            return TE_RC(TE_TESTER, TE_ESMALLBUF);
        }
        name = name_buf;
        if (setenv(TESTER_IPC_SERVER_ENV, name, 0) != 0)
        {
            ERROR("%s(): setenv() failed", __FUNCTION__);
            return TE_RC(TE_TESTER, TE_ENOMEM);
        }
    }

    assert(ctx != NULL);
    *ctx = TE_ALLOC(sizeof(**ctx));
    if (*ctx == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);
    (*ctx)->results = results;

    rc = ipc_init();
    if (rc != 0)
    {
        ERROR("%s(): ipc_init() failed: %r", __FUNCTION__, rc);
        free(*ctx);
        return rc;
    }

    rc = ipc_register_server(name, TESTER_IPC, &((*ctx)->ipcs));
    if (rc != 0)
    {
        ERROR("%s(): Failed to register IPC server %s: %r",
              __FUNCTION__, name, rc);
        (void)ipc_kill();
        free(*ctx);
        return rc;
    }
    INFO("IPC server '%s' registered", name);

    if (pthread_create(&(*ctx)->thread, NULL,
                       tester_verdicts_listener_thread, *ctx) != 0)
    {
        rc = TE_OS_RC(TE_TESTER, errno);
        (void)ipc_close_server((*ctx)->ipcs);
        (void)ipc_kill();
        free(*ctx);
        return rc;
    }

    return 0;
}

/* See the description in tester_result.h */
te_errno
tester_verdicts_listener_stop(tester_verdicts_listener **ctx)
{
    te_errno    rc;
    int         ret;

    assert(ctx != NULL);
    assert(*ctx != NULL);

    (*ctx)->stop = TRUE;
    ret = pthread_join((*ctx)->thread, NULL);
    if (ret != 0)
    {
        rc = TE_OS_RC(TE_TESTER, ret);
        ERROR("%s(): pthread_join() failed: %r", __FUNCTION__, rc);
        /* 
         * Do not close IPC server and deallocate resources 
         * since it can cause crash
         */ 
        return rc;
    }

    rc = ipc_close_server((*ctx)->ipcs);
    if (rc != 0)
    {
        ERROR("%s(): ipc_close_server() failed: %r", __FUNCTION__, rc);
        /* Do not call ipc_kill() to avoid unexpected crashes */
    }
    else
    {
        rc = ipc_kill();
        if (rc != 0)
        {
            ERROR("%s(): ipc_kill() failed: %r", __FUNCTION__, rc);
        }
    }

    free(*ctx);
    *ctx = NULL;

    return rc;
}
