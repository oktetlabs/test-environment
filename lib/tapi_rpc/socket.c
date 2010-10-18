/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of Socket API
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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

#include "te_config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#else
#define IFNAMSIZ        16
#endif

#include "te_printf.h"
#include "log_bufs.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_socket.h"
#include "tapi_rpcsock_macros.h"


int
rpc_socket(rcf_rpc_server *rpcs,
           rpc_socket_domain domain, rpc_socket_type type,
           rpc_socket_proto protocol)
{
    tarpc_socket_in  in;
    tarpc_socket_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(socket, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.domain = domain;
    in.type = type;
    in.proto = protocol;

    rcf_rpc_call(rpcs, "socket", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(socket, out.fd);

    TAPI_RPC_LOG("RPC (%s,%s): socket(%s, %s, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 domain_rpc2str(domain), socktype_rpc2str(type),
                 proto_rpc2str(protocol),
                 out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(socket, out.fd);
}

int
rpc_bind(rcf_rpc_server *rpcs,
         int s, const struct sockaddr *my_addr)
{
    tarpc_bind_in  in;
    tarpc_bind_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(bind, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.fd = s;
    sockaddr_input_h2rpc(my_addr, &in.addr);

    rcf_rpc_call(rpcs, "bind", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(bind, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): bind(%d, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 s, sockaddr_h2str(my_addr),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(bind, out.retval);
}

int rpc_bind_raw(rcf_rpc_server *rpcs, int s,
                 const struct sockaddr *my_addr, socklen_t addrlen)
{
    tarpc_bind_in  in;
    tarpc_bind_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(bind, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.fd = s;
    sockaddr_raw2rpc(my_addr, addrlen, &in.addr);

    rcf_rpc_call(rpcs, "bind", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(bind, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): bind(%d, %p, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 s, my_addr, addrlen, out.retval,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(bind, out.retval);
}

int
rpc_connect(rcf_rpc_server *rpcs,
            int s, const struct sockaddr *addr)
{
    rcf_rpc_op        op;
    tarpc_connect_in  in;
    tarpc_connect_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(connect, -1);
    }

    op = rpcs->op;
    in.fd = s;
    sockaddr_input_h2rpc(addr, &in.addr);

    rcf_rpc_call(rpcs, "connect", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(connect, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: connect(%d, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, sockaddr_h2str(addr),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(connect, out.retval);
}

int
rpc_connect_raw(rcf_rpc_server *rpcs, int s,
            const struct sockaddr *addr, socklen_t addrlen)
{
    rcf_rpc_op        op;
    tarpc_connect_in  in;
    tarpc_connect_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(connect, -1);
    }

    op = rpcs->op;
    in.fd = s;
    sockaddr_raw2rpc(addr, addrlen, &in.addr);

    rcf_rpc_call(rpcs, "connect", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(connect, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: connect(%d, %p, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, addr, addrlen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(connect, out.retval);
}

int
rpc_listen(rcf_rpc_server *rpcs, int fd, int backlog)
{
    tarpc_listen_in  in;
    tarpc_listen_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(listen, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.fd = fd;
    in.backlog = backlog;

    rcf_rpc_call(rpcs, "listen", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(listen, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): listen(%d, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name, fd, backlog,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(listen, out.retval);
}

int
rpc_accept_gen(rcf_rpc_server *rpcs,
               int s, struct sockaddr *addr, socklen_t *addrlen,
               socklen_t raddrlen)
{
    rcf_rpc_op       op;
    socklen_t        save_addrlen =
                         (addrlen == NULL) ? (socklen_t)-1 : *addrlen;
    tarpc_accept_in  in;
    tarpc_accept_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(accept, -1);
    }

    op = rpcs->op;
    if (addr != NULL && addrlen != NULL && *addrlen > raddrlen)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(accept, -1);
    }

    in.fd = s;
    if (addrlen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.len.len_len = 1;
        in.len.len_val = addrlen;
    }
    if (rpcs->op != RCF_RPC_WAIT)
    {
        sockaddr_raw2rpc(addr, raddrlen, &in.addr);
    }

    rcf_rpc_call(rpcs, "accept", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
    {
        sockaddr_rpc2h(&out.addr, addr, raddrlen,
                       NULL, addrlen);

        if (addrlen != NULL && out.len.len_val != NULL)
            *addrlen = out.len.len_val[0];
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(accept, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: accept(%d, %p[%u], %p(%u)) -> %d (%s) "
                 "peer=%s addrlen=%u",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, addr, raddrlen, addrlen, save_addrlen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 sockaddr_h2str(addr),
                 (addrlen == NULL) ? (socklen_t)-1 : *addrlen);

    RETVAL_INT(accept, out.retval);
}

ssize_t
rpc_recvfrom_gen(rcf_rpc_server *rpcs,
                 int s, void *buf, size_t len, rpc_send_recv_flags flags,
                 struct sockaddr *from, socklen_t *fromlen,
                 size_t rbuflen, socklen_t rfrombuflen)
{
    rcf_rpc_op         op;
    socklen_t          save_fromlen =
                           (fromlen == NULL) ? (socklen_t)-1 : *fromlen;

    tarpc_recvfrom_in  in;
    tarpc_recvfrom_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recvfrom, -1);
    }

    op = rpcs->op;

    if ((from != NULL && fromlen != NULL && *fromlen > rfrombuflen) ||
        (buf != NULL && len > rbuflen))
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(recvfrom, -1);
    }

    in.fd = s;
    in.len = len;
    if (fromlen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.fromlen.fromlen_len = 1;
        in.fromlen.fromlen_val = fromlen;
    }
    if (rpcs->op != RCF_RPC_WAIT)
    {
        sockaddr_raw2rpc(from, rfrombuflen, &in.from);
    }
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = rbuflen;
        in.buf.buf_val = buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "recvfrom", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);

        sockaddr_rpc2h(&out.from, from, rfrombuflen,
                       NULL, fromlen);

        if (fromlen != NULL && out.fromlen.fromlen_val != NULL)
            *fromlen = out.fromlen.fromlen_val[0];
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(recvfrom, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: recvfrom(%d, %p[%u], %u, %s, %p[%u], %d) "
                 "-> %d (%s) from=%s fromlen=%d",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, buf, rbuflen, len, send_recv_flags_rpc2str(flags),
                 from, rfrombuflen, (int)save_fromlen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 sockaddr_h2str(from),
                 (fromlen == NULL) ? -1 : (int)*fromlen);

    RETVAL_INT(recvfrom, out.retval);
}

ssize_t
rpc_recv_gen(rcf_rpc_server *rpcs,
                 int s, void *buf, size_t len, rpc_send_recv_flags flags,
                 size_t rbuflen)
{
    rcf_rpc_op     op;
    tarpc_recv_in  in;
    tarpc_recv_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recv, -1);
    }

    op = rpcs->op;

    if (buf != NULL && len > rbuflen)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(recv, -1);
    }

    in.fd = s;
    in.len = len;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = rbuflen;
        in.buf.buf_val = buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "recv", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(recv, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: recv(%d, %p[%u], %u, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, buf, rbuflen, len, send_recv_flags_rpc2str(flags),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(recv, out.retval);
}

tarpc_ssize_t
rpc_recvbuf_gen(rcf_rpc_server *rpcs, int fd, rpc_ptr buf, size_t buf_off,
                size_t count, rpc_send_recv_flags flags)
{
    rcf_rpc_op      op;
    tarpc_recvbuf_in  in;
    tarpc_recvbuf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(write, -1);
    }

    op = rpcs->op;

    in.fd  = fd;
    in.len = count;
    in.buf = buf;
    in.off = buf_off;

    rcf_rpc_call(rpcs, "recvbuf", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(readbuf, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: recvbuf(%d, %u (off %u), %u, %s) "
                 "-> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, buf, buf_off, count, send_recv_flags_rpc2str(flags),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(recvbuf, out.retval);
}

int
rpc_shutdown(rcf_rpc_server *rpcs, int s, rpc_shut_how how)
{
    tarpc_shutdown_in  in;
    tarpc_shutdown_out out;

    memset((char *)&in, 0, sizeof(in));
    memset((char *)&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(shutdown, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.fd = s;
    in.how = how;

    rcf_rpc_call(rpcs, "shutdown", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(shutdown, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s) shutdown(%d, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, s, shut_how_rpc2str(how),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(shutdown, out.retval);
}

ssize_t
rpc_sendto(rcf_rpc_server *rpcs,
           int s, const void *buf, size_t len,
           rpc_send_recv_flags flags,
           const struct sockaddr *to)
{
    rcf_rpc_op       op;
    tarpc_sendto_in  in;
    tarpc_sendto_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendto, -1);
    }

    op = rpcs->op;

    in.fd = s;
    in.len = len;
    if (rpcs->op != RCF_RPC_WAIT)
    {
        sockaddr_input_h2rpc(to, &in.to);
    }
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = len;
        in.buf.buf_val = (uint8_t *)buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "sendto", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendto, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: sendto(%d, %p, %u, %s, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, buf, len, send_recv_flags_rpc2str(flags),
                 sockaddr_h2str(to),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(sendto, out.retval);
}

ssize_t
rpc_sendto_raw(rcf_rpc_server *rpcs,
               int s, const void *buf, size_t len,
               rpc_send_recv_flags flags,
               const struct sockaddr *to, socklen_t tolen)
{
    rcf_rpc_op       op;
    tarpc_sendto_in  in;
    tarpc_sendto_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendto, -1);
    }

    op = rpcs->op;

    in.fd = s;
    in.len = len;
    if (rpcs->op != RCF_RPC_WAIT)
    {
        sockaddr_raw2rpc(to, tolen, &in.to);
    }
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = len;
        in.buf.buf_val = (uint8_t *)buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "sendto", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendto, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: sendto(%d, %p, %u, %s, %p, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, buf, len, send_recv_flags_rpc2str(flags),
                 to, tolen, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(sendto, out.retval);
}

ssize_t
rpc_send(rcf_rpc_server *rpcs,
           int s, const void *buf, size_t len,
           rpc_send_recv_flags flags)
{
    rcf_rpc_op     op;
    tarpc_send_in  in;
    tarpc_send_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(send, -1);
    }

    op = rpcs->op;

    in.fd = s;
    in.len = len;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = len;
        in.buf.buf_val = (uint8_t *)buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "send", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(send, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: send(%d, %p, %u, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, buf, len, send_recv_flags_rpc2str(flags),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(send, out.retval);
}

ssize_t
rpc_sendbuf_gen(rcf_rpc_server *rpcs, int s, rpc_ptr buf, size_t buf_off,
                size_t len, rpc_send_recv_flags flags)
{
    rcf_rpc_op     op;
    tarpc_sendbuf_in  in;
    tarpc_sendbuf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(send, -1);
    }

    op = rpcs->op;

    in.fd = s;
    in.len = len;
    in.buf = buf;
    in.off = buf_off;
    in.flags = flags;

    rcf_rpc_call(rpcs, "sendbuf", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(send, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: sendbuf(%d, %u (off %u), %u, %s) "
                 "-> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, buf, buf_off, len, send_recv_flags_rpc2str(flags),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(sendbuf, out.retval);
}

ssize_t
rpc_send_msg_more(rcf_rpc_server *rpcs, int s, rpc_ptr buf,
                size_t first_len, size_t second_len)
{
    rcf_rpc_op     op;
    tarpc_send_msg_more_in  in;
    tarpc_send_msg_more_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(send, -1);
    }

    op = rpcs->op;

    in.fd = s;
    in.first_len = first_len;
    in.second_len = second_len;
    in.buf = buf;

    rcf_rpc_call(rpcs, "send_msg_more", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(send_msg_more, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: send_msg_more(%d, %u , %u, %u) "
                 "-> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, buf, first_len, second_len,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(send_msg_more, out.retval);
}

ssize_t
rpc_sendmsg(rcf_rpc_server *rpcs,
            int s, const struct rpc_msghdr *msg, rpc_send_recv_flags flags)
{
    rcf_rpc_op        op;
    tarpc_sendmsg_in  in;
    tarpc_sendmsg_out out;

    struct tarpc_msghdr rpc_msg;

    struct tarpc_iovec   iovec_arr[RCF_RPC_MAX_IOVEC];
    struct tarpc_cmsghdr cmsg_hdrs[RCF_RPC_MAX_CMSGHDR];

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));
    memset(cmsg_hdrs, 0, sizeof(cmsg_hdrs));
    memset(&rpc_msg, 0, sizeof(rpc_msg));

    size_t i;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendmsg, -1);
    }

    op = rpcs->op;
    in.s = s;
    in.flags = flags;

    if (msg != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.msg.msg_val = &rpc_msg;
        in.msg.msg_len = 1;

        if (msg->msg_riovlen > RCF_RPC_MAX_IOVEC)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
            ERROR("Length of the I/O vector is too long (%u) - "
                  "increase RCF_RPC_MAX_IOVEC(%u)",
                  msg->msg_riovlen, RCF_RPC_MAX_IOVEC);
            RETVAL_INT(sendmsg, -1);
        }

        if (((msg->msg_iov != NULL) &&
             (msg->msg_iovlen > msg->msg_riovlen)))
        {
            ERROR("Inconsistent real and declared lengths of buffers");
            rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
            RETVAL_INT(sendmsg, -1);
        }

        for (i = 0; i < msg->msg_riovlen && msg->msg_iov != NULL; i++)
        {
            iovec_arr[i].iov_base.iov_base_val = msg->msg_iov[i].iov_base;
            iovec_arr[i].iov_base.iov_base_len = msg->msg_iov[i].iov_rlen;
            iovec_arr[i].iov_len = msg->msg_iov[i].iov_len;
        }

        if (msg->msg_iov != NULL)
        {
            rpc_msg.msg_iov.msg_iov_val = iovec_arr;
            rpc_msg.msg_iov.msg_iov_len = msg->msg_riovlen;
        }
        rpc_msg.msg_iovlen = msg->msg_iovlen;

        if (msg->msg_namelen <= sizeof(struct sockaddr_storage))
            sockaddr_input_h2rpc(msg->msg_name, &rpc_msg.msg_name);
        else
        {
            rpc_msg.msg_namelen = msg->msg_namelen;
            sockaddr_raw2rpc(msg->msg_name, msg->msg_rnamelen,
                             &rpc_msg.msg_name);
        }

        rpc_msg.msg_flags = (int)msg->msg_flags;

        if (msg->msg_control != NULL)
        {
            struct cmsghdr *c;
            unsigned int    i;

            struct tarpc_cmsghdr *rpc_c = 
                rpc_msg.msg_control.msg_control_val;

            rpc_msg.msg_control.msg_control_val = cmsg_hdrs;
            for (i = 0, c = CMSG_FIRSTHDR((struct msghdr *)msg); 
                 i < RCF_RPC_MAX_CMSGHDR && c != NULL;
                 i++, c = CMSG_NXTHDR((struct msghdr *)msg, c))
            {
                uint8_t *data = CMSG_DATA(c);
                
                if ((int)(c->cmsg_len) < (uint8_t *)c - data)
                {
                    ERROR("Incorrect content of cmsg is not supported");
                    rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
                    RETVAL_INT(sendmsg, -1);
                }
                
                rpc_c->level = socklevel_h2rpc(c->cmsg_level);
                rpc_c->type = sockopt_h2rpc(c->cmsg_level, c->cmsg_type);
                if ((rpc_c->data.data_len = 
                         c->cmsg_len - ((uint8_t *)c - data)) > 0)
                {
                    rpc_c->data.data_val = data;
                }
            }
            
            if (i == RCF_RPC_MAX_CMSGHDR)
            {
                rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
                ERROR("Too many cmsg headers - increase "
                      "RCF_RPC_MAX_CMSGHDR");
                RETVAL_INT(sendmsg, -1);
            }
            
            if (i == 0)
            {
                WARN("Incorrect msg_control length is not supported"); 
                rpc_msg.msg_control.msg_control_val = NULL;
            }
            rpc_msg.msg_control.msg_control_len = i;
        }
    }

    rcf_rpc_call(rpcs, "sendmsg", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendmsg, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: sendmsg(%d, %p "
                                    "(msg_name: %p, "
                                "msg_namelen: %" TE_PRINTF_SOCKLEN_T "d, "
                                    " msg_iov: %p, msg_iovlen: %d,"
                                    " msg_control: %p, msg_controllen: %d,"
                                    " msg_flags: %s)"
         ", %s) -> %d (%s)",
         rpcs->ta, rpcs->name, rpcop2str(op),
         s, msg,
         msg != NULL ? msg->msg_name : NULL,
         msg != NULL ? msg->msg_namelen : -1,
         msg != NULL ? msg->msg_iov : NULL,
         msg != NULL ? (int)msg->msg_iovlen : -1,
         msg != NULL ? msg->msg_control : NULL,
         msg != NULL ? (int)msg->msg_controllen : -1,
         msg != NULL ? send_recv_flags_rpc2str(msg->msg_flags) : "",
         send_recv_flags_rpc2str(flags),
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(sendmsg, out.retval);
}

ssize_t
rpc_recvmsg(rcf_rpc_server *rpcs,
            int s, struct rpc_msghdr *msg, rpc_send_recv_flags flags)
{
    char              str_buf[1024];
    rcf_rpc_op        op;
    tarpc_recvmsg_in  in;
    tarpc_recvmsg_out out;

    struct tarpc_msghdr rpc_msg;

    struct tarpc_iovec   iovec_arr[RCF_RPC_MAX_IOVEC];
    struct tarpc_cmsghdr cmsg_hdrs[RCF_RPC_MAX_CMSGHDR];

    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));
    memset(&rpc_msg, 0, sizeof(rpc_msg));
    memset(cmsg_hdrs, 0, sizeof(cmsg_hdrs));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recvmsg, -1);
    }

    op = rpcs->op;
    in.s = s;
    in.flags = flags;

    if (msg != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.msg.msg_val = &rpc_msg;
        in.msg.msg_len = 1;

        if (msg->msg_riovlen > RCF_RPC_MAX_IOVEC)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
            ERROR("Length of the I/O vector is too long (%u) - "
                  "increase RCF_RPC_MAX_IOVEC(%u)",
                  msg->msg_riovlen, RCF_RPC_MAX_IOVEC);
            RETVAL_INT(recvmsg, -1);
        }

        if (msg->msg_cmsghdr_num > RCF_RPC_MAX_CMSGHDR)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
            ERROR("Too many cmsg headers - increase RCF_RPC_MAX_CMSGHDR");
            RETVAL_INT(recvmsg, -1);
        }
        
        if (msg->msg_control != NULL && msg->msg_cmsghdr_num == 0)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
            ERROR("Number of cmsg headers is incorrect");
            RETVAL_INT(recvmsg, -1);
        }

        if (msg->msg_iovlen > msg->msg_riovlen ||
            msg->msg_namelen > msg->msg_rnamelen)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
            RETVAL_INT(recvmsg, -1);
        }

        for (i = 0; i < msg->msg_riovlen && msg->msg_iov != NULL; i++)
        {
            iovec_arr[i].iov_base.iov_base_val = msg->msg_iov[i].iov_base;
            iovec_arr[i].iov_base.iov_base_len = msg->msg_iov[i].iov_rlen;
            iovec_arr[i].iov_len = msg->msg_iov[i].iov_len;
        }

        if (msg->msg_iov != NULL)
        {
            rpc_msg.msg_iov.msg_iov_val = iovec_arr;
            rpc_msg.msg_iov.msg_iov_len = msg->msg_riovlen;
        }
        rpc_msg.msg_iovlen = msg->msg_iovlen;

        rpc_msg.msg_namelen = msg->msg_namelen;
        sockaddr_raw2rpc(msg->msg_name, msg->msg_rnamelen,
                         &rpc_msg.msg_name);

        rpc_msg.msg_flags = msg->msg_flags;

        if (msg->msg_control != NULL)
        {
            rpc_msg.msg_control.msg_control_val = cmsg_hdrs;
            rpc_msg.msg_control.msg_control_len = msg->msg_cmsghdr_num;
            cmsg_hdrs[0].data.data_val = msg->msg_control;
            cmsg_hdrs[0].data.data_len = msg->msg_controllen -
                                         msg->msg_cmsghdr_num *
                                         CMSG_ALIGN(sizeof(struct cmsghdr));
        } 
    }

    rcf_rpc_call(rpcs, "recvmsg", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(recvmsg, out.retval);

    snprintf(str_buf, sizeof(str_buf),
             "RPC (%s,%s)%s: recvmsg(%d, %p(",
             rpcs->ta, rpcs->name, rpcop2str(op), s, msg);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT &&
        msg != NULL && out.msg.msg_val != NULL)
    {
        rpc_msg = out.msg.msg_val[0];
        
        sockaddr_rpc2h(&rpc_msg.msg_name, msg->msg_name,
                       msg->msg_rnamelen,
                       NULL, &msg->msg_namelen);
        msg->msg_namelen = rpc_msg.msg_namelen;

        for (i = 0; i < msg->msg_riovlen && msg->msg_iov != NULL; i++)
        {
            msg->msg_iov[i].iov_len =
                rpc_msg.msg_iov.msg_iov_val[i].iov_len;
            memcpy(msg->msg_iov[i].iov_base,
                   rpc_msg.msg_iov.msg_iov_val[i].iov_base.iov_base_val,
                   msg->msg_iov[i].iov_rlen);
        }
        if (msg->msg_control != NULL)
        {
            struct cmsghdr *c;
            unsigned int    i;
            
            struct tarpc_cmsghdr *rpc_c = 
                rpc_msg.msg_control.msg_control_val;
            
            for (i = 0, c = CMSG_FIRSTHDR((struct msghdr *)msg); 
                 i < rpc_msg.msg_control.msg_control_len && c != NULL; 
                 i++, c = CMSG_NXTHDR((struct msghdr *)msg, c), rpc_c++)
            {
                c->cmsg_level = socklevel_rpc2h(rpc_c->level);
                c->cmsg_type = sockopt_rpc2h(rpc_c->type);
                c->cmsg_len = CMSG_LEN(rpc_c->data.data_len);
                if (rpc_c->data.data_val != NULL)
                    memcpy(CMSG_DATA(c), rpc_c->data.data_val, 
                           rpc_c->data.data_len);
            }
            
            if (c == NULL && i < rpc_msg.msg_control.msg_control_len)
            {
                ERROR("Unexpected lack of space in auxiliary buffer");
                rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
                RETVAL_INT(recvmsg, -1);
            }
            
            if (c != NULL)
                msg->msg_controllen = (char *)c - (char *)msg->msg_control;
        }

        msg->msg_flags = (rpc_send_recv_flags)rpc_msg.msg_flags;

        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf),
                 "msg_name: %p, msg_namelen: %" TE_PRINTF_SOCKLEN_T "d, "
                 "msg_iov: %p, msg_iovlen: %" TE_PRINTF_SIZE_T "d, "
                 "msg_control: %p, msg_controllen: "
                 "%" TE_PRINTF_SOCKLEN_T "d, msg_flags: %s",
                 msg->msg_name, msg->msg_namelen,
                 msg->msg_iov, msg->msg_iovlen,
                 msg->msg_control, msg->msg_controllen,
                 send_recv_flags_rpc2str(msg->msg_flags));
    }

    snprintf(str_buf + strlen(str_buf), sizeof(str_buf) - strlen(str_buf),
             "), %s) -> %ld (%s)",
             send_recv_flags_rpc2str(flags),
             (long)out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    TAPI_RPC_LOG("%s", str_buf);

    RETVAL_INT(recvmsg, out.retval);
}


int 
rpc_cmsg_data_parse_ip_pktinfo(rcf_rpc_server *rpcs,
                               uint8_t *data, uint32_t data_len,
                               struct in_addr *ipi_addr,
                               int *ipi_ifindex)
{
    tarpc_cmsg_data_parse_ip_pktinfo_in  in;
    tarpc_cmsg_data_parse_ip_pktinfo_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(cmsg_data_parse_ip_pktinfo, -1);
    }

    if (data == NULL || data_len == 0 ||
        ipi_addr == NULL || ipi_ifindex == 0)
    {
        ERROR("%s(): Invalid parameters to function", __FUNCTION__);
        RETVAL_INT(cmsg_data_parse_ip_pktinfo, -1);
    }
    in.data.data_val = data;
    in.data.data_len = data_len;
    
    rcf_rpc_call(rpcs, "cmsg_data_parse_ip_pktinfo", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(cmsg_data_parse_ip_pktinfo, 
                                          out.retval);
    if (RPC_IS_CALL_OK(rpcs))
    {
        ipi_addr->s_addr = out.ipi_addr;
        *ipi_ifindex = out.ipi_ifindex;
    }
    TAPI_RPC_LOG("RPC (%s,%s): "
                 "cmsg_data_parse_ip_pktinfo(%p, %u, %p->%s, %p->%d)"
                 " -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 data, data_len, ipi_addr, 
                 (out.retval == 0) ? inet_ntoa(*ipi_addr) : "undetermined",
                 ipi_ifindex,
                 (out.retval == 0) ? *ipi_ifindex : 0,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(cmsg_data_parse_ip_pktinfo, out.retval);
}

int
rpc_getsockname_gen(rcf_rpc_server *rpcs,
                    int s, struct sockaddr *name,
                    socklen_t *namelen,
                    socklen_t rnamelen)
{
    socklen_t   namelen_save =
        (namelen == NULL) ? (unsigned int)-1 : *namelen;

    tarpc_getsockname_in  in;
    tarpc_getsockname_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(getsockname, -1);
    }
    if (name != NULL && namelen != NULL && *namelen > rnamelen)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(getsockname, -1);
    }
    rpcs->op = RCF_RPC_CALL_WAIT;

    in.fd = s;
    if (namelen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.len.len_len = 1;
        in.len.len_val = namelen;
    }
    sockaddr_raw2rpc(name, rnamelen, &in.addr);

    rcf_rpc_call(rpcs, "getsockname", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
    {
        sockaddr_rpc2h(&out.addr, name, rnamelen,
                       NULL, namelen);

        if (namelen != NULL && out.len.len_val != NULL)
            *namelen = out.len.len_val[0];
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(getsockname, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): getsockname(%d, %p[%u], %u) -> %d (%s) "
                 "name=%s namelen=%u",
                 rpcs->ta, rpcs->name,
                 s, name, rnamelen, namelen_save,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 sockaddr_h2str(name),
                 (namelen == NULL) ? (unsigned int)-1 : *namelen);

    RETVAL_INT(getsockname, out.retval);
}

int
rpc_getpeername_gen(rcf_rpc_server *rpcs,
                    int s, struct sockaddr *name,
                    socklen_t *namelen,
                    socklen_t rnamelen)
{
    socklen_t   namelen_save =
        (namelen == NULL) ? (unsigned int)-1 : *namelen;

    tarpc_getpeername_in  in;
    tarpc_getpeername_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(getpeername, -1);
    }

    if (name != NULL && namelen != NULL && *namelen > rnamelen)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(getpeername, -1);
    }
    rpcs->op = RCF_RPC_CALL_WAIT;

    in.fd = s;
    if (namelen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.len.len_len = 1;
        in.len.len_val = namelen;
    }
    sockaddr_raw2rpc(name, rnamelen, &in.addr);

    rcf_rpc_call(rpcs, "getpeername", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
    {
        sockaddr_rpc2h(&out.addr, name, rnamelen,
                       NULL, namelen);

        if (namelen != NULL && out.len.len_val != NULL)
            *namelen = out.len.len_val[0];
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(getpeername, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): getpeername(%d, %p[%u], %u) -> %d (%s) "
                 "name=%s namelen=%u",
                 rpcs->ta, rpcs->name,
                 s, name, rnamelen, namelen_save,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 sockaddr_h2str(name),
                 (namelen == NULL) ? (unsigned int)-1 : *namelen);

    RETVAL_INT(getpeername, out.retval);
}

int
rpc_getsockopt_gen(rcf_rpc_server *rpcs,
                   int s, rpc_socklevel level, rpc_sockopt optname,
                   void *optval, void *raw_optval,
                   socklen_t *raw_optlen, socklen_t raw_roptlen)
{
    tarpc_getsockopt_in   in;
    tarpc_getsockopt_out  out;    
    struct option_value   val;
    te_log_buf         *opt_val_str = NULL;
    char                  opt_len_str[32] = "(nil)";
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(getsockopt, -1);
    }

    /*
     * Validate roptlen vs *optlen.
     * roptlen makes sence, iff optval is not NULL.
     */
    if (raw_optlen != NULL && raw_optval != NULL &&
        *raw_optlen > raw_roptlen)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(getsockopt, -1);
    }

    /* Copy parameters to tarpc_getsockopt_in structure */
    rpcs->op = RCF_RPC_CALL_WAIT;
    in.s = s;
    in.level = level;
    in.optname = optname;

    if (optval != NULL || raw_optlen != NULL)
    {
        opt_len_str[0] = '\0';
        if (optval != NULL)
        {
            TE_SPRINTF(opt_len_str, "AUTO");
        }
        if (raw_optlen != NULL)
        {
            snprintf(opt_len_str + strlen(opt_len_str),
                     sizeof(opt_len_str) - strlen(opt_len_str),
                     "%s%u",  (optval != NULL) ? "+" : "",
                     (unsigned)*raw_optlen);
        }
    }

    if (optval != NULL)
    {
        memset(&val, 0, sizeof(val));

        in.optval.optval_len = 1;
        in.optval.optval_val = &val;

        switch (optname)
        {
            case RPC_SO_LINGER:
                val.opttype = OPT_LINGER;
                val.option_value_u.opt_linger.l_onoff =
                    ((tarpc_linger *)optval)->l_onoff;
                val.option_value_u.opt_linger.l_linger =
                    ((tarpc_linger *)optval)->l_linger;
                break;

            case RPC_SO_RCVTIMEO:
            case RPC_SO_SNDTIMEO:
                val.opttype = OPT_TIMEVAL;
                val.option_value_u.opt_timeval =
                    *((tarpc_timeval *)optval);
                break;

            case RPC_IPV6_PKTOPTIONS:
                ERROR("IPV6_PKTOPTIONS is not supported yet");
                RETVAL_INT(getsockopt, -1);
                break;
    
            case RPC_IPV6_ADD_MEMBERSHIP:
            case RPC_IPV6_DROP_MEMBERSHIP:
            case RPC_IPV6_JOIN_ANYCAST:
            case RPC_IPV6_LEAVE_ANYCAST:
            {
                struct ipv6_mreq *opt = (struct ipv6_mreq *)optval;
                
                val.opttype = OPT_MREQ6;

                memcpy(&val.option_value_u.opt_mreq6.ipv6mr_multiaddr.
                       ipv6mr_multiaddr_val,
                       &opt->ipv6mr_multiaddr, sizeof(struct in6_addr));
                val.option_value_u.opt_mreq6.ipv6mr_ifindex =
                    opt->ipv6mr_interface;
                break;
            }

            case RPC_IP_ADD_MEMBERSHIP:
            case RPC_IP_DROP_MEMBERSHIP:
            case RPC_IP_MULTICAST_IF:
            {
                tarpc_mreqn *opt = (tarpc_mreqn *)optval;

                val.opttype = opt->type; 
                switch (opt->type)
                {
                    case OPT_IPADDR:
                        memcpy(&val.option_value_u.opt_ipaddr,
                               &opt->address, sizeof(opt->address));
                        break;

                    case OPT_MREQ:
                        memcpy(&val.option_value_u.opt_mreq.imr_multiaddr,
                               &opt->multiaddr, sizeof(opt->multiaddr));
                        memcpy(&val.option_value_u.opt_mreq.imr_address,
                               &opt->address, sizeof(opt->address));
                        break;

                    case OPT_MREQN:
                        memcpy(&val.option_value_u.opt_mreqn.imr_multiaddr,
                               &opt->multiaddr, sizeof(opt->multiaddr));
                        memcpy(&val.option_value_u.opt_mreqn.imr_address,
                               &opt->address, sizeof(opt->address));
                        val.option_value_u.opt_mreqn.imr_ifindex =
                            opt->ifindex;
                        break;

                    default:
                        ERROR("Unknown option type for %s get request",
                              sockopt_rpc2str(optname));
                        break;
                }
                break;
            }

            case RPC_TCP_INFO:
                val.opttype = OPT_TCP_INFO;

#define COPY_TCP_INFO_FIELD(_name) \
    val.option_value_u.opt_tcp_info._name = \
        ((struct tcp_info *)optval)->_name

                COPY_TCP_INFO_FIELD(tcpi_state);
                COPY_TCP_INFO_FIELD(tcpi_ca_state);
                COPY_TCP_INFO_FIELD(tcpi_retransmits);
                COPY_TCP_INFO_FIELD(tcpi_probes);
                COPY_TCP_INFO_FIELD(tcpi_backoff);
                COPY_TCP_INFO_FIELD(tcpi_options);
                COPY_TCP_INFO_FIELD(tcpi_snd_wscale);
                COPY_TCP_INFO_FIELD(tcpi_rcv_wscale);
                COPY_TCP_INFO_FIELD(tcpi_rto);
                COPY_TCP_INFO_FIELD(tcpi_ato);
                COPY_TCP_INFO_FIELD(tcpi_snd_mss);
                COPY_TCP_INFO_FIELD(tcpi_rcv_mss);
                COPY_TCP_INFO_FIELD(tcpi_unacked);
                COPY_TCP_INFO_FIELD(tcpi_sacked);
                COPY_TCP_INFO_FIELD(tcpi_lost);
                COPY_TCP_INFO_FIELD(tcpi_retrans);
                COPY_TCP_INFO_FIELD(tcpi_fackets);
                COPY_TCP_INFO_FIELD(tcpi_last_data_sent);
                COPY_TCP_INFO_FIELD(tcpi_last_ack_sent);
                COPY_TCP_INFO_FIELD(tcpi_last_data_recv);
                COPY_TCP_INFO_FIELD(tcpi_last_ack_recv);
                COPY_TCP_INFO_FIELD(tcpi_pmtu);
                COPY_TCP_INFO_FIELD(tcpi_rcv_ssthresh);
                COPY_TCP_INFO_FIELD(tcpi_rtt);
                COPY_TCP_INFO_FIELD(tcpi_rttvar);
                COPY_TCP_INFO_FIELD(tcpi_snd_ssthresh);
                COPY_TCP_INFO_FIELD(tcpi_snd_cwnd);
                COPY_TCP_INFO_FIELD(tcpi_advmss);
                COPY_TCP_INFO_FIELD(tcpi_reordering);

#undef COPY_TCP_INFO_FIELD

                break;

            case RPC_IPV6_NEXTHOP:
                val.opttype = OPT_IPADDR6;
                memcpy(val.option_value_u.opt_ipaddr6, optval,
                       sizeof(val.option_value_u.opt_ipaddr6));
                break;

            default:
                val.opttype = OPT_INT;
                val.option_value_u.opt_int = *(int *)optval;
                break;
        }
    }
    if (raw_optval != NULL)
    {
        in.raw_optval.raw_optval_len = raw_roptlen;
        in.raw_optval.raw_optval_val = raw_optval;
    }
    if (raw_optlen != NULL)
    {
        in.raw_optlen.raw_optlen_len = 1;
        in.raw_optlen.raw_optlen_val = raw_optlen;
    }

    rcf_rpc_call(rpcs, "getsockopt", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (optval != NULL)
        {
            opt_val_str = te_log_buf_alloc();
            switch (optname)
            {
                case RPC_SO_LINGER:
                    ((tarpc_linger *)optval)->l_onoff =
                        out.optval.optval_val[0].option_value_u.
                            opt_linger.l_onoff;
                    ((tarpc_linger *)optval)->l_linger =
                       out.optval.optval_val[0].option_value_u.
                            opt_linger.l_linger;
                    te_log_buf_append(opt_val_str,
                        "{ l_onoff: %d, l_linger: %d }",
                        ((tarpc_linger *)optval)->l_onoff,
                        ((tarpc_linger *)optval)->l_linger);
                    break;

                case RPC_SO_RCVTIMEO:
                case RPC_SO_SNDTIMEO:
                    *((tarpc_timeval *)optval) =
                        out.optval.optval_val[0].option_value_u.
                            opt_timeval;
                    te_log_buf_append(opt_val_str,
                        "{ tv_sec: %" TE_PRINTF_64 "d, "
                        "tv_usec: %" TE_PRINTF_64 "d }",
                        ((tarpc_timeval *)optval)->tv_sec,
                        ((tarpc_timeval *)optval)->tv_usec);
                    break;

                case RPC_IP_ADD_MEMBERSHIP:
                case RPC_IP_DROP_MEMBERSHIP:
                case RPC_IP_MULTICAST_IF:
                {
                    tarpc_mreqn *opt = (tarpc_mreqn *)optval;
                    char         addr_buf[INET_ADDRSTRLEN];
                    char         addr_buf2[INET_ADDRSTRLEN];
 
                    memset(opt, 0, sizeof(*opt));
                    opt->type = out.optval.optval_val[0].opttype;
                    switch (opt->type)
                    {
                        case OPT_IPADDR:
                            memcpy(&opt->address,
                                   &out.optval.optval_val[0].
                                       option_value_u.opt_ipaddr,
                                   sizeof(opt->address));
                            te_log_buf_append(opt_val_str, "{ %s }",
                                inet_ntop(AF_INET, &opt->address,
                                          addr_buf, sizeof(addr_buf)));
                            break;

                        case OPT_MREQ:
                            memcpy(&opt->multiaddr,
                                   &out.optval.optval_val[0].
                                     option_value_u.opt_mreq.imr_multiaddr,
                                   sizeof(opt->multiaddr));
                            memcpy(&opt->address,
                                   &out.optval.optval_val[0].
                                     option_value_u.opt_mreq.imr_address,
                                   sizeof(opt->address));
                            te_log_buf_append(opt_val_str,
                                "{ imr_multiaddr: %s, imr_interface: %s }",
                                inet_ntop(AF_INET, &opt->multiaddr,
                                          addr_buf, sizeof(addr_buf)),
                                inet_ntop(AF_INET, &opt->address,
                                          addr_buf2, sizeof(addr_buf2)));
                            break;

                        case OPT_MREQN:
                            memcpy(&opt->multiaddr,
                                   &out.optval.optval_val[0].
                                     option_value_u.opt_mreqn.imr_multiaddr,
                                   sizeof(opt->multiaddr));
                            memcpy(&opt->address,
                                   &out.optval.optval_val[0].
                                     option_value_u.opt_mreqn.imr_address,
                                   sizeof(opt->address));
                            opt->ifindex =
                                val.option_value_u.opt_mreqn.imr_ifindex;
                            te_log_buf_append(opt_val_str,
                                "{ imr_multiaddr: %s, imr_address: %s, "
                                "imr_ifindex: %d }",
                                inet_ntop(AF_INET, &opt->multiaddr,
                                          addr_buf, sizeof(addr_buf)),
                                inet_ntop(AF_INET, &opt->address,
                                          addr_buf2, sizeof(addr_buf2)),
                                opt->ifindex);
                            break;

                        default:
                            ERROR("Unknown option type for %s get reply",
                                  sockopt_rpc2str(optname));
                            break;
                    }
                    break;
                }

                case RPC_TCP_INFO:
#define COPY_TCP_INFO_FIELD(_name) \
    do {                                                 \
        ((struct tcp_info *)optval)->_name =             \
            out.optval.optval_val[0].option_value_u.     \
            opt_tcp_info._name;                          \
        te_log_buf_append(opt_val_str, #_name ": %u ", \
             out.optval.optval_val[0].                   \
                 option_value_u.opt_tcp_info._name);     \
    } while (0)

                    te_log_buf_append(opt_val_str, "{ ");
                    COPY_TCP_INFO_FIELD(tcpi_state);
                    COPY_TCP_INFO_FIELD(tcpi_ca_state);
                    COPY_TCP_INFO_FIELD(tcpi_retransmits);
                    COPY_TCP_INFO_FIELD(tcpi_probes);
                    COPY_TCP_INFO_FIELD(tcpi_backoff);
                    COPY_TCP_INFO_FIELD(tcpi_options);
                    COPY_TCP_INFO_FIELD(tcpi_snd_wscale);
                    COPY_TCP_INFO_FIELD(tcpi_rcv_wscale);
                    COPY_TCP_INFO_FIELD(tcpi_rto);
                    COPY_TCP_INFO_FIELD(tcpi_ato);
                    COPY_TCP_INFO_FIELD(tcpi_snd_mss);
                    COPY_TCP_INFO_FIELD(tcpi_rcv_mss);
                    COPY_TCP_INFO_FIELD(tcpi_unacked);
                    COPY_TCP_INFO_FIELD(tcpi_sacked);
                    COPY_TCP_INFO_FIELD(tcpi_lost);
                    COPY_TCP_INFO_FIELD(tcpi_retrans);
                    COPY_TCP_INFO_FIELD(tcpi_fackets);
                    COPY_TCP_INFO_FIELD(tcpi_last_data_sent);
                    COPY_TCP_INFO_FIELD(tcpi_last_ack_sent);
                    COPY_TCP_INFO_FIELD(tcpi_last_data_recv);
                    COPY_TCP_INFO_FIELD(tcpi_last_ack_recv);
                    COPY_TCP_INFO_FIELD(tcpi_pmtu);
                    COPY_TCP_INFO_FIELD(tcpi_rcv_ssthresh);
                    COPY_TCP_INFO_FIELD(tcpi_rtt);
                    COPY_TCP_INFO_FIELD(tcpi_rttvar);
                    COPY_TCP_INFO_FIELD(tcpi_snd_ssthresh);
                    COPY_TCP_INFO_FIELD(tcpi_snd_cwnd);
                    COPY_TCP_INFO_FIELD(tcpi_advmss);
                    COPY_TCP_INFO_FIELD(tcpi_reordering);
                    te_log_buf_append(opt_val_str, " }");
#undef COPY_TCP_INFO_FIELD
                    break;
                
                case RPC_IPV6_NEXTHOP:
                {
                    char addr_buf[INET6_ADDRSTRLEN];

                    memcpy(optval,
                           out.optval.optval_val[0].option_value_u.
                           opt_ipaddr6, sizeof(struct in6_addr));
                    te_log_buf_append(opt_val_str, "{ %s }",
                                        inet_ntop(AF_INET6, optval,
                                                  addr_buf,
                                                  sizeof(addr_buf)));
                    break;
                }

                default:
                    *(int *)optval =
                        out.optval.optval_val[0].option_value_u.opt_int;
                    if (level == RPC_SOL_SOCKET &&
                        optname == RPC_SO_ERROR)
                    {
                        te_log_buf_append(opt_val_str, "%s",
                                            te_rc_err2str(*(int *)optval));
                    }
                    else
                    {
                        te_log_buf_append(opt_val_str, "%d",
                                            *(int *)optval);
                    }
                    break;
            }
        }
        if (raw_optlen != NULL)
        {
            *raw_optlen = *out.raw_optlen.raw_optlen_val;
        }
        if (raw_optval != NULL)
        {
            unsigned int i;
            te_bool      show_hidden = FALSE;

            memcpy(raw_optval, out.raw_optval.raw_optval_val,
                   out.raw_optval.raw_optval_len);

            if (opt_val_str == NULL)
                opt_val_str = te_log_buf_alloc();
            te_log_buf_append(opt_val_str, "[");
            for (i = 0; i < out.raw_optval.raw_optval_len; ++i)
            {
                if (i == (raw_optlen == NULL ? 0 : *raw_optlen))
                {
                    show_hidden = TRUE;
                    te_log_buf_append(opt_val_str, " (");
                }
                te_log_buf_append(opt_val_str, " %#02x",
                                    ((uint8_t *)raw_optval)[i]);
            }
            te_log_buf_append(opt_val_str, "%s ]",
                                show_hidden ? " )" : "");
        }
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(getsockopt, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): getsockopt(%d, %s, %s, %p, %s) "
                 "-> %d (%s) optval=%s raw_optlen=%d",
                 rpcs->ta, rpcs->name,
                 s, socklevel_rpc2str(level), sockopt_rpc2str(optname),
                 optval != NULL ? optval : raw_optval, opt_len_str,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 opt_val_str == NULL ? "(nil)" :
                                       te_log_buf_get(opt_val_str),
                 (raw_optlen == NULL) ? -1 : (int)*raw_optlen);

    te_log_buf_free(opt_val_str);

    RETVAL_INT(getsockopt, out.retval);
}

int
rpc_setsockopt_gen(rcf_rpc_server *rpcs,
                   int s, rpc_socklevel level, rpc_sockopt optname,
                   const void *optval,
                   const void *raw_optval, socklen_t raw_optlen,
                   socklen_t raw_roptlen)
{
    tarpc_setsockopt_in     in;
    tarpc_setsockopt_out    out;
    struct option_value     val;
    te_log_buf           *opt_val_str = NULL;
    char                    opt_len_str[32] = { 0, };

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(setsockopt, -1);
    }
    if (raw_optval == NULL && raw_roptlen != 0)
    {
        ERROR("%s(): 'raw_roptlen' must be 0, if 'raw_optval' is NULL",
              __FUNCTION__);
        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
        RETVAL_INT(setsockopt, -1);
    }
    if (raw_optval != NULL && raw_roptlen < raw_optlen)
    {
        ERROR("%s(): 'raw_roptlen' must be greater or equal to "
              "'raw_optlen'", __FUNCTION__);
        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
        RETVAL_INT(setsockopt, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.s = s;
    in.level = level;
    in.optname = optname;
    in.raw_optlen = raw_optlen;

    if (optval != NULL)
        TE_SPRINTF(opt_len_str, "AUTO");
    if (raw_optval != NULL || raw_optlen != 0 || optval == NULL)
        snprintf(opt_len_str + strlen(opt_len_str),
                 sizeof(opt_len_str) - strlen(opt_len_str),
                 "%s%u", optval != NULL ? "+" : "", (unsigned)raw_optlen);

    if (optval != NULL)
    {
        opt_val_str = te_log_buf_alloc();

        in.optval.optval_len = 1;
        in.optval.optval_val = &val;
        switch (optname)
        {
            case RPC_SO_LINGER:
                val.opttype = OPT_LINGER;
                val.option_value_u.opt_linger =
                    *((tarpc_linger *)optval);
                te_log_buf_append(opt_val_str, 
                    "{ l_onoff: %d, l_linger: %d }",
                    ((tarpc_linger *)optval)->l_onoff,
                    ((tarpc_linger *)optval)->l_linger);
                break;

            case RPC_SO_RCVTIMEO:
            case RPC_SO_SNDTIMEO:
                val.opttype = OPT_TIMEVAL;
                val.option_value_u.opt_timeval =
                    *((tarpc_timeval *)optval);
                te_log_buf_append(opt_val_str, 
                    "{ tv_sec: %" TE_PRINTF_64 "d, "
                    "tv_usec: %" TE_PRINTF_64 "d }",
                    ((tarpc_timeval *)optval)->tv_sec,
                    ((tarpc_timeval *)optval)->tv_usec);
                break;

            case RPC_IPV6_PKTOPTIONS:
                ERROR("IPV6_PKTOPTIONS is not supported yet");
                rpcs->_errno = TE_RC(TE_TAPI, TE_ENOMEM);
                RETVAL_INT(setsockopt, -1);
                break;
    
            case RPC_IPV6_ADD_MEMBERSHIP:
            case RPC_IPV6_DROP_MEMBERSHIP:
            case RPC_IPV6_JOIN_ANYCAST:
            case RPC_IPV6_LEAVE_ANYCAST:
            {
                char buf[INET6_ADDRSTRLEN];

                inet_ntop(AF_INET6,
                          &((struct ipv6_mreq *)optval)->ipv6mr_multiaddr,
                          buf, sizeof(buf));
                
                val.opttype = OPT_MREQ6;
                val.option_value_u.opt_mreq6.ipv6mr_multiaddr.
                ipv6mr_multiaddr_val = (uint32_t *)
                    &((struct ipv6_mreq *)optval)->ipv6mr_multiaddr;
                
                val.option_value_u.opt_mreq6.ipv6mr_multiaddr.
                ipv6mr_multiaddr_len = sizeof(struct in6_addr);

                val.option_value_u.opt_mreq6.ipv6mr_ifindex =
                    ((struct ipv6_mreq *)optval)->ipv6mr_interface;

                te_log_buf_append(opt_val_str, 
                    "{ multiaddr: %s, ifindex: %d }",
                    buf, val.option_value_u.opt_mreq6.ipv6mr_ifindex);
                break;
            }

            case RPC_IP_ADD_MEMBERSHIP:
            case RPC_IP_DROP_MEMBERSHIP:
            case RPC_IP_MULTICAST_IF:
            {
                tarpc_mreqn *opt = (tarpc_mreqn *)optval;

                char addr_buf1[INET_ADDRSTRLEN];
                char addr_buf2[INET_ADDRSTRLEN];

                val.opttype = opt->type;
                        
                switch (opt->type)
                {
                    case OPT_IPADDR:
                    {
                        if (optname != RPC_IP_MULTICAST_IF)
                        {
                            ERROR("%s socket option does not support "
                                  "'struct in_addr' argument",
                                  sockopt_rpc2str(optname));
                            rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
                            RETVAL_INT(setsockopt, -1);
                        }
                        
                        memcpy(&val.option_value_u.opt_ipaddr,
                               (char *)&(opt->address),
                               sizeof(opt->address));
                        te_log_buf_append(opt_val_str, "{ %s }",
                                            inet_ntop(AF_INET,
                                                (char *)&(opt)->address,
                                                addr_buf1,
                                                sizeof(addr_buf1)));
                        break;
                    }
                    case OPT_MREQ:
                    {
                        memcpy(&val.option_value_u.opt_mreq.imr_multiaddr,
                              (char *)&(opt->multiaddr),
                              sizeof(opt->multiaddr));
                        memcpy(&val.option_value_u.opt_mreq.imr_address,
                               (char *)&(opt->address),
                               sizeof(opt->address));

                        te_log_buf_append(opt_val_str, 
                            "{ imr_multiaddr: %s, imr_interface: %s }",
                            inet_ntop(AF_INET,
                                      (char *)&(opt->multiaddr),
                                       addr_buf1, sizeof(addr_buf1)),
                            inet_ntop(AF_INET,
                                      (char *)&(opt->address),
                                       addr_buf2, sizeof(addr_buf2)));
                        break;
                    }
                    case OPT_MREQN:
                    {
                        memcpy(&val.option_value_u.opt_mreqn.imr_multiaddr,
                              (char *)&(opt->multiaddr),
                              sizeof(opt->multiaddr));
                        memcpy(&val.option_value_u.opt_mreqn.imr_address,
                               (char *)&(opt->address),
                               sizeof(opt->address));
                        val.option_value_u.opt_mreqn.imr_ifindex =
                        opt->ifindex;

                        te_log_buf_append(opt_val_str, 
                            "{ imr_multiaddr: %s, imr_address: %s, "
                            "imr_ifindex: %d }",
                            inet_ntop(AF_INET,
                                      (char *)&(opt->multiaddr),
                                       addr_buf1, sizeof(addr_buf1)),
                            inet_ntop(AF_INET,
                                      (char *)&(opt->address),
                                       addr_buf2, sizeof(addr_buf2)),
                            opt->ifindex);
                        break;
                    } 
                    
                    default:
                    {
                        ERROR("Invalid argument type %d for socket option",
                              opt->type);
                        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
                        RETVAL_INT(setsockopt, -1);
                    }
                   
                }

                break;
            }
            
            case RPC_SO_UPDATE_ACCEPT_CONTEXT:
                val.opttype = OPT_HANDLE;
                val.option_value_u.opt_handle = *(int *)optval;
                break;

            case RPC_IPV6_NEXTHOP:
                val.opttype = OPT_IPADDR6;
                memcpy(val.option_value_u.opt_ipaddr6, optval,
                       sizeof(struct in6_addr));
                break;

            default:
                val.opttype = OPT_INT;
                val.option_value_u.opt_int = *(int *)optval;
                te_log_buf_append(opt_val_str, "%d", *(int *)optval);
                break;
        }
    }

    if (raw_optval != NULL)
    {
        unsigned int i;
        te_bool      show_hidden = FALSE;

        if (opt_val_str == NULL)
            opt_val_str = te_log_buf_alloc();

        in.raw_optval.raw_optval_len = raw_roptlen;
        in.raw_optval.raw_optval_val = (void *)raw_optval;
        te_log_buf_append(opt_val_str, "[");
        for (i = 0; i < raw_roptlen; ++i)
        {
            if (i == raw_optlen)
            {
                show_hidden = TRUE;
                te_log_buf_append(opt_val_str, " (");
            }
            te_log_buf_append(opt_val_str, " %#02x",
                                ((uint8_t *)raw_optval)[i]);
        }
        te_log_buf_append(opt_val_str, "%s ]", show_hidden ? " )" : "");
    }

    rcf_rpc_call(rpcs, "setsockopt", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(setsockopt, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): setsockopt(%d, %s, %s, %s, %s) "
                 "-> %d (%s)", rpcs->ta, rpcs->name,
                 s, socklevel_rpc2str(level), sockopt_rpc2str(optname),
                 opt_val_str == NULL ? "(nil)" :
                                       te_log_buf_get(opt_val_str),
                 opt_len_str,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    te_log_buf_free(opt_val_str);

    RETVAL_INT(setsockopt, out.retval);
}
