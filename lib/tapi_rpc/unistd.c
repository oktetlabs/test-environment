/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of generic file API
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
#if HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#if HAVE_PWD_H
#include <pwd.h>
#endif

#include "tapi_rpc_internal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_misc.h"
#include "te_printf.h"


/** @name String buffers for snprintf() operations */
static char str_buf_1[4096];
static char str_buf_2[4096];
/*@}*/


int
rpc_pipe(rcf_rpc_server *rpcs,
         int filedes[2])
{
    tarpc_pipe_in  in;
    tarpc_pipe_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pipe, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    if (filedes != NULL)
    {
        in.filedes.filedes_len = 2;
        in.filedes.filedes_val = filedes;
    }

    rcf_rpc_call(rpcs, "pipe", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && filedes != NULL)
        memcpy(filedes, out.filedes.filedes_val, sizeof(int) * 2);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(pipe, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): pipe(%p) -> %d (%s) (%d,%d)",
                 rpcs->ta, rpcs->name, filedes,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 filedes != NULL ? filedes[0] : -1,
                 filedes != NULL ? filedes[1] : -1);

    RETVAL_INT(pipe, out.retval);
}

int
rpc_socketpair(rcf_rpc_server *rpcs,
               rpc_socket_domain domain, rpc_socket_type type,
               rpc_socket_proto protocol, int sv[2])
{
    tarpc_socketpair_in  in;
    tarpc_socketpair_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(socketpair, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.domain = domain;
    in.type = type;
    in.proto = protocol;
    if (sv != NULL)
    {
        in.sv.sv_len = 2;
        in.sv.sv_val = sv;
    }

    rcf_rpc_call(rpcs, "socketpair", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && sv != NULL)
        memcpy(sv, out.sv.sv_val, sizeof(int) * 2);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(socketpair, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): socketpair(%s, %s, %s, %p) -> "
                 "%d (%s) (%d,%d)",
                 rpcs->ta, rpcs->name,
                 domain_rpc2str(domain), socktype_rpc2str(type),
                 proto_rpc2str(protocol), sv,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 sv != NULL ? sv[0] : -1, sv != NULL ? sv[1] : -1);

    RETVAL_INT(socketpair, out.retval);
}

int
rpc_close(rcf_rpc_server *rpcs, int fd)
{
    rcf_rpc_op      op;
    tarpc_close_in  in;
    tarpc_close_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(close, -1);
    }

    op = rpcs->op;

    in.fd = fd;

    rcf_rpc_call(rpcs, "close", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(close, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: close(%d) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(close, out.retval);
}

ssize_t
rpc_write_at_offset(rcf_rpc_server *rpcs, int fd, char* buf,
                    size_t buflen, off_t offset)
{
    rcf_rpc_op                op;
    tarpc_write_at_offset_in  in;
    tarpc_write_at_offset_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(write_at_offset, -3);
    }

    op = rpcs->op;

    in.fd = fd;
    in.buf.buf_len = buflen;
    if (buf != NULL)
        in.buf.buf_val = buf;
    else
        in.buf.buf_val = NULL;
    in.offset = offset;

    rcf_rpc_call(rpcs, "write_at_offset", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: write_at_offset(%d, %p %d, %d)"
                 " -> %d, %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, buf, buflen, offset, out.offset, out.written,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    if (out.offset == (off_t)-1)  /* failed to repsition the file offset */
        RETVAL_INT(write_at_offset, -2);
    else
        RETVAL_INT(write_at_offset, out.written);
}

int
rpc_dup(rcf_rpc_server *rpcs,
        int oldfd)
{
    rcf_rpc_op      op;
    tarpc_dup_in    in;
    tarpc_dup_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(dup, -1);
    }

    op = rpcs->op;

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.oldfd = oldfd;

    rcf_rpc_call(rpcs, "dup", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(dup, out.fd);

    TAPI_RPC_LOG("RPC (%s,%s)%s: dup(%d) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 oldfd, out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(dup, out.fd);
}

int
rpc_dup2(rcf_rpc_server *rpcs,
         int oldfd, int newfd)
{
    rcf_rpc_op      op;
    tarpc_dup2_in   in;
    tarpc_dup2_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(dup2, -1);
    }

    op = rpcs->op;
    rpcs->op = RCF_RPC_CALL_WAIT;

    in.oldfd = oldfd;
    in.newfd = newfd;

    rcf_rpc_call(rpcs, "dup", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(dup2, out.fd);

    TAPI_RPC_LOG("RPC (%s,%s)%s: dup2(%d, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 oldfd, newfd, out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(dup2, out.fd);
}


int
rpc_read_gen(rcf_rpc_server *rpcs,
             int fd, void *buf, size_t count, size_t rbuflen)
{
    rcf_rpc_op     op;
    tarpc_read_in  in;
    tarpc_read_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    if (buf != NULL && count > rbuflen)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(read, -1);
    }

    in.fd = fd;
    in.len = count;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = rbuflen;
        in.buf.buf_val = buf;
    }
    rcf_rpc_call(rpcs, "read", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(read, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: read(%d, %p[%u], %u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, buf, rbuflen, count, out.retval,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(read, out.retval);
}

int
rpc_write(rcf_rpc_server *rpcs,
          int fd, const void *buf, size_t count)
{
    rcf_rpc_op      op;
    tarpc_write_in  in;
    tarpc_write_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(write, -1);
    }

    op = rpcs->op;

    in.fd = fd;
    in.len = count;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = count;
        in.buf.buf_val = (char *)buf;
    }

    rcf_rpc_call(rpcs, "write", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(write, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: write(%d, %p, %u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, buf, count,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(write, out.retval);
}

int
rpc_readv_gen(rcf_rpc_server *rpcs,
              int fd, const struct rpc_iovec *iov,
              size_t iovcnt, size_t riovcnt)
{
    rcf_rpc_op      op;
    tarpc_readv_in  in;
    tarpc_readv_out out;

    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(readv, -1);
    }

    op = rpcs->op;

    if (riovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
        RETVAL_INT(readv, -1);
    }

    if (iov != NULL && iovcnt > riovcnt)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(readv, -1);
    }


    in.fd = fd;
    in.count = iovcnt;

    VERB("IN readv(%d, %p[%u], %u)", fd, iov, riovcnt, riovcnt);
    if (iov != NULL)
    {
        in.vector.vector_len = riovcnt;
        in.vector.vector_val = iovec_arr;
        for (i = 0; i < riovcnt; i++)
        {
            VERB("IN readv() I/O vector #%d: %p[%u] %u",
                 i, iov[i].iov_base, iov[i].iov_rlen, iov[i].iov_len);
            iovec_arr[i].iov_base.iov_base_val = iov[i].iov_base;
            iovec_arr[i].iov_base.iov_base_len = iov[i].iov_rlen;
            iovec_arr[i].iov_len = iov[i].iov_len;
        }
    }

    rcf_rpc_call(rpcs, "readv", &in, &out);

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
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(readv, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: readv(%d, %p[%u], %u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, iov, riovcnt, iovcnt,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(readv, out.retval);
}

int
rpc_writev(rcf_rpc_server *rpcs,
           int fd, const struct rpc_iovec *iov, size_t iovcnt)
{
    rcf_rpc_op       op;
    tarpc_writev_in  in;
    tarpc_writev_out out;

    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(writev, -1);
    }

    op = rpcs->op;

    if (iovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
        RETVAL_INT(writev, -1);
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

    in.fd = fd;
    in.count = iovcnt;

    rcf_rpc_call(rpcs, "writev", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(writev, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: writev(%d, %p, %u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, iov, iovcnt,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(writev, out.retval);
}


rpc_fd_set_p
rpc_fd_set_new(rcf_rpc_server *rpcs)
{
    tarpc_fd_set_new_in  in;
    tarpc_fd_set_new_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(fd_set_new, RPC_NULL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    rcf_rpc_call(rpcs, "fd_set_new", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): fd_set_new() -> 0x%x (%s)",
                 rpcs->ta, rpcs->name,
                 (unsigned)out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(fd_set_new, out.retval);
}


void
rpc_fd_set_delete(rcf_rpc_server *rpcs, rpc_fd_set_p set)
{
    tarpc_fd_set_delete_in  in;
    tarpc_fd_set_delete_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(fd_set_delete);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_fd_set)set;

    rcf_rpc_call(rpcs, "fd_set_delete", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): fd_set_delete(0x%x) -> (%s)",
                 rpcs->ta, rpcs->name,
                 (unsigned)set, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(fd_set_delete);
}

void
rpc_do_fd_zero(rcf_rpc_server *rpcs, rpc_fd_set_p set)
{
    tarpc_do_fd_zero_in  in;
    tarpc_do_fd_zero_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(do_fd_zero);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_fd_set)set;

    rcf_rpc_call(rpcs, "do_fd_zero", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): do_fd_zero(0x%x) -> (%s)",
                 rpcs->ta, rpcs->name,
                 (unsigned)set, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(do_fd_zero);
}

void
rpc_do_fd_set(rcf_rpc_server *rpcs, int fd, rpc_fd_set_p set)
{
    tarpc_do_fd_set_in  in;
    tarpc_do_fd_set_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(do_fd_set);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_fd_set)set;
    in.fd = fd;

    rcf_rpc_call(rpcs, "do_fd_set", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): do_fd_set(%d, 0x%x) -> (%s)",
                 rpcs->ta, rpcs->name,
                 fd, (unsigned)set, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(do_fd_set);
}

void
rpc_do_fd_clr(rcf_rpc_server *rpcs, int fd, rpc_fd_set_p set)
{
    tarpc_do_fd_clr_in  in;
    tarpc_do_fd_clr_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(do_fd_clr);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_fd_set)set;
    in.fd = fd;

    rcf_rpc_call(rpcs, "do_fd_clr", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): do_fd_clr(%d, 0x%x) -> (%s)",
                 rpcs->ta, rpcs->name,
                 fd, (unsigned)set, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(do_fd_clr);
}

int
rpc_do_fd_isset(rcf_rpc_server *rpcs, int fd, rpc_fd_set_p set)
{
    tarpc_do_fd_isset_in  in;
    tarpc_do_fd_isset_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(do_fd_isset, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.set = (tarpc_fd_set)set;
    in.fd = fd;

    rcf_rpc_call(rpcs, "do_fd_isset", &in, &out);

    CHECK_RETVAL_VAR(do_fd_isset, out.retval,
                     (out.retval != 0 && out.retval != 1), -1);

    TAPI_RPC_LOG("RPC (%s,%s): do_fd_isset(%d, 0x%x) -> %d (%s)",
                 rpcs->ta, rpcs->name, fd, (unsigned)set,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(do_fd_isset, out.retval);
}

int
rpc_select(rcf_rpc_server *rpcs,
           int n, rpc_fd_set_p readfds, rpc_fd_set_p writefds,
           rpc_fd_set_p exceptfds, struct tarpc_timeval *timeout)
{
    rcf_rpc_op              op;
    tarpc_select_in         in;
    tarpc_select_out        out;
    struct tarpc_timeval    timeout_in;
    struct tarpc_timeval   *timeout_in_ptr = NULL;

    struct tarpc_timeval tv;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(select, -1);
    }

    op = rpcs->op;

    in.n = n;
    in.readfds = (tarpc_fd_set)readfds;
    in.writefds = (tarpc_fd_set)writefds;
    in.exceptfds = (tarpc_fd_set)exceptfds;

    if (timeout != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        tv.tv_sec = timeout->tv_sec;
        tv.tv_usec = timeout->tv_usec;
        in.timeout.timeout_len = 1;
        in.timeout.timeout_val = &tv;

        timeout_in_ptr = &timeout_in;
        timeout_in = *timeout;
    }

    rcf_rpc_call(rpcs, "select", &in, &out);

    if (op != RCF_RPC_CALL && timeout != NULL &&
        out.timeout.timeout_val != NULL && RPC_IS_CALL_OK(rpcs))
    {
        timeout->tv_sec = out.timeout.timeout_val[0].tv_sec;
        timeout->tv_usec = out.timeout.timeout_val[0].tv_usec;
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(select, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: select(%d, 0x%x, 0x%x, 0x%x, %s (%s)) "
                 "-> %d (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 n, (unsigned)readfds, (unsigned)writefds,
                 (unsigned)exceptfds, tarpc_timeval2str(timeout_in_ptr),
                 tarpc_timeval2str(timeout),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(select, out.retval);
}

int
rpc_pselect(rcf_rpc_server *rpcs,
            int n, rpc_fd_set_p readfds, rpc_fd_set_p writefds,
            rpc_fd_set_p exceptfds, struct timespec *timeout,
            const rpc_sigset_p sigmask)
{
    rcf_rpc_op        op;
    tarpc_pselect_in  in;
    tarpc_pselect_out out;

    struct tarpc_timespec tv;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pselect, -1);
    }

    op = rpcs->op;

    in.n = n;
    in.readfds = (tarpc_fd_set)readfds;
    in.writefds = (tarpc_fd_set)writefds;
    in.exceptfds = (tarpc_fd_set)exceptfds;
    in.sigmask = (tarpc_sigset_t)sigmask;

    if (timeout != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        tv.tv_sec = timeout->tv_sec;
        tv.tv_nsec = timeout->tv_nsec;
        in.timeout.timeout_len = 1;
        in.timeout.timeout_val = &tv;
    }

    rcf_rpc_call(rpcs, "pselect", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(pselect, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: pselect(%d, 0x%x, 0x%x, 0x%x, %s, 0x%x) "
                 "-> %d (%s)", rpcs->ta, rpcs->name,
                 rpcop2str(op), n, (unsigned)readfds, (unsigned)writefds,
                 (unsigned)exceptfds, timespec2str(timeout),
                 (unsigned)sigmask,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(pselect, out.retval);
}

/**
 * Convert poll() request to string.
 *
 * @param ufds      Array with requests per fd
 * @param ufds      Number of requests per fd
 * @param buf       Buffer to put string
 * @param buflen    Length of the buffer
 */
static void
pollreq2str(struct rpc_pollfd *ufds, unsigned int nfds,
            char *buf, size_t buflen)
{
    unsigned int    i;
    int             rc;
    
    if (buflen == 0)
    {
        ERROR("Too small buffer for poll request conversion");
        return;
    }

    if (ufds == NULL)
    {
        buf[0] = '\0';
        return;
    }

    do {
        rc = snprintf(buf, buflen, "{");
        if ((size_t)rc > buflen)
            break;
        buflen -= rc;
        buf += rc;
        for (i = 0; i < nfds; ++i)
        {
            rc = snprintf(buf, buflen, "{%d,%s,%s}",
                          ufds[i].fd,
                          poll_event_rpc2str(ufds[i].events),
                          poll_event_rpc2str(ufds[i].revents));
            if ((size_t)rc > buflen)
                break;
            buflen -= rc;
            buf += rc;
        }
        rc = snprintf(buf, buflen, "}");
        if ((size_t)rc > buflen)
            break;
        buflen -= rc;
        buf += rc;

        return;

    } while (0);

    ERROR("Too small buffer for poll request conversion");
}

int
rpc_poll_gen(rcf_rpc_server *rpcs,
             struct rpc_pollfd *ufds, unsigned int nfds,
             int timeout, unsigned int rnfds)
{
    rcf_rpc_op       op;
    tarpc_poll_in  in;
    tarpc_poll_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(poll, -1);
    }

    op = rpcs->op;

    /**
     * @attention It's assumed that rpc_pollfd is the same as tarpc_pollfd.
     * It's OK, because both structures are independent from particular
     * Socket API and used on the same host.
     */
    in.ufds.ufds_len = rnfds;
    in.ufds.ufds_val = (struct tarpc_pollfd *)ufds;
    in.timeout = timeout;
    in.nfds = nfds;

    pollreq2str(ufds, rnfds, str_buf_1, sizeof(str_buf_1));

    rcf_rpc_call(rpcs, "poll", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (ufds != NULL && out.ufds.ufds_val != NULL)
            memcpy(ufds, out.ufds.ufds_val, rnfds * sizeof(ufds[0]));
        pollreq2str(ufds, rnfds, str_buf_2, sizeof(str_buf_2));
    }
    else
    {
        *str_buf_2 = '\0';
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(poll, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: poll(%p%s, %d, %d) -> %d (%s) %s",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 ufds, str_buf_1, nfds, timeout,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)), str_buf_2);

    RETVAL_INT(poll, out.retval);
}

int
rpc_open(rcf_rpc_server *rpcs,
         const char *path, rpc_fcntl_flags flags, rpc_file_mode_flags mode)
{
    tarpc_open_in  in;
    tarpc_open_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(open, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)strdup(path); /* FIXME */
    }
    in.flags = flags;
    in.mode  = mode;

    rcf_rpc_call(rpcs, "open", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(open, out.fd);

    TAPI_RPC_LOG("RPC (%s,%s): open(%s, %s, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 path, fcntl_flags_rpc2str(flags),
                 file_mode_flags_rpc2str(mode),
                 out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(open, out.fd);
}


int
rpc_fcntl(rcf_rpc_server *rpcs, int fd,
            int cmd, int arg)
{
    tarpc_fcntl_in  in;
    tarpc_fcntl_out out;
    rcf_rpc_op      op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(fcntl, -1);
    }

    op = rpcs->op;

    in.fd = fd;
    in.cmd = cmd;
    in.arg = arg;

    rcf_rpc_call(rpcs, "fcntl", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(fcntl, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: fcntl(%d, %s, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 fd, fcntl_rpc2str(cmd),
                 arg, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(fcntl, out.retval);
}

pid_t
rpc_getpid(rcf_rpc_server *rpcs)
{
    tarpc_getpid_in  in;
    tarpc_getpid_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(getuid, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "getpid", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(getpid, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): getpid() -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(getpid, out.retval);
}


uid_t
rpc_getuid(rcf_rpc_server *rpcs)
{
    tarpc_getuid_in  in;
    tarpc_getuid_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(getuid, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "getuid", &in, &out);

    CHECK_RETVAL_VAR(getuid, out.uid, FALSE, 0);

    TAPI_RPC_LOG("RPC (%s,%s): getuid() -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 out.uid, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(getuid, out.uid);
}

int
rpc_setuid(rcf_rpc_server *rpcs,
           uid_t uid)
{
    tarpc_setuid_in  in;
    tarpc_setuid_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(setuid, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.uid = uid;

    rcf_rpc_call(rpcs, "setuid", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(setuid, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): setuid(%d) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 uid, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(setuid, out.retval);
}

uid_t
rpc_geteuid(rcf_rpc_server *rpcs)
{
    tarpc_geteuid_in  in;
    tarpc_geteuid_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(geteuid, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    rcf_rpc_call(rpcs, "geteuid", &in, &out);

    CHECK_RETVAL_VAR(geteuid, out.uid, FALSE, 0);

    TAPI_RPC_LOG("RPC (%s,%s): geteuid() -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 out.uid, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(geteuid, out.uid);
}

int
rpc_seteuid(rcf_rpc_server *rpcs,
            uid_t uid)
{
    tarpc_seteuid_in  in;
    tarpc_seteuid_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(seteuid, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.uid = uid;

    rcf_rpc_call(rpcs, "seteuid", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(seteuid, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): seteuid() -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 uid, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(seteuid, out.retval);
}

struct passwd *
rpc_getpwnam(rcf_rpc_server *rpcs, const char *name)
{
    tarpc_getpwnam_in  in;
    tarpc_getpwnam_out out;
    
    static struct passwd passwd = { NULL, NULL, 0, 0, NULL, NULL, NULL};
    
    struct passwd *res;
    
    free(passwd.pw_name);
    free(passwd.pw_passwd);
    free(passwd.pw_gecos);
    free(passwd.pw_dir);
    free(passwd.pw_shell);
    memset(&passwd, 0, sizeof(passwd));

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(getpwnam, NULL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    if ((in.name.name_val = strdup(name)) == NULL)
    {
        ERROR("Out of memory");
        RETVAL_PTR(getpwnam, NULL);
    }
    in.name.name_len = strlen(name) + 1;

    rcf_rpc_call(rpcs, "getpwnam", &in, &out);
                 
    CHECK_RETVAL_VAR(getpwnam, out.passwd.name.name_val,
                     FALSE, NULL);

    free(in.name.name_val);
    res = (!RPC_IS_CALL_OK(rpcs) || out.passwd.name.name_val == NULL) ?
          NULL : &passwd;

    if (res != NULL)
    {
        passwd.pw_name = out.passwd.name.name_val;
        passwd.pw_passwd = out.passwd.passwd.passwd_val;
        passwd.pw_uid = out.passwd.uid;
        passwd.pw_gid = out.passwd.gid;
        passwd.pw_gecos = out.passwd.gecos.gecos_val;
        passwd.pw_dir = out.passwd.dir.dir_val;
        passwd.pw_shell = out.passwd.shell.shell_val;
        memset(&out, 0, sizeof(out));
    }
    
    TAPI_RPC_LOG("RPC (%s,%s): getpwnam(%s) -> %p (%s)",
                 rpcs->ta, rpcs->name, name, res,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_PTR(getpwnam, res);
}

int
rpc_gettimeofday(rcf_rpc_server *rpcs,
                 tarpc_timeval *tv, tarpc_timezone *tz)
{
    tarpc_gettimeofday_in  in;
    tarpc_gettimeofday_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(gettimeofday, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    if (tv != NULL)
    {
        in.tv.tv_len = 1;
        in.tv.tv_val = tv;
    }
    if (tz != NULL)
    {
        in.tz.tz_len = 1;
        in.tz.tz_val = tz;
    }

    rcf_rpc_call(rpcs, "gettimeofday", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (tv != NULL && out.tv.tv_val != NULL)
        {
            tv->tv_sec  = out.tv.tv_val->tv_sec;
            tv->tv_usec = out.tv.tv_val->tv_usec;
        }
        if (tz != NULL && out.tz.tz_val != NULL)
        {
            tz->tz_minuteswest = out.tz.tz_val->tz_minuteswest;
            tz->tz_dsttime     = out.tz.tz_val->tz_dsttime;
        }
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(gettimeofday, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): gettimeofday(%p, %p) -> "
                 "%d (%s) tv=%s tz={%d,%d}",
                 rpcs->ta, rpcs->name, tv, tz,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 tarpc_timeval2str(tv),
                 (out.retval != 0 || tz == NULL) ? 0 :
                     (int)tz->tz_minuteswest, 
                 (out.retval != 0 || tz == NULL) ? 0 :
                     (int)tz->tz_dsttime);

    RETVAL_INT(gettimeofday, out.retval);
}

/**
 * Allocate a buffer of specified size in the TA address space.
 *
 * @param rpcs    RPC server handle
 * @param size    size of the buffer to be allocated
 *
 * @return   Allocated buffer identifier or RPC_NULL
 */
rpc_ptr
rpc_malloc(rcf_rpc_server *rpcs, size_t size)
{
    rcf_rpc_op          op;
    tarpc_malloc_in     in;
    tarpc_malloc_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(malloc, 0);
    }

    op = rpcs->op;

    in.size = size;

    rcf_rpc_call(rpcs, "malloc", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: malloc(%" TE_PRINTF_SIZE_T "u) -> "
                 "%u (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 size,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(malloc, (rpc_ptr)out.retval);
}

/**
 * Free the specified buffer in TA address space.
 *
 * @param rpcs   RPC server handle
 * @param buf    identifier of the buffer to be freed
 */
void
rpc_free(rcf_rpc_server *rpcs, rpc_ptr buf)
{
    rcf_rpc_op      op;
    tarpc_free_in   in;
    tarpc_free_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(free);
    }

    op = rpcs->op;

    in.buf = (tarpc_ptr)buf;

    rcf_rpc_call(rpcs, "free", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: free(%u) -> (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 buf, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(free);
}
