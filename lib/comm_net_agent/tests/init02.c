/** @file 
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * Author: Pavel A. Bolokhov <Pavel.Bolokhov@oktetlabs.ru>
 * 
 * @(#) $Id$
 */
#include "config.h"

#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include "synch.h"
#include "connection.h"
#include "workshop.h"
#include "debug.h"

#include "te_errno.h"
#include "comm_agent.h"

/* 
 * The delay (in SECONDS) that the remote station will hold in order to 
 * assure that the rcf_comm_agent_init() does not return before a connection
 * request has been sent
 */
#define REMOTE_CONNECT_DELAY       5 /* seconds */

#define TEST_BUFFER_SANITY()

/* thread handle of the remote station thread */
pthread_t remote_thread;

int       sleep_over = 0;  /* indicates that the remote station has
                           already passed its sleep period */

/**
 * The main routine of the remote station thread.
 *
 * @return   n/a
 */
void *
remote_station_proc(void *arg)
{
    struct sockaddr_in addr;
    int                i;

    DEBUG("\t\t\tRemote Station Thread started\n");
    /* create the socket */
    remote_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (remote_socket < 0)
    {
       char err_buf[BUFSIZ];

       strerror_r(errno, err_buf, sizeof(err_buf));
       fprintf(stderr, "\t\t\tremote_station_proc: "
              "can't create a socket: %s\n", err_buf);
       exit(1);
    }

    /* pre-connection synchronization */
    remote_synch(SYNCH_AGENT_CONNECTION_READY);

    /* now the local station is running rcf_comm_agent_init() */
    fprintf(stderr, "\t\t\tremote_station_proc: sleeping %d seconds...\n",
           REMOTE_CONNECT_DELAY);
#if (HAVE_NANOSLEEP == 1)
    {
       struct timespec t;
       
       t.tv_nsec = 0;
       t.tv_sec = REMOTE_CONNECT_DELAY;
       if (nanosleep(&t, NULL) < 0)
       {
           char err_buf[BUFSIZ];

           strerror_r(errno, err_buf, sizeof(err_buf));
           fprintf(stderr, "remote_station_proc: nanosleep() failed: %s\n", 
                  err_buf);
           exit(1);
       }
    }
#else
    sleep(REMOTE_CONNECT_DELAY);
#endif /* HAVE_NANOSLEEP == 1 */    

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(local_port_no));
    addr.sin_addr.s_addr = inet_addr(LOCAL_STATION_ADDRESS);
    if (connect(remote_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
       char err_buf[BUFSIZ];

       strerror_r(errno, err_buf, sizeof(err_buf));
       fprintf(stderr, "\t\t\tremote_station_proc: can't connect to "
              "the agent: %s\n", err_buf);
       return 1;
    }

    sleep_over = 1;    /* raise the flag to let the local station know
                       that we're thru with our *sleep() */

    /* synchronize at this point */
    remote_synch(10);

    /* close the connection */
    close(remote_socket);

#undef EXTRA_CONNECTS
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
    int   len;
    char *ptr;

    DEBUG("Local Station Thread started\n");

    /* initialize the connection */
    local_connection_init();

    if (!sleep_over)
    {
       fprintf(stderr, "ERROR: the call of rcf_comm_agent_init() returned "
              "before the remote station had sent a connection request\n");
       exit(3);
    }

    /* synchronize at this point */
    local_synch(10);

    /* close the connection */
    local_connection_close();
}

/** @page test_rcf_net_agent_init02 rcf_comm_agent_init() connection expectation
 *
 * @descr The local station calls @b rcf_comm_agent_init() and waits for some 
 * period of time. Then the remote station issues an incoming connection 
 * request. The function must not return before the request was issued.
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
