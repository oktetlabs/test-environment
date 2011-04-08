/** @file
 * @brief Test API - RPC to create client-server connections
 *
 * Routines to create client-server connections.
 *
 * Actually we can't say that in @c SOCK_DGRAM socket type there are
 * clients and servers, but we will call them so in case each socket
 * is connected to its peer, so that send() and recv() operations lead
 * to sending and receiving data to/from the particular remote peer.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2004-2006 Level5 Networks Corp.
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id: tapi_rpc.h 22478 2006-01-06 11:58:27Z arybchik $
 */

#ifndef __TE_TAPI_RPC_CLIENT_SERVER_H__
#define __TE_TAPI_RPC_CLIENT_SERVER_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_rpc_sys_socket.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create socket of type @p sock_type from @p domain domain and bind it
 * to  the specified address
 *
 * @param rpc             PCO where a socket is created
 * @param sock_type       Type of socket
 * @param proto           Protocol used for the socket
 * @param wild            Whether to bind socket to wildcard address
 * @param set_reuse_addr  Whether to set SO_REUSEADDR socket option on 
 *                        the socket
 * @param addr            Address the socket should be bound to 
 *                        (note that it is not allowed to have network 
 *                        address part of address set to wildcard, but
 *                        if you want to bind the socket to wildcard
 *                        address you should set @p wild parameter to
 *                        @c TRUE)
 *
 * @return Descriptor of created socket
 *
 * @retval @c -1 Creating or binding socket failed
 *
 * @note When the function returns @c -1 it reports the reason of the
 * failure with ERROR() macro.
 */
extern int rpc_create_and_bind_socket(rcf_rpc_server    *rpc,
                                      rpc_socket_type   sock_type,
                                      rpc_socket_proto  proto,
                                      te_bool           wild,
                                      te_bool           set_reuse_addr,
                                      const struct sockaddr  *addr);


/** @page lib-stream_server-alg Algorithm of creating a server socket of type @c SOCK_STREAM
 *
 * -# Call @b socket() on @p srvr PCO with the following parameters:
 *    @p domain, @c SOCK_STREAM, @p proto.
 *    Created socket is referred as @p srvr_s below;
 * -# If @p srvr_wild is true, fill in network address part of 
 *    @p srvr_bind_addr with wildcard network address;
 * -# Copy port part of @p srvr_addr to port part of @p srvr_bind_addr
 *    address;
 * -# Bind @p srvr_s socket to @p srvr_bind_addr address.
 * -# If port part of @p srvr_addr is zero (not specified), then call
 *    @b getsockname() on @p srvr_s socket to obtain the assigned port 
 *    and set it to the port part of @p srvr_addr.
 * -# Call @b listen() for @p srvr_s socket with default @a backlog.
 */

/**
 * Create a listening socket of type @c SOCK_STREAM on a particular PCO.
 *
 * @param srvr          PCO
 * @param domain        Domain for the socket
 * @param proto         Protocol for the socket
 * @param srvr_wild     Whether to bind server to wildcard address or not
 * @param srvr_addr     Server address to be used as a template 
 *                      for @b bind() on server side (IN/OUT)
 *
 * @return Created socket or -1.
 *
 * @copydoc lib-stream_server-alg
 */
extern int rpc_stream_server(rcf_rpc_server *srvr,
                             rpc_socket_proto proto, te_bool srvr_wild,
                             const struct sockaddr *srvr_addr);

/** @page lib-stream_client-alg Algorithm of creating a client socket of type
 * @c SOCK_STREAM
 *
 * -# Call @b socket() on @a clnt PCO with the following parameters:
 *    @p domain, @c SOCK_STREAM, @p proto.
 *    Created socket is referred as @p clnt_s below.
 * -# If @p clnt_addr is not equal to @c NULL, @b bind() @p clnt_s socket
 *    to @p clnt_addr address.
 */

/**
 * Create a client socket of type @c SOCK_STREAM ready to connect to
 * a remote peer (some listening socket)
 *
 * @param clnt          PCO where socket will be opened on
 * @param domain        Domain for the socket
 * @param proto         Protocol for the socket
 * @param clnt_addr     Address to bind client to or @c NULL
 * 
 * @return Created socket or -1.
 *
 * @copydoc lib-stream_client-alg
 */
extern int rpc_stream_client(rcf_rpc_server *clnt,
                             rpc_socket_domain domain,
                             rpc_socket_proto proto,
                             const struct sockaddr *clnt_addr);

/** @page lib-stream_client_server Create a connection with connection oriented sockets
 *
 * @param srvr          PCO for server
 * @param clnt          PCO for client
 * @param domain        Domain used in the connection
 * @param proto         Protocol used in the connection
 * @param srvr_addr     server address (cannot be @c NULL) to be used as 
 *                      a template for @b bind() on server side and for 
 *                      @b connect() on client side. Network address part of
 *                      the @p srvr_addr must be specified, but it is
 *                      allowed to left port part of @p srvr_addr
 *                      unspecified, which means we do not mind which
 *                      address the server is bound to (on return the actual
 *                      port used in established connection is set to the
 *                      port part of @p srvr_addr).
 * @param srvr_wild     bind server to wildcard address or not (although we
 *                      must specify network address in @p srvr_addr
 *                      parameter, it is still allowed to bind server socket
 *                      to the wildcard address)
 * @param clnt_addr     address to bind client to or @c NULL
 *
 * @par Step 1: Open @c SOCK_STREAM server socket
 * @copydoc lib-stream_server-alg
 *
 * @par Step 2: Open @c SOCK_STREAM client socket
 * @copydoc lib-stream_client-alg
 *
 * @par Step 3: Open connection
 * -# Initiate @b accept() for @p srvr_s socket;
 * -# Call @b connect() to connect client socket @p clnt_s to server
 *    with @p srvr_addr address;
 * -# Wait for @b accept() completion to get @p accepted_s socket;
 * -# Close srvr_s socket
 * -# Set @p accepted_s to @p srvr_s variable.
 *
 * @retval srvr_s       @c SOCK_STREAM socket reside on @p srvr
 * @retval clnt_s       @c SOCK_STREAM socket reside on @p clnt
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

/**
 * Create a connection of type SOCK_STREAM between two PCO
 *
 * @param srvr          PCO where server socket is created
 * @param clnt          PCO where client socket is created
 * @param proto         Protocol for the connection
 * @param srvr_addr     Server address to be used as a template 
 *                      for @b bind() on server side (IN/OUT)
 * @param clnt_addr     Address to bind client to or @c NULL
 * @param srvr_s        Descriptor of the socket reside on @p srvr (OUT)
 * @param clnt_s        Descriptor of the socket reside on @p clnt (OUT)
 *
 * @return Status of the operation
 *
 * @retval @c  0 Connection successfully created
 * @retval @c -1 Creating connection failed
 *
 * @note When the function returns @c -1 it reports the reason of the
 * failure with ERROR() macro.
 */
extern int rpc_stream_connection(rcf_rpc_server *srvr,
                                 rcf_rpc_server *clnt,
                                 rpc_socket_proto proto,
                                 const struct sockaddr *srvr_addr,
                                 const struct sockaddr *clnt_addr,
                                 int *srvr_s, int *clnt_s);

/** @page lib-dgram_client_server Create a connectionless pair of sockets that can communicate with each other without specifying any addresses in their I/O operations 
 *
 * @param srvr          PCO for server part of connection
 * @param clnt          PCO for client part of connection
 * @param domain        Domain used in the connection
 * @param proto         Protocol used in the connection
 * @param srvr_addr     server address (cannot be @c NULL) to be used as 
 *                      a template for @b bind() on server side and for 
 *                      @b connect() on client side.
 * @param clnt_addr     address to bind client to (cannot be @c NULL)
 *
 * -# Open @c SOCK_DGRAM socket @p srvr_s on @p srvr and bind it 
 *    to @p srvr_addr address;
 * -# Open @c SOCK_DGRAM socket @p clnt_s on @p clnt and bind it 
 *    to to @p clnt_addr address;
 * -# @b connect() @p clnt_s socket to @p srvr_s socket;
 * -# @b connect() @p srvr_s socket to @p clnt_s socket.
 *
 * @retval srvr_s       @c SOCK_DGRAM socket reside on @p srvr
 * @retval clnt_s       @c SOCK_DGRAM socket reside on @p clnt
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 */

/**
 * Create a connection of type @c SOCK_DGRAM between two PCO
 *
 * @param srvr          PCO where server socket is created
 * @param clnt          PCO where client socket is created
 * @param proto         Protocol for the connection
 * @param srvr_addr     Server address to be used as a template 
 *                      for @b bind() on server side
 * @param clnt_addr     Address to bind client to
 * @param srvr_s        Descriptor of the socket reside on @p srvr (OUT)
 * @param clnt_s        Descriptor of the socket reside on @p clnt (OUT)
 *
 * @return Status of the operation
 *
 * @retval @c  0 Connection successfully created
 * @retval @c -1 Creating connection failed
 *
 * @note When the function returns @c -1 it reports the reason of the
 * failure with ERROR() macro.
 */
extern int rpc_dgram_connection(rcf_rpc_server *srvr,
                                rcf_rpc_server *clnt,
                                rpc_socket_proto proto,
                                const struct sockaddr *srvr_addr,
                                const struct sockaddr *clnt_addr,
                                int *srvr_s, int *clnt_s);


/** @page lib-gen_connection Create a connection of an arbitrary type
 *
 * @objective Provide a generic way to create a connection of an arbitrary
 * type and from a particular domain
 *
 * @param srvr          PCO where server socket is created
 * @param clnt          PCO where client socket is created
 * @param sock_type     Socket type used in the connection
 * @param proto         Protocol for the connection
 * @param srvr_addr     Server address to be used as a template
 *                      for @b bind() on server side
 * @param clnt_addr     Address to bind client to
 * @param srvr_s        Descriptor of the socket reside on @p srvr
 *                      (accepted socket in the case of stream connection)
 *                      (OUT)
 * @param clnt_s        Descriptor of the socket reside on @p clnt (OUT)
 *
 * @note Division of two peers on server and client is purely abstract,
 *       because actually just after creating a connection of type 
 *       @c SOCK_STREAM we close real server socket and associate its
 *       child socket, with @p srvr_s parameter of the function.
 *
 * For connection of type @c SOCK_STREAM use algorithm
 * @ref lib-stream_client_server.
 *
 * For connection of type @c SOCK_DGRAM use algorithm
 * @ref lib-dgram_client_server.
 */

/**
 * Create a connection of an arbitrary type between two PCO.
 *
 * @copydoc lib-gen_connection
 *
 * @return Status of the operation
 *
 * @retval @c  0 Connection successfully created
 * @retval @c -1 Creating connection failed
 *
 * @note When the function returns @c -1 it reports the reason of the
 * failure with ERROR() macro.
 */
extern int rpc_gen_connection(rcf_rpc_server *srvr, rcf_rpc_server *clnt,
                              rpc_socket_type sock_type,
                              rpc_socket_proto proto,
                              const struct sockaddr *srvr_addr,
                              const struct sockaddr *clnt_addr,
                              int *srvr_s, int *clnt_s);

/** 
 * The macro is a wrapper over gen_connection function.
 * In case of failure it calls TEST_FAIL() macro, so that it should be
 * called in test context only.
 */
#define GEN_CONNECTION(srvr_, clnt_, sock_type_, proto_,                \
                       srvr_addr_, clnt_addr_, srvr_s_, clnt_s_)        \
    do {                                                                \
        if (rpc_gen_connection(srvr_, clnt_,                            \
                               sock_type_, proto_,                      \
                               srvr_addr_, clnt_addr_,                  \
                               srvr_s_, clnt_s_) != 0)                  \
        {                                                               \
            TEST_FAIL("Cannot create a connection of type %s",          \
                       socktype_rpc2str(sock_type_));                   \
        }                                                               \
    } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAPI_RPC_CLIENT_SERVER_H__ */
