/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief IPC library
 *
 * IPC server definitions.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_IPC_SERVER_H__
#define __TE_IPC_SERVER_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_defs.h"


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
 * @param conn          FALSE - connectionless server,
 *                      TRUE - connection-oriented server
 * @param p_ipcs        Location for IPC server handle
 *
 * @return Status code.
 */
extern int ipc_register_server(const char *name, te_bool conn,
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
 * Get a file descriptor of the server.
 *
 * @param ipcs          Pointer to the ipc_server structure returned
 *                      by ipc_register_server()
 * @param set           Set to be updated
 *
 * @return Maximum file descriptor number or -1.
 */
extern int ipc_get_server_fds(const struct ipc_server *ipcs,
                              fd_set *set);

/**
 * Is server ready on the base of knowledge in specified set of
 * file descriptors?
 *
 * @param ipcs          Pointer to the ipc_server structure returned
 *                      by ipc_register_server()
 * @param set           Set to be analyzed
 * @param max_fd        Maximum file descriptor to be analyzed
 *
 * @return Is server ready or not?
 */
extern te_bool ipc_is_server_ready(struct ipc_server *ipcs,
                                   const fd_set *set, int max_fd);

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
 *                                     if TE_ESMALLBUF returned).
 * @param p_ipcsc       Location for the pointer to ipc_server_client()
 *                      structure.  This variable will point to internal
 *                      IPC library data, user MUST NOT free it.
 *
 * @return Status code.
 *
 * @retval 0            Success
 * @retval TE_ESMALLBUF Buffer is too small for the message, the rest
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
