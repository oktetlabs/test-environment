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

#include "synch.h"
#include "connection.h"
#include "workshop.h"
#include "debug.h"

#include "te_errno.h"
#include "comm_agent.h"

/* The maximum number of replies the local station will send */
#define MAX_NUMBER_OF_REPLIES   20

/* Each 'REPLY_OMIT_ATTACHMENT_FREQUENCY'-th reply will miss the attachment */
#define REPLY_OMIT_ATTACHMENT_FREQUENCY  4

/* The minimum number of replies */
#define MIN_NUMBER_OF_REPLIES   2

/* The minimum size of the random commands being generated */
#define MIN_RANDOM_REPLY_SIZE       30

/* The maximum size of the random commands being generated */
#define MAX_RANDOM_REPLY_SIZE       30000

/* The maximal size of the random attachment */
#define REPLY_MAX_RANDOM_ATTACHMENT_SIZE    30000

/* The default total size of input and output buffers */
#define TOTAL_BUFFER_LENGTH     (MAX_RANDOM_REPLY_SIZE + \
                                 REPLY_MAX_RANDOM_ATTACHMENT_SIZE)

/* The default declared size of the input buffer */
#define DECLARED_BUFFER_LENGTH  TOTAL_BUFFER_LENGTH

#define TEST_BUFFER_SANITY()

/* thread handle of the remote station thread */
pthread_t remote_thread;

/* the actual number of replies the local station will send */
int       num_replies = 0;

/**
 * The main routine of the remote station thread.
 *
 * @return   n/a
 */
void *
remote_station_proc(void *arg)
{
    int   i;

    DEBUG("\t\t\tRemote Station Thread started\n");

    /* initialize the connection */
    remote_connection_init();

    alloc_input_buffer(TOTAL_BUFFER_LENGTH, TOTAL_BUFFER_LENGTH);

    /* synchronize at this point */
    remote_synch(10);

    /* now receive all the replies and check them on the fly */
    for (i = 0; i < num_replies; i++)
    {
	int   rc;
	char *buf_ptr = input_buffer;

	/* read in the next message */
	do {
	    rc = read(remote_socket, buf_ptr, 
		      TOTAL_BUFFER_LENGTH - (buf_ptr - input_buffer));
	    if (rc < 0)
	    {
		char err_buf[BUFSIZ];

		strerror_r(errno, err_buf, sizeof(err_buf));
		fprintf(stderr, 
			"\t\t\remote_station_proc: read() failed: "
			" %s\n", err_buf);
		exit(1);
	    }
	    buf_ptr += rc;

	    /* 
	     * at this point it is safe to access 
	     * 'declared_output_buffer_length' because the local station
	     * is hanging on the semaphore 
	     */
	    if (buf_ptr - input_buffer >= declared_output_buffer_length)
	    {
		break;
	    }
	} while (rc > 0);
	declared_input_buffer_length = buf_ptr - input_buffer;

	/* check the buffers */
	VERIFY_BUFFERS();
	DEBUG("\t\t\tremote_station_proc: reply number %d(%d) ok\n", 
	      i + 1, num_replies);

	/* 
	 * Post the semaphore to let the local station proceed with the
	 * next message 
	 */
	if (sem_post(&random_messages_semaphore) < 0)
	{
	    char err_buf[BUFSIZ];
	    
	    strerror_r(errno, err_buf, sizeof(err_buf));
	    fprintf(stderr, "\t\t\tremote_presend_random: "
		    "can't sem_post(): %s\n", err_buf);
	    exit(1);
	}
    }  /* for all replies */

    /* synchronize after the transfer */
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
    int   i;
    int   rc;
    int   len;
    char *ptr;

    DEBUG("Local Station Thread started\n");

    /* initialize the connection */
    local_connection_init();

    /* generate the number of replies */
    num_replies = MIN_NUMBER_OF_REPLIES + 
	(int)(random() % (MAX_NUMBER_OF_REPLIES - MIN_NUMBER_OF_REPLIES + 1));

    /* 
     * Synchronize at this point -- the remote station will now read 
     * the number of replies 
     */
    local_synch(10);

    alloc_output_buffer(TOTAL_BUFFER_LENGTH, TOTAL_BUFFER_LENGTH);

    for (i = 0; i < num_replies; i++)
    {
	int   do_generate_attachment = 0;
	char *buf_ptr;
	int   reply_size, attach_size;

	/* 
	 * Now we generate the size of the reply and the size
	 * of the attachment 
	 */
	reply_size = MIN_RANDOM_REPLY_SIZE + 
	    (int)(random() % (MAX_RANDOM_REPLY_SIZE - 
			      MIN_RANDOM_REPLY_SIZE + 1));
	
	/* Now throw the dice if we should omit the attachment */
	do_generate_attachment = 
	    (random() % REPLY_OMIT_ATTACHMENT_FREQUENCY != 0);
	
	if (do_generate_attachment)
	{
	    /* Generate the size of the attachment */
	    attach_size = 
		(int)(random() % REPLY_MAX_RANDOM_ATTACHMENT_SIZE) + 1;
	}
	else
	{
	    attach_size = 0;
	}

	/* ok, now generate the message itself */
	generate_command(output_buffer, reply_size, attach_size);
#if 0
	DEBUG("local_station_proc: generated a command of size %d, "
	      "attachment = %d\n", reply_size, attach_size);
	DEBUG("The command is:\n");
	output_buffer[reply_size + attach_size - 1] = '\0';
	DEBUG("%s\n", random_output_buffer);
#endif

	declared_output_buffer_length = reply_size + attach_size;

	/* now send the message */
	DEBUG("local_station_proc: sending a reply %d bytes long\n", 
	      declared_output_buffer_length);
	rc = rcf_comm_agent_reply(handle, output_buffer, 
				  (unsigned int) 
				  declared_output_buffer_length);
	if (rc != 0)
	{
	    char err_buf[BUFSIZ];

	    strerror_r(rc, err_buf, sizeof(err_buf));
	    fprintf(stderr, 
		    "local_station_proc: rcf_comm_agent_reply() failed:"
		    " %s\n", err_buf);
	    exit(1);
	}

	/* now wait while the remote station reads and checks the message */
	if (sem_wait(&random_messages_semaphore) < 0)
	{
	    char err_buf[BUFSIZ];

	    strerror_r(errno, err_buf, sizeof(err_buf));
	    fprintf(stderr, 
		    "\t\t\tremote_presend_random: can't sem_wait(): %s\n",
		    err_buf);
	    exit(1);
	}

	if (!CHECK_PROCEED())
	{
	    return;  /* consistency check failed on the other side */
	}
    } /* for all random replies */

    /* synchronize at this point */
    local_synch(20);

    /* close the connection */
    local_connection_close();
}

/** @page test_rcf_net_agent_reply01 rcf_comm_agent_reply() buffer transfer check
 *
 * @descr The function @b rcf_comm_agent_reply() must be invoked with 
 * several buffers of different sizes. The remote station must convince
 * the messages arrived consistently and uncorrupted.
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
