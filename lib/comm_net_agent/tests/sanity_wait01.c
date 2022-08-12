/** @file
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
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

    /*
     * Now the local station does its actions
     */

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
    int    rc;
    char   buffer[BUFSIZ];
    int    len;

    DEBUG("Local Station Thread started\n");

    /* initialize the connection */
    local_connection_init();

    /* synchronize at this point */
    local_synch(10);

    /* now call the rcf_comm_agent_wait() function three times */
    len = sizeof(buffer);
    if ((rc = rcf_comm_agent_wait(NULL, buffer, &len, NULL)) == 0 ||
       TE_RC_GET_ERROR(rc) == TE_ESMALLBUF ||
       TE_RC_GET_ERROR(rc) == TE_EPENDING)
    {
       fprintf(stderr, "ERROR: the call of "
              "rcf_comm_agent_wait(NULL, buffer, len, valid) "
              "succeeded while it shouldn't have to\n");
       exit(3);
    }

    len = sizeof(buffer);
    if ((rc = rcf_comm_agent_wait(handle, NULL, &len, NULL)) == 0 ||
       TE_RC_GET_ERROR(rc) == TE_ESMALLBUF ||
       TE_RC_GET_ERROR(rc) == TE_EPENDING)
    {
       fprintf(stderr, "ERROR: the call of "
              "rcf_comm_agent_wait(handle, NULL, len, valid) "
              "succeeded while it shouldn't have to\n");
       exit(3);
    }

    len = sizeof(buffer);
    if ((rc = rcf_comm_agent_wait(handle, buffer, NULL, NULL)) == 0 ||
       TE_RC_GET_ERROR(rc) == TE_ESMALLBUF ||
       TE_RC_GET_ERROR(rc) == TE_EPENDING)
    {
       fprintf(stderr, "ERROR: the call of "
              "rcf_comm_agent_wait(handle, buffer, NULL, valid) "
              "succeeded while it shouldn't have to\n");
       exit(3);
    }

    /* synchronize at this point */
    local_synch(20);

    /* close the connection */
    local_connection_close();
}

/** @page test_rcf_net_agent_sanity_wait01 rcf_comm_agent_wait() sanity check on NULL parameters
 *
 * @descr A connection between the local and the remote station is established.
 * Then the function @b rcf_comm_agent_wait() is called three times, setting
 * one of its first three parameters to NULL and the others to valid values,
 * so that all three first parameters will have held the value NULL.
 * The function must return a bad parameter failure each time.
 *
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
