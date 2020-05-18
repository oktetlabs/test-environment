
/** @file
 * @brief Functions to opearate with socket
 *
 * @defgroup ts_tapi_sockets High level TAPI to work with sockets
 * @ingroup te_ts_tapi
 * @{
 *
 * Definition of test API for working with socket.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_TAPI_SOCKETS_H__
#define __TE_TAPI_SOCKETS_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_sockaddr.h"
#include "te_rpc_types.h"
#include "te_dbuf.h"
#include "rcf_rpc.h"
#include "tapi_rpc_unistd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Transmitting functions list.
 */
#define TAPI_SOCK_SEND_FUNC_LIST \
    { "write", TARPC_SEND_FUNC_WRITE },      \
    { "writev", TARPC_SEND_FUNC_WRITEV },    \
    { "send", TARPC_SEND_FUNC_SEND },        \
    { "sendto", TARPC_SEND_FUNC_SENDTO },    \
    { "sendmsg", TARPC_SEND_FUNC_SENDMSG },  \
    { "sendmmsg", TARPC_SEND_FUNC_SENDMMSG }

#define TEST_GET_SOCK_SEND_FUNC(_var_name) \
    TEST_GET_ENUM_PARAM(_var_name, TAPI_SOCK_SEND_FUNC_LIST)

/**
 * Retrieve TCP state of a given socket.
 *
 * @param pco       RPC server handle
 * @param s         Socket descriptor
 *
 * @return TCP socket state.
 */
extern rpc_tcp_state tapi_get_tcp_sock_state(struct rcf_rpc_server *pco,
                                             int s);

/**
 * Read all the available data from a given socket and append it to
 * a given te_dbuf.
 *
 * @param rpcs        RPC server handle.
 * @param s           Socket.
 * @param read_data   Where to save read data.
 *
 * @return Length of read data on success or negative value in case of
 *         failure.
 */
extern ssize_t tapi_sock_read_data(rcf_rpc_server *rpcs, int s,
                                   te_dbuf *read_data);

/**
 * Send a raw TCPv4 packet packet with full ethernet header.
 *
 * @param rpcs            RPC server handle.
 * @param iov             Data vector
 * @param iov_len         Actual data vector length
 * @param ifindex         Interface index to send packet
 * @param raw_socket      Raw socket for sending data
 * @param remove_vlan_hdr Remove all VLAN headers if @c TRUE
 *
 * @note The function uses an already opened raw socket.
 * It should be created like this:
 * rpc_socket(rpcs, RPC_AF_PACKET, RPC_SOCK_RAW, RPC_IPPROTO_RAW);
 *
 * @return Status code
 */
extern te_errno tapi_sock_raw_tcpv4_send(rcf_rpc_server *rpcs, rpc_iovec *iov,
                                         int iovlen, int ifindex,
                                         int raw_socket,
                                         te_bool remove_vlan_hdr);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_SOCKETS_H__ */

/**@} <!-- END ts_tapi_sockets --> */
