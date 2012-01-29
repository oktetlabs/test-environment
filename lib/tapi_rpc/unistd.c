/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of generic file API
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
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

    if (filedes != NULL)
    {
        in.filedes.filedes_len = 2;
        in.filedes.filedes_val = filedes;
    }

    rcf_rpc_call(rpcs, "pipe", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && filedes != NULL)
        memcpy(filedes, out.filedes.filedes_val, sizeof(int) * 2);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(pipe, out.retval);
    TAPI_RPC_LOG(rpcs, pipe, "%p", "%d (%d,%d)",
                 filedes, out.retval,
                 filedes != NULL ? filedes[0] : -1,
                 filedes != NULL ? filedes[1] : -1);
    RETVAL_INT(pipe, out.retval);
}

int
rpc_pipe2(rcf_rpc_server *rpcs,
         int filedes[2], int flags)
{
    tarpc_pipe2_in  in;
    tarpc_pipe2_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pipe2, -1);
    }

    if (filedes != NULL)
    {
        in.filedes.filedes_len = 2;
        in.filedes.filedes_val = filedes;
    }

    in.flags = fcntl_flags_rpc2h(flags);

    rcf_rpc_call(rpcs, "pipe2", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && filedes != NULL)
        memcpy(filedes, out.filedes.filedes_val, sizeof(int) * 2);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(pipe2, out.retval);
    TAPI_RPC_LOG(rpcs, pipe2, "%p", "%d (%d,%d,%d)",
                 filedes, out.retval,
                 filedes != NULL ? filedes[0] : -1,
                 filedes != NULL ? filedes[1] : -1,
                 fcntl_flags_rpc2str(flags));
    RETVAL_INT(pipe2, out.retval);
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
    TAPI_RPC_LOG(rpcs, socketpair, "%s, %s, %s, %p", "%d (%d,%d)",
                 domain_rpc2str(domain), socktype_rpc2str(type),
                 proto_rpc2str(protocol), sv, out.retval,
                 sv != NULL ? sv[0] : -1, sv != NULL ? sv[1] : -1);
    RETVAL_INT(socketpair, out.retval);
}

int
rpc_close(rcf_rpc_server *rpcs, int fd)
{
    tarpc_close_in  in;
    tarpc_close_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(close, -1);
    }

    in.fd = fd;

    rcf_rpc_call(rpcs, "close", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(close, out.retval);
    TAPI_RPC_LOG(rpcs, close, "%d", "%d", fd, out.retval);
    RETVAL_INT(close, out.retval);
}

ssize_t
rpc_write_at_offset(rcf_rpc_server *rpcs, int fd, char* buf,
                    size_t buflen, off_t offset)
{
    tarpc_write_at_offset_in  in;
    tarpc_write_at_offset_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(write_at_offset, -3);
    }

    in.fd = fd;
    in.buf.buf_len = buflen;
    in.buf.buf_val = (uint8_t *)buf;
    in.offset = offset;

    rcf_rpc_call(rpcs, "write_at_offset", &in, &out);

    TAPI_RPC_LOG(rpcs, write_at_offset,
                 "%d, %p, %d, %lld", "%lld, %d",
                 fd, buf, buflen, offset, out.offset, out.written);

    if (out.offset == (off_t)-1)  /* failed to repsition the file offset */
        RETVAL_INT(write_at_offset, -2);
    else
        RETVAL_INT(write_at_offset, out.written);
}

int
rpc_dup(rcf_rpc_server *rpcs,
        int oldfd)
{
    tarpc_dup_in    in;
    tarpc_dup_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(dup, -1);
    }

    in.oldfd = oldfd;

    rcf_rpc_call(rpcs, "dup", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(dup, out.fd);
    TAPI_RPC_LOG(rpcs, dup, "%d", "%d", oldfd, out.fd);
    RETVAL_INT(dup, out.fd);
}

int
rpc_dup2(rcf_rpc_server *rpcs,
         int oldfd, int newfd)
{
    tarpc_dup2_in   in;
    tarpc_dup2_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(dup2, -1);
    }

    in.oldfd = oldfd;
    in.newfd = newfd;

    rcf_rpc_call(rpcs, "dup", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(dup2, out.fd);
    TAPI_RPC_LOG(rpcs, dup2, "%d, %d", "%d", oldfd, newfd, out.fd);
    RETVAL_INT(dup2, out.fd);
}


int
rpc_read_gen(rcf_rpc_server *rpcs,
             int fd, void *buf, size_t count, size_t rbuflen)
{
    tarpc_read_in  in;
    tarpc_read_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

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
    TAPI_RPC_LOG(rpcs, read, "%d, %p[%u], %u", "%d",
                 fd, buf, rbuflen, count, out.retval);
    RETVAL_INT(read, out.retval);
}

int
rpc_write(rcf_rpc_server *rpcs,
          int fd, const void *buf, size_t count)
{
    tarpc_write_in  in;
    tarpc_write_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(write, -1);
    }

    in.fd = fd;
    in.len = count;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = count;
        in.buf.buf_val = (uint8_t *)buf;
    }

    rcf_rpc_call(rpcs, "write", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(write, out.retval);
    TAPI_RPC_LOG(rpcs, write, "%d, %p, %u", "%d",
                 fd, buf, count, out.retval);
    RETVAL_INT(write, out.retval);
}

int
rpc_write_and_close(rcf_rpc_server *rpcs,
                    int fd, const void *buf, size_t count)
{
    tarpc_write_and_close_in  in;
    tarpc_write_and_close_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(write_and_close, -1);
    }

    in.fd = fd;
    in.len = count;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = count;
        in.buf.buf_val = (uint8_t *)buf;
    }

    rcf_rpc_call(rpcs, "write_and_close", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(write_and_close, out.retval);
    TAPI_RPC_LOG(rpcs, write_and_close, "%d, %p, %u", "%d",
                 fd, buf, count, out.retval);
    RETVAL_INT(write_and_close, out.retval);
}

tarpc_ssize_t
rpc_readbuf_gen(rcf_rpc_server *rpcs,
                int fd, rpc_ptr buf, size_t buf_off, size_t count)
{
    tarpc_readbuf_in  in;
    tarpc_readbuf_out out;

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

    rcf_rpc_call(rpcs, "readbuf", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(readbuf, out.retval);
    TAPI_RPC_LOG(rpcs, readbuf, "%d, %u (off %u), %u", "%d",
                 fd, buf, buf_off, count, out.retval);
    RETVAL_INT(readbuf, out.retval);
}


tarpc_ssize_t
rpc_writebuf_gen(rcf_rpc_server *rpcs,
                 int fd, rpc_ptr buf, size_t buf_off, size_t count)
{
    tarpc_writebuf_in  in;
    tarpc_writebuf_out out;

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

    rcf_rpc_call(rpcs, "writebuf", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(writebuf, out.retval);
    TAPI_RPC_LOG(rpcs, writebuf, "%d, %u (off %u), %u", "%d",
                 fd, buf, buf_off, count, out.retval);
    RETVAL_INT(writebuf, out.retval);
}


tarpc_off_t
rpc_lseek(rcf_rpc_server *rpcs,
          int fd, tarpc_off_t pos, rpc_lseek_mode mode)
{
    tarpc_lseek_in  in;
    tarpc_lseek_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(lseek, (tarpc_off_t)-1);
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.fd   = fd;
    in.pos  = pos;
    in.mode = mode;

    rcf_rpc_call(rpcs, "lseek", &in, &out);

    TAPI_RPC_LOG(rpcs, lseek, "%d, %lld, %s", "%lld",
                 fd, pos, lseek_mode_rpc2str(mode), out.retval);
    RETVAL_INT(lseek, out.retval);
}

int
rpc_fsync(rcf_rpc_server *rpcs, int fd)
{
    tarpc_fsync_in  in;
    tarpc_fsync_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(fsync, -1);
    }

    in.fd = fd;

    rcf_rpc_call(rpcs, "fsync", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(fsync, out.retval);
    TAPI_RPC_LOG(rpcs, fsync, "%d", "%d", fd, out.retval);
    RETVAL_INT(fsync, out.retval);
}

int
rpc_readv_gen(rcf_rpc_server *rpcs,
              int fd, const struct rpc_iovec *iov,
              size_t iovcnt, size_t riovcnt)
{
    char            str_buf[1024] = { '\0', };
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

    if (iov != NULL)
    {
        in.vector.vector_len = riovcnt;
        in.vector.vector_val = iovec_arr;
        snprintf(str_buf + strlen(str_buf),
                 sizeof(str_buf) - strlen(str_buf), "{");
        for (i = 0; i < riovcnt; i++)
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
    TAPI_RPC_LOG(rpcs, readv, "%d, %s, %u", "%d",
                 fd, (*str_buf == '\0') ? "(nil)" : str_buf, iovcnt,
                 out.retval);
    RETVAL_INT(readv, out.retval);
}

int
rpc_writev(rcf_rpc_server *rpcs,
           int fd, const struct rpc_iovec *iov, size_t iovcnt)
{
    char             str_buf[1024] = { '\0', };
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

    if (iovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
        RETVAL_INT(writev, -1);
    }

    if (iov != NULL)
    {
        in.vector.vector_val = iovec_arr;
        in.vector.vector_len = iovcnt;
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
    }

    in.fd = fd;
    in.count = iovcnt;

    rcf_rpc_call(rpcs, "writev", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(writev, out.retval);
    TAPI_RPC_LOG(rpcs, writev, "%d, %s, %u", "%d",
                 fd, (*str_buf == '\0') ? "(nil)" : str_buf, iovcnt,
                 out.retval);
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

    rcf_rpc_call(rpcs, "fd_set_new", &in, &out);

    TAPI_RPC_LOG(rpcs, fd_set_new, "", "0x%x", (unsigned)out.retval);
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

    in.set = (tarpc_fd_set)set;

    rcf_rpc_call(rpcs, "fd_set_delete", &in, &out);

    TAPI_RPC_LOG(rpcs, fd_set_delete, "0x%x", "", (unsigned)set);
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

    in.set = (tarpc_fd_set)set;

    rcf_rpc_call(rpcs, "do_fd_zero", &in, &out);

    TAPI_RPC_LOG(rpcs, do_fd_zero, "0x%x", "", (unsigned)set);
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

    in.set = (tarpc_fd_set)set;
    in.fd = fd;

    rcf_rpc_call(rpcs, "do_fd_set", &in, &out);

    TAPI_RPC_LOG(rpcs, do_fd_set, "%d, 0x%x", "", fd, (unsigned)set);
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

    in.set = (tarpc_fd_set)set;
    in.fd = fd;

    rcf_rpc_call(rpcs, "do_fd_clr", &in, &out);

    TAPI_RPC_LOG(rpcs, do_fd_clr, "%d, 0x%x", "", fd, (unsigned)set);
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

    in.set = (tarpc_fd_set)set;
    in.fd = fd;

    rcf_rpc_call(rpcs, "do_fd_isset", &in, &out);

    CHECK_RETVAL_VAR(do_fd_isset, out.retval,
                     (out.retval != 0 && out.retval != 1), -1);
    TAPI_RPC_LOG(rpcs, do_fd_isset, "%d, 0x%x", "%d",
                 fd, (unsigned)set, out.retval);
    RETVAL_INT(do_fd_isset, out.retval);
}

int
rpc_select(rcf_rpc_server *rpcs,
           int n, rpc_fd_set_p readfds, rpc_fd_set_p writefds,
           rpc_fd_set_p exceptfds, struct tarpc_timeval *timeout)
{
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

    if ((timeout != NULL) && (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT))
    {
        rpcs->timeout = TE_SEC2MS(timeout->tv_sec +
                                  TAPI_RPC_TIMEOUT_EXTRA_SEC) +
                        TE_US2MS(timeout->tv_usec);
    }

    rcf_rpc_call(rpcs, "select", &in, &out);

    if (rpcs->last_op != RCF_RPC_CALL && timeout != NULL &&
        out.timeout.timeout_val != NULL && RPC_IS_CALL_OK(rpcs))
    {
        timeout->tv_sec = out.timeout.timeout_val[0].tv_sec;
        timeout->tv_usec = out.timeout.timeout_val[0].tv_usec;
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(select, out.retval);
    TAPI_RPC_LOG(rpcs, select, "%d, 0x%x, 0x%x, 0x%x, %s (%s)", "%d",
                 n, (unsigned)readfds, (unsigned)writefds,
                 (unsigned)exceptfds, tarpc_timeval2str(timeout_in_ptr),
                 tarpc_timeval2str(timeout), out.retval);
    RETVAL_INT(select, out.retval);
}

int
rpc_pselect(rcf_rpc_server *rpcs,
            int n, rpc_fd_set_p readfds, rpc_fd_set_p writefds,
            rpc_fd_set_p exceptfds, struct tarpc_timespec *timeout,
            const rpc_sigset_p sigmask)
{
    tarpc_pselect_in  in;
    tarpc_pselect_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pselect, -1);
    }

    in.n = n;
    in.readfds = (tarpc_fd_set)readfds;
    in.writefds = (tarpc_fd_set)writefds;
    in.exceptfds = (tarpc_fd_set)exceptfds;
    in.sigmask = (tarpc_sigset_t)sigmask;

    if (timeout != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.timeout.timeout_len = 1;
        in.timeout.timeout_val = timeout;
    }

    if ((timeout != NULL) && (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT))
    {
        rpcs->timeout = TE_SEC2MS(timeout->tv_sec +
                                  TAPI_RPC_TIMEOUT_EXTRA_SEC) +
                        TE_NS2MS(timeout->tv_nsec);
    }

    rcf_rpc_call(rpcs, "pselect", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(pselect, out.retval);
    TAPI_RPC_LOG(rpcs, pselect, "%d, 0x%x, 0x%x, 0x%x, %s, 0x%x", "%d",
                 n, (unsigned)readfds, (unsigned)writefds,
                 (unsigned)exceptfds, tarpc_timespec2str(timeout),
                 (unsigned)sigmask, out.retval);
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

/**
 * Convert array of epoll events to human-readable string.
 *
 * @param evts      Array of events
 * @param n_evts    Number of events
 * @param buf       Buffer to put string
 * @param buflen    Length of the buffer
 */
static void
epollevt2str(struct rpc_epoll_event *evts, unsigned int n_evts,
             char *buf, size_t buflen)
{
    unsigned int    i;
    int             rc;

    if (buflen == 0)
    {
        ERROR("Too small buffer for epoll events conversion");
        return;
    }

    if (evts == NULL)
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
        for (i = 0; i < n_evts; ++i)
        {
            /* TODO: Correct union field should be chosen. */
            rc = snprintf(buf, buflen, "{%d,%s}",
                          evts[i].data.fd,
                          epoll_event_rpc2str(evts[i].events));
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

    ERROR("Too small buffer for epoll events conversion");
}

int
rpc_poll_gen(rcf_rpc_server *rpcs,
             struct rpc_pollfd *ufds, unsigned int nfds,
             int timeout, unsigned int rnfds)
{
    tarpc_poll_in  in;
    tarpc_poll_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(poll, -1);
    }

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

    if ((timeout > 0) && (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT))
    {
        rpcs->timeout = TE_SEC2MS(TAPI_RPC_TIMEOUT_EXTRA_SEC) + timeout;
    }

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
    TAPI_RPC_LOG(rpcs, poll, "%p%s, %u, %d", "%d %s",
                 ufds, str_buf_1, nfds, timeout,
                 out.retval, str_buf_2);
    RETVAL_INT(poll, out.retval);
}

int
rpc_ppoll_gen(rcf_rpc_server *rpcs,
              struct rpc_pollfd *ufds, unsigned int nfds,
              struct tarpc_timespec *timeout, const rpc_sigset_p sigmask,
              unsigned int rnfds)
{
    tarpc_ppoll_in  in;
    tarpc_ppoll_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ppoll, -1);
    }

    /**
     * @attention It's assumed that rpc_pollfd is the same as tarpc_ppollfd.
     * It's OK, because both structures are independent from particular
     * Socket API and used on the same host.
     */
    in.ufds.ufds_len = rnfds;
    in.ufds.ufds_val = (struct tarpc_pollfd *)ufds;
    in.nfds = nfds;
    in.sigmask = (tarpc_sigset_t)sigmask;

    if (timeout != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.timeout.timeout_len = 1;
        in.timeout.timeout_val = timeout;
    }

    if ((timeout != NULL) && (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT))
    {
        rpcs->timeout = TE_SEC2MS(timeout->tv_sec +
                                  TAPI_RPC_TIMEOUT_EXTRA_SEC) +
                        TE_NS2MS(timeout->tv_nsec);
    }

    pollreq2str(ufds, rnfds, str_buf_1, sizeof(str_buf_1));

    rcf_rpc_call(rpcs, "ppoll", &in, &out);

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

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(ppoll, out.retval);
    TAPI_RPC_LOG(rpcs, ppoll, "%p%s, %u, %s, 0x%x", "%d %s",
                 ufds, str_buf_1, nfds, tarpc_timespec2str(timeout),
                 (unsigned)sigmask, out.retval, str_buf_2);
    RETVAL_INT(ppoll, out.retval);
}

int
rpc_epoll_create(rcf_rpc_server *rpcs, int size)
{
    tarpc_epoll_create_in  in;
    tarpc_epoll_create_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(epoll_create, -1);
    }

    in.size = size;

    rcf_rpc_call(rpcs, "epoll_create", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(epoll_create, out.retval);
    TAPI_RPC_LOG(rpcs, epoll_create, "%d", "%d", size, out.retval);
    RETVAL_INT(epoll_create, out.retval);
}

int
rpc_epoll_create1(rcf_rpc_server *rpcs, int flags)
{
    tarpc_epoll_create1_in  in;
    tarpc_epoll_create1_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(epoll_create1, -1);
    }

    in.flags = flags;

    rcf_rpc_call(rpcs, "epoll_create1", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(epoll_create1, out.retval);
    TAPI_RPC_LOG(rpcs, epoll_create1, "%s", "%d",
                 epoll_flags_rpc2str(flags), out.retval);
    RETVAL_INT(epoll_create1, out.retval);
}

int
rpc_epoll_ctl(rcf_rpc_server *rpcs, int epfd, int oper, int fd,
              struct rpc_epoll_event *event)
{
    tarpc_epoll_ctl_in  in;
    tarpc_epoll_ctl_out out;
    tarpc_epoll_event *evt;
    evt = malloc(sizeof(tarpc_epoll_event));

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(epoll_ctl, -1);
    }

    in.epfd = epfd;
    in.op = oper;
    in.fd = fd;
    if (event)
    {
        evt->events = event->events;
        evt->data.type = TARPC_ED_INT;
        evt->data.tarpc_epoll_data_u.fd = event->data.fd;
        in.event.event_len = 1;
        in.event.event_val = evt;
    }
    else
    {
        in.event.event_len = 0;
        in.event.event_val = NULL;
    }

    rcf_rpc_call(rpcs, "epoll_ctl", &in, &out);

    free(evt);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(epoll_ctl, out.retval);

    if (event)
        epollevt2str(event, 1,  str_buf_1, sizeof(str_buf_1));
    else
        *str_buf_1 = '\0';

    TAPI_RPC_LOG(rpcs, epoll_ctl, "%d, %s, %d, %p%s", "%d",
                 epfd, rpc_epoll_ctl_op2str(oper), fd, event,
                 str_buf_1, out.retval);
    RETVAL_INT(epoll_ctl, out.retval);
}

int
rpc_epoll_wait_gen(rcf_rpc_server *rpcs, int epfd,
                   struct rpc_epoll_event *events, int rmaxev,
                   int maxevents, int timeout)
{
    tarpc_epoll_wait_in  in;
    tarpc_epoll_wait_out out;
    int i;
    tarpc_epoll_event *evts;

    evts = calloc(rmaxev, sizeof(tarpc_epoll_event));

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(epoll_wait, -1);
    }

    in.epfd = epfd;
    in.timeout = timeout;
    in.maxevents = maxevents;
    for (i = 0; i < rmaxev; i++)
    {
        evts[i].events = events[i].events;
        evts[i].data.type = TARPC_ED_INT;
        evts[i].data.tarpc_epoll_data_u.fd = events[i].data.fd;
    }
    in.events.events_len = rmaxev;
    in.events.events_val = (struct tarpc_epoll_event *)evts;

    if ((timeout > 0) && (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT))
    {
        rpcs->timeout = TE_SEC2MS(TAPI_RPC_TIMEOUT_EXTRA_SEC) + timeout;
    }

    rcf_rpc_call(rpcs, "epoll_wait", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (events != NULL && out.events.events_val != NULL)
        {
            for (i = 0; i < out.retval; i++)
            {
                events[i].events = out.events.events_val[i].events;
                events[i].data.fd =
                    out.events.events_val[i].data.tarpc_epoll_data_u.fd;
            }
        }
        epollevt2str(events, MAX(out.retval, 0),
                     str_buf_1, sizeof(str_buf_1));
    }
    else
    {
        *str_buf_1 = '\0';
    }
    free(evts);
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(epoll_wait, out.retval);
    TAPI_RPC_LOG(rpcs, epoll_wait, "%d, %p, %d, %d", "%d %s",
                 epfd, events, maxevents, timeout, out.retval, str_buf_1);
    RETVAL_INT(epoll_wait, out.retval);
}


int
rpc_epoll_pwait_gen(rcf_rpc_server *rpcs, int epfd,
                    struct rpc_epoll_event *events, int rmaxev,
                    int maxevents, int timeout, const rpc_sigset_p sigmask)
{
    tarpc_epoll_pwait_in  in;
    tarpc_epoll_pwait_out out;
    int i;
    tarpc_epoll_event *evts;

    evts = calloc(rmaxev, sizeof(tarpc_epoll_event));

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(epoll_pwait, -1);
    }

    in.epfd = epfd;
    in.timeout = timeout;
    in.maxevents = maxevents;
    in.sigmask = (tarpc_sigset_t)sigmask;
    for (i = 0; i < rmaxev; i++)
    {
        evts[i].events = events[i].events;
        evts[i].data.type = TARPC_ED_INT;
        evts[i].data.tarpc_epoll_data_u.fd = events[i].data.fd;
    }
    in.events.events_len = rmaxev;
    in.events.events_val = (struct tarpc_epoll_event *)evts;

    if ((timeout > 0) && (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT))
    {
        rpcs->timeout = TE_SEC2MS(TAPI_RPC_TIMEOUT_EXTRA_SEC) + timeout;
    }

    rcf_rpc_call(rpcs, "epoll_pwait", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (events != NULL && out.events.events_val != NULL)
        {
            for (i = 0; i < out.retval; i++)
            {
                events[i].events = out.events.events_val[i].events;
                events[i].data.fd =
                    out.events.events_val[i].data.tarpc_epoll_data_u.fd;
            }
        }
        epollevt2str(events, MAX(out.retval, 0),
                     str_buf_1, sizeof(str_buf_1));
    }
    else
    {
        *str_buf_1 = '\0';
    }
    free(evts);
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(epoll_pwait, out.retval);
    TAPI_RPC_LOG(rpcs, epoll_pwait, "%d, %p, %d, %d 0x%x", "%d %s",
                 epfd, events, maxevents, timeout, (unsigned)sigmask,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)), str_buf_1);
    RETVAL_INT(epoll_pwait, out.retval);
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
    TAPI_RPC_LOG(rpcs, open, "%s, %s, %s", "%d",
                 path, fcntl_flags_rpc2str(flags),
                 file_mode_flags_rpc2str(mode), out.fd);
    RETVAL_INT(open, out.fd);
}

int
rpc_open64(rcf_rpc_server *rpcs,
           const char *path, rpc_fcntl_flags flags,
           rpc_file_mode_flags mode)
{
    tarpc_open64_in  in;
    tarpc_open64_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(open64, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)strdup(path); /* FIXME */
    }
    in.flags = flags;
    in.mode  = mode;

    rcf_rpc_call(rpcs, "open64", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(open64, out.fd);
    TAPI_RPC_LOG(rpcs, open64, "%s, %s, %s", "%d",
                 path, fcntl_flags_rpc2str(flags),
                 file_mode_flags_rpc2str(mode), out.fd);
    RETVAL_INT(open64, out.fd);
}


int
rpc_fcntl(rcf_rpc_server *rpcs, int fd,
            int cmd, int arg)
{
    tarpc_fcntl_in  in;
    tarpc_fcntl_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(fcntl, -1);
    }

    in.fd = fd;
    in.cmd = cmd;
    in.arg = arg;

    rcf_rpc_call(rpcs, "fcntl", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(fcntl, out.retval);
    TAPI_RPC_LOG(rpcs, fcntl, "%d, %s, %d", "%d",
                 fd, fcntl_rpc2str(cmd), arg, out.retval);
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
        RETVAL_INT(getpid, -1);
    }

    rcf_rpc_call(rpcs, "getpid", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(getpid, out.retval);
    TAPI_RPC_LOG(rpcs, getpid, "", "%d", out.retval);
    RETVAL_INT(getpid, out.retval);
}

tarpc_pthread_t
rpc_pthread_self(rcf_rpc_server *rpcs)
{
    tarpc_pthread_self_in  in;
    tarpc_pthread_self_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pthread_self, -1);
    }

    rcf_rpc_call(rpcs, "pthread_self", &in, &out);

    TAPI_RPC_LOG(rpcs, pthread_self, "", "%llu",
                 (unsigned long long int) out.retval);
    return out.retval;
}

tarpc_uid_t
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

    rcf_rpc_call(rpcs, "getuid", &in, &out);

    CHECK_RETVAL_VAR(getuid, out.uid, FALSE, (tarpc_uid_t)-1);
    TAPI_RPC_LOG(rpcs, getuid, "", "%d", out.uid);
    RETVAL_INT(getuid, out.uid);
}

int
rpc_setuid(rcf_rpc_server *rpcs,
           tarpc_uid_t uid)
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

    in.uid = uid;

    rcf_rpc_call(rpcs, "setuid", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(setuid, out.retval);
    TAPI_RPC_LOG(rpcs, setuid, "%d", "%d", uid, out.retval);
    RETVAL_INT(setuid, out.retval);
}

tarpc_uid_t
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

    rcf_rpc_call(rpcs, "geteuid", &in, &out);

    CHECK_RETVAL_VAR(geteuid, out.uid, FALSE, (tarpc_uid_t)-1);
    TAPI_RPC_LOG(rpcs, geteuid, "", "%d", out.uid);
    RETVAL_INT(geteuid, out.uid);
}

int
rpc_seteuid(rcf_rpc_server *rpcs,
            tarpc_uid_t uid)
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

    in.uid = uid;

    rcf_rpc_call(rpcs, "seteuid", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(seteuid, out.retval);
    TAPI_RPC_LOG(rpcs, seteuid, "%d", "%d", uid, out.retval);
    RETVAL_INT(seteuid, out.retval);
}

int
rpc_access(rcf_rpc_server *rpcs,
           const char *path,
           int mode)
{
    tarpc_access_in  in;
    tarpc_access_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(seteuid, -1);
    }
    in.mode  = mode;
    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)strdup(path); /* FIXME */
    }

    rcf_rpc_call(rpcs, "access", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(access, out.retval);
    TAPI_RPC_LOG(rpcs, access, "%s, %s", "%d",
                 path, access_mode_flags_rpc2str(mode), out.retval);
    RETVAL_INT(access, out.retval);
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

    TAPI_RPC_LOG(rpcs, getpwnam, "%s", "%p", name, res);
    RETVAL_PTR(getpwnam, res);
}

int
rpc_uname(rcf_rpc_server *rpcs, struct utsname *buf)
{
    tarpc_uname_in  in;
    tarpc_uname_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(uname, -1);
    }

    rcf_rpc_call(rpcs, "uname", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(uname, out.retval);

    if (RPC_IS_CALL_OK(rpcs))
    {
        memset(buf, 0, sizeof(*buf));
#define GET_STR(_dst, _field)                               \
        do {                                                \
            strncpy(buf->_dst, out.buf._field._field##_val, \
                    sizeof(buf->_dst));                     \
            free(out.buf._field._field##_val);              \
        } while(0)

        GET_STR(sysname, sysname);
        GET_STR(nodename, nodename);
        GET_STR(release, release);
        GET_STR(version, osversion);
        GET_STR(machine, machine);
#undef GET_STR
        memset(&out, 0, sizeof(out));
    }

    TAPI_RPC_LOG(rpcs, uname, "", "%d", out.retval);
    RETVAL_INT(uname, out.retval);
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
    TAPI_RPC_LOG(rpcs, gettimeofday, "%p %p", "%d tv=%s tz={%d,%d}",
                 tv, tz, out.retval,
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
    tarpc_malloc_in     in;
    tarpc_malloc_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(malloc, 0);
    }

    in.size = size;

    rcf_rpc_call(rpcs, "malloc", &in, &out);

    TAPI_RPC_LOG(rpcs, malloc, "%" TE_PRINTF_SIZE_T "u", "%u",
                 size, out.retval);
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
    tarpc_free_in   in;
    tarpc_free_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(free);
    }

    in.buf = (tarpc_ptr)buf;

    rcf_rpc_call(rpcs, "free", &in, &out);

    TAPI_RPC_LOG(rpcs, free, "%u", "", buf);
    RETVAL_VOID(free);
}

/**
 * Allocate a buffer of specified size in the TA address space
 * aligned at a specified boundary.
 *
 * @param rpcs          RPC server handle
 * @param alignment     buffer alignment
 * @param size          size of the buffer to be allocated
 *
 * @return   Allocated buffer identifier or RPC_NULL
 */
rpc_ptr
rpc_memalign(rcf_rpc_server *rpcs, size_t alignment, size_t size)
{
    tarpc_memalign_in     in;
    tarpc_memalign_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(malloc, 0);
    }

    in.alignment = alignment;
    in.size      = size;

    rcf_rpc_call(rpcs, "memalign", &in, &out);

    TAPI_RPC_LOG(rpcs, memalign, "%" TE_PRINTF_SIZE_T "u "
                 "%" TE_PRINTF_SIZE_T "u", "%u",
                 alignment, size, out.retval);
    RETVAL_RPC_PTR(memalign, (rpc_ptr)out.retval);
}

int
rpc_setrlimit(rcf_rpc_server *rpcs,
              int resource, const tarpc_rlimit *rlim)
{
    tarpc_setrlimit_in  in;
    tarpc_setrlimit_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(setrlimit, -1);
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.resource = resource;
    if (rlim != NULL)
    {
        in.rlim.rlim_len = 1;
        in.rlim.rlim_val = (tarpc_rlimit *)rlim;
    }

    rcf_rpc_call(rpcs, "setrlimit", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(setrlimit, out.retval);
    TAPI_RPC_LOG(rpcs, setrlimit, "%s, %p{%u, %u}", "%d",
                 rlimit_resource_rpc2str(resource), rlim,
                 rlim == NULL ? 0 : rlim->rlim_cur,
                 rlim == NULL ? 0 : rlim->rlim_max,
                 out.retval);
    RETVAL_INT(setrlimit, out.retval);
}

int
rpc_getrlimit(rcf_rpc_server *rpcs,
              int resource, tarpc_rlimit *rlim)
{
    tarpc_getrlimit_in  in;
    tarpc_getrlimit_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(getrlimit, -1);
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.resource = resource;
    if (rlim != NULL)
    {
        in.rlim.rlim_len = 1;
        in.rlim.rlim_val = rlim;
    }

    rcf_rpc_call(rpcs, "getrlimit", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rlim != NULL && out.rlim.rlim_val != NULL)
    {
        *rlim = *out.rlim.rlim_val;
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(getrlimit, out.retval);
    TAPI_RPC_LOG(rpcs, getrlimit, "%s, %p", "%d {%u, %u}",
                 rlimit_resource_rpc2str(resource), rlim, out.retval,
                 rlim == NULL ? 0 : rlim->rlim_cur,
                 rlim == NULL ? 0 : rlim->rlim_max);
    RETVAL_INT(getrlimit, out.retval);
}

int
rpc_fstat(rcf_rpc_server *rpcs,
          int fd,
          rpc_stat *buf)
{
    tarpc_te_fstat_in  in;
    tarpc_te_fstat_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(te_fstat, -1);
    }

    if (buf == NULL)
    {
        ERROR("%s(): stat buffer pointer", __FUNCTION__);
        RETVAL_INT(te_fstat, -1);
    }

    in.fd = fd;

    rcf_rpc_call(rpcs, "te_fstat", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
        memcpy(buf, &out.buf, sizeof(out.buf));

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(fstat, out.retval);
    TAPI_RPC_LOG(rpcs, fstat, "%d", "%d", fd, out.retval);
    RETVAL_INT(te_fstat, out.retval);
}

int
rpc_fstat64(rcf_rpc_server *rpcs,
            int fd,
            rpc_stat *buf)
{
    tarpc_te_fstat64_in  in;
    tarpc_te_fstat64_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(te_fstat64, -1);
    }

    if (buf == NULL)
    {
        ERROR("%s(): stat buffer pointer", __FUNCTION__);
        RETVAL_INT(te_fstat64, -1);
    }
    
    in.fd = fd;

    rcf_rpc_call(rpcs, "te_fstat64", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
        memcpy(buf, &out.buf, sizeof(out.buf));

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(fstat, out.retval);
    TAPI_RPC_LOG(rpcs, fstat64, "%d", "%d", fd, out.retval);
    RETVAL_INT(te_fstat64, out.retval);
}

