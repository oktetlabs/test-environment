/** @file
 * @brief IPC library
 *
 * IPC client definitions.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_IPC_CLIENT_H__
#define __TE_IPC_CLIENT_H__

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


/** @name Structure to store state information of the IPC client. */
struct ipc_client;
typedef struct ipc_client ipc_client;
/*@}*/


/**
 * Initialize IPC library for the client.
 *
 * @param client_name   Unique name of the client (must be less than
 *                      UNIX_PATH_MAX)
 * @param p_client      Location for client handle
 *
 * @return Status code.
 */
extern int ipc_init_client(const char *client_name,
                           struct ipc_client **p_client);

/**
 * Get IPC client name.
 *
 * @param ipcc          Pointer to the ipc_client structure returned
 *                      by ipc_init_client()
 *
 * @return Pointer to '\0'-terminated string or NULL
 */
extern const char *ipc_client_name(const struct ipc_client *ipcc);


/**
 * Send the message to the server with specified name.
 *
 * @param ipcc          Pointer to the ipc_client structure returned
 *                      by ipc_init_client()
 * @param server_name   Name of the server, this name must be
 *                      registered by ipc_register_server()
 * @param msg           Pointer to message to send
 * @param msg_len       Length of the message to send
 *
 *
 * @return Status code.
 *
 * @retval 0            Success
 * @retval errno        Failure
 *
 * @todo
 *      It must NOT fail, if server has not been registered yet.
 */
extern int ipc_send_message(struct ipc_client *ipcc,
                            const char *server_name,
                            const void *msg, size_t msg_len);


/**
 * Send the message to the server with specified name and wait for the
 * answer.
 *
 * @param ipcc          Pointer to the ipc_client structure returned
 *                      by ipc_init_client()
 * @param server_name   Name of the server, this name must be
 *                      registered by ipc_register_server().
 * @param msg           Pointer to message to send
 * @param msg_len       Length of the message to send
 * @param recv_buf      Pointer to the buffer for answer.
 * @param p_buf_len     Pointer to the variable to store:
 *                          on entry - length of the buffer;
 *                          on exit - length of the message received
 *                                    (or full length of the message
 *                                     if TE_ESMALLBUF is returned).
 *
 * @return Status code.
 *
 * @retval 0            Success
 * @retval TE_ESMALLBUF  Receive buffer is too small for the message,
 *                      whole buffer is filled up, *p_buf_len
 *                      contains whole answer length,
 *                      ipc_receive_rest_answer() must be used to
 *                      receive the rest of the message.
 * @retval errno        Failure
 */
extern int ipc_send_message_with_answer(struct ipc_client *ipcc,
                                        const char *server_name,
                                        const void *msg,
                                        size_t      msg_len,
                                        void       *recv_buf,
                                        size_t     *p_buf_len);


/**
 * Receive (or wait for) the message from the server with specified name.
 *
 * @param ipcc          Pointer to the ipc_client structure returned
 *                      by ipc_init_client()
 * @param server_name   Name of the server, this name must be
 *                      registered by ipc_register_server()
 * @param buf           Pointer to the buffer for answer
 * @param p_buf_len     Pointer to the variable to store:
 *                          on entry - length of the buffer;
 *                          on exit - length of the message received
 *                                    (or full length of the message
 *                                     if TE_ESMALLBUF is returned).
 *
 * @return Status code.
 *
 * @retval 0            Success
 * @retval TE_ESMALLBUF  Receive buffer is too small for the message,
 *                      whole buffer is filled up, *p_buf_len
 *                      contains whole answer length,
 *                      ipc_receive_rest_answer() must be used to
 *                      receive the rest of the message.
 * @retval errno        Failure
 */
extern int ipc_receive_answer(struct ipc_client *ipcc,
                              const char *server_name,
                              void *buf, size_t *p_buf_len);


/**
 * Receive the rest of the message from the server with specified name.
 *
 * @param ipcc          Pointer to the ipc_client structure returned
 *                      by ipc_init_client()
 * @param server_name   Name of the server, this name must be
 *                      registered by ipc_register_server()
 * @param buf           Pointer to the buffer for answer
 * @param p_buf_len     Pointer to the variable to store:
 *                          on entry - length of the buffer;
 *                          on exit - length of the message received
 *                                    (or full length of the message
 *                                     if TE_ESMALLBUF is returned).
 *
 * @return Status code.
 *
 * @retval 0            Success
 * @retval TE_ESMALLBUF  Receive buffer is too small for the message,
 *                      whole buffer is filled up, *p_buf_len
 *                      contains whole answer length,
 *                      ipc_receive_rest_answer() must be used to
 *                      receive the rest of the message.
 * @retval errno        Failure
 */
extern int ipc_receive_rest_answer(struct ipc_client *ipcc,
                                   const char *server_name,
                                   void *buf, size_t *p_buf_len);

/**
 * Close IPC client.
 *
 * @param ipcc          Pointer to the ipc_client structure returned
 *                      by ipc_init_client()
 *
 * @return Status code.
 *
 * @retval 0            Success
 * @retval errno        Failure
 */
extern int ipc_close_client(struct ipc_client *ipcc);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_IPC_CLIENT_H__ */
