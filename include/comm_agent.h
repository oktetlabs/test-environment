/** @file
 * @brief Communication Library - Test Agent side
 *
 * Definition of routines provided for library user
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * Author: Andrey Ivanov <andron@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_COMM_AGENT_H__
#define __TE_COMM_AGENT_H__


/** This structure is used to store some context for each connection. */
struct rcf_comm_connection;


/**
 * Wait for incoming connection from the Test Engine side of the 
 * Communication library.
 *
 * @param config_str    Configuration string, content depends on the 
 *                      communication type (network, serial, etc)
 * @param p_rcc         Pointer to to pointer the rcf_comm_connection 
 *                      structure to be filled, used as handler.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval other value  errno.
 */
extern int rcf_comm_agent_init(const char *config_str, 
                               struct rcf_comm_connection **p_rcc);



/** 
 * Wait for command from the Test Engine via Network Communication library.
 *
 * @param rcc           Handler received from rcf_comm_agent_init
 * @param buffer        Buffer for data
 * @param pbytes        Pointer to variable with:
 *                      on entry - size of the buffer;
 *                      on return:
 *                      number of bytes really written if 0 returned (success);
 *                      unchanged if ETESMALLBUF returned;
 *                      number of bytes in the message (with attachment) 
 *                      if ETEPENDING returned. (Note: If the function
 *                      called a number of times to receive one big message,
 *                      a full number of bytes will be returned on first call.
 *                      On the next calls number of bytes in the message
 *                      minus number of bytes previously read by this function
 *                      will be returned.);
 *                      undefined if other errno returned.
 * @param pba           Address of the pointer that will hold on return 
 *                      address of the first byte of attachment (or NULL if 
 *                      no attachment attached to the command). If this 
 *                      function called more than once (to receive big
 *                      attachment) this pointer will be not touched.
 *
 * @return Status code.
 * @retval 0            Success (message received and written to the buffer).
 * @retval ETESMALLBUF  Buffer is too small for the message. The part of the 
 *                      message is written to the buffer. Other part(s) of 
 *                      the message can be read by the next calls to the 
 *                      rcf_comm_agent_wait. The ETSMALLBUF will be 
 *                      returned until last part of the message will be read.
 * @retval ETEPENDING   Attachment is too big to fit into the buffer. Part 
 *                      of the message with attachment is written to the
 *                      buffer. Other part(s) can be read by the next calls
 *                      to the rcf_comm_engine_receive. The ETEPENDING
 *                      will be returned until last part of the message will
 *                      be read.
 * @retval other value  errno.
 */
extern int rcf_comm_agent_wait(struct rcf_comm_connection *rcc, 
                               char *buffer, size_t *pbytes, char **pba);


/**
 * Send reply to the Test Engine side of Network Communication library.
 *
 * @param rcc           Handler received from rcf_comm_agent_connect.
 * @param p_buffer      Buffer with reply.
 * @param length        Length of the data to send.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval other value  errno.
 */
extern int rcf_comm_agent_reply(struct rcf_comm_connection *rcc,
                                const void *p_buffer, size_t length);

/** 
 * Close connection.
 *
 * @param p_rcc   Pointer to variable with handler received from 
 *                rcf_comm_agent_connect
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval other value  errno.
 */
extern int rcf_comm_agent_close(struct rcf_comm_connection **p_rcc);

#endif /* !__TE_COMM_AGENT_H__ */
