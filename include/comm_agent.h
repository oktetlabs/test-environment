/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Communication Library - Test Agent side
 *
 * Definition of routines provided for library user
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_COMM_AGENT_H__
#define __TE_COMM_AGENT_H__

#include "te_errno.h"

/** This structure is used to store some context for each connection. */
struct rcf_comm_connection;
typedef struct rcf_comm_connection rcf_comm_connection;

/**
 * Create a listener for accepting connection from RCF in TA.
 *
 * @note Normally this function is called from rcf_comm_agent_init().
 *       It is used outside of it only when it is required to
 *       create listener before starting TA, see
 *       agents/unix/ta_rcf_listener.c
 *
 * @param port      Local port to which the listener should be bound.
 * @param listener  Where to save socket FD.
 *
 * @return Status code.
 */
extern te_errno rcf_comm_agent_create_listener(int port, int *listener);

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
                               rcf_comm_connection **p_rcc);



/**
 * Wait for command from the Test Engine via Network Communication library.
 *
 * @param rcc           Handler received from rcf_comm_agent_init
 * @param buffer        Buffer for data
 * @param pbytes        Pointer to variable with:
 *                      on entry - size of the buffer;
 *                      on return:
 *                      number of bytes really written if 0 returned
 *                      (success); unchanged if TE_ESMALLBUF returned;
 *                      number of bytes in the message (with attachment) if
 *                      TE_EPENDING returned. (Note: If the function called
 *                      a number of times to receive one big message, a full
 *                      number of bytes will be returned on first call.  On
 *                      the next calls number of bytes in the message minus
 *                      number of bytes previously read by this function
 *                      will be returned.); undefined if other errno
 *                      returned.
 * @param pba           Address of the pointer that will hold on return
 *                      address of the first byte of attachment (or NULL if
 *                      no attachment attached to the command). If this
 *                      function called more than once (to receive big
 *                      attachment) this pointer will be not touched.
 *
 * @return Status code.
 * @retval 0            Success (message received and written to the
 *                      buffer).
 * @retval TE_ESMALLBUF Buffer is too small for the message. The part of
 *                      the message is written to the buffer. Other part(s)
 *                      of the message can be read by the next calls to the
 *                      rcf_comm_agent_wait. The ETSMALLBUF will be returned
 *                      until last part of the message will be read.
 * @retval TE_EPENDING  Attachment is too big to fit into the buffer. Part
 *                      of the message with attachment is written to the
 *                      buffer. Other part(s) can be read by the next calls
 *                      to the rcf_comm_engine_receive. The TE_EPENDING will
 *                      be returned until last part of the message will be
 *                      read.
 * @retval other value  errno.
 */
extern int rcf_comm_agent_wait(rcf_comm_connection *rcc,
                               char *buffer, size_t *pbytes, void **pba);


/**
 * Send reply to the Test Engine side of Network Communication library.
 *
 * @param rcc           Handler received from rcf_comm_agent_init.
 * @param p_buffer      Buffer with reply.
 * @param length        Length of the data to send.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval other value  errno.
 */
extern int rcf_comm_agent_reply(rcf_comm_connection *rcc,
                                const void *p_buffer, size_t length);

/**
 * Close connection.
 *
 * @param p_rcc   Pointer to variable with handler received from
 *                rcf_comm_agent_init.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval other value  errno.
 */
extern int rcf_comm_agent_close(rcf_comm_connection **p_rcc);

#endif /* !__TE_COMM_AGENT_H__ */
