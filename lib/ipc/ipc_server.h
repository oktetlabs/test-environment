/** @file
 * @brief IPC library
 *
 * IPC server definitions.
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

#ifndef __TE_IPC_SERVER_H__
#define __TE_IPC_SERVER_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


/** @name Structure to store state information about IPC server */
struct ipc_server;
typedef struct ipc_server ipc_server;
/*@}*/

/** @name Structure to store information about IPC server client */
struct ipc_server_client;
typedef struct ipc_server_client ipc_server_client;
/*@}*/


/**
 * Initialize IPC library. This function must be called before any
 * other functions in the IPC server library. This function must be
 * called only once.
 *
 * @return Status code.
 */
extern int ipc_init(void);


/**
 * Register IPC server.
 *
 * @param name          Name of the server (must be less than UNIX_PATH_MAX)
 * @param p_ipcs        Location for IPC server handle
 *
 * @return Status code.
 */
extern int ipc_register_server(const char *name,
                               struct ipc_server **p_ipcs);


/**
 * Get a file descriptor of the server.
 *
 * @param ipcs          Pointer to the ipc_server structure returned
 *                      by ipc_register_server().
 *
 * @return IPC server file descriptor.
 *
 * @bug
 *      It seems to be unusefull in the TCP implementation.
 */
extern int ipc_get_server_fd(const struct ipc_server *ipcs);

/**
 * Get name of the IPC server client.
 *
 * @param ipcsc         Pointer to the ipc_server_client structure
 *                      returned by ipc_receive_message()
 *
 * @return Pointer to '\0'-terminated string, "UNKNOWN" or NULL
 */
extern const char *
    ipc_server_client_name(const struct ipc_server_client *ipcsc);

/**
 * Receive a message from IPC client.
 *
 * @param ipcs          Pointer to the ipc_server structure returned
 *                      by ipc_register_server()
 * @param buf           Buffer for the message
 * @param p_buf_len     Pointer to the variable to store:
 *                          on entry - length of the buffer,
 *                          on exit - length of the received message
 *                                    (or the rest length of the message
 *                                     if ETESMALLBUF returned).
 * @param p_ipcsc       Location for the pointer to ipc_server_client()
 *                      structure.  This variable will point to internal
 *                      IPC library data, user MUST NOT free it.
 *
 * @return Status code.
 *
 * @retval 0            Success
 * @retval ETESMALLBUF  Buffer is too small for the message, the rest
 *                      of the message can be received by
 *                      ipc_receive_message() with this client pointer
 *                      in *p_ipcsc
 * @retval errno        Other failure
 */
extern int ipc_receive_message(struct ipc_server *ipcs,
                               void *buf, size_t *p_buf_len,
                               struct ipc_server_client **p_ipcsc);


/**
 * Send an answer to the client.
 *
 * @param ipcs          Pointer to the ipc_server structure returned
 *                      by ipc_register_server()
 * @param ipcsc         Variable returned by ipc_receive_message() with
 *                      pointer ipc_server_client structure
 * @param msg           Pointer to message to send
 * @param msg_len       Length of the message to send
 *
 * @return Status code.
 */
extern int ipc_send_answer(struct ipc_server *ipcs,
                           struct ipc_server_client *ipcsc,
                           const void *msg, size_t msg_len);


/**
 * Close the server. Free all resources allocated by the server.
 *
 * @param ipcs          Pointer to the ipc_server structure returned
 *                      by ipc_register_server()
 *
 * @return Status code.
 */
extern int ipc_close_server(struct ipc_server *ipcs);


/**
 * Close IPC library. No other IPC library functions except 
 * ipc_init() must be called after this function.
 *
 * @return Status code.
 */
extern int ipc_kill(void);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_IPC_SERVER_H__ */
