/** @file
 * @brief RCF Network Communication library
 *
 * C interface for network communication library from Test Engine side.
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
 * Author: Andrey G. Ivanov <andron@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_COMM_NET_ENGINE_H__
#define __TE_COMM_NET_ENGINE_H__

#include <sys/types.h>

#include "te_defs.h"


/** This structure is used to store some context for each connection. */
struct rcf_net_connection;


/** 
 * Connects to the Test Agent side of Network Communication library.
 *
 * @param  addr         - network address of the test agent
 * @param  port         - port of the test agent
 * @param  p_rnc        - pointer to to pointer to the rcf_net_connection 
 *                        structure to be filled, used as handler
 * @param  p_select_set - pointer to the fdset for reading to be modified
 *
 * @return Status code.
 * @retval 0            - success
 * @retval other value  - errno
 */
extern int rcf_net_engine_connect(const char *addr, const char *port, 
                                  struct rcf_net_connection **p_rnc, 
                                  fd_set *p_select_set);


/**
 * Transmits data to the Test Agent via Network Communication library.
 *
 * @param  rnc          - Handler received from rcf_net_engine_connect.
 * @param  data         - Data to be transmitted.
 * @param  length       - Length of the data.
 *
 * @return Status code.
 * @retval 0            - success
 * @retval other value  - errno
 */
extern int rcf_net_engine_transmit(struct rcf_net_connection *rnc,
                                   const char *data, size_t length);


/**
 * Check, if some data are pending on the test agent connection. This
 * routine never blocks.
 *
 * @param rnc       - Handler received from rcf_net_engine_connect.
 *
 * @return Status code.
 * @retval TRUE     - Data are pending.
 * @retval FALSE    - No data are pending.
 */
extern te_bool rcf_net_engine_is_ready(struct rcf_net_connection *rnc);


/**
 * Receive data from the Test Agent via Network Communication library.
 *
 * @param rnc       - Handler received from rcf_net_engine_connect
 * @param buffer    - Buffer for data
 * @param pbytes    - Pointer to variable with:
 *                      on entry - size of the buffer;
 *                      on return:
 *                          number of bytes really written if 0
 *                          returned (success);
 *                          unchanged if ETESMALLBUF returned;
 *                          number of bytes in the message (with
 *                          attachment) 
 *                      if ETEPENDING returned. (Note: If the
 *                      function called a number of times to receive
 *                      one big message, a full number of bytes will
 *                      be returned on first call.
 *                      On the next calls number of bytes in the
 *                      message minus number of bytes previously
 *                      read by this function will be returned);
 *                      undefined if other errno returned.
 * @param pba       - Address of the pointer that will hold on return
 *                    an address of the first byte of the attachment
 *                    (or NULL if no attachment attached to the command).
 *                    If this function called more than once (to receive
 *                    big attachment) this pointer will be not touched.
 *
 * @return Status code.
 * @retval 0            - success (message received and written to the buffer)
 * @retval ETESMALLBUF  - Buffer is too small for the message. The part
 *                        of the message is written to the buffer. Other
 *                        part(s) of the message can be read by the next
 *                        calls to the rcf_net_engine_receive().
 *                        The ETSMALLBUF will be returned until last part
 *                        of the message will be read.
 * @retval ETEPENDING   - Attachment is too big to fit into the buffer.
 *                        Part of the message with attachment is written
 *                        to the buffer.  Other part(s) can be read by
 *                        the next calls to the rcf_net_engine_receive().
 *                        The ETEPENDING will be returned until last part
 *                        of the message will be read.
 * @retval other value  - errno
 */
extern int rcf_net_engine_receive(struct rcf_net_connection *rnc,
                                  char *buffer, size_t *pbytes,
                                  char **pba);


/**
 * Close connection (socket) to the Test Agent and release the memory used 
 * by struct rcf_net_connection *rnc.
 *
 * @param p_rnc         Pointer to variable with  handler received from 
 *                      rcf_net_engine_connect
 *
 * @return Status code.
 * @retval 0            - success
 * @retval other value  - errno
 */
extern int rcf_net_engine_close(struct rcf_net_connection **p_rnc);


#endif /* !__TE_COMM_NET_ENGINE_H__ */
