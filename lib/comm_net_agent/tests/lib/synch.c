/** @file 
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side - Library
 * Thread Synchronization
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
#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif /* STDC_HEADERS */
#include <errno.h>

#include "synch.h"
#include "debug.h"

#if (HAVE_PTHREAD_BARRIER_T != 1)
#include <semaphore.h>
#endif /* HAVE_PTHREAD_BARRIER_T != 1 */

#if (HAVE_PTHREAD_BARRIER_T == 1)
static pthread_barrier_t barrier;
#else /* HAVE_PTHREAD_BARRIER_T */
static sem_t barrier; /* done as a semaphore */
#endif /* HAVE_PTHREAD_BARRIER_T != 1 */

/* 
 * This variable indicates that no error has occured so far. If it
 * is false, threads MUST NOT continue carrying out their execution.
 *
 * This variable should only be accessed via CHECK_PROCEED() and 
 * SET_PROCEED() macros.
 */
int proceed = 1;

int local_synch_point;  /* point at which the local station is waiting
                        for synchronization */
int remote_synch_point; /* point at which the remote station is waiting
                        for synchronization */

/* 
 * This semaphore allows the local station to access the initial_messages_no
 * variable
 */
sem_t random_number_semaphore;

/*
 * This semaphore allows both threads to synchronize after sending/receiving
 * each initial random message. The remote station waits until the local
 * station has checked the recently sent message, before sending a new
 * message
 */
sem_t random_messages_semaphore;

/**
 * Synchronizes the local station with the remote station at the 
 * synchronization point referenced by 'synch_point'.
 *
 * This function is supposed to be called by the remote station.
 *
 * @param synch_point   point of synchronization; both threads
 *                      will have their synchronization points
 *                      equal to 'synch_point' after the synchronization.
 *
 * @return n/a
 * 
 * @se If the remote station has reached a point further then the one
 *     being requested, the function will abort the execution.
 */
void
local_synch(int synch_point)
{
    DEBUG("synchronizing local station at point %d\n", synch_point);
    if (!CHECK_PROCEED())
    {
       fprintf(stderr, "local_synch: CHECK_PROCEED() failed\n");
       exit(3);
    }
    synch(synch_point, &local_synch_point, remote_synch_point);
}

/**
 * Synchronizes the remote station with the local station at the 
 * synchronization point referenced by 'synch_point'.
 *
 * This function is supposed to be called by the remote station.
 *
 * @param synch_point   point of synchronization; both threads
 *                      will have their synchronization points
 *                      equal to 'synch_point' after the synchronization.
 *
 * @return n/a
 * 
 * @se If the local station has reached a point further then the one
 *     being requested, the function will abort the execution.
 */
void
remote_synch(int synch_point)
{
    DEBUG("\t\t\tsynchronizing remote station at point %d\n", synch_point);
    if (!CHECK_PROCEED())
    {
       fprintf(stderr, "remote_synch: CHECK_PROCEED() failed\n");
       exit(3);
       return;
    }
    synch(synch_point, &remote_synch_point, local_synch_point);
}


/**
 * Synchronizes the current thread (which was at the point numbered
 * by 'curr_point') with the other thread.
 *
 * @param synch_point   point of synchronization; both threads
 *                      will have their synchronization points
 *                      equal to 'synch_point' after the synchronization.
 * @param curr_point    current point of the current thread OUT
 * @param other_side_point
 *                      current point of the other thread
 *
 * @return n/a
 * 
 * @se If the other station has reached a point further then the one
 *     being requested, the function will abort the execution.
 */
void
synch(int synch_point, int *curr_point, int other_side_point)
{
    int rc;

    if (other_side_point > synch_point)
    {
       fprintf(stderr, "synch: other station has gone too far "
              "(%d > %d)\n", other_side_point, synch_point);
       exit(2);
    }

    /* set current station synch point */
    *curr_point = synch_point;

    /* we have to wait for the other station */
#if (HAVE_PTHREAD_BARRIER_T == 1)
    rc = pthread_barrier_wait(&barrier);
    if (rc != 0 && rc != BARRIER_SERIAL_THREAD)
    {
       char err_buf[BUFSIZ];

       strerror_r(rc, err_buf, sizeof(err_buf));
       fprintf(stderr, "synch: pthread_barrier_wait() failed: %s\n", err_buf);
       exit(1);
    }
#else /* HAVE_PTHREAD_BARRIER_T */
    /* now check the other station synchpoint */
    if (other_side_point == synch_point)
    {
       /* we are being waited for */
       rc = sem_post(&barrier);
       if (rc < 0)
       {
           char err_buf[BUFSIZ];

           strerror_r(errno, err_buf, sizeof(err_buf));
           fprintf(stderr, "synch: sem_post() failed: %s\n", err_buf);
           exit(1);
       }
    }
    else
    {
       /* we have to wait */
       rc = sem_wait(&barrier);
       if (rc < 0)
       {
           char err_buf[BUFSIZ];

           strerror_r(errno, err_buf, sizeof(err_buf));
           fprintf(stderr, "synch: sem_wait() failed: %s\n", err_buf);
           exit(1);
       }
    }
#endif /* HAVE_PTHREAD_BARRIER_T != 1 */    
}

/**
 * Initialize the barrier. The function must be called at the 
 * test startup.
 *
 * @return n/a
 * 
 * @se If the function fails to initialize the barrier, it will
 *     abort the execution.
 */
void
barrier_init(void)
{
    int rc;

#if (HAVE_PTHREAD_BARRIER_T == 1)
    rc = pthread_barrier_init(&barrier, 
                           NULL,    /* default attributes */
                           2);      /* two threads must be synchronized */
    if (rc != EOK)
    {
       char err_buf[BUFSIZ];

       strerror_r(rc, err_buf, sizeof(err_buf));
       fprintf(stderr, "pthread_barrier_init() failed: %s\n", err_buf);
       exit(1);
    }
#else
    rc = sem_init(&barrier, 
                0,          /* no sharing between processes */
                0);         /* initial count value must be zero */
    if (rc < 0)
    {
       char err_buf[BUFSIZ];

       strerror_r(errno, err_buf, sizeof(err_buf));
       fprintf(stderr, "sem_init() failed: %s\n", err_buf);
       exit(1);
    }
#endif /* HAVE_PTHREAD_BARRIER_T != 1 */

    /* initialize the starting point number */
    local_synch_point = 0;
    remote_synch_point = 0;

    /* 
     * Initialize the semaphores of the random number and 
     * of the random messages 
     */
    if (sem_init(&random_number_semaphore, 0, 0) < 0)
    {
       char err_buf[BUFSIZ];

       strerror_r(errno, err_buf, sizeof(err_buf));
       fprintf(stderr, "\t\t\tbarrier_init: can't sem_init(): %s\n",
              err_buf);
       exit(1);
    }
    if (sem_init(&random_messages_semaphore, 0, 0) < 0)
    {
       char err_buf[BUFSIZ];

       strerror_r(errno, err_buf, sizeof(err_buf));
       fprintf(stderr, "\t\t\tbarrier_init: can't sem_init(): %s\n",
              err_buf);
       exit(1);
    }
}

/**
 * Shutdown the barrier. The function must be called after 
 * test execution (and so no error checking is performed).
 *
 * @return n/a
 * 
 */
void
barrier_close(void)
{
#if (HAVE_PTHREAD_BARRIER_T == 1)
    pthread_barrier_destroy(&barrier);
#else
    sem_destroy(&barrier);
#endif /* HAVE_PTHREAD_BARRIER_T != 1 */
}
