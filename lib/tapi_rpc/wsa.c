/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of WSA-specific routines
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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

#include "tapi_rpc_internal.h"
#include "tapi_rpc_winsock2.h"


int
rpc_wsa_socket(rcf_rpc_server *rpcs,
               rpc_socket_domain domain, rpc_socket_type type,
               rpc_socket_proto protocol, uint8_t *info, int info_len,
               te_bool overlapped)
{
    tarpc_socket_in  in;
    tarpc_socket_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.domain = domain;
    in.type = type;
    in.proto = protocol;
    in.info.info_val = info;
    in.info.info_len = info_len;
    in.flags = overlapped;

    rcf_rpc_call(rpcs, _socket, &in, (xdrproc_t)xdr_tarpc_socket_in,
                 &out, (xdrproc_t)xdr_tarpc_socket_out);

    RING("RPC (%s,%s): socket(%s, %s, %s) -> %d (%s)",
         rpcs->ta, rpcs->name,
         domain_rpc2str(domain), socktype_rpc2str(type),
         proto_rpc2str(protocol), out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VAL(out.fd, socket);
}

int
rpc_connect_ex(rcf_rpc_server *rpcs,
               int s, const struct sockaddr *addr,
               socklen_t addrlen,
               void *buf, ssize_t len_buf,
               ssize_t *bytes_sent,
               rpc_overlapped overlapped)
{
    rcf_rpc_op        op;
    tarpc_connect_ex_in  in;
    tarpc_connect_ex_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }
    op = rpcs->op;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.fd = s;
    if (addr != NULL)
    {
        if (addrlen >= SA_COMMON_LEN)
        {
            in.addr.sa_family = addr_family_h2rpc(addr->sa_family);
            in.addr.sa_data.sa_data_len = addrlen - SA_COMMON_LEN;
            in.addr.sa_data.sa_data_val = (uint8_t *)(addr->sa_data);
        }
        else
        {
            in.addr.sa_family = RPC_AF_UNSPEC;
            in.addr.sa_data.sa_data_len = 0;
            /* Any not-NULL pointer is suitable here */
            in.addr.sa_data.sa_data_val = (uint8_t *)addr;
        }
    }
    in.len = addrlen;
    if (buf == NULL)
        in.buf.buf_len = 0;
    else
        in.buf.buf_len = len_buf;

    if (bytes_sent == NULL)
        in.len_sent.len_sent_len = 0;
    else
        in.len_sent.len_sent_len = 1;

    in.buf.buf_val = buf;
    in.len_sent.len_sent_val = bytes_sent;
    in.len_buf = len_buf;
    in.overlapped = (tarpc_overlapped)overlapped;

    rcf_rpc_call(rpcs, _connect_ex,
                 &in, (xdrproc_t)xdr_tarpc_connect_ex_in,
                 &out, (xdrproc_t)xdr_tarpc_connect_ex_out);

    RING("RPC (%s,%s)%s: connect_ex(%d, %s, %u, ..., %p, ...) -> %s (%s)",
         rpcs->ta, rpcs->name, rpcop2str(op),
         s, sockaddr2str(addr), addrlen, overlapped,
         out.retval ? "true" : "false", errno_rpc2str(RPC_ERRNO(rpcs)));

    if (bytes_sent != NULL)
        *bytes_sent = out.len_sent.len_sent_val[0];

    RETVAL_VAL(out.retval, connect_ex);
}

int
rpc_disconnect_ex(rcf_rpc_server *rpcs, int s,
                  rpc_overlapped overlapped, int flags)
{
    rcf_rpc_op        op;
    tarpc_disconnect_ex_in  in;
    tarpc_disconnect_ex_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }
    op = rpcs->op;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.fd = s;
    in.overlapped = (tarpc_overlapped)overlapped;
    in.flags = flags;
    rcf_rpc_call(rpcs, _disconnect_ex, &in,
                 (xdrproc_t)xdr_tarpc_disconnect_ex_in,
                 &out, (xdrproc_t)xdr_tarpc_disconnect_ex_out);

    RING("RPC (%s,%s)%s: disconnect_ex(%d, %p, %d) -> %s (%s)",
         rpcs->ta, rpcs->name, rpcop2str(op),
         s, overlapped, flags,
         out.retval ? "true" : "false", errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VAL(out.retval, disconnect_ex);
}

int
rpc_wsa_accept(rcf_rpc_server *rpcs,
               int s, struct sockaddr *addr,
               socklen_t *addrlen, socklen_t raddrlen,
               accept_cond *cond, int cond_num)
{
    rcf_rpc_op       op;
    socklen_t        save_addrlen =
                         (addrlen == NULL) ? (socklen_t)-1 : *addrlen;
    tarpc_wsa_accept_in  in;
    tarpc_wsa_accept_out out;

    struct tarpc_accept_cond rpc_cond[RCF_RPC_MAX_ACCEPT_CONDS];

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    if (cond_num > RCF_RPC_MAX_ACCEPT_CONDS)
    {
        ERROR("Too many conditions are specified for WSAAccept condition"
              "function");
        rpcs->_errno = TE_RC(TE_RCF, EINVAL);
        return -1;
    }

    if ((cond == NULL && cond_num > 0) ||
        (cond != NULL && cond_num == 0))
    {
        rpcs->_errno = TE_RC(TE_RCF, EINVAL);
        return -1;
    }

    op = rpcs->op;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.fd = s;
    if (addrlen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.len.len_len = 1;
        in.len.len_val = addrlen;
    }
    if (addr != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        if (raddrlen >= SA_COMMON_LEN)
        {
            in.addr.sa_family = addr_family_h2rpc(addr->sa_family);
            in.addr.sa_data.sa_data_len = raddrlen - SA_COMMON_LEN;
            in.addr.sa_data.sa_data_val = addr->sa_data;
        }
        else
        {
            in.addr.sa_family = RPC_AF_UNSPEC;
            in.addr.sa_data.sa_data_len = 0;
            /* Any not-NULL pointer is suitable here */
            in.addr.sa_data.sa_data_val = (uint8_t *)addr;
        }
    }

    if (cond != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        int i;

        in.cond.cond_len = cond_num;
        in.cond.cond_val = rpc_cond;
        for (i = 0; i < cond_num; i++)
        {
            rpc_cond[i].port = cond[i].port;
            rpc_cond[i].verdict = cond[i].verdict == CF_ACCEPT ?
                                  TARPC_CF_ACCEPT :
                                  cond[i].verdict == CF_REJECT ?
                                  TARPC_CF_REJECT : TARPC_CF_DEFER;
        }
    }

    rcf_rpc_call(rpcs, _wsa_accept,
                 &in,  (xdrproc_t)xdr_tarpc_wsa_accept_in,
                 &out, (xdrproc_t)xdr_tarpc_wsa_accept_out);

    if (RPC_CALL_OK)
    {
        if (addr != NULL && out.addr.sa_data.sa_data_val != NULL)
        {
            memcpy(addr->sa_data, out.addr.sa_data.sa_data_val,
                   out.addr.sa_data.sa_data_len);
            addr->sa_family = addr_family_rpc2h(out.addr.sa_family);
        }

        if (addrlen != NULL && out.len.len_val != NULL)
            *addrlen = out.len.len_val[0];
    }

    RING("RPC (%s,%s)%s: WSAAccept(%d, %p[%u], %p(%u)) -> %d (%s) "
         "peer=%s addrlen=%u",
         rpcs->ta, rpcs->name, rpcop2str(op),
         s, addr, raddrlen, addrlen, save_addrlen,
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
         sockaddr2str(addr), (addrlen == NULL) ? (socklen_t)-1 : *addrlen);

    RETVAL_VAL(out.retval, wsa_accept);
}


int
rpc_accept_ex(rcf_rpc_server *rpcs, int s, int s_a,
              size_t len, rpc_overlapped overlapped,
              size_t *bytes_received)
{
    rcf_rpc_op          op;

    tarpc_accept_ex_in  in;
    tarpc_accept_ex_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.fd = s;
    in.fd_a = s_a;
    in.buflen = len;
    if (bytes_received == NULL)
        in.count.count_len = 0;
    else
        in.count.count_len = 1;
    in.count.count_val = bytes_received;
    in.overlapped = (tarpc_overlapped)overlapped;
    rcf_rpc_call(rpcs, _accept_ex,
                 &in, (xdrproc_t)xdr_tarpc_accept_ex_in,
                 &out, (xdrproc_t)xdr_tarpc_accept_ex_out);
    if (RPC_CALL_OK)
    {
        if ((bytes_received != NULL) && (out.count.count_val != 0))
            *bytes_received = out.count.count_val[0];
    }
    RING("RPC (%s,%s)%s: accept_ex(%d, %d, %d, ...) -> %s (%s %s) "
         "bytes received %u",
         rpcs->ta, rpcs->name, rpcop2str(op),
         s, s_a, len,
         out.retval ? "true" : "false", errno_rpc2str(RPC_ERRNO(rpcs)),
         win_error_rpc2str(out.common.win_error),
         (bytes_received == NULL) ? (size_t)-1 : *bytes_received);
    RETVAL_VAL(out.retval, accept_ex);
}

void
rpc_get_accept_addr(rcf_rpc_server *rpcs,
                    int s, void *buf, size_t buflen, size_t len,
                    struct sockaddr *laddr,
                    struct sockaddr *raddr)
{
    tarpc_get_accept_addr_in  in;
    tarpc_get_accept_addr_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return;
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.fd = s;
    in.buflen = len;
    if (laddr != NULL)
    {
        in.laddr.sa_family = addr_family_h2rpc(laddr->sa_family);
        in.laddr.sa_data.sa_data_len =
            sizeof(struct sockaddr_storage) - SA_COMMON_LEN;
        in.laddr.sa_data.sa_data_val = laddr->sa_data;
    }
    if (raddr != NULL)
    {
        in.raddr.sa_family = addr_family_h2rpc(raddr->sa_family);
        in.raddr.sa_data.sa_data_len =
            sizeof(struct sockaddr_storage) - SA_COMMON_LEN;
        in.raddr.sa_data.sa_data_val = raddr->sa_data;
    }
    in.buf.buf_val = buf;
    in.buf.buf_len = buflen;
    rcf_rpc_call(rpcs, _get_accept_addr, &in,
                 (xdrproc_t)xdr_tarpc_get_accept_addr_in,
                 &out, (xdrproc_t)xdr_tarpc_get_accept_addr_out);
    if (RPC_CALL_OK)
    {
        if ((laddr != NULL) && (out.laddr.sa_data.sa_data_val != NULL))
        {
            memcpy(laddr->sa_data, out.laddr.sa_data.sa_data_val,
                   out.laddr.sa_data.sa_data_len);
            laddr->sa_family = addr_family_rpc2h(out.laddr.sa_family);
        }
        if ((raddr != NULL) && (out.raddr.sa_data.sa_data_val != NULL))
        {
            memcpy(raddr->sa_data, out.raddr.sa_data.sa_data_val,
                   out.raddr.sa_data.sa_data_len);
            raddr->sa_family = addr_family_rpc2h(out.raddr.sa_family);
        }
    }
    RING("RPC (%s,%s): get_accept_addr(%d, ...) -> "
         "(%s %s) laddr=%s raddr=%s",
         rpcs->ta, rpcs->name,
         s, errno_rpc2str(RPC_ERRNO(rpcs)),
         win_error_rpc2str(out.common.win_error),
         ((out.laddr.sa_data.sa_data_val == NULL) || (laddr == NULL)) ?
         "NULL" : sockaddr2str(laddr),
         ((out.raddr.sa_data.sa_data_val == NULL) || (raddr == NULL)) ?
         "NULL" : sockaddr2str(raddr));
    RETVAL_VOID(get_accept_addr);
}

int
rpc_transmit_file(rcf_rpc_server *rpcs, int s, char *file,
                  ssize_t len, ssize_t len_per_send,
                  rpc_overlapped overlapped,
                  void *head, ssize_t head_len,
                  void *tail, ssize_t tail_len, ssize_t flags)
{
    rcf_rpc_op        op;
    tarpc_transmit_file_in  in;
    tarpc_transmit_file_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }
#if 0
    if ((buf != NULL) && (bytes_sent == NULL))
    {
        return -1;
    }
#endif
    op = rpcs->op;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.fd = s;
    if (file == NULL)
        in.file.file_len = 0;
    else
        in.file.file_len = strlen(file) + 1;
    in.file.file_val = file;
    in.len = len;
    in.len_per_send = len_per_send;
    in.overlapped = (tarpc_overlapped)overlapped;
    if (head != NULL)
    {
        in.head.head_val = head;
        in.head.head_len = head_len;
    }
    if (tail != NULL)
    {
        in.tail.tail_val = tail;
        in.tail.tail_len = tail_len;
    }
    in.flags = flags;
    rcf_rpc_call(rpcs, _transmit_file, &in,
                 (xdrproc_t)xdr_tarpc_transmit_file_in,
                 &out, (xdrproc_t)xdr_tarpc_transmit_file_out);

    RING("RPC (%s,%s)%s: transmit_file(%d, %s, %d, %d, %p, ...) -> %s (%s)",
         rpcs->ta, rpcs->name, rpcop2str(op),
         s, file, len, len_per_send, overlapped,
         out.retval ? "true" : "false", errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VAL(out.retval, transmit_file);
}

ssize_t
rpc_wsa_recv_ex(rcf_rpc_server *rpcs,
                int s, void *buf, size_t len,
                rpc_send_recv_flags *flags, size_t rbuflen)
{
    rcf_rpc_op            op;
    tarpc_wsa_recv_ex_in  in;
    tarpc_wsa_recv_ex_out out;

    rpc_send_recv_flags in_flags = flags == NULL ? 0 : *flags;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (buf != NULL && len > rbuflen)
    {
        rpcs->_errno = TE_RC(TE_RCF, EINVAL);
        return -1;
    }

    in.fd = s;
    in.len = len;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = rbuflen;
        in.buf.buf_val = buf;
    }
    if (flags != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.flags.flags_len = 1;
        in.flags.flags_val = (int *)flags;
    }

    rcf_rpc_call(rpcs, _wsa_recv_ex,
                 &in, (xdrproc_t)xdr_tarpc_wsa_recv_ex_in,
                 &out, (xdrproc_t)xdr_tarpc_wsa_recv_ex_out);

    if (RPC_CALL_OK)
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);

        if (flags != NULL && out.flags.flags_val != NULL)
            *flags = out.flags.flags_val[0];
    }

    RING("RPC (%s,%s)%s: WSARecvEx(%d, %p[%u], 0x%X (%u->%u), %s) -> "
         "%d (%s %s)",
         rpcs->ta, rpcs->name, rpcop2str(op),
         s, buf, rbuflen, len,
         flags, send_recv_flags_rpc2str(in_flags),
         send_recv_flags_rpc2str(flags == NULL ? 0 : *flags),
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
         win_error_rpc2str(out.common.win_error));

    RETVAL_VAL(out.retval, wsa_recv_ex);
}

rpc_wsaevent
rpc_create_event(rcf_rpc_server *rpcs)
{
    tarpc_create_event_in  in;
    tarpc_create_event_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return NULL;
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rcf_rpc_call(rpcs, _create_event, &in,
                 (xdrproc_t)xdr_tarpc_create_event_in,
                 &out, (xdrproc_t)xdr_tarpc_create_event_out);

    RING("RPC (%s,%s): create_event() -> %p (%s)",
         rpcs->ta, rpcs->name,
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_PTR(out.retval, create_event);
}

int
rpc_close_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent)
{
    tarpc_close_event_in  in;
    tarpc_close_event_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return FALSE;
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.hevent = (tarpc_wsaevent)hevent;

    rcf_rpc_call(rpcs, _close_event,
                 &in, (xdrproc_t)xdr_tarpc_close_event_in,
                 &out, (xdrproc_t)xdr_tarpc_close_event_out);

    RING("RPC (%s,%s): close_event(%p) -> %s (%s)",
         rpcs->ta, rpcs->name, hevent,
         out.retval ? "true" : "false", errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VAL(out.retval, close_event);
}

int
rpc_reset_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent)
{
    tarpc_reset_event_in  in;
    tarpc_reset_event_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return FALSE;
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.hevent = (tarpc_wsaevent)hevent;

    rcf_rpc_call(rpcs, _reset_event,
                 &in, (xdrproc_t)xdr_tarpc_reset_event_in,
                 &out, (xdrproc_t)xdr_tarpc_reset_event_out);

    RING("RPC (%s,%s): reset_event(%p) -> %s (%s)",
         rpcs->ta, rpcs->name, hevent,
         out.retval ? "true" : "false", errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VAL(out.retval, reset_event);
}

rpc_overlapped
rpc_create_overlapped(rcf_rpc_server *rpcs, rpc_wsaevent hevent,
                      unsigned int offset, unsigned int offset_high)
{
    tarpc_create_overlapped_in  in;
    tarpc_create_overlapped_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return NULL;
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.hevent = (tarpc_wsaevent)hevent;
    in.offset = offset;
    in.offset_high = offset_high;

    rcf_rpc_call(rpcs, _create_overlapped, &in,
                 (xdrproc_t)xdr_tarpc_create_overlapped_in,
                 &out, (xdrproc_t)xdr_tarpc_create_overlapped_out);

    RING("RPC (%s,%s): create_overlapped(%p) -> %p (%s)",
         rpcs->ta, rpcs->name, hevent,
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_PTR(out.retval, create_overlapped);
}

void
rpc_delete_overlapped(rcf_rpc_server *rpcs,
                      rpc_overlapped overlapped)
{
    tarpc_delete_overlapped_in  in;
    tarpc_delete_overlapped_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return;
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.overlapped = (tarpc_overlapped)overlapped;

    rcf_rpc_call(rpcs, _delete_overlapped,
                 &in, (xdrproc_t)xdr_tarpc_delete_overlapped_in,
                 &out, (xdrproc_t)xdr_tarpc_delete_overlapped_out);

    RING("RPC (%s,%s): delete_overlapped(%p)",
         rpcs->ta, rpcs->name, overlapped);

    RETVAL_VOID(delete_overlapped);
}

int
rpc_completion_callback(rcf_rpc_server *rpcs,
                        int *called, int *error, int *bytes,
                        rpc_overlapped *overlapped)
{
    tarpc_completion_callback_in  in;
    tarpc_completion_callback_out out;

    int rc = 0;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }
    if (called == NULL || error == NULL || bytes == NULL ||
        overlapped == NULL)
    {
        rpcs->_errno = TE_RC(TE_RCF, EINVAL);
        return -1;
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rcf_rpc_call(rpcs, _completion_callback,
                 &in, (xdrproc_t)xdr_tarpc_completion_callback_in,
                 &out, (xdrproc_t)xdr_tarpc_completion_callback_out);

    RING("RPC (%s,%s): completion_callback() -> %d %d %d %p",
         rpcs->ta, rpcs->name, out.called, out.error, out.bytes,
         out.overlapped);

    if (RPC_CALL_OK)
    {
        *called = out.called;
        *error = out.error;
        *bytes = out.bytes;
        *overlapped = (rpc_overlapped)(out.overlapped);
    }
    else
        rc = -1;

    RETVAL_VAL(rc, completion_callback);
}

int
rpc_wsa_event_select(rcf_rpc_server *rpcs,
                     int s, rpc_wsaevent event_object,
                     rpc_network_event event)
{
    tarpc_event_select_in  in;
    tarpc_event_select_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.fd = s;
    in.event_object = (tarpc_wsaevent)event_object;
    in.event = event;

    rcf_rpc_call(rpcs, _event_select,
                 &in,  (xdrproc_t)xdr_tarpc_event_select_in,
                 &out, (xdrproc_t)xdr_tarpc_event_select_out);

    RING("RPC (%s,%s): event_select(%d, 0x%X, %s) -> %d (%s)",
         rpcs->ta, rpcs->name,
         s, event_object, network_event_rpc2str(event), out.retval,
         errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RC(event_select);
}

int
rpc_enum_network_events(rcf_rpc_server *rpcs,
                        int s, rpc_wsaevent event_object,
                        rpc_network_event *event)
{
    tarpc_enum_network_events_in  in;
    tarpc_enum_network_events_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.fd = s;
    in.event_object = (tarpc_wsaevent)event_object;
    if (event == NULL)
        in.event.event_len = 0;
    else
        in.event.event_len = 1;
    in.event.event_val = (unsigned long *)event;

    rcf_rpc_call(rpcs, _enum_network_events,
                 &in,  (xdrproc_t)xdr_tarpc_enum_network_events_in,
                 &out, (xdrproc_t)xdr_tarpc_enum_network_events_out);
    if (RPC_CALL_OK)
    {
        if (event != NULL && out.event.event_val != NULL)
            *event = out.event.event_val[0];
    }
    RING("RPC (%s,%s): enum_network_events(%d, %d, 0x%X) -> %d (%s) "
         "returned event %s",
         rpcs->ta, rpcs->name, s, event_object, event,
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
         network_event_rpc2str(event != NULL ? *event : 0));

    RETVAL_RC(enum_network_events);
}

rpc_hwnd
rpc_create_window(rcf_rpc_server *rpcs)
{
    tarpc_create_window_in  in;
    tarpc_create_window_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return NULL;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    rcf_rpc_call(rpcs, _create_window,
                 &in, (xdrproc_t)xdr_tarpc_create_window_in,
                 &out, (xdrproc_t)xdr_tarpc_create_window_out);

    RING("RPC (%s,%s): create_window() -> %p (%s)",
         rpcs->ta, rpcs->name,
         out.hwnd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_PTR(out.hwnd, create_window);
}

void
rpc_destroy_window(rcf_rpc_server *rpcs, rpc_hwnd hwnd)
{
    tarpc_destroy_window_in  in;
    tarpc_destroy_window_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.hwnd = (tarpc_hwnd)hwnd;

    rcf_rpc_call(rpcs, _destroy_window,
                 &in,  (xdrproc_t)xdr_tarpc_destroy_window_in,
                 &out, (xdrproc_t)xdr_tarpc_destroy_window_out);

    RING("RPC (%s,%s): destroy_window(%p) -> (%s)",
         rpcs->ta, rpcs->name,
         hwnd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(destroy_window);
}

int
rpc_wsa_async_select(rcf_rpc_server *rpcs,
                     int s, rpc_hwnd hwnd, rpc_network_event event)
{
    tarpc_wsa_async_select_in  in;
    tarpc_wsa_async_select_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.hwnd = (tarpc_hwnd)hwnd;
    in.sock = s;
    in.event = event;

    rcf_rpc_call(rpcs, _wsa_async_select,
                 &in,  (xdrproc_t)xdr_tarpc_wsa_async_select_in,
                 &out, (xdrproc_t)xdr_tarpc_wsa_async_select_out);

    RING("RPC (%s,%s): wsa_async_select(%p, %d, %s) -> %d (%s)",
         rpcs->ta, rpcs->name,
         hwnd, s, network_event_rpc2str(event),
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RC(wsa_async_select);
}

int
rpc_peek_message(rcf_rpc_server *rpcs,
                     rpc_hwnd hwnd, int *s, rpc_network_event *event)
{
    tarpc_peek_message_in  in;
    tarpc_peek_message_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return 0;
    }

    if (s == NULL || event == NULL)
    {
        ERROR("Null pointer is passed to rpc_peek_msg");
        return 0;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.hwnd = (tarpc_hwnd)hwnd;

    rcf_rpc_call(rpcs, _peek_message,
                 &in,  (xdrproc_t)xdr_tarpc_peek_message_in,
                 &out, (xdrproc_t)xdr_tarpc_peek_message_out);

    RING("RPC (%s,%s): peek_message(%p) -> %d (%s) event %s",
         rpcs->ta, rpcs->name,
         hwnd, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
         network_event_rpc2str(out.event));

    *s = out.sock;
    *event = out.event;

    RETVAL_VAL(out.retval, wsa_async_select);
}

int
rpc_wsa_send(rcf_rpc_server *rpcs,
             int s, const struct rpc_iovec *iov,
             size_t iovcnt, rpc_send_recv_flags flags,
             int *bytes_sent, rpc_overlapped overlapped,
             te_bool callback)
{
    rcf_rpc_op       op;
    tarpc_wsa_send_in  in;
    tarpc_wsa_send_out out;

    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    size_t i;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (iovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, ENOMEM);
        return -1;
    }

    for (i = 0; i < iovcnt && iov != NULL; i++)
    {
        iovec_arr[i].iov_base.iov_base_val = iov[i].iov_base;
        iovec_arr[i].iov_base.iov_base_len = iov[i].iov_rlen;
        iovec_arr[i].iov_len = iov[i].iov_len;
    }

    if (iov != NULL)
    {
        in.vector.vector_val = iovec_arr;
        in.vector.vector_len = iovcnt;
    }

    in.s = s;
    in.count = iovcnt;
    in.overlapped = (tarpc_overlapped)overlapped;
    in.callback = callback;
    if (bytes_sent != NULL)
    {
        in.bytes_sent.bytes_sent_len = 1;
        in.bytes_sent.bytes_sent_val = bytes_sent;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, _wsa_send, &in, (xdrproc_t)xdr_tarpc_wsa_send_in,
                 &out, (xdrproc_t)xdr_tarpc_wsa_send_out);

    RING("RPC (%s,%s)%s: wsa_send() -> %d (%s)",
         rpcs->ta, rpcs->name, rpcop2str(op), out.retval,
         errno_rpc2str(RPC_ERRNO(rpcs)));

    if (RPC_CALL_OK)
    {
        if (bytes_sent != NULL && out.bytes_sent.bytes_sent_val != NULL)
            *bytes_sent = out.bytes_sent.bytes_sent_val[0];
    }

    RETVAL_RC(wsa_send);
}

int
rpc_wsa_recv(rcf_rpc_server *rpcs,
             int s, const struct rpc_iovec *iov,
             size_t iovcnt, size_t riovcnt,
             rpc_send_recv_flags *flags,
             int *bytes_received, rpc_overlapped overlapped,
             te_bool callback)
{
    rcf_rpc_op      op;
    tarpc_wsa_recv_in  in;
    tarpc_wsa_recv_out out;

    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    size_t i;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (riovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, ENOMEM);
        return -1;
    }

    if (iov != NULL && iovcnt > riovcnt)
    {
        rpcs->_errno = TE_RC(TE_RCF, EINVAL);
        return -1;
    }

    in.s = s;
    in.count = iovcnt;
    in.overlapped = (tarpc_overlapped)overlapped;
    in.callback = callback;
    if (bytes_received != NULL)
    {
        in.bytes_received.bytes_received_len = 1;
        in.bytes_received.bytes_received_val = bytes_received;
    }
    if (flags != NULL)
    {
        in.flags.flags_len = 1;
        in.flags.flags_val = (int *)flags;
    }

    if (iov != NULL)
    {
        in.vector.vector_len = riovcnt;
        in.vector.vector_val = iovec_arr;
        for (i = 0; i < riovcnt; i++)
        {
            VERB("IN wsa_recv() I/O vector #%d: %p[%u] %u",
                 i, iov[i].iov_base, iov[i].iov_rlen, iov[i].iov_len);
            iovec_arr[i].iov_base.iov_base_val = iov[i].iov_base;
            iovec_arr[i].iov_base.iov_base_len = iov[i].iov_rlen;
            iovec_arr[i].iov_len = iov[i].iov_len;
        }
    }

    rcf_rpc_call(rpcs, _wsa_recv, &in, (xdrproc_t)xdr_tarpc_wsa_recv_in,
                 &out, (xdrproc_t)xdr_tarpc_wsa_recv_out);

    if (RPC_CALL_OK && iov != NULL && out.vector.vector_val != NULL)
    {
        for (i = 0; i < riovcnt; i++)
        {
            ((struct rpc_iovec *)iov)[i].iov_len =
                out.vector.vector_val[i].iov_len;
            if ((iov[i].iov_base != NULL) &&
                (out.vector.vector_val[i].iov_base.iov_base_val != NULL))
            {
                memcpy(iov[i].iov_base,
                       out.vector.vector_val[i].iov_base.iov_base_val,
                       iov[i].iov_rlen);
            }
        }
        if (bytes_received != NULL &&
            out.bytes_received.bytes_received_val != NULL)
        {
            *bytes_received = out.bytes_received.bytes_received_val[0];
        }
        if (flags != NULL && out.flags.flags_val != NULL)
            *flags = out.flags.flags_val[0];
    }

    RING("RPC (%s,%s)%s: wsa_recv() -> %d (%s %s)",
         rpcs->ta, rpcs->name, rpcop2str(op),
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
         win_error_rpc2str(out.common.win_error));

    RETVAL_RC(wsa_recv);
}

int
rpc_wsa_send_to(rcf_rpc_server *rpcs, int s, const struct rpc_iovec *iov,
                size_t iovcnt, rpc_send_recv_flags flags, int *bytes_sent,
                const struct sockaddr *to, socklen_t tolen,
                rpc_overlapped overlapped, te_bool callback)
{
    rcf_rpc_op            op;
    tarpc_wsa_send_to_in  in;
    tarpc_wsa_send_to_out out;

    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    size_t i;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (iovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, ENOMEM);
        return -1;
    }

    for (i = 0; i < iovcnt && iov != NULL; i++)
    {
        iovec_arr[i].iov_base.iov_base_val = iov[i].iov_base;
        iovec_arr[i].iov_base.iov_base_len = iov[i].iov_rlen;
        iovec_arr[i].iov_len = iov[i].iov_len;
    }

    if (iov != NULL)
    {
        in.vector.vector_val = iovec_arr;
        in.vector.vector_len = iovcnt;
    }

    in.s = s;
    in.count = iovcnt;
    in.overlapped = (tarpc_overlapped)overlapped;
    in.callback = callback;
    if (bytes_sent != NULL)
    {
        in.bytes_sent.bytes_sent_len = 1;
        in.bytes_sent.bytes_sent_val = bytes_sent;
    }
    in.flags = flags;

    if (to != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        if (tolen >= SA_COMMON_LEN)
        {
            in.to.sa_family = addr_family_h2rpc(to->sa_family);
            in.to.sa_data.sa_data_len = tolen - SA_COMMON_LEN;
            in.to.sa_data.sa_data_val = (uint8_t *)(to->sa_data);
        }
        else
        {
            in.to.sa_family = RPC_AF_UNSPEC;
            in.to.sa_data.sa_data_len = 0;
            /* Any no-NULL pointer is suitable here */
            in.to.sa_data.sa_data_val = (uint8_t *)to;
        }
    }
    in.tolen = tolen;

    rcf_rpc_call(rpcs, _wsa_send_to, &in,
                 (xdrproc_t)xdr_tarpc_wsa_send_to_in,
                 &out, (xdrproc_t)xdr_tarpc_wsa_send_to_out);

    RING("RPC (%s,%s)%s: wsa_send_to(%s, %u) -> %d (%s)",
         rpcs->ta, rpcs->name, rpcop2str(op),
         sockaddr2str(to), tolen,         
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    if (RPC_CALL_OK)
    {
        if (bytes_sent != NULL && out.bytes_sent.bytes_sent_val != NULL)
            *bytes_sent = out.bytes_sent.bytes_sent_val[0];
    }

    RETVAL_RC(wsa_send_to);
}

int
rpc_wsa_recv_from(rcf_rpc_server *rpcs, int s,
                  const struct rpc_iovec *iov, size_t iovcnt,
                  size_t riovcnt, rpc_send_recv_flags *flags,
                  int *bytes_received, struct sockaddr *from,
                  socklen_t *fromlen, rpc_overlapped overlapped,
                  te_bool callback)
{
    rcf_rpc_op              op;
    tarpc_wsa_recv_from_in  in;
    tarpc_wsa_recv_from_out out;

    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    size_t i;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (riovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, ENOMEM);
        return -1;
    }

    if (iov != NULL && iovcnt > riovcnt)
    {
        rpcs->_errno = TE_RC(TE_RCF, EINVAL);
        return -1;
    }

    in.s = s;
    in.count = iovcnt;
    in.overlapped = (tarpc_overlapped)overlapped;
    in.callback = callback;
    if (bytes_received != NULL)
    {
        in.bytes_received.bytes_received_len = 1;
        in.bytes_received.bytes_received_val = bytes_received;
    }
    if (flags != NULL)
    {
        in.flags.flags_len = 1;
        in.flags.flags_val = (int *)flags;
    }

    if (iov != NULL)
    {
        in.vector.vector_len = riovcnt;
        in.vector.vector_val = iovec_arr;
        for (i = 0; i < riovcnt; i++)
        {
            VERB("IN wsa_recv_from() I/O vector #%d: %p[%u] %u",
                 i, iov[i].iov_base, iov[i].iov_rlen, iov[i].iov_len);
            iovec_arr[i].iov_base.iov_base_val = iov[i].iov_base;
            iovec_arr[i].iov_base.iov_base_len = iov[i].iov_rlen;
            iovec_arr[i].iov_len = iov[i].iov_len;
        }
    }

    if (fromlen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.fromlen.fromlen_len = 1;
        in.fromlen.fromlen_val = fromlen;
    }
    if (from != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        if ((fromlen != NULL) && (*fromlen >= SA_COMMON_LEN))
        {
            in.from.sa_family = addr_family_h2rpc(from->sa_family);
            in.from.sa_data.sa_data_len = *fromlen - SA_COMMON_LEN;
            in.from.sa_data.sa_data_val = from->sa_data;
        }
        else
        {
            in.from.sa_family = RPC_AF_UNSPEC;
            in.from.sa_data.sa_data_len = 0;
            /* Any not-NULL pointer is suitable here */
            in.from.sa_data.sa_data_val = (uint8_t *)from;
        }
    }

    rcf_rpc_call(rpcs, _wsa_recv_from, &in,
                 (xdrproc_t)xdr_tarpc_wsa_recv_from_in,
                 &out, (xdrproc_t)xdr_tarpc_wsa_recv_from_out);

    if (RPC_CALL_OK)
    {
        if (iov != NULL && out.vector.vector_val != NULL)
        {
            for (i = 0; i < riovcnt; i++)
            {
                ((struct rpc_iovec *)iov)[i].iov_len =
                    out.vector.vector_val[i].iov_len;
                if ((iov[i].iov_base != NULL) &&
                   (out.vector.vector_val[i].iov_base.iov_base_val != NULL))
                {
                    memcpy(iov[i].iov_base,
                           out.vector.vector_val[i].iov_base.iov_base_val,
                           iov[i].iov_rlen);
                }
            }
            if (bytes_received != NULL &&
                out.bytes_received.bytes_received_val != NULL)
            {
                *bytes_received = out.bytes_received.bytes_received_val[0];
            }
            if (flags != NULL && out.flags.flags_val != NULL)
                *flags = out.flags.flags_val[0];
        }

        if (from != NULL && out.from.sa_data.sa_data_val != NULL)
        {
            memcpy(from->sa_data, out.from.sa_data.sa_data_val,
                   out.from.sa_data.sa_data_len);
            from->sa_family = addr_family_rpc2h(out.from.sa_family);
        }

        if (fromlen != NULL && out.fromlen.fromlen_val != NULL)
            *fromlen = out.fromlen.fromlen_val[0];    
    }

    RING("RPC (%s,%s)%s: wsa_recv_from(%s, %u) -> %d (%s %s)",
         rpcs->ta, rpcs->name, rpcop2str(op), sockaddr2str(from),
         (fromlen == NULL) ? (unsigned int)-1 : *fromlen,
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
         win_error_rpc2str(out.common.win_error));

    RETVAL_RC(wsa_recv_from);
}

int
rpc_wsa_send_disconnect(rcf_rpc_server *rpcs,
                        int s, const struct rpc_iovec *iov)
{
    rcf_rpc_op                    op;
    tarpc_wsa_send_disconnect_in  in;
    tarpc_wsa_send_disconnect_out out;
    struct tarpc_iovec            iovec_arr;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&iovec_arr, 0, sizeof(iovec_arr));

    in.s = s;

    if (iov != NULL)
    {
        iovec_arr.iov_base.iov_base_val = iov->iov_base;
        iovec_arr.iov_base.iov_base_len = iov->iov_rlen;
        iovec_arr.iov_len = iov->iov_len;

        in.vector.vector_val = &iovec_arr;
        in.vector.vector_len = 1;
    }

    rcf_rpc_call(rpcs, _wsa_send_disconnect, &in,
                 (xdrproc_t)xdr_tarpc_wsa_send_disconnect_in,
                 &out, (xdrproc_t)xdr_tarpc_wsa_send_disconnect_out);

    RING("RPC (%s,%s)%s: wsa_send_disconnect() -> %d (%s)",
         rpcs->ta, rpcs->name, rpcop2str(op), out.retval,
         errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RC(wsa_send_disconnect);
}

int
rpc_wsa_recv_disconnect(rcf_rpc_server *rpcs,
                        int s, const struct rpc_iovec *iov)
{
    rcf_rpc_op                    op;
    tarpc_wsa_recv_disconnect_in  in;
    tarpc_wsa_recv_disconnect_out out;
    struct tarpc_iovec            iovec_arr;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&iovec_arr, 0, sizeof(iovec_arr));

    in.s = s;

    if (iov != NULL)
    {
        iovec_arr.iov_base.iov_base_val = iov->iov_base;
        iovec_arr.iov_base.iov_base_len = iov->iov_rlen;
        iovec_arr.iov_len = iov->iov_len;

        in.vector.vector_val = &iovec_arr;
        in.vector.vector_len = 1;
    }

    rcf_rpc_call(rpcs, _wsa_recv_disconnect, &in,
                 (xdrproc_t)xdr_tarpc_wsa_recv_disconnect_in,
                 &out, (xdrproc_t)xdr_tarpc_wsa_recv_disconnect_out);

    if (RPC_CALL_OK && iov != NULL && out.vector.vector_val != NULL)
    {
        ((struct rpc_iovec *)iov)->iov_len = out.vector.vector_val->iov_len;
        if ((iov->iov_base != NULL) &&
            (out.vector.vector_val->iov_base.iov_base_val != NULL))
        {
            memcpy(iov->iov_base,
                   out.vector.vector_val->iov_base.iov_base_val,
                   iov->iov_rlen);
        }
    }

    RING("RPC (%s,%s)%s: wsa_recv_disconnect() -> %d (%s %s)",
         rpcs->ta, rpcs->name, rpcop2str(op),
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
         win_error_rpc2str(out.common.win_error));

    RETVAL_RC(wsa_recv_disconnect);
}

int
rpc_get_overlapped_result(rcf_rpc_server *rpcs,
                          int s, rpc_overlapped overlapped,
                          int *bytes, te_bool wait,
                          rpc_send_recv_flags *flags,
                          char *buf, int buflen)
{
    rcf_rpc_op op;

    tarpc_get_overlapped_result_in  in;
    tarpc_get_overlapped_result_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.s = s;
    in.overlapped = (tarpc_overlapped)overlapped;
    in.wait = wait;
    if (bytes != NULL)
    {
        in.bytes.bytes_len = 1;
        in.bytes.bytes_val = bytes;
    }
    if (flags != NULL)
    {
        in.flags.flags_len = 1;
        in.flags.flags_val = (int *)flags;
    }

    rcf_rpc_call(rpcs, _get_overlapped_result, &in,
                 (xdrproc_t)xdr_tarpc_get_overlapped_result_in,
                 &out, (xdrproc_t)xdr_tarpc_get_overlapped_result_out);

    if (out.retval)
    {
        int filled = 0;
        int i;

        for (i = 0; i < (int)out.vector.vector_len; i++)
        {
            int copy_len =
                buflen - filled < (int)out.vector.vector_val[i].iov_len ?
                buflen - filled : (int)out.vector.vector_val[i].iov_len;

            memcpy(buf + filled,
                   out.vector.vector_val[i].iov_base.iov_base_val,
                   copy_len);
            filled += copy_len;
        }
    }
    if (RPC_CALL_OK)
    {
        if (bytes != NULL && out.bytes.bytes_val != NULL)
            *bytes = out.bytes.bytes_val[0];

        if (flags != NULL && out.flags.flags_val != NULL)
            *flags = out.flags.flags_val[0];
    }
    RING("RPC (%s,%s)%s: get_overlapped_result(%d, %p, ...) -> %s (%s %s) "
         "bytes transferred %u",
         rpcs->ta, rpcs->name, rpcop2str(op), s, overlapped,
         out.retval ? "true" : "false", errno_rpc2str(RPC_ERRNO(rpcs)),
         win_error_rpc2str(out.common.win_error),
         out.bytes.bytes_val != NULL ? *bytes : 0);

    RETVAL_VAL(out.retval, get_overlapped_result);
}

int
rpc_wsa_duplicate_socket(rcf_rpc_server *rpcs,
                         int s, int pid, uint8_t *info, int *info_len)
{
    rcf_rpc_op op;

    tarpc_duplicate_socket_in  in;
    tarpc_duplicate_socket_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    if ((info == NULL) != (info_len == NULL))
    {
        rpcs->_errno = TE_RC(TE_RCF, EINVAL);
        return -1;
    }

    if (info_len != NULL && *info_len == 0)
    {
        rpcs->_errno = TE_RC(TE_RCF, EINVAL);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.s = s;
    in.pid = pid;
    if (info_len != NULL)
    {
        in.info.info_len = *info_len;
        in.info.info_val = info;
    }

    rcf_rpc_call(rpcs, _duplicate_socket, &in,
                 (xdrproc_t)xdr_tarpc_duplicate_socket_in,
                 &out, (xdrproc_t)xdr_tarpc_duplicate_socket_out);

    RING("RPC (%s,%s)%s: duplicate_socket(%d, %d) -> %d (%s)",
         rpcs->ta, rpcs->name, rpcop2str(op), s, pid,
         out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    if (RPC_CALL_OK)
    {
        if (info_len != NULL)
            *info_len = out.info.info_len;
        memcpy(info, out.info.info_val, out.info.info_len);
    }

    RETVAL_RC(duplicate_socket);
}

int
rpc_wait_multiple_events(rcf_rpc_server *rpcs,
                        int count, rpc_wsaevent *events,
                        te_bool wait_all, uint32_t timeout,
                        te_bool alertable, int rcount)
{
    rcf_rpc_op       op;
    tarpc_wait_multiple_events_in  in;
    tarpc_wait_multiple_events_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server rpcs", __FUNCTION__);
        return -1;
    }

    if (events != NULL && count > rcount)
    {
        rpcs->_errno = TE_RC(TE_RCF, EINVAL);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.count = count;
    in.events.events_len = rcount;
    in.events.events_val = (tarpc_wsaevent *)events;
    in.wait_all = wait_all;
    in.timeout = timeout;
    in.alertable = alertable;
    rcf_rpc_call(rpcs, _wait_multiple_events,
                 &in, (xdrproc_t)xdr_tarpc_wait_multiple_events_in,
                 &out, (xdrproc_t)xdr_tarpc_wait_multiple_events_out);

    if (RPC_CALL_OK)
    {
        switch (out.retval)
        {
            case TARPC_WSA_WAIT_FAILED:
                out.retval = WSA_WAIT_FAILED;
                break;

            case TARPC_WAIT_IO_COMPLETION:
                out.retval = WAIT_IO_COMPLETION;
                break;

            case TARPC_WSA_WAIT_TIMEOUT:
                out.retval = WSA_WAIT_TIMEOUT;
                break;

            default:
                out.retval = WSA_WAIT_EVENT_0 +
                             (out.retval - TARPC_WSA_WAIT_EVENT_0);
        }
    }

    RING("RPC (%s,%s)%s: wait_multiple_events(%d, %p, %s, %d, %s) "
         " -> %s (%s %s)",
         rpcs->ta, rpcs->name, rpcop2str(op),
         count, events, wait_all ? "true" : "false", timeout,
         alertable ? "true" : "false", wsa_wait_rpc2str(out.retval),
         errno_rpc2str(RPC_ERRNO(rpcs)),
         win_error_rpc2str(out.common.win_error));

    RETVAL_VAL(out.retval, wait_multiple_events);
}
