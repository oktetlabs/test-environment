/** @file
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
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

    /*
     * Now the local station does its actions
     */

    /* synchronize at this point */
    remote_synch(20);
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
    struct rcf_comm_connection
         *my_handle;

    DEBUG("Local Station Thread started\n");


    /* now call the rcf_comm_agent_init() function two times */
    if ((rc = rcf_comm_agent_init(NULL, &my_handle)) == 0)
    {
       fprintf(stderr, "ERROR: the call of rcf_comm_agent_init(NULL, p_rcc) "
              "succeeded while it shouldn't have to\n");
       exit(3);
    }

    /*
     * The second call is disabled because it will case a coredump
     * in the current implementation of RCF agent library
     */
    strcpy(buffer, local_port_no);
    if ((rc = rcf_comm_agent_init(buffer, NULL)) == 0)
    {
       fprintf(stderr, "ERROR: the call of rcf_comm_agent_init(port, NULL) "
              "succeeded while it shouldn't have to\n");
       exit(3);
    }

    /* synchronize at this point */
    local_synch(20);
}


/** @page test_rcf_net_agent_sanity_init01 rcf_comm_agent_init() sanity check on NULL parameters
 *
 * @descr The function @b rcf_comm_agent_init() is invoked one time with
 * the parameter @b config_str set to NULL, and the second time with the
 * parameter @b p_rcc set to NULL. Both times the function must return
 * a bad parameter failure.
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
