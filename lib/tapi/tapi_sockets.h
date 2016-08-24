
/** @file
 * @brief Functions to opearate with socket 
 *
 * Definition of test API for working with socket.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id:
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
    { "sendmsg", TARPC_SEND_FUNC_SENDMSG }

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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_SOCKETS_H__ */
