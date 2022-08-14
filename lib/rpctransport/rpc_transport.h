/** @file
 * @brief RCF RPC
 *
 * Different transports which can be used for interaction between
 * RPC server and TA.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __RPC_TRANSPORT_H__
#define __RPC_TRANSPORT_H__

typedef int rpc_transport_handle;

/**
 * Initialize RPC transport.
 *
 * @param tmp_path Path to folder where temporary
 *                 files should be stored
 */
extern te_errno rpc_transport_init(const char *tmp_path);

/**
 * Shutdown RPC transport.
 */
extern void rpc_transport_shutdown(void);

/**
 * Await connection from RPC server.
 *
 * @param name          name of the RPC server
 * @param p_handle      connection handle location
 *
 * @return Status code.
 */
extern te_errno rpc_transport_connect_rpcserver(
    const char *name,
    rpc_transport_handle *p_handle);

/**
 * Connect from RPC server to Test Agent
 *
 * @param name  name of the RPC server
 * @param p_handle      connection handle location
 *
 * @return Status code.
 */
extern te_errno rpc_transport_connect_ta(const char *name,
                                         rpc_transport_handle *p_handle);

/**
 * Break the connection.
 *
 * @param handle      connection handle
 */
extern void rpc_transport_close(rpc_transport_handle handle);

/**
 * Reset set of descriptors to wait.
 */
extern void rpc_transport_read_set_init(void);

/**
 * Add the handle to the read set.
 *
 * @param handle        connection handle
 */
extern void rpc_transport_read_set_add(rpc_transport_handle handle);

/**
 * Wait for the read event.
 *
 * @param timeout       timeout in seconds
 *
 * @return TRUE is the read event is received or FALSE otherwise
 */
extern te_bool rpc_transport_read_set_wait(int timeout);

/**
 * Check if data are pending on the connection.
 *
 * @param handle        connection handle
 *
 * @return TRUE is data are pending or FALSE otherwise
 */
extern te_bool rpc_transport_is_readable(rpc_transport_handle handle);

/**
 * Receive message with specified timeout.
 *
 * @param handle   connection handle
 * @param buf      buffer for reading
 * @param p_len    IN: buffer length; OUT: length of received message
 * @param timeout  timeout in seconds
 *
 * @return Status code.
 * @retval TE_ETIMEDOUT         Timeout ocurred
 * @retval TE_ECONNRESET        Connection is broken
 * @retval TE_ENOMEM            Message is too long
 */
extern te_errno rpc_transport_recv(rpc_transport_handle handle,
                                   uint8_t *buf, size_t *p_len,
                                   int timeout);

/**
 * Send message.
 *
 * @param handle   connection handle
 * @param buf      buffer for writing
 * @param len      message length
 *
 * @return Status code.
 * @retval TE_ECONNRESET        Connection is broken
 */
extern te_errno rpc_transport_send(rpc_transport_handle handle,
                                   const uint8_t *buf, size_t len);

#endif /* !__RPC_TRANSPORT_H__ */
