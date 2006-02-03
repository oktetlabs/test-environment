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
 *
 * $Id$
 */

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "tapi_rpc_internal.h"
#include "te_printf.h"
#include "tapi_rpc_winsock2.h"
#include "conf_api.h"
#include "tapi_rpc_misc.h"

#define FILL_CALLBACK(func) \
   do {                                                                 \
       if ((in.callback = strdup(callback == NULL ? ""                  \
                                                  : callback)) == NULL) \
       {                                                                \
           ERROR("Out of memory");                                      \
           RETVAL_INT(func, -1);                                        \
       }                                                                \
   } while (0)                                     

int
rpc_wsa_socket(rcf_rpc_server *rpcs,
               rpc_socket_domain domain, rpc_socket_type type,
               rpc_socket_proto protocol, uint8_t *info, int info_len,
               rpc_open_sock_flags flags)
{
    tarpc_wsa_socket_in  in;
    tarpc_wsa_socket_out out;

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
    in.info.info_val = info;
    in.info.info_len = info_len;
    in.flags = flags;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "wsa_socket", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(wsa_socket, out.fd);

    TAPI_RPC_LOG("RPC (%s,%s): WSASocket(%s, %s, %s, %x, %d, %s) -> "
                 "%d (%s)",
                 rpcs->ta, rpcs->name,
                 domain_rpc2str(domain), socktype_rpc2str(type),
                 proto_rpc2str(protocol),
                 info, info_len, open_sock_flags_rpc2str(flags),
                 out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(socket, out.fd);
}

te_bool
rpc_connect_ex(rcf_rpc_server *rpcs,
               int s, const struct sockaddr *addr,
               socklen_t addrlen,
               rpc_ptr buf, ssize_t len_buf,
               size_t *bytes_sent,
               rpc_overlapped overlapped)
{
    
    tarpc_connect_ex_in  in;
    tarpc_connect_ex_out out;
    rcf_rpc_op           op;
    tarpc_size_t         sent = PTR_VAL(bytes_sent);
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_BOOL(connect_ex, FALSE);
    }
    op = rpcs->op;
    
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

    in.send_buf = buf;
    
    in.len_sent.len_sent_val = bytes_sent == NULL ? NULL : &sent;
    in.buflen = len_buf;
    in.overlapped = (tarpc_overlapped)overlapped;

    rcf_rpc_call(rpcs, "connect_ex", &in, &out);

    if (bytes_sent != NULL && out.len_sent.len_sent_val != NULL)
        *bytes_sent = out.len_sent.len_sent_val[0];

    if (op == RCF_RPC_CALL)
        out.retval = TRUE;
    
    CHECK_RETVAL_VAR_IS_BOOL(connect_ex, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: ConnectEx(%d, %s, %u, %u, %u, %d, %u) "
                 "-> %s (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 s, sockaddr2str(addr), addrlen, 
                 buf, len_buf, PTR_VAL(bytes_sent), 
                 overlapped,
                 out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(connect_ex, out.retval);
}

te_bool
rpc_disconnect_ex(rcf_rpc_server *rpcs, int s,
                  rpc_overlapped overlapped, int flags)
{
    rcf_rpc_op        op;
    tarpc_disconnect_ex_in  in;
    tarpc_disconnect_ex_out out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_BOOL(disconnect_ex, FALSE);
    }
    op = rpcs->op;
    
    in.fd = s;
    in.overlapped = (tarpc_overlapped)overlapped;
    in.flags = flags;

    rcf_rpc_call(rpcs, "disconnect_ex", &in, &out);

    if (op == RCF_RPC_CALL)
        out.retval = TRUE;

    CHECK_RETVAL_VAR_IS_BOOL(disconnect_ex, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: DisconnectEx(%d, %u, %d) -> %s (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, overlapped, flags,
                 out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(disconnect_ex, out.retval);
}

int
rpc_wsa_accept(rcf_rpc_server *rpcs,
               int s, struct sockaddr *addr,
               socklen_t *addrlen, socklen_t raddrlen,
               accept_cond *cond, int cond_num)
{
    rcf_rpc_op           op;
    tarpc_wsa_accept_in  in;
    tarpc_wsa_accept_out out;

    struct tarpc_accept_cond rpc_cond[RCF_RPC_MAX_ACCEPT_CONDS];

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_accept, -1);
    }

    if (cond_num > RCF_RPC_MAX_ACCEPT_CONDS)
    {
        ERROR("Too many conditions are specified for WSAAccept condition"
              "function");
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(wsa_accept, -1);
    }

    if ((cond == NULL && cond_num > 0) ||
        (cond != NULL && cond_num == 0))
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(wsa_accept, -1);
    }

    op = rpcs->op;
    
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

    rcf_rpc_call(rpcs, "wsa_accept", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
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

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(wsa_accept, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: WSAAccept(%d, %p, %p(%u)) -> %d (%s) "
                 "peer=%s addrlen=%u",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, addr, addrlen, PTR_VAL(addrlen),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 sockaddr2str(addr), PTR_VAL(addrlen));

    RETVAL_INT(wsa_accept, out.retval);
}


te_bool
rpc_accept_ex(rcf_rpc_server *rpcs, int s, int s_a,
              rpc_ptr buf, size_t len, size_t laddr_len, size_t raddr_len, 
              size_t *bytes_received, rpc_overlapped overlapped)
{
    rcf_rpc_op          op;
    tarpc_accept_ex_in  in;
    tarpc_accept_ex_out out;
    tarpc_size_t        received = PTR_VAL(bytes_received);
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_BOOL(accept_ex, FALSE);
    }

    op = rpcs->op;
    
    in.fd = s;
    in.fd_a = s_a;
    in.out_buf = buf;
    in.buflen = len;
    in.laddr_len = laddr_len;
    in.raddr_len = raddr_len;
    
    if (bytes_received == NULL)
        in.count.count_len = 0;
    else
        in.count.count_len = 1;
    in.count.count_val = bytes_received == NULL ? NULL : &received;
    in.overlapped = (tarpc_overlapped)overlapped;

    rcf_rpc_call(rpcs, "accept_ex", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if ((bytes_received != NULL) && (out.count.count_val != NULL))
            *bytes_received = out.count.count_val[0];
    }

    if (op == RCF_RPC_CALL)
        out.retval = TRUE;

    CHECK_RETVAL_VAR_IS_BOOL(accept_ex, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: AcceptEx(%d, %d, %u, %d, %d, %d, %d, %u)"
                 " -> %s (%s) bytes received %u", rpcs->ta, rpcs->name, 
                 rpcop2str(op), s, s_a, buf, len, laddr_len, raddr_len,
                 PTR_VAL(bytes_received), overlapped,
                 out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)),
                 PTR_VAL(bytes_received));

    RETVAL_BOOL(accept_ex, out.retval);
}

void
rpc_get_accept_addr(rcf_rpc_server *rpcs,
                    int s, rpc_ptr buf, size_t len,
                    struct sockaddr *laddr,
                    struct sockaddr *raddr)
{
    tarpc_get_accept_addr_in  in;
    tarpc_get_accept_addr_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(get_accept_addr);
    }
    
    in.fd = s;
    in.buflen = len;
    in.buf = (tarpc_ptr)buf;
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

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "get_accept_addr", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
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
    
    TAPI_RPC_LOG("RPC (%s,%s): "
                 "GetAcceptExSockaddrs(%d, %u, %d, %p, %p) -> "
                 "(%s) laddr=%s raddr=%s",
                 rpcs->ta, rpcs->name,
                 s, buf, len, laddr, raddr, 
                 errno_rpc2str(RPC_ERRNO(rpcs)),
                 ((out.laddr.sa_data.sa_data_val == NULL) ||
                  (laddr == NULL)) ? "NULL" : sockaddr2str(laddr),
                 ((out.raddr.sa_data.sa_data_val == NULL) ||
                  (raddr == NULL)) ? "NULL" : sockaddr2str(raddr));
    
    RETVAL_VOID(get_accept_addr);
}

te_bool
rpc_transmit_file(rcf_rpc_server *rpcs, int s, int file,
                  ssize_t len, ssize_t len_per_send,
                  rpc_overlapped overlapped,
                  void *head, ssize_t head_len,
                  void *tail, ssize_t tail_len, ssize_t flags)
{
    rcf_rpc_op               op;
    tarpc_transmit_file_in   in;
    tarpc_transmit_file_out  out;
 
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_BOOL(transmit_file, FALSE);
    }
#if 0
    if ((buf != NULL) && (bytes_sent == NULL))
    {
        RETVAL_BOOL(transmit_file, FALSE);
    }
#endif
    op = rpcs->op;
    
    in.fd = s;
    in.file = (tarpc_handle)file;
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

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
        rpcs->timeout = RCF_RPC_DEFAULT_TIMEOUT * 10;

    rcf_rpc_call(rpcs, "transmit_file", &in, &out);
    
    if (op == RCF_RPC_CALL)
        out.retval = TRUE;

    CHECK_RETVAL_VAR_IS_BOOL(transmit_file, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "TransmitFile(%d, %d, %d, %d, %u, %p, %d, %p, %d, %d) "
                 "-> %s (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 s, file, len, len_per_send, overlapped,
                 head, head_len, tail, tail_len, flags,
                 out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(transmit_file, out.retval);
}

/**
 * Attention: when using the overlapped I/O the supplied buffers head
 * and tail will be freed when you call rpc_wsa_get_overlapped_result().
 */
te_bool
rpc_transmitfile_tabufs(rcf_rpc_server *rpcs, int s, int file,
                        ssize_t len, ssize_t bytes_per_send,
                        rpc_overlapped overlapped,
                        rpc_ptr head, ssize_t head_len,
                        rpc_ptr tail, ssize_t tail_len, ssize_t flags)
{
    rcf_rpc_op                    op;
    tarpc_transmitfile_tabufs_in  in;
    tarpc_transmitfile_tabufs_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_BOOL(transmitfile_tabufs, FALSE);
    }
    op = rpcs->op;
    
    in.s = s;
    in.file = file;
    in.len = len;
    in.bytes_per_send = bytes_per_send;
    in.overlapped = (tarpc_overlapped)overlapped;
    in.head = (tarpc_ptr)head;
    in.head_len = head_len;
    in.tail = (tarpc_ptr)tail;
    in.tail_len = tail_len;
    in.flags = flags;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
        rpcs->timeout = RCF_RPC_DEFAULT_TIMEOUT * 10;

    rcf_rpc_call(rpcs, "transmitfile_tabufs", &in, &out);

    if (op == RCF_RPC_CALL)
        out.retval = TRUE;

    CHECK_RETVAL_VAR_IS_BOOL(transmitfile_tabufs, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "TransmitFile(%d, %d, %d, %d, %u, "
                 "%p, %d, %p, %d, %d) -> %s (%s)", 
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, file, len, bytes_per_send, overlapped,
                 head, head_len, tail, tail_len, flags,
                 out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(transmitfile_tabufs, out.retval);
}

int
rpc_create_file(rcf_rpc_server *rpcs, char *name,
                rpc_cf_access_right desired_access,
                rpc_cf_share_mode share_mode,
                rpc_ptr security_attributes,
                rpc_cf_creation_disposition creation_disposition,
                rpc_cf_flags_attributes flags_attributes,
                int template_file)
{
    rcf_rpc_op            op;
    tarpc_create_file_in  in;
    tarpc_create_file_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(create_file, -1);
    }
    op = rpcs->op;
    
    in.name.name_val = name;
    if (name == NULL)
        in.name.name_len = 0;
    else
        in.name.name_len = strlen(name) + 1;

    in.desired_access = desired_access;
    in.share_mode = share_mode;
    in.security_attributes = (tarpc_ptr)security_attributes;
    in.creation_disposition = creation_disposition;
    in.flags_attributes = flags_attributes;
    in.template_file = template_file;

    rcf_rpc_call(rpcs, "create_file", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(create_file, out.handle);

    TAPI_RPC_LOG("RPC (%s,%s)%s: CreateFile(%s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 name, out.handle, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(create_file, out.handle);
}

int
rpc_closesocket(rcf_rpc_server *rpcs, int s)
{
    rcf_rpc_op             op;
    tarpc_closesocket_in  in;
    tarpc_closesocket_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(closesocket, -1);
    }
    op = rpcs->op;
    
    in.s = s;

    rcf_rpc_call(rpcs, "closesocket", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: closesocket(%d) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(closesocket, out.retval);
}

te_bool
rpc_has_overlapped_io_completed(rcf_rpc_server *rpcs,
                                rpc_overlapped overlapped)
{
    rcf_rpc_op                            op;
    tarpc_has_overlapped_io_completed_in  in;
    tarpc_has_overlapped_io_completed_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(has_overlapped_io_completed, -1);
    }

    op = rpcs->op;

    in.overlapped = (tarpc_overlapped)overlapped;

    rcf_rpc_call(rpcs, "has_overlapped_io_completed", &in, &out);

    /* No check - it's strange to assume FALSE as failure here */

    TAPI_RPC_LOG("RPC (%s,%s)%s: HasOverlappedIoCompleted(%u)"
                 " -> %s (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 overlapped, out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(has_overlapped_io_completed, out.retval);
}

te_bool 
rpc_cancel_io(rcf_rpc_server *rpcs, int fd)
{
    rcf_rpc_op          op;
    tarpc_cancel_io_in  in;
    tarpc_cancel_io_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(cancel_io, -1);
    }

    op = rpcs->op;

    in.fd = fd;

    rcf_rpc_call(rpcs, "cancel_io", &in, &out);

    CHECK_RETVAL_VAR_IS_BOOL(cancel_io, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: CancelIo(%d)"
                 " -> %s (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(cancel_io, out.retval);
}

int
rpc_create_io_completion_port(rcf_rpc_server *rpcs,
                              int file_handle,
                              int existing_completion_port,
                              int completion_key,
                              unsigned int number_of_concurrent_threads)
{
    rcf_rpc_op                          op;
    tarpc_create_io_completion_port_in  in;
    tarpc_create_io_completion_port_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(create_io_completion_port, 0);
    }

    op = rpcs->op;

    in.file_handle = file_handle;
    in.existing_completion_port = existing_completion_port;
    in.completion_key = completion_key;
    in.number_of_concurrent_threads = number_of_concurrent_threads;

    rcf_rpc_call(rpcs, "create_io_completion_port", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(create_io_completion_port, 
                                      out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: CreateIoCompletionPort(%d, %d, %d, %u)"
                 " -> %x (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 file_handle, existing_completion_port,
                 completion_key, number_of_concurrent_threads,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(create_io_completion_port, out.retval);
}

te_bool
rpc_get_queued_completion_status(rcf_rpc_server *rpcs,
                                 int completion_port,
                                 unsigned int *number_of_bytes,
                                 int *completion_key,
                                 rpc_overlapped *overlapped,
                                 unsigned int milliseconds)
{
    rcf_rpc_op                             op;
    tarpc_get_queued_completion_status_in  in;
    tarpc_get_queued_completion_status_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_BOOL(get_queued_completion_status, FALSE);
    }

    op = rpcs->op;

    in.completion_port = completion_port;
    in.milliseconds = milliseconds;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
        rpcs->timeout = RCF_RPC_DEFAULT_TIMEOUT * 10;

    rcf_rpc_call(rpcs, "get_queued_completion_status", &in, &out);
    
    if (op == RCF_RPC_CALL)
        out.retval = TRUE;
    
    CHECK_RETVAL_VAR_IS_BOOL(get_queued_completion_status, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: GetQueuedCompletionStatus(%d, %u)"
                 " -> %s %u, %d, %d (%s)", rpcs->ta, rpcs->name,
                 rpcop2str(op), 
                 completion_port, milliseconds,
                 out.retval ? "true" : "false", 
                 out.number_of_bytes, out.completion_key, out.overlapped, 
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    if (number_of_bytes != NULL)
        *number_of_bytes = out.number_of_bytes;
    if (completion_key != NULL)
        *completion_key = out.completion_key;
    if (overlapped != NULL)
        *overlapped = out.overlapped;

    RETVAL_BOOL(get_queued_completion_status, out.retval);
}

te_bool
rpc_post_queued_completion_status(rcf_rpc_server *rpcs,
                                  int completion_port,
                                  unsigned int number_of_bytes,
                                  int completion_key,
                                  rpc_overlapped overlapped)
{
    rcf_rpc_op                              op;
    tarpc_post_queued_completion_status_in  in;
    tarpc_post_queued_completion_status_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_BOOL(post_queued_completion_status, FALSE);
    }

    op = rpcs->op;

    in.completion_port = completion_port;
    in.number_of_bytes = number_of_bytes;
    in.completion_key = completion_key;
    in.overlapped = overlapped;

    rcf_rpc_call(rpcs, "post_queued_completion_status", &in, &out);

    if (op == RCF_RPC_CALL)
        out.retval = TRUE;

    CHECK_RETVAL_VAR_IS_BOOL(post_queued_completion_status, out.retval);
    
    TAPI_RPC_LOG("RPC (%s,%s)%s: PostQueuedCompletionStatus"
                 "(%d, %u, %d, %u) -> %s (%s)", rpcs->ta,
                 rpcs->name, rpcop2str(op), completion_port,
                 number_of_bytes, completion_key, overlapped, 
                 out.retval ? "true" : "false", 
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(post_queued_completion_status, out.retval);
}

int
rpc_get_current_process_id(rcf_rpc_server *rpcs)
{
    rcf_rpc_op                       op;
    tarpc_get_current_process_id_in  in;
    tarpc_get_current_process_id_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(get_current_process_id, -1);
    }

    op = rpcs->op;

    rcf_rpc_call(rpcs, "get_current_process_id", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: GetCurrentProcessId() -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(get_current_process_id, out.retval);
}

void
rpc_get_sys_info(rcf_rpc_server *rpcs, rpc_sys_info *sys_info)
{
    rcf_rpc_op             op;
    tarpc_get_sys_info_in  in;
    tarpc_get_sys_info_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(get_sys_info);
    }

    op = rpcs->op;

    rcf_rpc_call(rpcs, "get_sys_info", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: GetSysInfo() -> %llu, %u, %u (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 out.ram_size, out.page_size, out.number_of_processors,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    if (sys_info != NULL)
    {
        sys_info->ram_size = out.ram_size;
        sys_info->page_size = out.page_size;
        sys_info->number_of_processors = out.number_of_processors;
    }

    RETVAL_VOID(get_sys_info);
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

    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_recv_ex, -1);
    }

    op = rpcs->op;


    if (buf != NULL && len > rbuflen)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(wsa_recv_ex, -1);
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

    rcf_rpc_call(rpcs, "wsa_recv_ex", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);

        if (flags != NULL && out.flags.flags_val != NULL)
            *flags = out.flags.flags_val[0];
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(wsa_recv_ex, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: WSARecvEx(%d, %p, %u, %s)"
                 " -> %d (%s) flags %s",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, buf, len, send_recv_flags_rpc2str(in_flags),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 send_recv_flags_rpc2str(PTR_VAL(flags)));

    RETVAL_INT(wsa_recv_ex, out.retval);
}

rpc_wsaevent
rpc_create_event(rcf_rpc_server *rpcs)
{
    tarpc_create_event_in  in;
    tarpc_create_event_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(create_event, RPC_NULL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    rcf_rpc_call(rpcs, "create_event", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): WSACreateEvent() -> %u (%s)",
                 rpcs->ta, rpcs->name,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(create_event, out.retval);
}

te_bool
rpc_close_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent)
{
    tarpc_close_event_in  in;
    tarpc_close_event_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_BOOL(close_event, FALSE);
    }

    in.hevent = (tarpc_wsaevent)hevent;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "close_event", &in, &out);

    CHECK_RETVAL_VAR_IS_BOOL(close_event, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): WSACloseEvent(%u) -> %s (%s)",
                 rpcs->ta, rpcs->name, hevent,
                 out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(close_event, out.retval);
}

te_bool
rpc_reset_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent)
{
    tarpc_reset_event_in  in;
    tarpc_reset_event_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_BOOL(reset_event, FALSE);
    }

    in.hevent = (tarpc_wsaevent)hevent;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "reset_event", &in, &out);

    CHECK_RETVAL_VAR_IS_BOOL(reset_event, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): WSAResetEvent(%u) -> %s (%s)",
                 rpcs->ta, rpcs->name, hevent,
                 out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(reset_event, out.retval);
}

te_bool
rpc_set_event(rcf_rpc_server *rpcs, rpc_wsaevent hevent)
{
    tarpc_set_event_in  in;
    tarpc_set_event_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_BOOL(set_event, FALSE);
    }

    in.hevent = (tarpc_wsaevent)hevent;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "set_event", &in, &out);

    CHECK_RETVAL_VAR_IS_BOOL(set_event, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): WSASetEvent(%u) -> %s (%s)",
                 rpcs->ta, rpcs->name, hevent,
                 out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(set_event, out.retval);
}

rpc_overlapped
rpc_create_overlapped(rcf_rpc_server *rpcs, rpc_wsaevent hevent,
                      unsigned int offset, unsigned int offset_high, ...)
{
    tarpc_create_overlapped_in  in;
    tarpc_create_overlapped_out out;
    
    va_list ap;
    
    va_start(ap, offset_high);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(create_overlapped, RPC_NULL);
    }


    in.hevent = (tarpc_wsaevent)hevent;
    in.offset = offset;
    in.offset_high = offset_high;
    in.cookie1 = va_arg(ap, uint32_t);
    in.cookie2 = va_arg(ap, uint32_t);
    
    va_end(ap);

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "create_overlapped", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): create_overlapped(%u, %u, %u) -> %u (%s)",
                 rpcs->ta, rpcs->name, hevent, offset, offset_high,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(create_overlapped, out.retval);
}

void
rpc_delete_overlapped(rcf_rpc_server *rpcs,
                      rpc_overlapped overlapped)
{
    tarpc_delete_overlapped_in  in;
    tarpc_delete_overlapped_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(delete_overlapped);
    }


    in.overlapped = (tarpc_overlapped)overlapped;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "delete_overlapped", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): delete_overlapped(%u)",
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

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(completion_callback, -1);
    }
    if (called == NULL || error == NULL || bytes == NULL ||
        overlapped == NULL)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(completion_callback, -1);
    }


    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "completion_callback", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        *called = out.called;
        *error = out.error;
        *bytes = out.bytes;
        *overlapped = (rpc_overlapped)(out.overlapped);
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(completion_callback, rc);

    TAPI_RPC_LOG("RPC (%s,%s): completion_callback() -> "
                 "called %d times;  error %d; bytes %d; overlapped %u",
                 rpcs->ta, rpcs->name, out.called, out.error, out.bytes,
                 out.overlapped);

    RETVAL_INT(completion_callback, rc);
}

int
rpc_wsa_event_select(rcf_rpc_server *rpcs,
                     int s, rpc_wsaevent event_object,
                     rpc_network_event event)
{
    tarpc_event_select_in  in;
    tarpc_event_select_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(event_select, -1);
    }


    in.fd = s;
    in.hevent = (tarpc_wsaevent)event_object;
    in.event = event;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "event_select", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(event_select, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): WSAEventSelect(%d, %u, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 s, event_object, network_event_rpc2str(event),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(event_select, out.retval);
}

int
rpc_enum_network_events(rcf_rpc_server *rpcs,
                        int s, rpc_wsaevent event_object,
                        rpc_network_event *event)
{
    tarpc_enum_network_events_in  in;
    tarpc_enum_network_events_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(enum_network_events, -1);
    }


    in.fd = s;
    in.hevent = (tarpc_wsaevent)event_object;
    if (event == NULL)
        in.event.event_len = 0;
    else
        in.event.event_len = 1;
    in.event.event_val = (uint32_t *)event;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "enum_network_events", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (event != NULL && out.event.event_val != NULL)
            *event = out.event.event_val[0];
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(enum_network_events, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): WSAEnumNetworkEvents(%d, %u, %p) "
                 "-> %d (%s) returned event %s",
                 rpcs->ta, rpcs->name, s, event_object, event,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 network_event_rpc2str(PTR_VAL(event)));

    RETVAL_INT(enum_network_events, out.retval);
}

rpc_hwnd
rpc_create_window(rcf_rpc_server *rpcs)
{
    tarpc_create_window_in  in;
    tarpc_create_window_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(create_window, RPC_NULL);
    }


    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "create_window", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): create_window() -> %u (%s)",
                 rpcs->ta, rpcs->name,
                 out.hwnd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(create_window, out.hwnd);
}

void
rpc_destroy_window(rcf_rpc_server *rpcs, rpc_hwnd hwnd)
{
    tarpc_destroy_window_in  in;
    tarpc_destroy_window_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(destroy_window);
    }


    in.hwnd = (tarpc_hwnd)hwnd;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "destroy_window", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): destroy_window(%u) -> (%s)",
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

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_async_select, -1);
    }


    in.hwnd = (tarpc_hwnd)hwnd;
    in.sock = s;
    in.event = event;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "wsa_async_select", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_async_select, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): WSAAsyncSelect(%u, %d, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 hwnd, s, network_event_rpc2str(event),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_async_select, out.retval);
}

int
rpc_peek_message(rcf_rpc_server *rpcs,
                     rpc_hwnd hwnd, int *s, rpc_network_event *event)
{
    tarpc_peek_message_in  in;
    tarpc_peek_message_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(peek_message, 0);
    }

    if (s == NULL || event == NULL)
    {
        ERROR("Null pointer is passed to rpc_peek_msg");
        RETVAL_INT(peek_message, 0);
    }


    in.hwnd = (tarpc_hwnd)hwnd;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "peek_message", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(peek_message, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): PeekMessage(%u) -> %d (%s) event %s",
                 rpcs->ta, rpcs->name,
                 hwnd, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 network_event_rpc2str(out.event));

    *s = out.sock;
    *event = out.event;

    RETVAL_INT(peek_message, out.retval);
}

int
rpc_wsa_send(rcf_rpc_server *rpcs,
             int s, const struct rpc_iovec *iov,
             size_t iovcnt, rpc_send_recv_flags flags,
             int *bytes_sent, rpc_overlapped overlapped,
             const char *callback)
{
    rcf_rpc_op         op;
    tarpc_wsa_send_in  in;
    tarpc_wsa_send_out out;

    char               str_buf[1024] = { '\0', };
    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];
    
    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_send, -1);
    }

    op = rpcs->op;


    if (iovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
        RETVAL_INT(wsa_send, -1);
    }
 
    snprintf(str_buf + strlen(str_buf),
             sizeof(str_buf) - strlen(str_buf), "{");

    for (i = 0; i < iovcnt && iov != NULL; i++)
    {
        iovec_arr[i].iov_base.iov_base_val = iov[i].iov_base;
        iovec_arr[i].iov_base.iov_base_len = iov[i].iov_rlen;
        iovec_arr[i].iov_len = iov[i].iov_len;
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf),
                 "%s{%"TE_PRINTF_SIZE_T"u, %p[%"TE_PRINTF_SIZE_T"u]}",
                 (i == 0) ? "" : ", ", iov[i].iov_len,
                 iov[i].iov_base, iov[i].iov_rlen);
    }
    snprintf(str_buf + strlen(str_buf),
             sizeof(str_buf) - strlen(str_buf), "}");

    if (iov != NULL)
    {
        in.vector.vector_val = iovec_arr;
        in.vector.vector_len = iovcnt;
    }

    in.s = s;
    in.count = iovcnt;
    in.overlapped = (tarpc_overlapped)overlapped;
    FILL_CALLBACK(wsa_send);
    if (bytes_sent != NULL)
    {
        in.bytes_sent.bytes_sent_len = 1;
        in.bytes_sent.bytes_sent_val = bytes_sent;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "wsa_send", &in, &out);
    
    free(in.callback);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (bytes_sent != NULL && out.bytes_sent.bytes_sent_val != NULL)
            *bytes_sent = out.bytes_sent.bytes_sent_val[0];
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_send, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSASend(%d, %s, %u, %s, %d, %u, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), 
                 s, (*str_buf == '\0') ? "(nil)" : str_buf, iovcnt,
                 send_recv_flags_rpc2str(flags), PTR_VAL(bytes_sent),
                 overlapped, callback, out.retval,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_send, out.retval);
}

int
rpc_wsa_recv(rcf_rpc_server *rpcs,
             int s, const struct rpc_iovec *iov,
             size_t iovcnt, size_t riovcnt,
             rpc_send_recv_flags *flags,
             int *bytes_received, rpc_overlapped overlapped,
             const char *callback)
{
    rcf_rpc_op         op;
    tarpc_wsa_recv_in  in;
    tarpc_wsa_recv_out out;

    char               str_buf[1024] = { '\0', };
    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_recv, -1);
    }

    op = rpcs->op;


    if (riovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
        RETVAL_INT(wsa_recv, -1);
    }

    if (iov != NULL && iovcnt > riovcnt)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(wsa_recv, -1);
    }

    in.s = s;
    in.count = iovcnt;
    in.overlapped = (tarpc_overlapped)overlapped;
    FILL_CALLBACK(wsa_recv);
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
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf), "{");
        for (i = 0; i < riovcnt; i++)
        {
            VERB("IN wsa_recv(%d, %s, %d, ) I/O vector #%d: %p[%u] %u",
                 i, iov[i].iov_base, iov[i].iov_rlen, iov[i].iov_len);
            iovec_arr[i].iov_base.iov_base_val = iov[i].iov_base;
            iovec_arr[i].iov_base.iov_base_len = iov[i].iov_rlen;
            iovec_arr[i].iov_len = iov[i].iov_len;
            snprintf(str_buf + strlen(str_buf),
                     sizeof(str_buf) - strlen(str_buf),
                     "%s{%"TE_PRINTF_SIZE_T"u, %p[%"TE_PRINTF_SIZE_T"u]}",
                     (i == 0) ? "" : ", ", iov[i].iov_len,
                     iov[i].iov_base, iov[i].iov_rlen);
        }
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf), "}");
   }

    rcf_rpc_call(rpcs, "wsa_recv", &in, &out);

    free(in.callback);

    if (RPC_IS_CALL_OK(rpcs) &&
        iov != NULL && out.vector.vector_val != NULL)
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

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_recv, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSARecv(%d, %s, %d, %s, %d, %u, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, (*str_buf == '\0') ? "(nil)" : str_buf, 
                 iovcnt, send_recv_flags_rpc2str(PTR_VAL(flags)), 
                 PTR_VAL(bytes_received),
                 overlapped, callback,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_recv, out.retval);
}

int
rpc_wsa_send_to(rcf_rpc_server *rpcs, int s, const struct rpc_iovec *iov,
                size_t iovcnt, rpc_send_recv_flags flags, int *bytes_sent,
                const struct sockaddr *to, socklen_t tolen,
                rpc_overlapped overlapped, const char *callback)
{
    rcf_rpc_op            op;
    tarpc_wsa_send_to_in  in;
    tarpc_wsa_send_to_out out;

    char               str_buf[1024] = { '\0', };
    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_send_to, -1);
    }

    op = rpcs->op;

    if (iovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
        RETVAL_INT(wsa_send_to, -1);
    }
 
    snprintf(str_buf + strlen(str_buf),
             sizeof(str_buf) - strlen(str_buf), "{");

    for (i = 0; i < iovcnt && iov != NULL; i++)
    {
        iovec_arr[i].iov_base.iov_base_val = iov[i].iov_base;
        iovec_arr[i].iov_base.iov_base_len = iov[i].iov_rlen;
        iovec_arr[i].iov_len = iov[i].iov_len;
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf),
                 "%s{%"TE_PRINTF_SIZE_T"u, %p[%"TE_PRINTF_SIZE_T"u]}",
                 (i == 0) ? "" : ", ", iov[i].iov_len,
                 iov[i].iov_base, iov[i].iov_rlen);
    }
    snprintf(str_buf + strlen(str_buf),
             sizeof(str_buf) - strlen(str_buf), "}");


    if (iov != NULL)
    {
        in.vector.vector_val = iovec_arr;
        in.vector.vector_len = iovcnt;
    }

    in.s = s;
    in.count = iovcnt;
    in.overlapped = (tarpc_overlapped)overlapped;
    FILL_CALLBACK(wsa_send_to);
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

    rcf_rpc_call(rpcs, "wsa_send_to", &in, &out);

    free(in.callback);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (bytes_sent != NULL && out.bytes_sent.bytes_sent_val != NULL)
            *bytes_sent = out.bytes_sent.bytes_sent_val[0];
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_send_to, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSASendTo(%d, %s, %d, %s, %d, %s, %u, %u, %s) "
                 "-> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, (*str_buf == '\0') ? "(nil)" : str_buf,
                 iovcnt, send_recv_flags_rpc2str(flags),
                 PTR_VAL(bytes_sent),
                 sockaddr2str(to), tolen,         
                 overlapped, callback,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_send_to, out.retval);
}

int
rpc_wsa_recv_from(rcf_rpc_server *rpcs, int s,
                  const struct rpc_iovec *iov, size_t iovcnt,
                  size_t riovcnt, rpc_send_recv_flags *flags,
                  int *bytes_received, struct sockaddr *from,
                  socklen_t *fromlen, rpc_overlapped overlapped,
                  const char *callback)
{
    rcf_rpc_op              op;
    tarpc_wsa_recv_from_in  in;
    tarpc_wsa_recv_from_out out;

    char               str_buf[1024] = { '\0', };
    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_recv_from, -1);
    }

    op = rpcs->op;


    if (riovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
        RETVAL_INT(wsa_recv_from, -1);
    }

    if (iov != NULL && iovcnt > riovcnt)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(wsa_recv_from, -1);
    }

    in.s = s;
    in.count = iovcnt;
    in.overlapped = (tarpc_overlapped)overlapped;
    FILL_CALLBACK(wsa_recv_from);
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

        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf), "{");

        for (i = 0; i < riovcnt; i++)
        {
            VERB("IN wsa_recv_from() I/O vector #%d: %p[%u] %u",
                 i, iov[i].iov_base, iov[i].iov_rlen, iov[i].iov_len);
            iovec_arr[i].iov_base.iov_base_val = iov[i].iov_base;
            iovec_arr[i].iov_base.iov_base_len = iov[i].iov_rlen;
            iovec_arr[i].iov_len = iov[i].iov_len;
            snprintf(str_buf + strlen(str_buf),
                     sizeof(str_buf) - strlen(str_buf),
                     "%s{%"TE_PRINTF_SIZE_T"u, %p[%"TE_PRINTF_SIZE_T"u]}",
                     (i == 0) ? "" : ", ", iov[i].iov_len,
                     iov[i].iov_base, iov[i].iov_rlen);
        }
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf), "}");

   }

    if ((overlapped != RPC_NULL) && ((from != NULL) || (fromlen != NULL)))
    {
        ERROR("%s(): currently can't deal with non-NULL 'from' or "
              "'fromlen' when overlapped is non-NULL", __FUNCTION__);
        RETVAL_INT(wsa_recv_from, -1);
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

    rcf_rpc_call(rpcs, "wsa_recv_from", &in, &out);

    free(in.callback);

    if (RPC_IS_CALL_OK(rpcs))
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

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_recv_from, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSARecvFrom(%d, %s, %d, %s, %d, %s, %u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), 
                 s, (*str_buf == '\0') ? "(nil)" : str_buf,
                 iovcnt, send_recv_flags_rpc2str(PTR_VAL(flags)), 
                 PTR_VAL(bytes_received), sockaddr2str(from),
                 PTR_VAL(fromlen), 
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_recv_from, out.retval);
}

int
rpc_wsa_send_disconnect(rcf_rpc_server *rpcs,
                        int s, const struct rpc_iovec *iov)
{
    rcf_rpc_op                    op;
    tarpc_wsa_send_disconnect_in  in;
    tarpc_wsa_send_disconnect_out out;
    struct tarpc_iovec            iovec_arr;

    char  str_buf[1024] = { '\0', };

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&iovec_arr, 0, sizeof(iovec_arr));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_send_disconnect, -1);
    }

    op = rpcs->op;


    in.s = s;

    if (iov != NULL)
    {
        iovec_arr.iov_base.iov_base_val = iov->iov_base;
        iovec_arr.iov_base.iov_base_len = iov->iov_rlen;
        iovec_arr.iov_len = iov->iov_len;

        in.vector.vector_val = &iovec_arr;
        in.vector.vector_len = 1;
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf), "{");
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf),
                 "{%"TE_PRINTF_SIZE_T"u, %p[%"TE_PRINTF_SIZE_T"u]}",
                 iov->iov_len,
                 iov->iov_base, iov->iov_rlen);
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf), "}");
    }

    rcf_rpc_call(rpcs, "wsa_send_disconnect", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_send_disconnect, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: WSASendDisconnect(%d, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), 
                 s, (*str_buf == '\0') ? "(nil)" : str_buf,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_send_disconnect, out.retval);
}

int
rpc_wsa_recv_disconnect(rcf_rpc_server *rpcs,
                        int s, const struct rpc_iovec *iov)
{
    rcf_rpc_op                    op;
    tarpc_wsa_recv_disconnect_in  in;
    tarpc_wsa_recv_disconnect_out out;
    struct tarpc_iovec            iovec_arr;

    char  str_buf[1024] = { '\0', };

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&iovec_arr, 0, sizeof(iovec_arr));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_recv_disconnect, -1);
    }

    op = rpcs->op;


    in.s = s;

    if (iov != NULL)
    {
        iovec_arr.iov_base.iov_base_val = iov->iov_base;
        iovec_arr.iov_base.iov_base_len = iov->iov_rlen;
        iovec_arr.iov_len = iov->iov_len;

        in.vector.vector_val = &iovec_arr;
        in.vector.vector_len = 1;
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf), "{");
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf),
                 "{%"TE_PRINTF_SIZE_T"u, %p[%"TE_PRINTF_SIZE_T"u]}",
                 iov->iov_len,
                 iov->iov_base, iov->iov_rlen);
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf), "}");
    }

    rcf_rpc_call(rpcs, "wsa_recv_disconnect", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) &&
        iov != NULL && out.vector.vector_val != NULL)
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

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_recv_disconnect, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: WSARecvDisconnect(%d, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, (*str_buf == '\0') ? "(nil)" : str_buf,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_recv_disconnect, out.retval);
}

int
rpc_wsa_recv_msg(rcf_rpc_server *rpcs, int s,
                 struct rpc_msghdr *msg, int *bytes_received,
                 rpc_overlapped overlapped, const char *callback)
{
    char                   str_buf[1024] = {'\0', };
    rcf_rpc_op             op;
    tarpc_wsa_recv_msg_in  in;
    tarpc_wsa_recv_msg_out out;
    struct tarpc_msghdr    rpc_msg;
    struct tarpc_iovec     iovec_arr[RCF_RPC_MAX_IOVEC];
    struct tarpc_cmsghdr   cmsg_hdrs[RCF_RPC_MAX_CMSGHDR];
    size_t                 i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));
    memset(cmsg_hdrs, 0, sizeof(cmsg_hdrs));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_recv_msg, -1);
    }

    op = rpcs->op;

    memset(&rpc_msg, 0, sizeof(rpc_msg));

    in.s = s;

    if (msg != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        if ((overlapped != RPC_NULL) &&
            ((msg->msg_name != NULL) || (msg->msg_control != NULL)))
        {
            ERROR("%s(): currently can't deal with non-NULL 'msg_name' "
                  "or 'msg_control' when 'overlapped' is non-NULL",
                  __FUNCTION__);
            RETVAL_INT(wsa_recv_msg, -1);
        }

        in.msg.msg_val = &rpc_msg;
        in.msg.msg_len = 1;

        if (msg->msg_riovlen > RCF_RPC_MAX_IOVEC)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
            ERROR("Length of the I/O vector is too long (%u) - "
                  "increase RCF_RPC_MAX_IOVEC(%u)",
                  msg->msg_riovlen, RCF_RPC_MAX_IOVEC);
            RETVAL_INT(wsa_recv_msg, -1);
        }

        if (msg->msg_cmsghdr_num > RCF_RPC_MAX_CMSGHDR)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
            ERROR("Too many cmsg headers - increase RCF_RPC_MAX_CMSGHDR");
            RETVAL_INT(wsa_recv_msg, -1);
        }
        
        if (msg->msg_control != NULL && msg->msg_cmsghdr_num == 0)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
            ERROR("Number of cmsg headers is incorrect");
            RETVAL_INT(wsa_recv_msg, -1);
        }

        if (msg->msg_iovlen > msg->msg_riovlen ||
            msg->msg_namelen > msg->msg_rnamelen)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
            RETVAL_INT(wsa_recv_msg, -1);
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

        if (msg->msg_name != NULL)
        {
            if (msg->msg_rnamelen >= SA_COMMON_LEN)
            {
                rpc_msg.msg_name.sa_family = addr_family_h2rpc(
                    ((struct sockaddr *)(msg->msg_name))->sa_family);
                rpc_msg.msg_name.sa_data.sa_data_len =
                    msg->msg_rnamelen - SA_COMMON_LEN;
                rpc_msg.msg_name.sa_data.sa_data_val =
                    ((struct sockaddr *)(msg->msg_name))->sa_data;
            }
            else
            {
                rpc_msg.msg_name.sa_family = RPC_AF_UNSPEC;
                rpc_msg.msg_name.sa_data.sa_data_len = 0;
                /* Any not-NULL pointer is suitable here */
                rpc_msg.msg_name.sa_data.sa_data_val =
                    (uint8_t *)(msg->msg_name);
            }
        }
        rpc_msg.msg_namelen = msg->msg_namelen;
        rpc_msg.msg_flags = msg->msg_flags;

        if (msg->msg_control != NULL)
        {
            rpc_msg.msg_control.msg_control_val = cmsg_hdrs;
            rpc_msg.msg_control.msg_control_len = msg->msg_cmsghdr_num;
            cmsg_hdrs[0].data.data_val = msg->msg_control;
            cmsg_hdrs[0].data.data_len = msg->msg_controllen -
                                         msg->msg_cmsghdr_num *
                                         sizeof(struct cmsghdr);
        }
    }

    if (bytes_received != NULL)
    {
        in.bytes_received.bytes_received_len = 1;
        in.bytes_received.bytes_received_val = bytes_received;
    }
    in.overlapped = (tarpc_overlapped)overlapped;
    FILL_CALLBACK(wsa_recv_msg);

    rcf_rpc_call(rpcs, "wsa_recv_msg", &in, &out);

    free(in.callback);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_recv_msg, out.retval);

    snprintf(str_buf, sizeof(str_buf),
             "RPC (%s,%s)%s: wsa_recv_msg(%d, %p(",
             rpcs->ta, rpcs->name, rpcop2str(op), s, msg);

    if (RPC_IS_CALL_OK(rpcs)) {
    
        if (msg != NULL && out.msg.msg_val != NULL)
        {
            rpc_msg = out.msg.msg_val[0];

            if (msg->msg_name != NULL)
            {
                ((struct sockaddr *)(msg->msg_name))->sa_family =
                    addr_family_rpc2h(rpc_msg.msg_name.sa_family);
                memcpy(((struct sockaddr *)(msg->msg_name))->sa_data,
                       rpc_msg.msg_name.sa_data.sa_data_val,
                       rpc_msg.msg_name.sa_data.sa_data_len);
            }
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
                    RETVAL_INT(wsa_recv_msg, -1);
                }
                
                if (c != NULL)
                    msg->msg_controllen = 
                        (char *)c - (char *)msg->msg_control;
            }

            msg->msg_flags = (rpc_send_recv_flags)rpc_msg.msg_flags;

            snprintf(str_buf + strlen(str_buf),
                     sizeof(str_buf) - strlen(str_buf),
                     "msg_name: %p, "
                     "msg_namelen: %" TE_PRINTF_SOCKLEN_T "d, "
                     "msg_iov: %p, "
                     "msg_iovlen: %" TE_PRINTF_SIZE_T "d, "
                     "msg_control: %p, "
                     "msg_controllen: %" TE_PRINTF_SOCKLEN_T "d, "
                     "msg_flags: %s",
                     msg->msg_name, msg->msg_namelen,
                     msg->msg_iov, msg->msg_iovlen,
                     msg->msg_control, msg->msg_controllen,
                     send_recv_flags_rpc2str(msg->msg_flags));
        }

        if (bytes_received != NULL &&
            out.bytes_received.bytes_received_val != NULL)
        {
            *bytes_received = out.bytes_received.bytes_received_val[0];
        }
    }

    snprintf(str_buf + strlen(str_buf), sizeof(str_buf) - strlen(str_buf),
             "), %d, %u, %p", *bytes_received, overlapped, callback);
    snprintf(str_buf + strlen(str_buf), sizeof(str_buf) - strlen(str_buf),
             ") -> %d (%s)",
             out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    TAPI_RPC_LOG("%s", str_buf);

    RETVAL_INT(wsa_recv_msg, out.retval);
}

te_bool
rpc_wsa_get_overlapped_result(rcf_rpc_server *rpcs,
                              int s, rpc_overlapped overlapped,
                              int *bytes, te_bool wait,
                              rpc_send_recv_flags *flags,
                              char *buf, int buflen)
{
    tarpc_wsa_get_overlapped_result_in   in;
    tarpc_wsa_get_overlapped_result_out  out;
    rcf_rpc_op                           op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_get_overlapped_result, -1);
    }

    op = rpcs->op;

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
    in.get_data = buf == NULL ? FALSE : TRUE;

    rcf_rpc_call(rpcs, "wsa_get_overlapped_result", &in, &out);

    if ((out.retval) && (buf != NULL) && (buflen > 0))
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
    if (RPC_IS_CALL_OK(rpcs))
    {
        if (bytes != NULL && out.bytes.bytes_val != NULL)
            *bytes = out.bytes.bytes_val[0];

        if (flags != NULL && out.flags.flags_val != NULL)
            *flags = out.flags.flags_val[0];
    }

    if (op == RCF_RPC_CALL)
        out.retval = TRUE;

    CHECK_RETVAL_VAR_IS_BOOL(wsa_get_overlapped_result, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAGetOverlappedResult(%d, %u, %s]) "
                 "-> %s (%s) bytes transferred %u flags %s",
                 rpcs->ta, rpcs->name, rpcop2str(op), 
                 s, overlapped, wait ? "wait" : "don't wait",
                 out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)),
                 PTR_VAL(bytes), send_recv_flags_rpc2str(PTR_VAL(flags)));

    RETVAL_BOOL(wsa_get_overlapped_result, out.retval);
}

int
rpc_wsa_duplicate_socket(rcf_rpc_server *rpcs,
                         int s, int pid, uint8_t *info, int *info_len)
{
    rcf_rpc_op op;

    tarpc_duplicate_socket_in  in;
    tarpc_duplicate_socket_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(duplicate_socket, -1);
    }

    if ((info == NULL) != (info_len == NULL))
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(duplicate_socket, -1);
    }

    if (info_len != NULL && *info_len == 0)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(duplicate_socket, -1);
    }

    op = rpcs->op;


    in.s = s;
    in.pid = pid;
    if (info_len != NULL)
    {
        in.info.info_len = *info_len;
        in.info.info_val = info;
    }

    rcf_rpc_call(rpcs, "duplicate_socket", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (info_len != NULL)
            *info_len = out.info.info_len;
        memcpy(info, out.info.info_val, out.info.info_len);
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(duplicate_socket, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSADuplicateSocket(%d, %d, %p, %p) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), s, pid,
                 info, info_len,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(duplicate_socket, out.retval);
}

int
rpc_wait_for_multiple_events(rcf_rpc_server *rpcs,
                             int count, rpc_wsaevent *events,
                             te_bool wait_all, uint32_t timeout,
                             te_bool alertable)
{
    tarpc_wait_for_multiple_events_in  in;
    tarpc_wait_for_multiple_events_out out;

    rcf_rpc_op op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wait_for_multiple_events, WSA_WAIT_FAILED);
    }

    op = rpcs->op;
    in.events.events_len = count;
    in.events.events_val = (tarpc_wsaevent *)events;
    in.wait_all = wait_all;
    in.timeout = timeout;
    in.alertable = alertable;

    rcf_rpc_call(rpcs, "wait_for_multiple_events", &in, &out);
    
    if (RPC_IS_CALL_OK(rpcs) && op != RCF_RPC_CALL)
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

    CHECK_RETVAL_VAR(wait_for_multiple_events, out.retval, FALSE,
                     WSA_WAIT_FAILED);

    TAPI_RPC_LOG("RPC (%s,%s)%s: WSAWaitForMultipleEvents"
                 "(%d, %p, %s, %d, %s) -> %s (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 count, events, wait_all ? "true" : "false", timeout,
                 alertable ? "true" : "false",
                 wsa_wait_rpc2str(out.retval),
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wait_for_multiple_events, out.retval);
}

/**
 * Check, if RPC server is located on TA with winsock2.
 *
 * @param rpcs  RPC server handle
 *
 * @return TRUE, if it is definitely known that winsock2 is used and FALSE
 *         otherwise
 */
te_bool
rpc_is_winsock2(rcf_rpc_server *rpcs)
{
    rpc_wsaevent hevent;
    char        *value;
    te_bool      result;
    int          rc;
    cfg_handle   handle;


    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return FALSE;
    }
    
    /* First check that instance for TA exists in the configurator */
    rc = cfg_get_instance_fmt(NULL, &value, "/volatile:/ta_sockets:%s",
                              rpcs->ta);
    if (rc == 0)
    {
        result = strcmp(value, "winsock2") == 0;
        
        free(value);
        
        return result;
    }             

    RPC_AWAIT_IUT_ERROR(rpcs);
    hevent = rpc_create_event(rpcs);
    if (hevent == RPC_NULL)
    {
        if (RPC_ERRNO(rpcs) != RPC_ERPCNOTSUPP)
        {
            ERROR("RPC failed with unexpected error");
            return FALSE;
        }
        result = FALSE;
    }
    else
    {
        rpc_close_event(rpcs, hevent);
        result = TRUE;
    }
    
    if ((rc = cfg_add_instance_fmt(&handle, CVT_STRING, 
                                   result ? "winsock2" : "berkeley",
                                   "/volatile:/ta_sockets:%s", 
                                   rpcs->ta)) != 0)
    {
        ERROR("Failed to add /volatile:/ta_sockets:%s ; rc = 0x%x",
              rpcs->ta, rc);
    }                                   
    
    return result;
}

int
rpc_wsa_address_to_string(rcf_rpc_server *rpcs, struct sockaddr *addr,
                          socklen_t addrlen, uint8_t *info, int info_len,
                          char *addrstr, ssize_t *addrstr_len)
{
    rcf_rpc_op                      op;
    tarpc_wsa_address_to_string_in  in;
    tarpc_wsa_address_to_string_out out;
    tarpc_size_t                    alen = 0;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_address_to_string, -1);
    }


    op = rpcs->op;

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
    in.addrlen = addrlen;

    in.info.info_val = info;
    in.info.info_len = info_len;

    in.addrstr.addrstr_val = addrstr;
    if (addrstr_len != NULL)
        in.addrstr.addrstr_len = *addrstr_len;
    else
        in.addrstr.addrstr_len = 0;

    in.addrstr_len.addrstr_len_val = addrstr_len == NULL ? NULL : &alen;
    in.addrstr_len.addrstr_len_len = 1;

    rcf_rpc_call(rpcs, "wsa_address_to_string", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if ((addrstr_len != NULL) &&
            (out.addrstr_len.addrstr_len_val != NULL))
        {
            *addrstr_len = *out.addrstr_len.addrstr_len_val;

            if ((addrstr != NULL) && (out.addrstr.addrstr_val != NULL))
            {
                memcpy(addrstr, out.addrstr.addrstr_val, *addrstr_len);
            }        
        }
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_address_to_string,
                                                    out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAAddressToString(%s, %u, %p, %d, %s, %d) "
                 "-> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 sockaddr2str(addr), addrlen, info, info_len,
                 addrstr, *addrstr_len,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_address_to_string, out.retval);
}

int
rpc_wsa_string_to_address(rcf_rpc_server *rpcs, char *addrstr,
                          rpc_socket_domain address_family,
                          uint8_t *info, int info_len,
                          struct sockaddr *addr, socklen_t *addrlen)
{
    rcf_rpc_op                      op;
    tarpc_wsa_string_to_address_in  in;
    tarpc_wsa_string_to_address_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_string_to_address, -1);
    }


    op = rpcs->op;

    in.addrstr.addrstr_val = addrstr;
    in.addrstr.addrstr_len = strlen(addrstr) + 1;

    in.address_family = address_family;

    in.info.info_val = info;
    in.info.info_len = info_len;
    
    in.addrlen.addrlen_val = addrlen;
    if (addrlen != NULL)
        in.addrlen.addrlen_len = 1;
     else
        in.addrlen.addrlen_len = 0;

    rcf_rpc_call(rpcs, "wsa_string_to_address", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if ((addrlen != NULL) && (out.addrlen.addrlen_val != NULL))
            *addrlen = *out.addrlen.addrlen_val;

        if ((addr != NULL) && (out.addr.sa_data.sa_data_val != NULL))
        {
            memcpy(addr->sa_data, out.addr.sa_data.sa_data_val,
                                 out.addr.sa_data.sa_data_len);
            addr->sa_family = addr_family_rpc2h(out.addr.sa_family);
        }
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_string_to_address,
                                                    out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAStringToAddress(%s, %s, %p, %d, %s, %u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 addrstr, domain_rpc2str(address_family),
                 info, info_len, sockaddr2str(addr),
                 *addrlen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_string_to_address, out.retval);
}

/* WSACancelAsyncRequest */
int
rpc_wsa_cancel_async_request(rcf_rpc_server *rpcs,
                             rpc_handle async_task_handle)
{
    rcf_rpc_op                         op;
    tarpc_wsa_cancel_async_request_in  in;
    tarpc_wsa_cancel_async_request_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_cancel_async_request, -1);
    }


    op = rpcs->op;

    in.async_task_handle = (tarpc_handle)async_task_handle;

    rcf_rpc_call(rpcs, "wsa_cancel_async_request", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_cancel_async_request,
                                                       out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: WSACancelAsyncRequest(%u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 async_task_handle,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_cancel_async_request, out.retval);
}

/**
 * Allocate a WSABUF structure, a buffer of specified length and
 * fill in the fields of the allocated structure according to the
 * allocated buffer pointer and length in the TA address space.
 */
int
rpc_alloc_wsabuf(rcf_rpc_server *rpcs, size_t len,
                 rpc_ptr *wsabuf, rpc_ptr *wsabuf_buf)
{
    rcf_rpc_op             op;
    tarpc_alloc_wsabuf_in  in;
    tarpc_alloc_wsabuf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(alloc_wsabuf, -1);
    }

    op = rpcs->op;

    in.len = len;

    rcf_rpc_call(rpcs, "alloc_wsabuf", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: alloc_wsabuf(%d, %p, %p) -> "
                 "%u (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 len, wsabuf, wsabuf_buf,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    *wsabuf = out.wsabuf;
    *wsabuf_buf = out.wsabuf_buf;

    RETVAL_INT(alloc_wsabuf, out.retval);
}

void 
rpc_free_wsabuf(rcf_rpc_server *rpcs, rpc_ptr wsabuf)
{
    rcf_rpc_op            op;
    tarpc_free_wsabuf_in  in;
    tarpc_free_wsabuf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(free_wsabuf);
    }

    op = rpcs->op;

    in.wsabuf = (tarpc_ptr)wsabuf;

    rcf_rpc_call(rpcs, "free_wsabuf", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: free_wsabuf(%u) -> (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 wsabuf,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(free_wsabuf);
}

/**
 * WSAConnect() call. Can be used with nonblocking sockets.
 *
 * @param caller_wsabuf          A pointer to the WSABUF structure in the TA
 *                               virtual address space (can be obtained by
 *                               a call to rpc_alloc_wsabuf()).
 * @param callee_wsabuf          A pointer to the WSABUF structure in the
 *                               TA virtual address space.
 */
int
rpc_wsa_connect(rcf_rpc_server *rpcs, int s, const struct sockaddr *addr,
                socklen_t addrlen, rpc_ptr caller_wsabuf,
                rpc_ptr callee_wsabuf, rpc_qos *sqos)
{
    rcf_rpc_op            op;
    tarpc_wsa_connect_in  in;
    tarpc_wsa_connect_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_connect, -1);
    }

    op = rpcs->op;
    
    in.s = s;

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
    in.addrlen = addrlen;

    in.caller_wsabuf = caller_wsabuf;
    in.callee_wsabuf = callee_wsabuf;

    if (sqos == NULL)
        in.sqos_is_null = TRUE;
    else
    {
        in.sqos_is_null = FALSE;
        in.sqos.sending = sqos->sending;
        in.sqos.receiving = sqos->receiving;
        in.sqos.provider_specific_buf.provider_specific_buf_val =
            sqos->provider_specific_buf;
        in.sqos.provider_specific_buf.provider_specific_buf_len =
            sqos->provider_specific_buf_len;
    }

    rcf_rpc_call(rpcs, "wsa_connect", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_connect, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAConnect(%d, %s, %u, %u, %u, %p) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, sockaddr2str(addr), addrlen,
                 caller_wsabuf, callee_wsabuf, sqos,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_connect, out.retval);
}

/**
 * Convert the data from wsa_ioctl_request structure to the output buffer.
 */
static int 
convert_wsa_ioctl_result(rpc_ioctl_code code,
                         wsa_ioctl_request *res, char *buf)
{
    switch (code)
    {
        case RPC_SIO_ADDRESS_LIST_QUERY:
        {
            struct sockaddr *addr;
            unsigned int    i;
            
            *(unsigned int *)buf = res->wsa_ioctl_request_u.req_saa.
                                        req_saa_len;
            buf += sizeof(unsigned int);
            
            if (res->wsa_ioctl_request_u.req_saa.req_saa_len * 
                sizeof(struct sockaddr_storage) > RPC_WSA_IOCTL_OUTBUF_MAX)
            {
                return -1;
            }

            for (i = 0; 
                 i < res->wsa_ioctl_request_u.req_saa.req_saa_len;
                 i++)
            {
                addr = (struct sockaddr *)
                           (((struct sockaddr_storage *)buf) + i);

                addr->sa_family = addr_family_rpc2h(
                    res->wsa_ioctl_request_u.req_saa.req_saa_val[i]
                        .sa_family);

                memcpy(addr->sa_data,
                       res->wsa_ioctl_request_u.req_saa.req_saa_val[i].
                           sa_data.sa_data_val,
                       res->wsa_ioctl_request_u.req_saa.req_saa_val[i].
                           sa_data.sa_data_len);
 
            }

            break;
        }

        case RPC_SIO_GET_BROADCAST_ADDRESS:
        case RPC_SIO_ROUTING_INTERFACE_QUERY:
        {
            struct sockaddr *addr = (struct sockaddr *)buf;

            addr->sa_family = addr_family_rpc2h(
                res->wsa_ioctl_request_u.req_sa.sa_family);
            memcpy(addr->sa_data,
                   res->wsa_ioctl_request_u.req_sa.sa_data.sa_data_val,
                   res->wsa_ioctl_request_u.req_sa.sa_data.sa_data_len);
            break;
        }

        case RPC_SIO_GET_EXTENSION_FUNCTION_POINTER:
            *(rpc_ptr *)buf = res->wsa_ioctl_request_u.req_ptr;
            break;

        case RPC_SIO_GET_GROUP_QOS:
        case RPC_SIO_GET_QOS:
        {
            tarpc_qos *rqos;
            rpc_qos   *qos;

            rqos = &res->wsa_ioctl_request_u.req_qos;
            qos = (rpc_qos *)buf;

            qos->sending = *(tarpc_flowspec*)&rqos->sending;
            qos->receiving = *(tarpc_flowspec*)&rqos->receiving;

            if (sizeof(*qos) + 
                rqos->provider_specific_buf.provider_specific_buf_len >
                RPC_WSA_IOCTL_OUTBUF_MAX)
            {
                return -1;
            }
            qos->provider_specific_buf_len = 
                rqos->provider_specific_buf.provider_specific_buf_len;
            qos->provider_specific_buf = buf + sizeof(*qos);
            memcpy(qos->provider_specific_buf,
                   rqos->provider_specific_buf.provider_specific_buf_val,
                   qos->provider_specific_buf_len);
        }
        
        default:
            *(int *)buf = res->wsa_ioctl_request_u.req_int;
            break;

    }

    return 0;
}

/**
 * Calls WSAIoctl() at TA side. The formatted results of the overlapped
 * operation can be obtained by rpc_get_wsa_ioctl_overlapped_result().
 */
int
rpc_wsa_ioctl(rcf_rpc_server *rpcs, int s, rpc_ioctl_code control_code,
              char *inbuf, unsigned int inbuf_len, char *outbuf,
              unsigned int outbuf_len, unsigned int *bytes_returned,
              rpc_overlapped overlapped, const char *callback)
{
    rcf_rpc_op           op;
    tarpc_wsa_ioctl_in   in;
    tarpc_wsa_ioctl_out  out;
    
    wsa_ioctl_request in_req, out_req;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_ioctl, -1);
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&in_req, 0, sizeof(in_req));
    memset(&out_req, 0, sizeof(out_req));

    in.s = s;
    in.code = control_code;
    in.outbuf_len = outbuf_len;
    in.inbuf_len = inbuf_len;
    in.overlapped = (tarpc_overlapped)overlapped;
    FILL_CALLBACK(wsa_ioctl);
    if (bytes_returned != NULL)
    {
        in.bytes_returned.bytes_returned_val = bytes_returned;
        in.bytes_returned.bytes_returned_len = 1;
    }
    if (outbuf != NULL)
    {
        in.outbuf.outbuf_val = &out_req;
        in.outbuf.outbuf_len = 1;
    }
    if (inbuf != NULL)
    {
        in.inbuf.inbuf_val = &in_req;
        in.inbuf.inbuf_len = 1;
    }
    else
        goto call;

    switch (control_code)
    {
        case RPC_FIONBIO:
        case RPC_FIONREAD:
        case RPC_SIO_CHK_QOS:
        case RPC_SIO_MULTIPOINT_LOOPBACK:
        case RPC_SIO_MULTICAST_SCOPE:
        case RPC_SIO_RCVALL:
        case RPC_SIO_RCVALL_IGMPMCAST:
        case RPC_SIO_RCVALL_MCAST:
        case RPC_SIO_UDP_CONNRESET:
            in_req.type = WSA_IOCTL_INT;
            in_req.wsa_ioctl_request_u.req_int = *(int *)inbuf;
            break;

        case RPC_SIO_FIND_ROUTE:
        case RPC_SIO_ROUTING_INTERFACE_CHANGE:
        case RPC_SIO_ROUTING_INTERFACE_QUERY:
        {
            struct sockaddr *addr = (struct sockaddr *)inbuf;
            tarpc_sa        *rpc_addr = &in_req.wsa_ioctl_request_u.req_sa;

            in_req.type = WSA_IOCTL_SA;
            rpc_addr->sa_family = addr_family_h2rpc(addr->sa_family);
            rpc_addr->sa_data.sa_data_val = addr->sa_data;
            rpc_addr->sa_data.sa_data_len =
                sizeof(struct sockaddr) - SA_COMMON_LEN;
            break;
        }

        case RPC_SIO_GET_EXTENSION_FUNCTION_POINTER:
            in_req.type = WSA_IOCTL_GUID;
            in_req.wsa_ioctl_request_u.req_guid = *(tarpc_guid *)inbuf;
            break;

        case RPC_SIO_KEEPALIVE_VALS:
            in_req.type = WSA_IOCTL_TCP_KEEPALIVE;
            in_req.wsa_ioctl_request_u.req_tka = 
                *(tarpc_tcp_keepalive *)inbuf;
            break;

        case RPC_SIO_SET_QOS:
        {
            tarpc_qos *rqos;
            rpc_qos   *qos;

            rqos = &in_req.wsa_ioctl_request_u.req_qos;
            qos = (rpc_qos *)inbuf;

            in_req.type = WSA_IOCTL_QOS;
            /* rpc_flowspec and tarpc_flowspec declarations are
             * identical, so we can freely cast each to other. */
            rqos->sending = *(tarpc_flowspec *)&qos->sending;
            rqos->receiving = *(tarpc_flowspec *)&qos->receiving;
            rqos->provider_specific_buf.provider_specific_buf_val =
                qos->provider_specific_buf;
            rqos->provider_specific_buf.provider_specific_buf_len =
                qos->provider_specific_buf_len;
            break;
        }

        case RPC_SIO_ASSOCIATE_HANDLE:
        case RPC_SIO_TRANSLATE_HANDLE:
            ERROR("SIO_*_HANDLE are not supported yet");
            RETVAL_INT(wsa_ioctl, -1);
            break;

        default:
            in_req.type = WSA_IOCTL_VOID; /* No input data */
    }

call:
    rcf_rpc_call(rpcs, "wsa_ioctl", &in, &out);

    free(in.callback);

    if (RPC_IS_CALL_OK(rpcs) && (out.retval == 0))
    {
        if (bytes_returned != NULL)
            *bytes_returned = *(out.bytes_returned.bytes_returned_val);
            
        if (outbuf != NULL && out.outbuf.outbuf_val != NULL)
        {
            if (convert_wsa_ioctl_result(control_code, 
                                         out.outbuf.outbuf_val,
                                         outbuf) < 0)
            {
                ERROR("Cannot convert the result: increase "
                      "RPC_WSA_IOCTL_OUTBUF_MAX");
                      
                out.retval = -1;
            }
        }
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(wsa_ioctl, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAIoctl(%d, %s, %p, %u, %p, %u, %u, %u, %s) "
                 "-> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, ioctl_rpc2str(control_code),
                 inbuf, inbuf_len, outbuf, outbuf_len,
                 PTR_VAL(bytes_returned), overlapped, callback,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_ioctl, out.retval);
}

te_bool
rpc_get_wsa_ioctl_overlapped_result(rcf_rpc_server *rpcs,
                                    int s, rpc_overlapped overlapped,
                                    int *bytes, te_bool wait,
                                    rpc_send_recv_flags *flags,
                                    char *buf, 
                                    rpc_ioctl_code control_code)
{
    rcf_rpc_op op;

    tarpc_get_wsa_ioctl_overlapped_result_in  in;
    tarpc_get_wsa_ioctl_overlapped_result_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(get_wsa_ioctl_overlapped_result, -1);
    }

    op = rpcs->op;

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
    in.code = control_code;

    rcf_rpc_call(rpcs, "get_wsa_ioctl_overlapped_result", &in, &out);

    if (op == RCF_RPC_CALL)
        out.retval = TRUE;

    CHECK_RETVAL_VAR_IS_BOOL(get_wsa_ioctl_overlapped_result, out.retval);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (bytes != NULL && out.bytes.bytes_val != NULL)
            *bytes = out.bytes.bytes_val[0];

        if (flags != NULL && out.flags.flags_val != NULL)
            *flags = out.flags.flags_val[0];

        if (out.retval && (buf != NULL))
        {
            if (convert_wsa_ioctl_result(control_code, &out.result, 
                                         buf) < 0)
            {
                out.retval = 0;
            }
        }
    }

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAGetOverlappedResult(%d, %u, %s) for ioctl %s"
                 "-> %s (%s) bytes transferred %u",
                 rpcs->ta, rpcs->name, rpcop2str(op), 
                 s, overlapped, wait? "wait" : "don't wait",
                 ioctl_rpc2str(control_code),
                 out.retval ? "true" : "false",
                 errno_rpc2str(RPC_ERRNO(rpcs)), PTR_VAL(bytes));

    RETVAL_BOOL(get_wsa_ioctl_overlapped_result, out.retval);
}


/**
 * @param buf   A valid pointer in the TA virtual address space
 *              (can be obtained by a call to rpc_alloc_buf()).
 */
rpc_handle
rpc_wsa_async_get_host_by_addr(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                               unsigned int wmsg, char *addr,
                               ssize_t addrlen, rpc_socket_type type,
                               rpc_ptr buf, ssize_t buflen) 
{
    rcf_rpc_op                           op;
    tarpc_wsa_async_get_host_by_addr_in  in;
    tarpc_wsa_async_get_host_by_addr_out out;

    char str_buf[1024] = {'\0', };
    int i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(wsa_async_get_host_by_addr,  RPC_NULL);
    }

    for (i = 0; i < addrlen; i++)
        snprintf(str_buf, sizeof(str_buf) - strlen(str_buf),
                 "%02x", *(addr + i));
    op = rpcs->op;

    in.hwnd = (tarpc_hwnd)hwnd;
    in.wmsg = wmsg;

    in.addr.addr_val = addr;
    if (addr != NULL)
        in.addr.addr_len = addrlen;
    else
        in.addr.addr_len = 0;

    in.addrlen = addrlen;
    in.type = type;

    in.buf = buf;
    in.buflen = buflen;

    rcf_rpc_call(rpcs, "wsa_async_get_host_by_addr", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAAsyncGetHostByAddr(%u, %u, %s, %s, %p, %d) -> "
                 "%p (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 hwnd, wmsg, 
                 (*str_buf == '\0') ? "(nil)" : str_buf, 
                 socktype_rpc2str(type), buf, buflen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(wsa_async_get_host_by_addr, out.retval);
}

rpc_handle
rpc_wsa_async_get_host_by_name(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                               unsigned int wmsg, char *name,
                               rpc_ptr buf, ssize_t buflen) 
{
    rcf_rpc_op                           op;
    tarpc_wsa_async_get_host_by_name_in  in;
    tarpc_wsa_async_get_host_by_name_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(wsa_async_get_host_by_name, RPC_NULL);
    }

    op = rpcs->op;

    in.hwnd = (tarpc_hwnd)hwnd;
    in.wmsg = wmsg;

    in.name.name_val = name;
    if (name != NULL)
        in.name.name_len = strlen(name) + 1;
    else
        in.name.name_len = 0;

    in.buf = buf;
    in.buflen = buflen;

    rcf_rpc_call(rpcs, "wsa_async_get_host_by_name", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAAsyncGetHostByName(%u, %u, %s, %p, %d) -> "
                 "%p (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 hwnd, wmsg, name, buf, buflen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(wsa_async_get_host_by_name, out.retval);
}

rpc_handle
rpc_wsa_async_get_proto_by_name(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                                unsigned int wmsg, char *name,
                                rpc_ptr buf, ssize_t buflen) 
{
    rcf_rpc_op                            op;
    tarpc_wsa_async_get_proto_by_name_in  in;
    tarpc_wsa_async_get_proto_by_name_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(wsa_async_get_proto_by_name, RPC_NULL);
    }

    op = rpcs->op;

    in.hwnd = (tarpc_hwnd)hwnd;
    in.wmsg = wmsg;

    in.name.name_val = name;
    if (name != NULL)
        in.name.name_len = strlen(name) + 1;
    else
        in.name.name_len = 0;

    in.buf = buf;
    in.buflen = buflen;

    rcf_rpc_call(rpcs, "wsa_async_get_proto_by_name", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAAsyncGetProtoByName(%u, %u, %s, %p, %d) -> "
                 "%p (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 hwnd, wmsg, name, buf, buflen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(wsa_async_get_proto_by_name, out.retval);
}

rpc_handle
rpc_wsa_async_get_proto_by_number(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                                  unsigned int wmsg, int number,
                                  rpc_ptr buf, ssize_t buflen) 
{
    rcf_rpc_op                              op;
    tarpc_wsa_async_get_proto_by_number_in  in;
    tarpc_wsa_async_get_proto_by_number_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(wsa_async_get_proto_by_number, RPC_NULL);
    }

    op = rpcs->op;

    in.hwnd = (tarpc_hwnd)hwnd;
    in.wmsg = wmsg;
    in.number = number;
    in.buf = buf;
    in.buflen = buflen;

    rcf_rpc_call(rpcs, "wsa_async_get_proto_by_number", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAAsyncGetProtoByNumber(%u, %u, %d, %p, %d) -> "
                 "%p (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 hwnd, wmsg, number, buf, buflen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(wsa_async_get_proto_by_number, out.retval);
}

rpc_handle
rpc_wsa_async_get_serv_by_name(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                               unsigned int wmsg, char *name, char *proto,
                               rpc_ptr buf, ssize_t buflen) 
{
    rcf_rpc_op                           op;
    tarpc_wsa_async_get_serv_by_name_in  in;
    tarpc_wsa_async_get_serv_by_name_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(wsa_async_get_serv_by_name, RPC_NULL);
    }

    op = rpcs->op;

    in.hwnd = (tarpc_hwnd)hwnd;
    in.wmsg = wmsg;

    in.name.name_val = name;
    if (name != NULL)
        in.name.name_len = strlen(name) + 1;
    else
        in.name.name_len = 0;

    in.proto.proto_val = proto;
    if (proto != NULL)
        in.proto.proto_len = strlen(proto) + 1;
    else
        in.proto.proto_len = 0;

    in.buf = buf;
    in.buflen = buflen;

    rcf_rpc_call(rpcs, "wsa_async_get_serv_by_name", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAAsyncGetServByName(%u, %u, %s, %s, %p, %d) -> "
                 "%p (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 hwnd, wmsg, name, proto, buf, buflen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(wsa_async_get_serv_by_name, out.retval);
}

rpc_handle
rpc_wsa_async_get_serv_by_port(rcf_rpc_server *rpcs, rpc_hwnd hwnd,
                               unsigned int wmsg, int port, char *proto,
                               rpc_ptr buf, ssize_t buflen) 
{
    rcf_rpc_op                           op;
    tarpc_wsa_async_get_serv_by_port_in  in;
    tarpc_wsa_async_get_serv_by_port_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(wsa_async_get_serv_by_port, RPC_NULL);
    } 

    op = rpcs->op;

    in.hwnd = (tarpc_hwnd)hwnd;
    in.wmsg = wmsg;
    in.port = port;

    in.proto.proto_val = proto;
    if (proto != NULL)
        in.proto.proto_len = strlen(proto) + 1;
    else
        in.proto.proto_len = 0;

    in.buf = buf;
    in.buflen = buflen;

    rcf_rpc_call(rpcs, "wsa_async_get_serv_by_port", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAAsyncGetServByPort(%u, %u, %d, %s, %p, %d) -> "
                 "%p (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 hwnd, wmsg, port, proto, buf, buflen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(wsa_async_get_serv_by_port, out.retval);
}

/**
 * Joins a leaf node into a multipoint session.
 *
 * @param caller_wsabuf          A pointer to the WSABUF structure in the TA
 *                               virtual address space (can be obtained by
 *                               a call to rpc_alloc_wsabuf()).
 * @param callee_wsabuf          A pointer to the WSABUF structure in the
 *                               TA virtual address space.
 */
int 
rpc_wsa_join_leaf(rcf_rpc_server *rpcs, int s, struct sockaddr *addr, 
                  socklen_t addrlen, rpc_ptr caller_wsabuf, 
                  rpc_ptr callee_wsabuf, rpc_qos *sqos, 
                  rpc_join_leaf_flags flags)
{
    rcf_rpc_op            op;
    tarpc_wsa_join_leaf_in  in;
    tarpc_wsa_join_leaf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(wsa_connect, -1);
    }

    op = rpcs->op;
    
    in.s = s;
    in.flags = flags;
    
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
    in.addrlen = addrlen;

    in.caller_wsabuf = caller_wsabuf;
    in.callee_wsabuf = callee_wsabuf;

    if (sqos == NULL)
        in.sqos_is_null = TRUE;
    else
    {
        in.sqos_is_null = FALSE;
        in.sqos.sending = *(tarpc_flowspec*)&sqos->sending;
        in.sqos.receiving = *(tarpc_flowspec*)&sqos->receiving;
        in.sqos.provider_specific_buf.provider_specific_buf_val =
            sqos->provider_specific_buf;
        in.sqos.provider_specific_buf.provider_specific_buf_len =
            sqos->provider_specific_buf_len;
    }

    rcf_rpc_call(rpcs, "wsa_join_leaf", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(wsa_join_leaf, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "WSAJoinLeaf(%d, %s, %u, %u, %u, %p, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 s, sockaddr2str(addr), addrlen, 
                 caller_wsabuf, callee_wsabuf, sqos, 
                 join_leaf_flags_rpc2str(flags),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(wsa_join_leaf, out.retval);
}

te_bool 
rpc_read_file(rcf_rpc_server *rpcs,
              int fd, void *buf, size_t count,
              size_t *received, rpc_overlapped overlapped)
{
    tarpc_read_file_in  in;
    tarpc_read_file_out out;
    rcf_rpc_op          op;
    tarpc_size_t        rcvd = PTR_VAL(received);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return FALSE;
    }

    op = rpcs->op;

    in.fd = fd;
    in.len = count;
    in.buf.buf_val = buf;
    in.buf.buf_len = buf == NULL ? 0 : count;
    if (received == NULL)
        in.received.received_len = 0;
    else
        in.received.received_len = 1;
    in.received.received_val = received == NULL ? NULL : &rcvd;
    in.overlapped = (tarpc_overlapped)overlapped;
    
    rcf_rpc_call(rpcs, "read_file", &in, &out);

    if (op == RCF_RPC_CALL)
        out.retval = TRUE;

    CHECK_RETVAL_VAR_IS_BOOL(read_file, out.retval);

    if (op != RCF_RPC_CALL && out.retval)
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);

        if (received != NULL && out.received.received_val != 0)
            *received = out.received.received_val[0];
    }
    
    TAPI_RPC_LOG("RPC (%s,%s)%s: ReadFile(%d, %p, %u, %p, %u) "
                 "-> %d %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, buf, count, received, overlapped, 
                 out.retval, PTR_VAL(received), 
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(read_file, out.retval);
}                  

te_bool 
rpc_write_file(rcf_rpc_server *rpcs,
               int fd, void *buf, size_t count,
               size_t *sent, rpc_overlapped overlapped)
{
    tarpc_write_file_in  in;
    tarpc_write_file_out out;
    rcf_rpc_op           op;
    tarpc_size_t         snd = PTR_VAL(sent);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return FALSE;
    }

    op = rpcs->op;

    in.fd = fd;
    in.len = count;
    in.buf.buf_val = buf;
    in.buf.buf_len = buf == NULL ? 0 : count;
    if (sent == NULL)
        in.sent.sent_len = 0;
    else
        in.sent.sent_len = 1;
    in.sent.sent_val = sent == NULL ? NULL : &snd;
    in.overlapped = (tarpc_overlapped)overlapped;
    
    rcf_rpc_call(rpcs, "write_file", &in, &out);

    if (op == RCF_RPC_CALL)
        out.retval = TRUE;

    if (op != RCF_RPC_CALL && out.retval)
    {
        if (sent != NULL && out.sent.sent_val != 0)
            *sent = out.sent.sent_val[0];
    }

    CHECK_RETVAL_VAR_IS_BOOL(write_file, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: WriteFile(%d, %p, %u, %p, %u) "
                 "-> %d %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, buf, count, sent, overlapped, 
                 out.retval, PTR_VAL(sent), 
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_BOOL(write_file, out.retval);
}               
