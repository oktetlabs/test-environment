/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Verdicts listener implementation.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#include "tester_msg.h"
#include "tester_result.h"


struct tester_test_msg_listener {
    struct ipc_server      *ipcs;       /**< IPC server data */
    pthread_t               thread;     /**< Thread control data */
    tester_test_results    *results;    /**< List with tests which
                                             are in progress to store
                                             data from received messages */
    volatile te_bool        stop;       /**< Control to stop listener */
};


/**
 * Register received message.
 *
 * @param results       List with tests which are in progress
 * @param id            ID of the test which sent a message
 * @param type          Message type
 * @param msg           Message body (owned by the routine;
 *                      it is currently assumed to be null-terminated
 *                      string)
 */
static void
register_message(tester_test_results *results,
                 test_id id, te_test_msg_type type, char *msg)
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
              "in progress: %r - drop message for test %u",
              rc, (unsigned)id);
        free(msg);
    }
    else if (type == TE_TEST_MSG_VERDICT || type == TE_TEST_MSG_ARTIFACT)
    {
        for (test = SLIST_FIRST(&results->list);
             test != NULL && test->id != id;
             test = SLIST_NEXT(test, links));

        if (test == NULL)
        {
            ERROR("Message from the test %u which is not "
                  "running:\n%s", id, msg);
            free(msg);
        }
        else
        {
            te_test_verdict *p = TE_ALLOC(sizeof(*p));

            if (p != NULL)
            {
                p->str = msg;
                if (type == TE_TEST_MSG_VERDICT)
                    TAILQ_INSERT_TAIL(&test->result.verdicts, p, links);
                else
                    TAILQ_INSERT_TAIL(&test->result.artifacts, p, links);
            }
        }

        ret = pthread_mutex_unlock(&results->lock);
        if (ret != 0)
        {
            rc = TE_OS_RC(TE_TESTER, ret);
            ERROR("Failed to unlock list with status of tests which "
                  "are in progress: %r - deadlock is possible", rc);
        }
    }
    else
    {
        ERROR("Unknown test message type %u", (unsigned int)type);
        free(msg);
    }
}

/**
 * Receive and process IPC message from a test.
 *
 * @param ipcs          IPC server to receive message
 * @param results       List with tests which are in progress
 */
static void
receive_and_process_message(struct ipc_server *ipcs,
                            tester_test_results *results)
{
    te_errno                    rc;
    tester_test_msg_hdr         hdr;
    size_t                      len = sizeof(hdr);
    struct ipc_server_client   *client = NULL;
    char                       *str;

    rc = ipc_receive_message(ipcs, &hdr, &len, &client);
    if (rc == 0)
    {
        if (len == sizeof(hdr))
        {
            WARN("Empty message is received from the test "
                 "with ID %u - ignore", (unsigned)(hdr.id));
        }
        else
        {
            ERROR("Too small IPC message is received - ignore");
        }
    }
    else if (TE_RC_GET_ERROR(rc) != TE_ESMALLBUF)
    {
        ERROR("Failed to receive message: %r - "
              "try to continue", rc);
    }
    else
    {
        /* TODO - Correct allocation and usage below */
        str = malloc(len);
        if (str == NULL)
        {
            ERROR("Failed to allocate memory for message body - skip");
        }
        else
        {
            rc = ipc_receive_message(ipcs, str, &len, &client);
            if (rc != 0)
            {
                ERROR("Failed to receive message body: %r - skip", rc);
            }
            else
            {
                register_message(results, hdr.id, hdr.type, str);

                /*
                 * Send confirmation that test message
                 * has been processed
                 */
                rc = ipc_send_answer(ipcs, client, NULL, 0);
                if (rc != 0)
                {
                    ERROR("Failed to send test message processing "
                          "confirmation: %r - test %u will hang on",
                          rc, (unsigned)(hdr.id));
                }
            }
        }
    }
}

/**
 * Entry point of the test message listener thread.
 */
static void *
tester_test_msg_listener_thread(void *opaque)
{
    tester_test_msg_listener   *ctx = opaque;
    fd_set                      fds;
    int                         max_fd;
    struct timeval              timeout;
    int                         ret;
    te_errno                    rc;

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
        else if (ret > 0 && ipc_is_server_ready(ctx->ipcs, &fds, max_fd))
        {
            receive_and_process_message(ctx->ipcs, ctx->results);
        }
    }

    return NULL;
}

/* See the description in tester_result.h */
te_errno
tester_test_msg_listener_start(tester_test_msg_listener **ctx,
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
                       tester_test_msg_listener_thread, *ctx) != 0)
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
tester_test_msg_listener_stop(tester_test_msg_listener **ctx)
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
