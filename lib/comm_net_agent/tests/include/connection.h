/** @file 
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side - Library
 * Connections API
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
#ifndef __TE_COMM_NET_AGENT_TESTS_LIB_CONNECTION_H__
#define __TE_COMM_NET_AGENT_TESTS_LIB_CONNECTION_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <semaphore.h>

#include "comm_agent.h"

/* Address which the remote station will knock at */
#define LOCAL_STATION_ADDRESS "193.125.193.44"

/* Port number at which the local station will be listening for connections */
extern char  *local_port_no;

/* Network Communication Library connection handle */
extern struct rcf_comm_connection *handle;

/* Communication socket used by the remote station */
extern int remote_socket;

/*
 * Usually 'output_buffer' is used by the remote station, and 'input_buffer'
 * is used by the local station. But in rare cases, when agent's transmitting
 * activity is tested, the things will be the other way around.
 */
extern char *input_buffer;         /* Input buffer for messages */
extern int   total_input_buffer_length;  /* Size of the buffer as allocated */
extern int   declared_input_buffer_length;  /* Size of the data on the buffer,
                                        * as in the customs declaration */

extern char *output_buffer;        /* Output buffer for messages */
extern int   total_output_buffer_length; /* Size of the buffer as allocated */
extern int   declared_output_buffer_length; /* Size of the data on the buffer,
                                         * as in the customs declaration */

/**
 * Initializes the agent side connection.
 *
 * @retval  0           Connection opened successfully
 * @retval  1           Connection initialization failed due to
 *                      the Network Communication Library API failure
 */
extern int local_connection_init(void);

/**
 * Shuts down the agent side connection.
 *
 * @return  n/a
 */
extern void local_connection_close(void);

/**
 * Initializes the remote station's side connection. Sets random seed.
 *
 * @retval  0           Connection opened successfully
 * @retval  1           Connection initialization failed
 */
extern int remote_connection_init(void);

/**
 * Shuts down the remote station's side connection
 *
 * @return      n/a
 */
extern void remote_connection_close(void);

/**
 * Allocates memory for the input buffer of size 'size' with declared size 
 * 'declared_size'.
 *
 * @param   size           total size of the buffer
 * @param   declared_size  declared size of the buffer
 *
 * @return  the pointer to the allocated area
 */
extern char *alloc_input_buffer(int size, int declared_size);

/**
 * Allocates memory for the output buffer of size 'size' with declared size 
 * 'declared_size'.
 *
 * @param   size           total size of the buffer
 * @param   declared_size  declared size of the buffer
 *
 * @return  the pointer to the allocated area
 */
extern char *alloc_output_buffer(int size, int declared_size);

/**
 * Transmits as many as 'initial_messages_no' initial commands to
 * the local station.
 *
 *
 * @retval 0     Transmission successful
 * @retval 1     Transmission failed due to a failure (or improper 
 *               behavior) at the local station.
 */
extern int remote_presend_random();

/**
 * Receives initial random messages from the remote station and
 * checks each message.
 *
 * @retval 0     All messages are ok
 * @retval 1     Receipt of messages failed due to malfunction of
 *               the Network Communication Library
 */
extern int local_receive_random();

#ifdef __cplusplus
}
#endif
#endif /* __TE_COMM_NET_AGENT_TESTS_LIB_CONNECTION_H__ */
