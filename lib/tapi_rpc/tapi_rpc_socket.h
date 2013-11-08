/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of Socket API
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
#include "te_rpc_sys_socket.h"
#include "te_rpc_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_socket TAPI for socket API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * Get socket domain native for the address family.
 *
 * @param af        An address family
 *
 * @return Domain
 */
static inline int
socket_domain_by_af(int af)
{
    switch (af)
    {
        case AF_INET:   return PF_INET;
        case AF_INET6:  return PF_INET6;
        default:        return PF_MAX;
    }
}

/**
 * Get RPC socket domain native for the address family.
 *
 * @param af        An address family
 *
 * @return RPC domain
 */
static inline rpc_socket_domain
rpc_socket_domain_by_af(int af)
{
    return domain_h2rpc(socket_domain_by_af(af));
}

/**
 * Get RPC socket domain native for the address.
 *
 * @param addr      An address
 *
 * @return RPC domain
 */
static inline rpc_socket_domain
rpc_socket_domain_by_addr(const struct sockaddr *addr)
{
    return rpc_socket_domain_by_af(addr->sa_family);
}
 

/**
 * Create an endpoint for communication on the RPC server side. 
 *
 * @param rpcs      RPC server handle
 * @param domain    communication domain.
 *                  Select the protocol family used for communication
 *                  supported protocol families are difined in 
 *                  te_rpc_sys_socket.h
 * @param type      defines the semantic of communication. Current defined 
 *                  types can be found in te_rpc_sys_socket.h
 * @param protocol  specifies the protocol to be used. If @b protocol is 
 *                  set to RPC_PROTO_DEF, the system selects the default 
 *                  protocol number for the socket domain and type 
 *                  specified.
 * 
 * @return The socket descriptor to be used. Otherwise -1 is returned when 
 *         error occured
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
 * @return  0 on success
 *         -1 on error and error code is set appropriately to @b RPC_XXX 
 *         where @b XXX - standard errno for @b shutdown() 
 * @note For more information about @b shutdown() see its corresponding 
 *       manual page
 */
extern int rpc_shutdown(rcf_rpc_server *rpcs,
                        int s, rpc_shut_how how);

/**
 * Transmit a message to socket descriptor s on RPC server side.
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
                        rpc_send_recv_flags flags);

/**
 * Transmit a message to descriptor @b s.
 * This operation takes place on RPC server side.
 *
 * @param rpcs  RPC server handle
 * @param s     socket descriptor
 * @param buf   pointer to buffer containing message to be sent
 * @param len   message length in bytes
 * @param flags bitwise OR of zero or more of the following flags:
 *               - RPC_MSG_OOB send out-of-band data if supported.
 *               - RPC_MSG_DONTWAIT enable non-blocking operation.
 *               Other supported flags can be found in
 *               te_rpc_sys_socket.h.
 * @param to     target address
 *
 * @return Number of bytes sent, otherwise -1.
 *
 * @note See @b sendto() manual page for more information about flags and 
 *       error code set when this routine failed.
 */
extern ssize_t rpc_sendto(rcf_rpc_server *rpcs,
                          int s, const void *buf, size_t len,
                          rpc_send_recv_flags flags,
                          const struct sockaddr *to);

extern ssize_t rpc_sendto_raw(rcf_rpc_server *rpcs,
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
 * @param len     buffer length passed to recv()
 * @param flags   bitwise OR of zero or more of the following flags:
 *                 - RPC_MSG_OOB send out-of-band data if supported.
 *                 - RPC_MSG_DONTWAIT enable non-blocking operation.
 *                 Other supported flags can be found in
 *                 te_rpc_sys_socket.h
 * @param rbuflen size of the buffer @b buf
 *
 * @return Number of bytes received, otherwise -1 
 */

extern ssize_t rpc_recv_gen(rcf_rpc_server *rpcs,
                            int s, void *buf, size_t len,
                            rpc_send_recv_flags flags,
                            size_t rbuflen);

/**
 * Receive messages and store them in the buffer @b buf of length @b len.
 * This operation takes place on RPC server side.
 *
 * @param rpcs  RPC server handle
 * @param s     socket descriptor
 * @param buf   pointer to buffer which store received messages
 * @param len   size of the buffer @b buf
 * @param flags bitwise OR of zero or more of the following flags:
 *               - RPC_MSG_OOB send out-of-band data if supported.
 *               - RPC_MSG_DONTWAIT enable non-blocking operation.
 *               Other supported flags can be found in
 *               te_rpc_sys_socket.h
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
 *                   te_rpc_sys_socket.h
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
 *                   te_rpc_sys_socket.h
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
                            len, (from == NULL ||
                                  fromlen == NULL) ? 0 : *fromlen);
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

struct rpc_mmsghdr {
    struct rpc_msghdr msg_hdr;  /* Message header */
    unsigned int      msg_len;  /* Number of received bytes for header */
};

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
 * @return Length of message, otherwise -1 is returned when an error 
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
 * @param flags     bitwise OR of zero or more of the flags; 
 *                  see @b rpc_recv for more information
 *
 * @return Length of received data, otherwise -1 when an error occured.
 */
extern ssize_t rpc_recvmsg(rcf_rpc_server *rpcs,
                           int s, struct rpc_msghdr *msg,
                           rpc_send_recv_flags flags);

/**
 * Transmit a message to socket descriptor s on RPC server side.
 * This operation takes place on RPC server side and buffer is stored on
 * the same side.
 *
 * @param rpcs      RPC server handle
 * @param s         socket descriptor
 * @param buf       RPC pointer to buffer containing the message to be sent
 * @param buf_off   offset in the buffer above
 * @param len       length of the message in bytes
 * @param flags     bitwise OR of zero or more of the flags; 
 *                  see @b rpc_send for more information
 *
 * @return number of bytes actually sent, otherwise -1.
 * @note See @b send() manual page for more information
 */
extern ssize_t rpc_sendbuf_gen(rcf_rpc_server *rpcs, 
                               int s, rpc_ptr buf, size_t buf_off, 
                               size_t len, rpc_send_recv_flags flags);
static inline ssize_t
rpc_sendbuf(rcf_rpc_server *rpcs, int s, rpc_ptr buf, size_t len, 
            rpc_send_recv_flags flags)
{
    return rpc_sendbuf_gen(rpcs, s, buf, 0, len, flags);
}
static inline ssize_t
rpc_sendbuf_off(rcf_rpc_server *rpcs, int s, rpc_ptr_off *buf, size_t len, 
                rpc_send_recv_flags flags)
{
    return rpc_sendbuf_gen(rpcs, s, buf->base, buf->offset, len, flags);
}

/**
 * Transmit 2 messages to socket descriptor @b s on RPC server side by 
 * using 2 send calls. The first send call transmit first @b first_len
 * bytes of @b buf with MSG_MORE flag, the second send call transmit
 * next @b second_len bytes of @b buf with zero flag. This operation 
 * takes place on RPC serverside and buffer is stored on the same side.
 *
 * @param rpcs       RPC server handle
 * @param s          socket descriptor
 * @param buf        RPC pointer to buffer containing the message to be sent
 * @param first_len  length of the first message in bytes
 * @param second_len length of the second message in bytes
 *
 * @return On succes, number of bytes actually sent, otherwise -1.
 * @note See @b send() manual page for more information
 */
extern ssize_t rpc_send_msg_more(rcf_rpc_server *rpcs, int s, rpc_ptr buf, 
                                size_t first_len, size_t second_len);


/**
 * Receive messages and store them in the buffer @b buf of length @b len.
 * This operation takes place on RPC server side and buffer is stored on
 * the same side.
 *
 * @param rpcs  RPC server handle
 * @param s     socket descriptor
 * @param buf   RPC pointer to buffer which store received messages
 * @param len   size of the buffer @b buf
 * @param flags bitwise OR of zero or more of the following flags:
 *               - RPC_MSG_OOB send out-of-band data if supported.
 *               - RPC_MSG_DONTWAIT enable non-blocking operation.
 *               Other supported flags can be found in
 *               te_rpc_sys_socket.h
 *
 * @return Number of bytes received, otherwise -1 when error occured
 */
extern tarpc_ssize_t rpc_recvbuf_gen(rcf_rpc_server *rpcs,
                                     int fd, rpc_ptr buf, size_t buf_off,
                                     size_t count,
                                     rpc_send_recv_flags flags);
static inline tarpc_ssize_t
rpc_recvbuf(rcf_rpc_server *rpcs, int fd, rpc_ptr buf,
            size_t count, rpc_send_recv_flags flags)
{
    return rpc_recvbuf_gen(rpcs, fd, buf, 0, count, flags);
}
static inline tarpc_ssize_t
rpc_recvbuf_off(rcf_rpc_server *rpcs, int fd, rpc_ptr_off *buf,
                size_t count, rpc_send_recv_flags flags)
{
    return rpc_recvbuf_gen(rpcs, fd, buf->base, buf->offset, count, flags);
}

/**
 * Parse TA-dependent in_pktinfo structure type data returned 
 * in msg_control data field of rpc_msghdr during rpc_recvmsg() 
 * call in case when IP_PKTINFO option is enabled on socket.
 *
 * @param rpcs          RPC server handle
 * @param data          msg_control data
 * @param data_len      msg_control data length
 * @param ipi_spec_dst  ipi_spec_dst value of in_pktinfo type data (OUT)
 * @param ipi_addr      ipi_addr value of in_pktinfo type data (OUT)
 * @param ipi_ifindex   ipi_ifindex value of in_pktinfo type data (OUT) 
 *  
 * @return Status code
 */ 
extern int rpc_cmsg_data_parse_ip_pktinfo(rcf_rpc_server *rpcs,
                                          uint8_t *data, uint32_t data_len,
                                          struct in_addr *ipi_spec_dst,
                                          struct in_addr *ipi_addr,
                                          int *ipi_ifindex);
/**
 * Assign a name to an unnamed socket. When o socket is created, it exists
 * in an address family, but it does not have a name assigned to it. 
 * Servers use @b bind() to associate themselves with a well-known port.
 * This operation takes place on RPC server side.
 *
 * @param rpcs      RPC server handle
 * @param s         socket descriptor
 * @param my_addr   pointer to a @b sockaddr structure
 *
 * @return 0 on success or -1 on failure 
 * @note See @b bind manual page for more infrormation.
 */
extern int rpc_bind(rcf_rpc_server *rpcs,
                    int s, const struct sockaddr *my_addr);

/**
 * The same as rpc_bind(), but there is @a addrlen parameter. It is used to
 * specify length to be passed to bind().
 */
extern int rpc_bind_len(rcf_rpc_server *rpcs, int s,
                        const struct sockaddr *my_addr, socklen_t addrlen);

extern int rpc_bind_raw(rcf_rpc_server *rpcs, int s,
                        const struct sockaddr *my_addr, socklen_t addrlen);

/**
 * Check that given @p port is free on the given @p pco.
 */
extern int rpc_check_port_is_free(rcf_rpc_server *rpcs, uint16_t port);

/**
 * Attempt to associate a socket descriptor @b s with a peer process at 
 * address @b addr. This operation takes place on RPC server side.
 * 
 * @param rpcs      RPC server handle
 * @param s         socket descriptor
 * @param addr      peer address to which the socket has to be connected
 *
 * @return   0 on success or -1 on failure
 * @note   See @b connect manual page for more information
 */
extern int rpc_connect(rcf_rpc_server *rpcs,
                       int s, const struct sockaddr *addr);

extern int rpc_connect_raw(rcf_rpc_server *rpcs, int s,
                           const struct sockaddr *addr, socklen_t addrlen);

/**
 * Try to listen for incomming connections for connection oriented sockets
 * This operation takes place on RPC server side.
 *
 * @param rpcs        RPC server handle
 * @param s           listening socket descriptor
 * @param backlog     maximum number of connections waiting to be accepted
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_listen(rcf_rpc_server *rpcs,
                      int s, int backlog);

/**
 * This generic routine extract the first connection request of the queue
 * of pending connections, allocate a new descriptor for the connected 
 * socket. This operation takes place on RPC server side.
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
 * @return Socket for new connection or -1
 */
extern int rpc_accept_gen(rcf_rpc_server *rpcs,
                          int s, struct sockaddr *addr,
                          socklen_t *addrlen,
                          socklen_t raddrlen);


/**
 * This function does the same thing as rpc_gen_accept() but also take
 * flags parameter (where SOCK_CLOEXEC and/or SOCK_NONBLOCK flags can be
 * set). This operation takes place on RPC server side.
 *
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
 * @param flags    RPC_SOCK_NONBCLOCK, RPC_SOCK_CLOEXEC flags can be set
 *
 * @return Socket for new connection or -1
 */
extern int rpc_accept4_gen(rcf_rpc_server *rpcs,
                          int s, struct sockaddr *addr,
                          socklen_t *addrlen,
                          socklen_t raddrlen,
                          int flags);

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
 * @return Socket for new connection or -1
 */
static inline int
rpc_accept(rcf_rpc_server *rpcs,
           int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return rpc_accept_gen(rpcs, s, addr, addrlen,
                          (addr == NULL ||
                           addrlen == NULL) ? 0 : *addrlen);
}

/**
 * This function does the same thing as rpc_gen_accept() but also take
 * flags parameter (where SOCK_CLOEXEC and/or SOCK_NONBLOCK flags can be
 * set).
 *
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
 * @param flags    RPC_SOCK_NONBLOCK and/or RPC_SOCK_CLOEXEC
 *
 * @return Socket for new connection or -1
 */
static inline int
rpc_accept4(rcf_rpc_server *rpcs,
           int s, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
    return rpc_accept4_gen(rpcs, s, addr, addrlen,
                          (addr == NULL ||
                           addrlen == NULL) ? 0 : *addrlen,
                           flags);
}

typedef struct rpc_tcp_info {
    rpc_tcp_state   tcpi_state;
    uint8_t         tcpi_ca_state;
    uint8_t         tcpi_retransmits;
    uint8_t         tcpi_probes;
    uint8_t         tcpi_backoff;
    uint8_t         tcpi_options;
    uint8_t         tcpi_snd_wscale : 4, tcpi_rcv_wscale : 4;

    uint32_t        tcpi_rto;
    uint32_t        tcpi_ato;
    uint32_t        tcpi_snd_mss;
    uint32_t        tcpi_rcv_mss;

    uint32_t        tcpi_unacked;
    uint32_t        tcpi_sacked;
    uint32_t        tcpi_lost;
    uint32_t        tcpi_retrans;
    uint32_t        tcpi_fackets;

    /* Times. */
    uint32_t        tcpi_last_data_sent;
    uint32_t        tcpi_last_ack_sent;
    uint32_t        tcpi_last_data_recv;
    uint32_t        tcpi_last_ack_recv;

    /* Metrics. */
    uint32_t        tcpi_pmtu;
    uint32_t        tcpi_rcv_ssthresh;
    uint32_t        tcpi_rtt;
    uint32_t        tcpi_rttvar;
    uint32_t        tcpi_snd_ssthresh;
    uint32_t        tcpi_snd_cwnd;
    uint32_t        tcpi_advmss;
    uint32_t        tcpi_reordering;

    uint32_t        tcpi_rcv_rtt;
    uint32_t        tcpi_rcv_space;

    uint32_t        tcpi_total_retrans;
} rpc_tcp_info;

/** Storage sufficient for any fixed-size socket option value */
typedef union rpc_sockopt_value {
    int             v_int;
    tarpc_linger    v_linger;
    tarpc_timeval   v_tv;
    tarpc_mreqn     v_mreqn;
    struct in_addr  v_ip4addr;
    struct in6_addr v_ip6addr;
} rpc_sockopt_value;

/**
 * This generic routine gets options associated with a socket.
 * This operation takes place on RPC server side.
 *
 * @note For more information about supported option level, 
 * see te_rpc_sys_socket.h.
 *
 * @param rpcs     RPC server handle
 * @param s        socket descriptor
 * @param level    protocol level at which the option resides. 
 *                 Following values can be specified:
 *                  - @b RPC_SOL_SOCKET  socket level
 *                  - @b RPC_SOL_IP      IPPROTO_IP level
 *                  - @b RPC_SOL_IPV6    IPPROTO_IPV6 level
 *                  - @b RPC_SOL_TCP     IPPROTO_TCP level
 * @param optname  option name
 * @param optval   pointer to a buffer containing the value associated 
 *                 with the selected option.
 * @param optlen   initially points to the length of supplied buffer.
 *                 On return contain the actual size of the buffer.
 * @param roptlen  maximal length of the buffer or zero 
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_getsockopt_gen(rcf_rpc_server *rpcs,
                              int s, rpc_socklevel level,
                              rpc_sockopt optname,
                              void *optval, void *raw_optval,
                              socklen_t *raw_optlen,
                              socklen_t raw_roptlen);

static inline int
rpc_getsockopt_raw(rcf_rpc_server *rpcs,
                   int s, rpc_sockopt optname,
                   void *raw_optval, socklen_t *raw_optlen)
{
    return rpc_getsockopt_gen(rpcs, s, rpc_sockopt2level(optname),
                              optname, NULL, raw_optval, raw_optlen,
                              raw_optlen == NULL ? 0 : *raw_optlen);
}

/**
 * Query options associated with a socket. This operation takes place
 * on RPC server side.
 *
 * The function should be used for fixed-size options when valid option
 * level is required. RPC socket option name is unambiguously mapped to
 * socket option level (see rpc_sockopt2level()).
 *
 * @note For more information about supported option level, 
 * see te_rpc_sys_socket.h.
 *
 * @param rpcs      RPC server handle
 * @param s         Socket descriptor
 * @param optname   Option name
 * @param optval    Location for option value value (size of location
 *                  depends on option name and assumed to be sufficient)
 *
 * @return 0 on success or -1 on failure
 *
 * @sa rpc_getsockopt_gen()
 */
static inline int
rpc_getsockopt(rcf_rpc_server *rpcs,
               int s, rpc_sockopt optname, void *optval)
{
    return rpc_getsockopt_gen(rpcs, s, rpc_sockopt2level(optname),
                              optname, optval, NULL, NULL, 0);
}

/**
 * Set option on socket opened on RPC server.
 *
 * @param rpcs          RPC server handle
 * @param s             Socket descriptor
 * @param level         Protocol level at which the option resides
 * @param optname       Option name
 * @param optval        Pointer to option value to be set or NULL
 *                      (length is defined by option name)
 * @param raw_optval    Pointer to option value as sequence of bytes
 *                      (if @a optval is not NULL, @a raw_optval goes
 *                      after binary representation of @a optval)
 * @param raw_optlen    Size of the @a raw_optval specified in call
 * @param raw_roptlen   Real size of the @a raw_optval
 *
 * @return 0 on success or -1 on failure
 *
 * @sa rpc_setsockopt(), rpc_setsockopt_raw()
 */
extern int rpc_setsockopt_gen(rcf_rpc_server *rpcs,
                              int s, rpc_socklevel level,
                              rpc_sockopt optname,
                              const void *optval,
                              const void *raw_optval,
                              socklen_t raw_optlen,
                              socklen_t raw_roptlen);

/**
 * Set option on socket opened on RPC server using raw option value
 * representation.
 *
 * See rpc_setsockopt_gen() for parameters description. @a level is
 * derived from @a optname using rpc_sockopt2level() function.
 *
 * @return 0 on success or -1 on failure
 *
 * @sa rpc_setsockopt_gen()
 */
static inline int
rpc_setsockopt_raw(rcf_rpc_server *rpcs,
                   int s, rpc_sockopt optname,
                   const void *raw_optval, socklen_t raw_optlen)
{
    return rpc_setsockopt_gen(rpcs, s, rpc_sockopt2level(optname),
                              optname, NULL, raw_optval, raw_optlen,
                              raw_optlen);
}

/**
 * Set option on socket opened on RPC server using convered value only.
 *
 * See rpc_setsockopt_gen() for parameters description. @a level is
 * derived from @a optname using rpc_sockopt2level() function.
 * @a raw_optval and @a raw_optlen are passed as @c NULL and @c 0
 * correspondingly.
 *
 * @return 0 on success or -1 on failure
 *
 * @sa rpc_setsockopt_gen()
 */
static inline int
rpc_setsockopt(rcf_rpc_server *rpcs,
               int s, rpc_sockopt optname, const void *optval)
{
    return rpc_setsockopt_gen(rpcs, s, rpc_sockopt2level(optname),
                              optname, optval, NULL, 0, 0);
}


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
                               (name == NULL ||
                                namelen == NULL) ? 0 : *namelen);
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
 * Query the peer name of a specified socket.
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
                               (name == NULL ||
                                namelen == NULL) ? 0 : *namelen);
}

/**
 * Receive data from connected or non-connected socket
 * This operation takes place on RPC server side.
 *
 * @param rpcs      RPC server handle
 * @param fd        file descriptor
 * @param mmsg      array of @b rpc_mmsghdr structures that holds the
 *                  received messages
 * @param vlen      length of mmsg array
 * @param flags     bitwise OR of zero or more of the flags; 
 *                  see @b rpc_recv for more information
 * @param timeout   Timeout for receiving
 *
 * @return Number of received packets, otherwise -1 when an error occured.
 */
extern int rpc_recvmmsg_alt(rcf_rpc_server *rpcs, int fd,
                            struct rpc_mmsghdr *mmsg, unsigned int vlen,
                            rpc_send_recv_flags flags,
                            struct tarpc_timespec *timeout);

/**
 * Send data from connected or non-connected socket with help of
 * sendmmsg() function. This operation takes place on RPC server side.
 *
 * @param rpcs      RPC server handle
 * @param fd        file descriptor
 * @param mmsg      array of @b rpc_mmsghdr structures that holds the
 *                  messages to be sent
 * @param vlen      length of mmsg array
 * @param flags     bitwise OR of zero or more of the flags; 
 *                  see @b rpc_send for more information
 *
 * @return Number of sent packets, otherwise -1 when an error occured.
 *
 * @se msg_len field of each message will be set to number of actually
 *     sent bytes
 */
extern int rpc_sendmmsg_alt(rcf_rpc_server *rpcs, int fd,
                            struct rpc_mmsghdr *mmsg, unsigned int vlen,
                            rpc_send_recv_flags flags);

extern int rpc_socket_connect_close(rcf_rpc_server *rpcs,
                                    const struct sockaddr *addr,
                                    uint32_t time2run);

extern int rpc_socket_listen_close(rcf_rpc_server *rpcs,
                                   const struct sockaddr *addr,
                                   uint32_t time2run);
/**@} <!-- END te_lib_rpc_socket --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_SOCKET_H__ */
