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


extern int rpc_socket(rcf_rpc_server *rpcs,
                      rpc_socket_domain domain, rpc_socket_type type,
                      rpc_socket_proto protocol);


extern int rpc_shutdown(rcf_rpc_server *rpcs,
                        int s, rpc_shut_how how);


extern ssize_t rpc_send(rcf_rpc_server *rpcs,
                        int s, const void *buf, size_t len,
                        int flags);

extern ssize_t rpc_sendto(rcf_rpc_server *rpcs,
                          int s, const void *buf, size_t len,
                          int flags,
                          const struct sockaddr *to, socklen_t tolen);


extern ssize_t rpc_recv_gen(rcf_rpc_server *rpcs,
                            int s, void *buf, size_t len,
                            rpc_send_recv_flags flags,
                            size_t rbuflen);

static inline ssize_t
rpc_recv(rcf_rpc_server *rpcs,
         int s, void *buf, size_t len, rpc_send_recv_flags flags)
{
    return rpc_recv_gen(rpcs, s, buf, len, flags, len);
}

extern ssize_t rpc_recvfrom_gen(rcf_rpc_server *rpcs,
                                int s, void *buf, size_t len,
                                rpc_send_recv_flags flags,
                                struct sockaddr *from, socklen_t *fromlen,
                                size_t rbuflen, socklen_t rfrombuflen);

static inline ssize_t
rpc_recvfrom(rcf_rpc_server *rpcs,
             int s, void *buf, size_t len,
             rpc_send_recv_flags flags,
             struct sockaddr *from, socklen_t *fromlen)
{
    return rpc_recvfrom_gen(rpcs, s, buf, len, flags, from, fromlen,
                            len, (fromlen == NULL) ? 0 : *fromlen);
}


typedef struct rpc_msghdr {
    void             *msg_name;         /**< protocol address */
    socklen_t         msg_namelen;      /**< size of protocol address */
    struct rpc_iovec *msg_iov;          /**< scatter/gather array */
    size_t            msg_iovlen;       /**< # elements in msg_iov */
    void             *msg_control;      /**< ancillary data; must be
                                             aligned for a cmsghdr
                                             stucture */
    socklen_t            msg_controllen; /**< length of ancillary data */
    rpc_send_recv_flags  msg_flags;      /**< flags returned by recvmsg() */

    /* Non-standard fields for test purposes */
    socklen_t         msg_rnamelen;     /**< real size of protocol
                                             address buffer to be copied
                                             by RPC */
    size_t            msg_riovlen;      /**< real number of elements
                                             in msg_iov */
    socklen_t         msg_rcontrollen;  /**< real length of the
                                             ancillary data buffer bo be
                                             copied by RPC */
} rpc_msghdr;

extern ssize_t rpc_sendmsg(rcf_rpc_server *rpcs,
                           int s, const struct rpc_msghdr *msg,
                           rpc_send_recv_flags flags);

extern ssize_t rpc_recvmsg(rcf_rpc_server *rpcs,
                           int s, struct rpc_msghdr *msg,
                           rpc_send_recv_flags flags);


extern int rpc_bind(rcf_rpc_server *rpcs,
                    int s, const struct sockaddr *my_addr,
                    socklen_t addrlen);

extern int rpc_connect(rcf_rpc_server *rpcs,
                       int s, const struct sockaddr *addr,
                       socklen_t addrlen);

extern int rpc_listen(rcf_rpc_server *rpcs,
                      int s, int backlog);


extern int rpc_accept_gen(rcf_rpc_server *rpcs,
                          int s, struct sockaddr *addr,
                          socklen_t *addrlen,
                          socklen_t raddrlen);

static inline int
rpc_accept(rcf_rpc_server *rpcs,
           int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return rpc_accept_gen(rpcs, s, addr, addrlen,
                          (addrlen == NULL) ? 0 : *addrlen);
}

extern int rpc_getsockopt_gen(rcf_rpc_server *rpcs,
                              int s, rpc_socklevel level,
                              rpc_sockopt optname,
                              void *optval, socklen_t *optlen,
                              socklen_t roptlen);

static inline int
rpc_getsockopt(rcf_rpc_server *rpcs,
               int s, rpc_socklevel level, rpc_sockopt optname,
               void *optval, socklen_t *optlen)
{
    return rpc_getsockopt_gen(rpcs, s, level, optname, optval, optlen,
                              (optlen != NULL) ? *optlen : 0);
}


extern int rpc_setsockopt(rcf_rpc_server *rpcs,
                          int s, rpc_socklevel level, rpc_sockopt optname,
                          const void *optval, socklen_t optlen);


extern int rpc_getsockname_gen(rcf_rpc_server *rpcs,
                               int s, struct sockaddr *name,
                               socklen_t *namelen,
                               socklen_t rnamelen);

static inline int
rpc_getsockname(rcf_rpc_server *rpcs,
                int s, struct sockaddr *name, socklen_t *namelen)
{
    return rpc_getsockname_gen(rpcs, s, name, namelen,
                               (namelen == NULL) ? 0 : *namelen);
}

extern int rpc_getpeername_gen(rcf_rpc_server *rpcs,
                               int s, struct sockaddr *name,
                               socklen_t *namelen,
                               socklen_t rnamelen);

static inline int
rpc_getpeername(rcf_rpc_server *rpcs,
                int s, struct sockaddr *name, socklen_t *namelen)
{
    return rpc_getpeername_gen(rpcs, s, name, namelen,
                               (namelen == NULL) ? 0 : *namelen);
}


#endif /* !__TE_TAPI_RPC_SOCKET_H__ */
