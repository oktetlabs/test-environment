/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of Socket API
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_SOCKET_H__
#define __TE_TAPI_RPC_SOCKET_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "rcf_rpc.h"
#include "tapi_rpcsock_defs.h"

/**
 * Create an endpoint for communication on the RPC server side. 
 *
 * @param rpcs      RPC server handle
 * @param domain    communication domain.
 *                  Select the protocol family used for communication
 *                  supported protocol families are difined in 
 *                  tapi_rpcsock_defs.h
 * @param type      defines the semantic of communication. Current defined 
 *                  types can be found in tapi_rpcsock_defs.h
 * @param protocol  specifies the protocol to be used. If @b protocol is 
 *                  set to RPC_PROTO_DEF, the system selects the default 
 *                  protocol number for the socket domain and type 
 *                  specified.
 * 
 * @return the socket descriptor to be used. Otherwise -1 is returned when 
 *         error occured.
 */

extern int rpc_socket(rcf_rpc_server *rpcs,
                      rpc_socket_domain domain, rpc_socket_type type,
                      rpc_socket_proto protocol);


/**
 * End communication on socket descriptor @b s in one or both directions 
 * on RPC server side.
 *
 * @param rpcs  RPC server handle
 * @param s     socket descriptor 
 * @param how  specifies the shutdown condition and can be specified in 
 *             the following ways:
 *              - RPC_SHUT_RD   receiving not allowed anymore
 *              - RPC_SHUT_WR   sending not allowed anymore
 *              - RPC_SHUT_RDWR receiving and sending not allowed anymore
 * 
 * @retval  0 on success
 *         -1 on error and error code is set appropriately to @b RPC_XXX 
 *         where @b XXX - standard errno for @b shutdown() 
 * @note For more information about @b shutdown() see its corresponding 
 *       manual page
 */
extern int rpc_shutdown(rcf_rpc_server *rpcs,
                        int s, rpc_shut_how how);

/**
 * Transmit a message to socket descriptor s on RPC server side
 *
 * @param rpcs  RPC server handle
 * @param s     socket descriptor
 * @param buf   pointer to buffer containing the message to be sent
 * @param len   length of the message in bytes
 * @param flags bitwise OR of zero or more of the following flags:
 *               - MSG_OOB send out-of-band data if supported.
 *               - MSG_DONTWAIT enable non-blocking operation.
 *               See @b send() manual page for more information about all 
 *               supported flags.
 *
 * @return number of bytes actually sent, otherwise -1.
 * @note See @b send() manual page for more information
 */
extern ssize_t rpc_send(rcf_rpc_server *rpcs,
                        int s, const void *buf, size_t len,
                        int flags);

/**
 * Transmit a message to descriptor @b s.
 * This operation takes place on RPC server side
 *
 * @param rpcs  RPC server handle
 * @param s     socket descriptor
 * @param buf   pointer to buffer containing message to be sent
 * @param len   message length in bytes
 * @param flags bitwise OR of zero or more of the following flags:
 *               - RPC_MSG_OOB send out-of-band data if supported.
 *               - RPC_MSG_DONTWAIT enable non-blocking operation.
 *               Other supported flags can be found in tapi_rpcsock_defs.h
 * @param to     target address
 * @param tolen  size of target address
 *
 * @return Number of bytes sent, otherwise -1.
 *
 * @note See @b sendto() manual page for more information about flags and 
 *       error code set when this routine failed.
 */
extern ssize_t rpc_sendto(rcf_rpc_server *rpcs,
                          int s, const void *buf, size_t len,
                          rpc_send_recv_flags flags,
                          const struct sockaddr *to, socklen_t tolen);

/**
 * Generic routine for receiving messages and store them in the buffer @b 
 * buf of length @b len. This operation takes place on RPC server side.
 *
 * @param rpcs    RPC server handle
 * @param s       socket descriptor
 * @param buf     pointer to buffer which store received messages
 * @param len     size of the buffer @b buf
 * @param flags   bitwise OR of zero or more of the following flags:
 *                 - RPC_MSG_OOB send out-of-band data if supported.
 *                 - RPC_MSG_DONTWAIT enable non-blocking operation.
 *                 Other supported flags can be found in tapi_rpcsock_defs.h
 * @param rbuflen number of bytes to be received.
 *
 * @return Number of bytes received, otherwise -1 
 */

extern ssize_t rpc_recv_gen(rcf_rpc_server *rpcs,
                            int s, void *buf, size_t len,
                            rpc_send_recv_flags flags,
                            size_t rbuflen);

/**
 * Receive messages and store them in the buffer @b buf of length @b len.
 * This operation takes place on RPC server side
 *
 * @param rpcs  RPC server handle
 * @param s     socket descriptor
 * @param buf   pointer to buffer which store received messages
 * @param len   size of the buffer @b buf
 * @param flags bitwise OR of zero or more of the following flags:
 *               - RPC_MSG_OOB send out-of-band data if supported.
 *               - RPC_MSG_DONTWAIT enable non-blocking operation.
 *               Other supported flags can be found in tapi_rpcsock_defs.h
 *
 * @return Number of bytes received, otherwise -1 when error occured
 */
static inline ssize_t
rpc_recv(rcf_rpc_server *rpcs,
         int s, void *buf, size_t len, rpc_send_recv_flags flags)
{
    return rpc_recv_gen(rpcs, s, buf, len, flags, len);
}

/**
 * Generic routine for receiving data from a connected or non-connected 
 * socket. This operation takes place on RPC server side.
 * Behavior of this routine depends on  specified RPC operation:
 * @c RCF_RPC_CALL - return immediately without waiting for remote 
 *                   procedure to complete (non-blocking call).
 * @c RCF_RPC_WAIT - wait for non-blocking call to complete
 * @c RCF_RPC_CALL_WAIT - wait for remote procedure to complete before 
 *    returning (blocking call)
 
 * @param rpcs      RPC server handle
 * @param s         Socket descriptor
 * @param buf       pointer to buffer which store received data
 * @param len       size of the buffer @b buf
 * @param flags     bitwise OR of zero or more of the following flags:
 *                   - RPC_MSG_OOB send out-of-band data if supported.
 *                   - RPC_MSG_DONTWAIT enable non-blocking operation.
 *                   Other supported flags can be found in 
 *                   tapi_rpcsock_defs.h
 * @param from       pointer to source address of the message or NULL (OUT)
 * @param fromlen    size of source address @b from
 * @param rbuflen    number of bytes to be received.
 * @param rfrombuflen size of source address @b from (if not NULL)
 *
 * @return Number of bytes received, otherwise -1 
 */
extern ssize_t rpc_recvfrom_gen(rcf_rpc_server *rpcs,
                                int s, void *buf, size_t len,
                                rpc_send_recv_flags flags,
                                struct sockaddr *from, socklen_t *fromlen,
                                size_t rbuflen, socklen_t rfrombuflen);

/**
 * Receive data from a connected or non-connected socket. 
 * This operation takes place on RPC server side.
 *
 * @param rpcs      RPC server handle
 * @param s         Socket descriptor
 * @param buf       pointer to buffer which store received data
 * @param len       size of the buffer @b buf
 * @param flags     bitwise OR of zero or more of the following flags:
 *                   - RPC_MSG_OOB send out-of-band data if supported.
 *                   - RPC_MSG_DONTWAIT enable non-blocking operation.
 *                   Other supported flags can be found in 
 *                   tapi_rpcsock_defs.h
 * @param from       pointer to source address of the message or NULL (OUT)
 * @param fromlen    size of source address @b from.
 *
 * @return Number of bytes received, otherwise -1 
 */
static inline ssize_t
rpc_recvfrom(rcf_rpc_server *rpcs,
             int s, void *buf, size_t len,
             rpc_send_recv_flags flags,
             struct sockaddr *from, socklen_t *fromlen)
{
    return rpc_recvfrom_gen(rpcs, s, buf, len, flags, from, fromlen,
                            len, (fromlen == NULL) ? 0 : *fromlen);
}

/** Store information about message */
typedef struct rpc_msghdr {
    /* Standard fields - do not change types/order! */
    void             *msg_name;         /**< protocol address */
    socklen_t         msg_namelen;      /**< size of protocol address */
    struct rpc_iovec *msg_iov;          /**< scatter/gather array */
    size_t            msg_iovlen;       /**< elements in msg_iov */
    void             *msg_control;      /**< ancillary data */
    
    socklen_t            msg_controllen; /**< length of ancillary data */
    rpc_send_recv_flags  msg_flags;      /**< flags returned by recvmsg() */

    /* Non-standard fields for test purposes */
    socklen_t         msg_rnamelen;     /**< real size of protocol
                                             address buffer to be copied
                                             by RPC */
    size_t            msg_riovlen;      /**< real number of elements
                                             in msg_iov */
    int               msg_cmsghdr_num;  /**< Number of elements in
                                             the array */ 
} rpc_msghdr;

/**
 * Send message to a connected or non-connected socket.
 * This operation takes place on RPC server side.
 *
 * @param rpcs      RPC server handle
 * @param s         socket descriptor
 * @param msg       pointer to a rpc_msghdr structure that hold the 
 *                  message to be sent
 * @param flags     bitwise OR of zero or more of the following flags:
 *                   - @b RPC_MSG_OOB send out-of-band data if supported.
 *                   - @b RPC_MSG_DONTWAIT enable non-blocking operation.
 *                   - @b RPC_MSG_PEEK  do not remove data from the queue. 
 *                   - @b RPC_MSG_DONTROUTE send to directly connected 
 *                        network.
 *                   - @b RPC_MSG_WAITALL block until full request is 
 *                        specified.
 *                   - @b RPC_MSG_NOSIGNAL  turn off raising of SIGPIPE.
 *                   - @b RPC_MSG_TRUNC    return the real length of the
 *                        packet.
 *                   - @b RPC_MSG_CTRUNC    control data lost before 
 *                        delivery.
 *                   - @b RPC_MSG_ERRQUEUE queued errors should be 
 *                        received from the socket error queue.
 *                   - @b RPC_MSG_MCAST   datagram was received as a 
 *                        link-layer multicast. 
 *                   - @b RPC_MSG_BCAST datagram was received as a 
 *                        link-layer broadcast.
 *
 * @return length of message, otherwise -1 is returned when an error 
 *         occured 
 */
extern ssize_t rpc_sendmsg(rcf_rpc_server *rpcs,
                           int s, const struct rpc_msghdr *msg,
                           rpc_send_recv_flags flags);

/**
 * Receive data from connected or non-connected socket
 * This operation takes place on RPC server side.
 *
 * @param rpcs      RPC server handle
 * @param s         socket descriptor
 * @param msg       pointer to a @b rpc_msghdr structure that hold the
 *                  received message 
 * @param flags     bitwise OR of zero or more of the following flags:
 *                   - RPC_MSG_OOB send out-of-band data if supported.
 *                   - RPC_MSG_DONTWAIT enable non-blocking operation.
 *                   Other supported flags can be found in 
 *                   tapi_rpcsock_defs.h
 *
 * @return length of received data, otherwise -1 when an error occured.
 */
extern ssize_t rpc_recvmsg(rcf_rpc_server *rpcs,
                           int s, struct rpc_msghdr *msg,
                           rpc_send_recv_flags flags);


/**
 * Assign a name to an unnamed socket. When o socket is created, it exists
 * in an address family, but it does not have a name assigned to it. 
 * Servers use @b bind() to associate themselves with a well-known port.
 * This operation takes place on RPC server side.
 *
 * @param rpcs      RPC server handle
 * @param s         socket descriptor
 * @param my_addr   pointer to a @b sockaddr structure
 * @param addrlen   size of @b my_addr
 *
 * @retval 0 on success
 *        -1 on failure 
 * @note See @b bind manual page for more infrormation.
 */
extern int rpc_bind(rcf_rpc_server *rpcs,
                    int s, const struct sockaddr *my_addr,
                    socklen_t addrlen);

/**
 * Attempt to associate a socket descriptor @b s with a peer process at 
 * address @b addr. This operation takes place on RPC server side.
 * 
 * @param rpcs      RPC server handle
 * @param s         socket descriptor
 * @param addr      peer address to which the socket has to be connected
 * @param addrlen   size of peer address @b addr
 *
 * @retval   0 on success
 *         -1 on failure
 * @note   See @b connect manual page for more information
 */
extern int rpc_connect(rcf_rpc_server *rpcs,
                       int s, const struct sockaddr *addr,
                       socklen_t addrlen);

/**
 * Try to listen for incomming connections for connection oriented sockets
 * This operation takes place on RPC server side.
 *
 * @param rpcs        RPC server handle
 * @param s           listening socket descriptor
 * @param backlog     maximum number of connections waiting to be accepted
 *
 * @retval 0 on success
 *        -1 on failure
 */
extern int rpc_listen(rcf_rpc_server *rpcs,
                      int s, int backlog);

/**
 * This generic routine extract the first connection request of the queue
 * of pending connections, allocate a new descriptor for the connected 
 * socket.This operation takes place on RPC server side.
 * The behavior of this routine depends on  specified RPC operation:
 * @c RCF_RPC_CALL - return immediately without waiting for remote 
 * procedure to complete (non-blocking call).
 * @c RCF_RPC_WAIT - wait for non-blocking call to complete
 * @c RCF_RPC_CALL_WAIT - wait for remote procedure to complete before 
 *    returning (blocking call)
 *
 * @param rpcs     RPC server handle
 * @param s        listening socket descriptor
 * @param addr     pointer to a sockaddr structure
 * @param addrlen  pointer to size of structure pointed by @b addr. 
 *                 On return contain the actual size of the returned
 *                 in @b addr address 
 * @param raddrlen real size of @b addr
 *
 * @retval 0 on success
 *        -1 on failure
 */
extern int rpc_accept_gen(rcf_rpc_server *rpcs,
                          int s, struct sockaddr *addr,
                          socklen_t *addrlen,
                          socklen_t raddrlen);
/**
 * Extract the first connection request of the queue of pending 
 * connections, allocate a new descriptor for the connected socket
 * This operation takes place on RPC server side.
 * The behavior of this routine depends on  specified RPC operation:
 * @c RCF_RPC_CALL - return immediately without waiting for remote 
 * procedure to complete (non-blocking call).
 * @c RCF_RPC_WAIT - wait for non-blocking call to complete
 * @c RCF_RPC_CALL_WAIT - wait for remote procedure to complete before 
 *    returning (blocking call)
 *
 * @param rpcs     RPC server handle
 * @param s        listening socket descriptor
 * @param addr     pointer to a sockaddr structure
 * @param addrlen  contain size of structure pointed by @b addr. On return 
 *                 contain the actual size of the returned in @b addr 
 *                 address 
 *
 * @retval 0 on success
 *        -1 on failure
 */
static inline int
rpc_accept(rcf_rpc_server *rpcs,
           int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return rpc_accept_gen(rpcs, s, addr, addrlen,
                          (addrlen == NULL) ? 0 : *addrlen);
}

/**
 * This generic routine manipulates options associated with a socket.
 * This operation takes place on RPC server side.
 *
 * @note For more information about supported option level, 
 * see tapi_rpcsock_defs.h.
 *
 * @param rpcs     RPC server handle
 * @param s        socket descriptor
 * @param level    protocol level at which the option resides. 
 *                 Following values can be specified:
 *                  - @b RPC_SOL_SOCKET  socket level
 *                  - @b RPC_SOL_IP      IPPROTO_IP level
 *                  - @b RPC_SOL_TCP     IPPROTO_TCP level
 * @param optname  option name
 * @param optval   pointer to a buffer containing the value associated 
 *                 with the selected option.
 * @param optlen   initially points to the length of supplied buffer.
 *                 On return contain the actual size of the buffer.
 * @param roptlen  maximal length of the buffer or zero 
 *
 * @retval  0 on success
 *         -1 on failure
 */
extern int rpc_getsockopt_gen(rcf_rpc_server *rpcs,
                              int s, rpc_socklevel level,
                              rpc_sockopt optname,
                              void *optval, socklen_t *optlen,
                              socklen_t roptlen);


/**
 * Query options associated with a socket.
 * This operation takes place on RPC server side.
 *
 * @note For more information about supported option level, 
 * see tapi_rpcsock_defs.h.
 *
 * @param rpcs     RPC server handle
 * @param s        socket descriptor
 * @param level    protocol level at which the option resides. 
 *                 Following values can be specified:
 *                  - @b RPC_SOL_SOCKET  socket level
 *                  - @b RPC_SOL_IP      IPPROTO_IP level
 *                  - @b RPC_SOL_TCP     IPPROTO_TCP level
 * @param optname  Option name
 * @param optval   pointer to a buffer containing the value associated 
 *                 with the selected option.
 * @param optlen   initially points to the length of supplied buffer.
 *                 On return contain the actual size of the buffer.
 * @param roptlen  maximal length of the buffer or zero 
 *
 * @retval  0 on success
 *         -1 on failure
 */
static inline int
rpc_getsockopt(rcf_rpc_server *rpcs,
               int s, rpc_socklevel level, rpc_sockopt optname,
               void *optval, socklen_t *optlen)
{
    return rpc_getsockopt_gen(rpcs, s, level, optname, optval, optlen,
                              (optlen != NULL) ? *optlen : 0);
}

/**
 * Set options associated with a socket.
 * This operation takes place on RPC server side
 *
 * @note See @b setsockopt manual page for more information.
 * @param rpcs     RPC server handle.
 * @param s        socket descriptor 
 * @param level    protocol level at which the option resides. 
 *                 Following values can be specified:
 *                  - @b RPC_SOL_SOCKET  socket level
 *                  - @b RPC_SOL_IP      IPPROTO_IP level
 *                  - @b RPC_SOL_TCP     IPPROTO_TCP level
 * @param optname  Option name
 * @param optval   pointer to a buffer containing the value associated 
 *                 with the selected option.
 * @param optlen   initially point to the length of supplied buffer. On
 *                 contain the actual size of the @b optval buffer
 *
 * @retval   0 on success
 *          -1 on failure
 */
extern int rpc_setsockopt(rcf_rpc_server *rpcs,
                          int s, rpc_socklevel level, rpc_sockopt optname,
                          const void *optval, socklen_t optlen);


/**
 *  Query the current name of the specified socket on RPC server side,
 *  store this address in the @b sockaddr structure ponited by @b name,
 *  and store the lenght of this address to the object pointed by @b
 *  namelen.

 *
 *  @note See @b getsockname manual page for more information
 *  @param rpcs      RPC server handle
 *  @param s         Socket descriptor whose name is requested
 *  @param name      pointer to a @b sockaddr structure that should
 *                   contain the requested name
 *  @param namelen   pointer to the size of the @b sockaddr structure
 *  @param rnamelen  real size of the sockaddr structure
 *
 *  @return 0 on success, otherwise -1 is returned on failure
 */
extern int rpc_getsockname_gen(rcf_rpc_server *rpcs,
                               int s, struct sockaddr *name,
                               socklen_t *namelen,
                               socklen_t rnamelen);


/**
 * Query the name associated with a specified socket.
 *  
 * @param rpcs      RPC server handle
 * @param s         socket descriptor
 * @param name      pointer to a @b struct sockaddr which on return 
 *                   contain the name of the socket.
 * @param namelen   initially indicate the size of the parameter @b name.
 *                  On return it contains the actual size of the socket 
 *                  name
 *                   
 * @return  0 on success or -1 when failed.
 */
static inline int
rpc_getsockname(rcf_rpc_server *rpcs,
                int s, struct sockaddr *name, socklen_t *namelen)
{
    return rpc_getsockname_gen(rpcs, s, name, namelen,
                               (namelen == NULL) ? 0 : *namelen);
}

/**
 *  Query the peer name of a specified socket on RPC server side,
 *  store this address in the @b sockaddr structure ponited by @b name,
 *  and store the lenght of this address to the object pointed by @b
 *  namelen.
 *
 *  @note See @b getpeername manual page for more information
 *  @param rpcs      RPC server handle
 *  @param s         Socket descriptor whose peer name is requested
 *  @param name      pointer to a @b sockaddr structure that should
 *                   contain the requested peer name
 *  @param namelen   pointer to the size of the @b sockaddr structure
 *  @param rnamelen  real size of the @b sockaddr structure
 *
 *  @return 0 on success, otherwise -1 is returned on failure
 */
extern int rpc_getpeername_gen(rcf_rpc_server *rpcs,
                               int s, struct sockaddr *name,
                               socklen_t *namelen,
                               socklen_t rnamelen);


/**
 * Query the peer name of a specified socket
 * 
 * @param  rpcs     RPC server handle
 * @param  s        socket descriptor
 * @param  name     pointer to a buffer which on return contains the 
 *                  name of the peer socket.
 * @param  namelen  Initially indicate the size of the parameter @b name.
 *                  On return it contains the actual size of the socket name
 * 
 * @return   0 on success otherwise returns -1 on failure
 */
static inline int
rpc_getpeername(rcf_rpc_server *rpcs,
                int s, struct sockaddr *name, socklen_t *namelen)
{
    return rpc_getpeername_gen(rpcs, s, name, namelen,
                               (namelen == NULL) ? 0 : *namelen);
}


#endif /* !__TE_TAPI_RPC_SOCKET_H__ */
