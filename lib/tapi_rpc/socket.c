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
#include "te_alloc.h"

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

    in.domain = domain;
    in.type = type;
    in.proto = protocol;

    rcf_rpc_call(rpcs, "socket", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(socket, out.fd);
    TAPI_RPC_LOG(rpcs, socket, "%s, %s, %s", "%d",
                 domain_rpc2str(domain), socktype_rpc2str(type),
                 proto_rpc2str(protocol), out.fd);
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

    in.fd = s;
    sockaddr_input_h2rpc(my_addr, &in.addr);

    rcf_rpc_call(rpcs, "bind", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(bind, out.retval);
    TAPI_RPC_LOG(rpcs, bind, "%d, %s", "%d", s, sockaddr_h2str(my_addr),
                 out.retval);
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

    in.fd = s;
    sockaddr_raw2rpc(my_addr, addrlen, &in.addr);

    rcf_rpc_call(rpcs, "bind", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(bind, out.retval);
    TAPI_RPC_LOG(rpcs, bind, "%d, %p, %d", "%d",
                 s, my_addr, addrlen, out.retval);
    RETVAL_INT(bind, out.retval);
}

int
rpc_check_port_is_free(rcf_rpc_server *rpcs, uint16_t port)
{
    tarpc_check_port_is_free_in  in;
    tarpc_check_port_is_free_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(bind, -1);
    }

    in.port = port;

    rcf_rpc_call(rpcs, "check_port_is_free", &in, &out);
    CHECK_RETVAL_VAR(check_port_is_free, out.retval,
                     (out.retval != TRUE && out.retval != FALSE), FALSE);
    TAPI_RPC_LOG(rpcs, check_port_is_free, "%d", "%d",
                 (int)port, out.retval);
    TAPI_RPC_OUT(check_port_is_free, FALSE);
    return out.retval; /* no jumps! */
}

int
rpc_connect(rcf_rpc_server *rpcs,
            int s, const struct sockaddr *addr)
{
    tarpc_connect_in  in;
    tarpc_connect_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(connect, -1);
    }

    in.fd = s;
    sockaddr_input_h2rpc(addr, &in.addr);

    rcf_rpc_call(rpcs, "connect", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(connect, out.retval);
    TAPI_RPC_LOG(rpcs, connect, "%d, %s", "%d",
                 s, sockaddr_h2str(addr), out.retval);
    RETVAL_INT(connect, out.retval);
}

int
rpc_connect_raw(rcf_rpc_server *rpcs, int s,
            const struct sockaddr *addr, socklen_t addrlen)
{
    tarpc_connect_in  in;
    tarpc_connect_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(connect, -1);
    }

    in.fd = s;
    sockaddr_raw2rpc(addr, addrlen, &in.addr);

    rcf_rpc_call(rpcs, "connect", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(connect, out.retval);
    TAPI_RPC_LOG(rpcs, connect, "%d, %p, %d", "%d",
                 s, addr, addrlen, out.retval);
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

    in.fd = fd;
    in.backlog = backlog;

    rcf_rpc_call(rpcs, "listen", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(listen, out.retval);
    TAPI_RPC_LOG(rpcs, listen, "%d, %d", "%d",
                 fd, backlog, out.retval);
    RETVAL_INT(listen, out.retval);
}

#define MAKE_ACCEPT_CALL(_accept_func) \
    if (rpcs == NULL)                                           \
    {                                                           \
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__); \
        RETVAL_INT(_accept_func, -1);                           \
    }                                                           \
                                                                \
    if (addr != NULL && addrlen != NULL && *addrlen > raddrlen) \
    {                                                           \
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);                \
        RETVAL_INT(_accept_func, -1);                           \
    }                                                           \
                                                                \
    in.fd = s;                                                  \
    if (addrlen != NULL && rpcs->op != RCF_RPC_WAIT)            \
    {                                                           \
        in.len.len_len = 1;                                     \
        in.len.len_val = addrlen;                               \
    }                                                           \
    if (rpcs->op != RCF_RPC_WAIT)                               \
    {                                                           \
        sockaddr_raw2rpc(addr, raddrlen, &in.addr);             \
    }                                                           \
                                                                \
    rcf_rpc_call(rpcs, #_accept_func, &in, &out);               \
                                                                \
    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)       \
    {                                                           \
        sockaddr_rpc2h(&out.addr, addr, raddrlen,               \
                       NULL, addrlen);                          \
                                                                \
        if (addrlen != NULL && out.len.len_val != NULL)         \
            *addrlen = out.len.len_val[0];                      \
    }                                                           \
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(_accept_func,             \
                                      out.retval)

int
rpc_accept_gen(rcf_rpc_server *rpcs,
               int s, struct sockaddr *addr, socklen_t *addrlen,
               socklen_t raddrlen)
{
    socklen_t save_addrlen =
                (addrlen == NULL) ? (socklen_t)-1 : *addrlen;

    tarpc_accept_in  in;
    tarpc_accept_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    MAKE_ACCEPT_CALL(accept);

    TAPI_RPC_LOG(rpcs, accept, "%d, %p[%u], %p(%u)",
                 "%d peer=%s addrlen=%u",
                 s, addr, raddrlen, addrlen, save_addrlen,
                 out.retval, sockaddr_h2str(addr),
                 (addrlen == NULL) ? (socklen_t)-1 : *addrlen);
    RETVAL_INT(accept, out.retval);
}

int
rpc_accept4_gen(rcf_rpc_server *rpcs,
               int s, struct sockaddr *addr, socklen_t *addrlen,
               socklen_t raddrlen, int flags)
{
    socklen_t save_addrlen =
                (addrlen == NULL) ? (socklen_t)-1 : *addrlen;

    tarpc_accept4_in  in;
    tarpc_accept4_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.flags = socket_flags_rpc2h(flags);

    MAKE_ACCEPT_CALL(accept4);

    TAPI_RPC_LOG(rpcs, accept4, "%d, %p[%u], %p(%u), %s",
                 "%d peer=%s addrlen=%u",
                 s, addr, raddrlen, addrlen, save_addrlen,
                 socket_flags_rpc2str(flags),
                 out.retval, sockaddr_h2str(addr),
                 (addrlen == NULL) ? (socklen_t)-1 : *addrlen);
    RETVAL_INT(accept4, out.retval);
}

#undef MAKE_ACCEPT_CALL

ssize_t
rpc_recvfrom_gen(rcf_rpc_server *rpcs,
                 int s, void *buf, size_t len, rpc_send_recv_flags flags,
                 struct sockaddr *from, socklen_t *fromlen,
                 size_t rbuflen, socklen_t rfrombuflen)
{
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
    TAPI_RPC_LOG(rpcs, recvfrom, "%d, %p[%u], %u, %s, %p[%u], %d",
                 "%d from=%s fromlen=%d",
                 s, buf, rbuflen, len, send_recv_flags_rpc2str(flags),
                 from, rfrombuflen, (int)save_fromlen,
                 out.retval, sockaddr_h2str(from),
                 (fromlen == NULL) ? -1 : (int)*fromlen);
    RETVAL_INT(recvfrom, out.retval);
}

ssize_t
rpc_recv_gen(rcf_rpc_server *rpcs,
                 int s, void *buf, size_t len, rpc_send_recv_flags flags,
                 size_t rbuflen)
{
    tarpc_recv_in  in;
    tarpc_recv_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recv, -1);
    }

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
    TAPI_RPC_LOG(rpcs, recv, "%d, %p[%u], %u, %s", "%d",
                 s, buf, rbuflen, len, send_recv_flags_rpc2str(flags),
                 out.retval);
    RETVAL_INT(recv, out.retval);
}

tarpc_ssize_t
rpc_recvbuf_gen(rcf_rpc_server *rpcs, int fd, rpc_ptr buf, size_t buf_off,
                size_t count, rpc_send_recv_flags flags)
{
    tarpc_recvbuf_in  in;
    tarpc_recvbuf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(write, -1);
    }

    in.fd  = fd;
    in.len = count;
    in.buf = buf;
    in.off = buf_off;

    rcf_rpc_call(rpcs, "recvbuf", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(readbuf, out.retval);
    TAPI_RPC_LOG(rpcs, recvbuf, "%d, %u (off %u), %u, %s", "%d",
                 fd, buf, buf_off, count, send_recv_flags_rpc2str(flags),
                 out.retval);
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

    in.fd = s;
    in.how = how;

    rcf_rpc_call(rpcs, "shutdown", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(shutdown, out.retval);
    TAPI_RPC_LOG(rpcs, shutdown, "%d, %s", "%d",
                 s, shut_how_rpc2str(how), out.retval);
    RETVAL_INT(shutdown, out.retval);
}

ssize_t
rpc_sendto(rcf_rpc_server *rpcs,
           int s, const void *buf, size_t len,
           rpc_send_recv_flags flags,
           const struct sockaddr *to)
{
    tarpc_sendto_in  in;
    tarpc_sendto_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendto, -1);
    }

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
    TAPI_RPC_LOG(rpcs, sendto, "%d, %p, %u, %s, %s", "%d",
                 s, buf, len, send_recv_flags_rpc2str(flags),
                 sockaddr_h2str(to),
                 out.retval);
    RETVAL_INT(sendto, out.retval);
}

ssize_t
rpc_sendto_raw(rcf_rpc_server *rpcs,
               int s, const void *buf, size_t len,
               rpc_send_recv_flags flags,
               const struct sockaddr *to, socklen_t tolen)
{
    tarpc_sendto_in  in;
    tarpc_sendto_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendto, -1);
    }

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
    TAPI_RPC_LOG(rpcs, sendto, "%d, %p, %u, %s, %p, %d", "%d",
                 s, buf, len, send_recv_flags_rpc2str(flags),
                 to, tolen, out.retval);
    RETVAL_INT(sendto, out.retval);
}

ssize_t
rpc_send(rcf_rpc_server *rpcs,
           int s, const void *buf, size_t len,
           rpc_send_recv_flags flags)
{
    tarpc_send_in  in;
    tarpc_send_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(send, -1);
    }

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
    TAPI_RPC_LOG(rpcs, send, "%d, %p, %u, %s", "%d",
                 s, buf, len, send_recv_flags_rpc2str(flags), out.retval);
    RETVAL_INT(send, out.retval);
}

ssize_t
rpc_sendbuf_gen(rcf_rpc_server *rpcs, int s, rpc_ptr buf, size_t buf_off,
                size_t len, rpc_send_recv_flags flags)
{
    tarpc_sendbuf_in  in;
    tarpc_sendbuf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(send, -1);
    }

    in.fd = s;
    in.len = len;
    in.buf = buf;
    in.off = buf_off;
    in.flags = flags;

    rcf_rpc_call(rpcs, "sendbuf", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(send, out.retval);
    TAPI_RPC_LOG(rpcs, sendbuf, "%d, %u (off %u), %u, %s", "%d",
                 s, buf, buf_off, len, send_recv_flags_rpc2str(flags),
                 out.retval);
    RETVAL_INT(sendbuf, out.retval);
}

ssize_t
rpc_send_msg_more(rcf_rpc_server *rpcs, int s, rpc_ptr buf,
                size_t first_len, size_t second_len)
{
    tarpc_send_msg_more_in  in;
    tarpc_send_msg_more_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(send, -1);
    }

    in.fd = s;
    in.first_len = first_len;
    in.second_len = second_len;
    in.buf = buf;

    rcf_rpc_call(rpcs, "send_msg_more", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(send_msg_more, out.retval);
    TAPI_RPC_LOG(rpcs, send_msg_more, "%d, %u , %u, %u", "%d",
                 s, buf, first_len, second_len, out.retval);
    RETVAL_INT(send_msg_more, out.retval);
}

/**
 * Release memory allocated for value of tarpc_msghdr type.
 *
 * @param msg   Handle to memory to be freed
 */
static void
tarpc_msghdr_free(tarpc_msghdr *msg)
{
    int i;

    free(msg->msg_iov.msg_iov_val);
    for (i = 0; i < (int)msg->msg_control.msg_control_len; i++)
        free(msg->msg_control.msg_control_val[i].data.data_val);
    free(msg->msg_control.msg_control_val);
}

/**
 * Transform value of rpc_msghdr type to value of tarpc_msghdr type.
 *
 * @param rpc_msg           Pointer to value of type rpc_msghdr
 * @param tarpc_msg         Where value of type tarpc_msghdr should be
 *                          saved
 *
 * @return Status code
 */
static te_errno
msghdr_rpc2tarpc(const rpc_msghdr *rpc_msg, tarpc_msghdr *tarpc_msg)
{
    int             i;
    tarpc_iovec    *iovec_arr;

    if (rpc_msg->msg_riovlen > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Length of the I/O vector is too long (%u) - "
              "increase RCF_RPC_MAX_IOVEC(%u)",
              rpc_msg->msg_riovlen, RCF_RPC_MAX_IOVEC);
        return TE_ENOMEM;
    }

    if (((rpc_msg->msg_iov != NULL) &&
         (rpc_msg->msg_iovlen > rpc_msg->msg_riovlen)))
    {
        ERROR("Inconsistent real and declared lengths of buffers");
        return TE_EINVAL;
    }

    if (rpc_msg->msg_iov != NULL)
    {
        iovec_arr = TE_ALLOC(sizeof(tarpc_iovec) * rpc_msg->msg_riovlen);

        for (i = 0;
             i < (int)rpc_msg->msg_riovlen;
             i++)
        {
            iovec_arr[i].iov_base.iov_base_val =
                            rpc_msg->msg_iov[i].iov_base;
            iovec_arr[i].iov_base.iov_base_len =
                            rpc_msg->msg_iov[i].iov_rlen;
            iovec_arr[i].iov_len = rpc_msg->msg_iov[i].iov_len;
        }

        tarpc_msg->msg_iov.msg_iov_val = iovec_arr;
        tarpc_msg->msg_iov.msg_iov_len = rpc_msg->msg_riovlen;
    }
    tarpc_msg->msg_iovlen = rpc_msg->msg_iovlen;

    if (rpc_msg->msg_namelen <= sizeof(struct sockaddr_storage))
        sockaddr_input_h2rpc(rpc_msg->msg_name, &tarpc_msg->msg_name);
    else
    {
        tarpc_msg->msg_namelen = rpc_msg->msg_namelen;
        sockaddr_raw2rpc(rpc_msg->msg_name, rpc_msg->msg_rnamelen,
                         &tarpc_msg->msg_name);
    }

    tarpc_msg->msg_flags = (int)rpc_msg->msg_flags;

    if (rpc_msg->msg_control != NULL)
    {
        int             rc;

        struct tarpc_cmsghdr *rpc_c;

        rpc_c = tarpc_msg->msg_control.msg_control_val =
                TE_ALLOC(sizeof(tarpc_cmsghdr) * RCF_RPC_MAX_CMSGHDR);

        rc = msg_control_h2rpc(
                        (uint8_t *)CMSG_FIRSTHDR((struct msghdr *)rpc_msg),
                        rpc_msg->msg_controllen,
                        &tarpc_msg->msg_control.msg_control_val,
                        &tarpc_msg->msg_control.msg_control_len);

        if (rc != 0)
        {
            ERROR("%s(): failed to convert control message to TARPC"
                  "format", __FUNCTION__);
            return rc;
        }
    }

    return 0;
}

ssize_t
rpc_sendmsg(rcf_rpc_server *rpcs,
            int s, const struct rpc_msghdr *msg, rpc_send_recv_flags flags)
{
    tarpc_sendmsg_in  in;
    tarpc_sendmsg_out out;

    struct tarpc_msghdr rpc_msg;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&rpc_msg, 0, sizeof(rpc_msg));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendmsg, -1);
    }

    in.s = s;
    in.flags = flags;

    if (msg != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.msg.msg_val = &rpc_msg;
        in.msg.msg_len = 1;

        msghdr_rpc2tarpc(msg, &rpc_msg);
    }

    rcf_rpc_call(rpcs, "sendmsg", &in, &out);

    tarpc_msghdr_free(&rpc_msg);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendmsg, out.retval);
    TAPI_RPC_LOG(rpcs, sendmsg, "%d, %p (msg_name: %p, "
                                "msg_namelen: %" TE_PRINTF_SOCKLEN_T "d, "
                                "msg_iov: %p, msg_iovlen: %d, "
                                "msg_control: %p, msg_controllen: %d, "
                                "msg_flags: %s)"
         ", %s)", "%d",
         s, msg,
         msg != NULL ? msg->msg_name : NULL,
         msg != NULL ? (int)msg->msg_namelen : -1,
         msg != NULL ? msg->msg_iov : NULL,
         msg != NULL ? (int)msg->msg_iovlen : -1,
         msg != NULL ? msg->msg_control : NULL,
         msg != NULL ? (int)msg->msg_controllen : -1,
         msg != NULL ? send_recv_flags_rpc2str(msg->msg_flags) : "",
         send_recv_flags_rpc2str(flags),
         out.retval);
    RETVAL_INT(sendmsg, out.retval);
}

ssize_t
rpc_recvmsg(rcf_rpc_server *rpcs,
            int s, struct rpc_msghdr *msg, rpc_send_recv_flags flags)
{
    char              str_buf[1024];
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
            uint8_t *first_cmsg =
                            (uint8_t *)CMSG_FIRSTHDR((struct msghdr *)msg);
            int      retval;

            retval = msg_control_rpc2h(rpc_msg.msg_control.msg_control_val,
                                       rpc_msg.msg_control.msg_control_len,
                                       first_cmsg,
                                       (size_t *)&msg->msg_controllen);
            if (retval != 0)
            {
                ERROR("%s(): control message conversion failed",
                      __FUNCTION__);
                rpcs->_errno = TE_RC(TE_RCF, retval);
                RETVAL_INT(recvmsg, -1);
            }
        }

        msg->msg_flags = (rpc_send_recv_flags)rpc_msg.msg_flags;

        snprintf(str_buf, sizeof(str_buf),
                 "msg_name: %p, msg_namelen: %" TE_PRINTF_SOCKLEN_T "d, "
                 "msg_iov: %p, msg_iovlen: %" TE_PRINTF_SIZE_T "d, "
                 "msg_control: %p, msg_controllen: "
                 "%" TE_PRINTF_SOCKLEN_T "d, msg_flags: %s",
                 msg->msg_name, msg->msg_namelen,
                 msg->msg_iov, msg->msg_iovlen,
                 msg->msg_control, msg->msg_controllen,
                 send_recv_flags_rpc2str(msg->msg_flags));
    }

    TAPI_RPC_LOG(rpcs, recvmsg, "%d, %p(%s), %s", "%ld",
                 s, msg, str_buf, send_recv_flags_rpc2str(flags),
                 (long)out.retval);
    RETVAL_INT(recvmsg, out.retval);
}

int 
rpc_cmsg_data_parse_ip_pktinfo(rcf_rpc_server *rpcs,
                               uint8_t *data, uint32_t data_len,
                               struct in_addr *ipi_spec_dst,
                               struct in_addr *ipi_addr,
                               int *ipi_ifindex)
{
    tarpc_cmsg_data_parse_ip_pktinfo_in     in;
    tarpc_cmsg_data_parse_ip_pktinfo_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    UNUSED(rpcs);
    UNUSED(data);
    UNUSED(data_len);
    UNUSED(ipi_spec_dst);
    UNUSED(ipi_addr);
    UNUSED(ipi_ifindex);

    RING("%s(): this function is no longer supported since "
         "IP_PKTINFO is now processed correctly by TE", __FUNCTION__);
    RETVAL_INT(cmsg_data_parse_ip_pktinfo, -1);
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

    in.fd = s;
    if (namelen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.len.len_len = 1;
        in.len.len_val = namelen;
    }
    sockaddr_raw2rpc(name, rnamelen, &in.addr);

    rcf_rpc_call(rpcs, "getsockname", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_CALL)
    {
        sockaddr_rpc2h(&out.addr, name, rnamelen,
                       NULL, namelen);

        if (namelen != NULL && out.len.len_val != NULL)
            *namelen = out.len.len_val[0];
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(getsockname, out.retval);
    TAPI_RPC_LOG(rpcs, getsockname, "%d, %p[%u], %u",
                 "%d name=%s namelen=%u",
                 s, name, rnamelen, namelen_save,
                 out.retval, sockaddr_h2str(name),
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

    in.fd = s;
    if (namelen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.len.len_len = 1;
        in.len.len_val = namelen;
    }
    sockaddr_raw2rpc(name, rnamelen, &in.addr);

    rcf_rpc_call(rpcs, "getpeername", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_CALL)
    {
        sockaddr_rpc2h(&out.addr, name, rnamelen,
                       NULL, namelen);

        if (namelen != NULL && out.len.len_val != NULL)
            *namelen = out.len.len_val[0];
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(getpeername, out.retval);
    TAPI_RPC_LOG(rpcs, getpeername, "%d, %p[%u], %u",
                 "%d name=%s namelen=%u",
                 s, name, rnamelen, namelen_save,
                 out.retval, sockaddr_h2str(name),
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
    te_log_buf           *opt_val_str = NULL;
    char                  opt_len_str[32] = "(nil)";
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
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
        ((struct rpc_tcp_info *)optval)->_name

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
    do {                                                     \
        ((struct rpc_tcp_info *)optval)->_name =             \
            out.optval.optval_val[0].option_value_u.         \
            opt_tcp_info._name;                              \
        if (strcmp(#_name, "tcpi_state") == 0)               \
            te_log_buf_append(opt_val_str, #_name ": %s ",   \
                 tcp_state_rpc2str(out.optval.optval_val[0]. \
                     option_value_u.opt_tcp_info._name));    \
        else if (strcmp(#_name, "tcpi_ca_state") == 0)       \
            te_log_buf_append(opt_val_str, #_name ": %s ",   \
                 tcp_ca_state_rpc2str(out.optval.            \
                     optval_val[0].option_value_u.           \
                     opt_tcp_info._name));                   \
         else if (strcmp(#_name, "tcpi_options") == 0)       \
            te_log_buf_append(opt_val_str, #_name ": %s ",   \
                 tcpi_options_rpc2str(out.optval.            \
                     optval_val[0].option_value_u.           \
                     opt_tcp_info._name));                   \
        else                                                 \
            te_log_buf_append(opt_val_str, #_name ": %u ",   \
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

            if (optname != RPC_IP_PKTOPTIONS || out.retval < 0 ||
                out.optval.optval_val[0].option_value_u.
                    opt_ip_pktoptions.opt_ip_pktoptions_len == 0 ||
                raw_optlen == NULL)
                memcpy(raw_optval, out.raw_optval.raw_optval_val,
                       out.raw_optval.raw_optval_len);
            else if (*out.raw_optlen.raw_optlen_val > 0)
            {
                unsigned int    len = out.optval.optval_val[0].
                                        option_value_u.
                                        opt_ip_pktoptions.
                                        opt_ip_pktoptions_len;

                struct tarpc_cmsghdr *rpc_c = out.optval.optval_val[0].
                                                option_value_u.
                                                opt_ip_pktoptions.
                                                opt_ip_pktoptions_val;

                int      rc;
                int      tmp_optlen = raw_roptlen;

                rc = msg_control_rpc2h(rpc_c, len, raw_optval,
                                       (size_t *)&tmp_optlen);
                if (rc != 0)
                {
                    ERROR("%s(): failed to convert control message",
                          __FUNCTION__);
                    rpcs->_errno = TE_RC(TE_RCF, rc);
                    RETVAL_INT(getsockopt, -1);
                }
                if (raw_optlen != NULL)
                    *raw_optlen = tmp_optlen;
            }

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
    TAPI_RPC_LOG(rpcs, getsockopt, "%d, %s, %s, %p, %s",
                 "%d optval=%s raw_optlen=%d",
                 s, socklevel_rpc2str(level), sockopt_rpc2str(optname),
                 optval != NULL ? optval : raw_optval, opt_len_str,
                 out.retval,
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

            case RPC_MCAST_JOIN_GROUP:
            case RPC_MCAST_LEAVE_GROUP:
            {
                char gr_addr_buf[INET_ADDRSTRLEN];
                struct group_req *opt = (struct group_req *)optval;
                struct sockaddr *group = (struct sockaddr *)&opt->gr_group;
                struct sockaddr_in *group_in = (struct sockaddr_in *)group;

                val.opttype = OPT_GROUP_REQ;
                val.option_value_u.opt_group_req.gr_interface =
                    opt->gr_interface;
                sockaddr_input_h2rpc(group,
                            &val.option_value_u.opt_group_req.gr_group);

                if (group->sa_family == AF_INET)
                {
                    te_log_buf_append(opt_val_str,
                                      "{ gr_group: %s, gr_interface: %d }",
                                      inet_ntop(AF_INET,
                                                &group_in->sin_addr,
                                                gr_addr_buf,
                                                sizeof(gr_addr_buf)),
                                      opt->gr_interface);
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
    TAPI_RPC_LOG(rpcs, setsockopt, "%d, %s, %s, %s, %s", "%d",
                 s, socklevel_rpc2str(level), sockopt_rpc2str(optname),
                 opt_val_str == NULL ? "(nil)" :
                                       te_log_buf_get(opt_val_str),
                 opt_len_str, out.retval);

    te_log_buf_free(opt_val_str);

    RETVAL_INT(setsockopt, out.retval);
}

int
rpc_recvmmsg_alt(rcf_rpc_server *rpcs, int fd, struct rpc_mmsghdr *mmsg,
                 unsigned int vlen, rpc_send_recv_flags flags,
                 struct tarpc_timespec *timeout)
{
    char                  str_buf[1024];
    rcf_rpc_op            op;
    tarpc_recvmmsg_alt_in  in;
    tarpc_recvmmsg_alt_out out;

    struct rpc_msghdr *msg;
    unsigned int       j;

    struct tarpc_mmsghdr rpc_mmsg[RCF_RPC_MAX_MSGHDR];
    struct tarpc_msghdr *rpc_msg;
    struct tarpc_iovec   iovec_arr[RCF_RPC_MAX_MSGHDR][RCF_RPC_MAX_IOVEC];
    struct tarpc_cmsghdr cmsg_hdrs[RCF_RPC_MAX_MSGHDR][RCF_RPC_MAX_CMSGHDR];

    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(rpc_mmsg, 0, sizeof(rpc_mmsg));
    memset(iovec_arr, 0, sizeof(iovec_arr));
    memset(cmsg_hdrs, 0, sizeof(cmsg_hdrs));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recvmmsg_alt, -1);
    }

    op = rpcs->op;
    in.fd = fd;
    in.flags = flags;
    in.vlen = vlen;

    if (timeout != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.timeout.timeout_len = 1;
        in.timeout.timeout_val = timeout;
    }

    if (mmsg != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.mmsg.mmsg_val = rpc_mmsg;
        in.mmsg.mmsg_len = vlen;

        for (j = 0; j < vlen; j++)
        {
            msg = &mmsg[j].msg_hdr;
            rpc_msg = &rpc_mmsg[j].msg_hdr;
            rpc_mmsg[j].msg_len = mmsg[j].msg_len;

            if (msg->msg_riovlen > RCF_RPC_MAX_IOVEC)
            {
                rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
                ERROR("Length of the I/O vector is too long (%u) - "
                      "increase RCF_RPC_MAX_IOVEC(%u)",
                      msg->msg_riovlen, RCF_RPC_MAX_IOVEC);
                RETVAL_INT(recvmmsg_alt, -1);
            }

            if (msg->msg_cmsghdr_num > RCF_RPC_MAX_CMSGHDR)
            {
                rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
                ERROR("Too many cmsg headers - increase "
                      "RCF_RPC_MAX_CMSGHDR");
                RETVAL_INT(recvmmsg_alt, -1);
            }

            if (msg->msg_control != NULL && msg->msg_cmsghdr_num == 0)
            {
                rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
                ERROR("Number of cmsg headers is incorrect");
                RETVAL_INT(recvmmsg_alt, -1);
            }

            if (msg->msg_iovlen > msg->msg_riovlen ||
                msg->msg_namelen > msg->msg_rnamelen)
            {
                rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
                RETVAL_INT(recvmmsg_alt, -1);
            }

            for (i = 0; i < msg->msg_riovlen && msg->msg_iov != NULL; i++)
            {
                iovec_arr[j][i].iov_base.iov_base_val =
                    msg->msg_iov[i].iov_base;
                iovec_arr[j][i].iov_base.iov_base_len =
                    msg->msg_iov[i].iov_rlen;
                iovec_arr[j][i].iov_len = msg->msg_iov[i].iov_len;
            }

            if (msg->msg_iov != NULL)
            {
                rpc_msg->msg_iov.msg_iov_val = iovec_arr[j];
                rpc_msg->msg_iov.msg_iov_len = msg->msg_riovlen;
            }
            rpc_msg->msg_iovlen = msg->msg_iovlen;

            rpc_msg->msg_namelen = msg->msg_namelen;
            sockaddr_raw2rpc(msg->msg_name, msg->msg_rnamelen,
                             &rpc_msg->msg_name);

            rpc_msg->msg_flags = msg->msg_flags;

            if (msg->msg_control != NULL)
            {
                rpc_msg->msg_control.msg_control_val = cmsg_hdrs[j];
                rpc_msg->msg_control.msg_control_len = msg->msg_cmsghdr_num;
                cmsg_hdrs[j][0].data.data_val = msg->msg_control;
                cmsg_hdrs[j][0].data.data_len = msg->msg_controllen -
                                             msg->msg_cmsghdr_num *
                                        CMSG_ALIGN(sizeof(struct cmsghdr));
            }
        }
    }

    rcf_rpc_call(rpcs, "recvmmsg_alt", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(recvmmsg_alt, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT &&
        mmsg != NULL && out.mmsg.mmsg_val != NULL)
    {
        for (j = 0; j < out.mmsg.mmsg_len; j++)
        {
            msg = &mmsg[j].msg_hdr;
            rpc_msg = &out.mmsg.mmsg_val[j].msg_hdr;
            mmsg[j].msg_len = out.mmsg.mmsg_val[j].msg_len;

            sockaddr_rpc2h(&rpc_msg->msg_name, msg->msg_name,
                           msg->msg_rnamelen,
                           NULL, &msg->msg_namelen);
            msg->msg_namelen = rpc_msg->msg_namelen;

            for (i = 0; i < msg->msg_riovlen && msg->msg_iov != NULL; i++)
            {
                msg->msg_iov[i].iov_len =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_len;
                memcpy(msg->msg_iov[i].iov_base,
                       rpc_msg->msg_iov.msg_iov_val[i].iov_base.
                                                        iov_base_val,
                       msg->msg_iov[i].iov_rlen);
            }
            if (msg->msg_control != NULL)
            {
                uint8_t *first_cmsg =
                            (uint8_t *)CMSG_FIRSTHDR((struct msghdr *)msg);
                int      retval;

                retval = msg_control_rpc2h(
                                    rpc_msg->msg_control.msg_control_val,
                                    rpc_msg->msg_control.msg_control_len,
                                    first_cmsg,
                                    &msg->msg_controllen);
                if (retval != 0)
                {
                    ERROR("%s(): cmsghdr data conversion failed",
                          __FUNCTION__);
                    rpcs->_errno = TE_RC(TE_RCF, retval);
                }
            }

            msg->msg_flags = (rpc_send_recv_flags)rpc_msg->msg_flags;

            snprintf(str_buf + strlen(str_buf),
                     sizeof(str_buf) - strlen(str_buf),
                     "{{msg_name: %p, msg_namelen: %" TE_PRINTF_SOCKLEN_T
                     "d, msg_iov: %p, msg_iovlen: %" TE_PRINTF_SIZE_T "d, "
                     "msg_control: %p, msg_controllen: "
                     "%" TE_PRINTF_SOCKLEN_T "d, msg_flags: %s}, %d}",
                     msg->msg_name, msg->msg_namelen,
                     msg->msg_iov, msg->msg_iovlen,
                     msg->msg_control, msg->msg_controllen,
                     send_recv_flags_rpc2str(msg->msg_flags),
                     mmsg[j].msg_len);
        }
    }

    snprintf(str_buf + strlen(str_buf), sizeof(str_buf) - strlen(str_buf),
             "), %d, %s, ", vlen, send_recv_flags_rpc2str(flags));
    if (timeout == NULL)
        snprintf(str_buf + strlen(str_buf), sizeof(str_buf) -
                    strlen(str_buf), "(nil)");
    else
        snprintf(str_buf + strlen(str_buf), sizeof(str_buf) -
                    strlen(str_buf),
                 "{%lld,%lld}", (long long int)timeout->tv_sec,
                 (long long int)timeout->tv_nsec);

    TAPI_RPC_LOG(rpcs, recvmmsg_alt, "%d, %p(%s", "%d",
                 fd, mmsg, str_buf, out.retval);
    RETVAL_INT(recvmmsg_alt, out.retval);
}

int
rpc_sendmmsg_alt(rcf_rpc_server *rpcs, int fd, struct rpc_mmsghdr *mmsg,
                 unsigned int vlen, rpc_send_recv_flags flags)
{
    char                   str_buf[1024];
    rcf_rpc_op             op;
    tarpc_sendmmsg_alt_in  in;
    tarpc_sendmmsg_alt_out out;

    struct rpc_msghdr *msg;
    unsigned int       j;

    struct tarpc_mmsghdr rpc_mmsg[RCF_RPC_MAX_MSGHDR];
    struct tarpc_msghdr *rpc_msg;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(rpc_mmsg, 0, sizeof(rpc_mmsg));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendmmsg_alt, -1);
    }

    op = rpcs->op;
    in.fd = fd;
    in.flags = flags;
    in.vlen = vlen;

    if (mmsg != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.mmsg.mmsg_val = rpc_mmsg;
        in.mmsg.mmsg_len = vlen;

        for (j = 0; j < vlen; j++)
        {
            msg = &mmsg[j].msg_hdr;
            rpc_msg = &rpc_mmsg[j].msg_hdr;
            rpc_mmsg[j].msg_len = mmsg[j].msg_len;

            msghdr_rpc2tarpc(msg, rpc_msg);
        }
    }

    rcf_rpc_call(rpcs, "sendmmsg_alt", &in, &out);

    for (j = 0; j < vlen; j++)
    {
        rpc_msg = &rpc_mmsg[j].msg_hdr;
        tarpc_msghdr_free(rpc_msg);
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendmmsg_alt, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT &&
        mmsg != NULL && out.mmsg.mmsg_val != NULL)
    {
        for (j = 0; j < out.mmsg.mmsg_len; j++)
        {
            msg = &mmsg[j].msg_hdr;
            rpc_msg = &out.mmsg.mmsg_val[j].msg_hdr;

            mmsg[j].msg_len = out.mmsg.mmsg_val[j].msg_len;

            snprintf(str_buf + strlen(str_buf),
                     sizeof(str_buf) - strlen(str_buf),
                     "{{msg_name: %p, msg_namelen: %" TE_PRINTF_SOCKLEN_T
                     "d, msg_iov: %p, msg_iovlen: %" TE_PRINTF_SIZE_T "d, "
                     "msg_control: %p, msg_controllen: "
                     "%" TE_PRINTF_SOCKLEN_T "d, msg_flags: %s}, %d}",
                     msg->msg_name, msg->msg_namelen,
                     msg->msg_iov, msg->msg_iovlen,
                     msg->msg_control, msg->msg_controllen,
                     send_recv_flags_rpc2str(msg->msg_flags),
                     mmsg[j].msg_len);
        }
    }

    snprintf(str_buf + strlen(str_buf), sizeof(str_buf) - strlen(str_buf),
             "), %d, %s", vlen, send_recv_flags_rpc2str(flags));
    TAPI_RPC_LOG(rpcs, sendmmsg_alt, "%d, %p(%s", "%d",
                 fd, mmsg, str_buf, out.retval);
    RETVAL_INT(sendmmsg_alt, out.retval);
}

int
rpc_socket_connect_close(rcf_rpc_server *rpcs,
                         const struct sockaddr *addr,
                         uint32_t time2run)
{
    tarpc_socket_connect_close_in  in;
    tarpc_socket_connect_close_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(socket_connect_close, -1);
    }

    in.time2run = time2run;
    sockaddr_input_h2rpc(addr, &in.addr);

    rcf_rpc_call(rpcs, "socket_connect_close", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(socket_connect_close, out.retval);
    TAPI_RPC_LOG(rpcs, socket_connect_close, "%s, %d", "%d",
                 sockaddr_h2str(addr), time2run, out.retval);
    RETVAL_INT(socket_connect_close, out.retval);
}

int
rpc_socket_listen_close(rcf_rpc_server *rpcs,
                        const struct sockaddr *addr,
                        uint32_t time2run)
{
    tarpc_socket_listen_close_in  in;
    tarpc_socket_listen_close_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(socket_listen_close, -1);
    }

    in.time2run = time2run;
    sockaddr_input_h2rpc(addr, &in.addr);

    rcf_rpc_call(rpcs, "socket_listen_close", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(socket_listen_close, out.retval);
    TAPI_RPC_LOG(rpcs, socket_listen_close, "%s, %d", "%d",
                 sockaddr_h2str(addr), time2run, out.retval);
    RETVAL_INT(socket_listen_close, out.retval);
}
