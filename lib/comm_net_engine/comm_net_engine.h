/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RCF Network Communication library
 *
 * C interface for network communication library from Test Engine side.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_COMM_NET_ENGINE_H__
#define __TE_COMM_NET_ENGINE_H__

#include <sys/types.h>

#include "te_defs.h"


/** TCP interval between successfull keep-alive probes */
#define TE_COMM_NET_ENGINE_KEEPIDLE     15
/** TCP interval between failed keep-alive probes */
#define TE_COMM_NET_ENGINE_KEEPINTVL    1
/** Number of TCP keep-alive probes before failure */
#define TE_COMM_NET_ENGINE_KEEPCNT      15


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
 *                          unchanged if TE_ESMALLBUF returned;
 *                          number of bytes in the message (with
 *                          attachment)
 *                      if TE_EPENDING returned. (Note: If the
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
 * @retval 0              Success (message received and written to the
 *                        buffer)
 * @retval TE_ESMALLBUF   Buffer is too small for the message. The part
 *                        of the message is written to the buffer. Other
 *                        part(s) of the message can be read by the next
 *                        calls to the rcf_net_engine_receive().
 *                        The ETSMALLBUF will be returned until last part
 *                        of the message will be read.
 * @retval TE_EPENDING    Attachment is too big to fit into the buffer.
 *                        Part of the message with attachment is written
 *                        to the buffer.  Other part(s) can be read by
 *                        the next calls to the rcf_net_engine_receive().
 *                        The TE_EPENDING will be returned until last part
 *                        of the message will be read.
 * @retval other value    errno
 */
extern int rcf_net_engine_receive(struct rcf_net_connection *rnc,
                                  char *buffer, size_t *pbytes,
                                  char **pba);


/**
 * Close connection (socket) to the Test Agent and release the memory used
 * by struct rcf_net_connection *rnc.
 *
 * @param p_rnc         Pointer to variable with handler received from
 *                      rcf_net_engine_connect
 * @param p_select_set  Pointer to the fdset for reading to be modified
 *
 * @return Status code.
 * @retval 0            - success
 * @retval other value  - errno
 */
extern int rcf_net_engine_close(struct rcf_net_connection **p_rnc,
                                fd_set *p_select_set);


#endif /* !__TE_COMM_NET_ENGINE_H__ */
