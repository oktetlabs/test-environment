/** @file 
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side - Library
 * Thread Synchronization API
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
#ifndef __TE_COMM_NET_AGENT_TESTS_LIB_SYNCH_H__
#define __TE_COMM_NET_AGENT_TESTS_LIB_SYNCH_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#include <semaphore.h>

/* 
 * Checks if execution of the current thread can be resumed.
 */
#define CHECK_PROCEED() (proceed)

/*
 * Sets the value of the proceed variable to 0. 
 * Any thread which has discovered that the variable has the value of 
 * zero, must interrupt its execution.
 */
#define FAIL_PROCEED()                            \
do {                                          \
    proceed = 0;                            \
} while (0);

/* 
 * This variable indicates that no error has occured so far. If it
 * is false, threads must not continue carrying out their execution
 *
 * This variable should only be accessed via CHECK_PROCEED() and 
 * SET_PROCEED() macros.
 */
extern int proceed;

/*
 * The point at which the local station is ready to receive connections
 */
#define SYNCH_AGENT_CONNECTION_READY 5

extern int local_synch_point;  /* point at which the local station is waiting
                              for synchronization */
extern int remote_synch_point; /* point at which the remote station is waiting
                              for synchronization */

/* 
 * This semaphore allows the local station to access the initial_messages_no
 * variable
 */
extern sem_t random_number_semaphore;

/*
 * This semaphore allows both threads to synchronize after sending/receiving
 * each initial random message. The remote station waits until the local
 * station has checked the recently sent message, before sending a new
 * message
 */
extern sem_t random_messages_semaphore;


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
extern void local_synch(int synch_point);

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
extern void remote_synch(int synch_point);

/**
 * Initialize the barrier. The function must be called at the 
 * test startup.
 *
 * @return n/a
 * 
 * @se If the function fails to initialize the barrier, it will
 *     abort the execution.
 */
extern void barrier_init(void);

/**
 * Shutdown the barrier. The function must be called after 
 * test execution (and so no error checking is performed).
 *
 * @return n/a
 * 
 */
extern void barrier_close(void);

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
extern void synch(int synch_point, int *curr_point, int other_side_point);

#ifdef __cplusplus
}
#endif
#endif /* __TE_COMM_NET_AGENT_TESTS_LIB_SYNCH_H__ */
