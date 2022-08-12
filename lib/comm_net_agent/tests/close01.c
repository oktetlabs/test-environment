/** @file
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * Author: Pavel A. Bolokhov <Pavel.Bolokhov@oktetlabs.ru>
 *
 */

#include <stdio.h>
#include <pthread.h>

#include "synch.h"
#include "connection.h"
#include "workshop.h"
#include "debug.h"

#include "te_errno.h"
#include "comm_agent.h"

#define DUMMY_BUFFER_SIZE   1024

#define TEST_BUFFER_SANITY()

/* thread handle of the remote station thread */
pthread_t remote_thread;

/**
 * The main routine of the remote station thread.
 *
 * @return   n/a
 */
void *
remote_station_proc(void *arg)
{
    DEBUG("\t\t\tRemote Station Thread started\n");

    /* initialize the connection */
    remote_connection_init();

    /* synchronize at this point */
    remote_synch(10);

    /* the local station now closes the connection */
    close(remote_socket);
    remote_socket = -1;

    /* synchronize at this point */
    remote_synch(20);

    /* close the connection */
    remote_connection_close();
}

/**
 * The main routine of the local station thread.
 *
 * @return   n/a
 */
void *
local_station_proc(void *arg)
{
    int   rc;
    char  buffer[DUMMY_BUFFER_SIZE];

    DEBUG("Local Station Thread started\n");

    /* initialize the connection */
    local_connection_init();

    /* synchronize at this point */
    local_synch(10);

    /* now close the connection */
    if ((rc = rcf_comm_agent_close(&handle)) != 0)
    {
        char err_buf[BUFSIZ];

        strerror_r(rc, err_buf, sizeof(err_buf));
        fprintf(stderr, "local_station_proc: rcf_comm_agent_close() failed: "
                "%s\n", err_buf);
        exit(3);
    }

    /* prepare the fake buffer */
    memset(buffer, 'a', sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    /* check the rcf_comm_agent_reply() function */
    if (rcf_comm_agent_reply(handle, buffer, sizeof(buffer)) == 0)
    {
        fprintf(stderr, "ERROR: the call of rcf_comm_agent_reply() "
                "succeeded while it shouldn't have to\n");
        exit(3);
    }
    /* check the rcf_comm_agent_wait() function */
    if (rcf_comm_agent_reply(handle, buffer, sizeof(buffer)) == 0)
    {
        fprintf(stderr, "ERROR: the call of rcf_comm_agent_wait() "
                "succeeded while it shouldn't have to\n");
        exit(3);
    }

    /* close the connection */
    local_connection_close();
}

/** @page test_rcf_net_agent_close01 rcf_comm_agent_close() connection switch off check
 *
 * @descr The remote station requests a new connection, which is accepted
 * by the local station and then closed with the function
 * @b rcf_comm_agent_close(). The functions @b rcf_comm_agent_reply() and
 * @b rcf_comm_agent_wait() must fail when invoked with the closed connection
 * handler.
 *
 * @author Pavel A. Bolokhov <Pavel.Bolokhov@oktetlabs.ru>
 *
 * @return Test result
 * @retval 0            Test succeeded
 * @retval positive     Test failed
 *
 */
int
main(int argc, char *argv[])
{
    int rc;

    /* initialize thread synchronization features */
    barrier_init();

    /* check test precondition sanity */
    TEST_BUFFER_SANITY();

    /* launch the remote station thread */
    rc = pthread_create(&remote_thread, /* attr */ NULL,
                        remote_station_proc, /* arg */ NULL);
    if (rc != 0)
    {
        char err_buf[BUFSIZ];

        strerror_r(errno, err_buf, sizeof(err_buf));
        fprintf(stderr, "main: pthread_create() failed: %s\n", err_buf);
        exit(1);
    }

    /* launch the local station in the current thread */
    local_station_proc(NULL);

    /* indicate that the test has passed successfully */
    PRINT_TEST_OK();

    /* shutdown thread synchronization features */
    barrier_close();

    return 0;
}
