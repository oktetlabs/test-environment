/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of generic file API
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
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
#include "tapi_rpc_time.h"
#include "tapi_test.h"
#include "tapi_mem.h"
#include "te_printf.h"
#include "te_bufs.h"
#include "te_dbuf.h"
#include "te_str.h"
#include "te_rpc_pthread.h"


/** @name String buffers for snprintf() operations */
static char str_buf_1[8192];
static char str_buf_2[8192];
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
    if ((int)fcntl_flags_h2rpc(in.flags) != flags)
        in.flags = in.flags | (~fcntl_flags_h2rpc(in.flags) & flags);

    rcf_rpc_call(rpcs, "pipe2", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && filedes != NULL &&
        rpcs->last_op != RCF_RPC_CALL)
        memcpy(filedes, out.filedes.filedes_val, sizeof(int) * 2);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(pipe2, out.retval);
    TAPI_RPC_LOG(rpcs, pipe2, "%p, %s", "%d (%d,%d)",
                 filedes, fcntl_flags_rpc2str(flags), out.retval,
                 filedes != NULL ? filedes[0] : -1,
                 filedes != NULL ? filedes[1] : -1);
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

    rcf_rpc_call(rpcs, "dup2", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(dup2, out.fd);
    TAPI_RPC_LOG(rpcs, dup2, "%d, %d", "%d", oldfd, newfd, out.fd);
    RETVAL_INT(dup2, out.fd);
}

int
rpc_dup3(rcf_rpc_server *rpcs,
         int oldfd, int newfd, int flags)
{
    tarpc_dup3_in   in;
    tarpc_dup3_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(dup3, -1);
    }

    in.flags = fcntl_flags_rpc2h(flags);
    in.oldfd = oldfd;
    in.newfd = newfd;

    rcf_rpc_call(rpcs, "dup3", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(dup3, out.fd);
    TAPI_RPC_LOG(rpcs, dup3, "%d, %d, %s", "%d", oldfd, newfd,
                 fcntl_flags_rpc2str(flags), out.fd);
    RETVAL_INT(dup3, out.fd);
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

    in.chk_func = TEST_BEHAVIOUR(use_chk_funcs);

    if (buf != NULL && count > rbuflen && !(in.chk_func))
    {
        ERROR("%s(): count > rbuflen and __read_chk() is not tested",
              __FUNCTION__);
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
    TAPI_RPC_LOG(rpcs, read, "%d, %p[%u], %u, chk_func=%s", "%d",
                 fd, buf, rbuflen, count,
                 (in.chk_func ? "TRUE" : "FALSE"), out.retval);
    RETVAL_INT(read, out.retval);
}

int
rpc_pread(rcf_rpc_server *rpcs, int fd, void* buf, size_t count,
          tarpc_off_t offset)
{
    tarpc_pread_in  in;
    tarpc_pread_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    in.chk_func = TEST_BEHAVIOUR(use_chk_funcs);

    in.fd = fd;
    in.len = count;
    in.offset = offset;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = count;
        in.buf.buf_val = buf;
    }

    rcf_rpc_call(rpcs, "pread", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(pread, out.retval);
    TAPI_RPC_LOG(rpcs, pread, "%d, %p, %u, %jd, chk_func=%s", "%d",
                 fd, buf, count, (intmax_t)offset,
                 (in.chk_func ? "TRUE" : "FALSE"), out.retval);
    RETVAL_INT(pread, out.retval);
}

int
rpc_read_via_splice(rcf_rpc_server *rpcs,
                    int fd, void *buf, size_t count)
{
    tarpc_read_via_splice_in  in;
    tarpc_read_via_splice_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    in.fd = fd;
    in.len = count;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = count;
        in.buf.buf_val = buf;
    }
    rcf_rpc_call(rpcs, "read_via_splice", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(read_via_splice, out.retval);
    TAPI_RPC_LOG(rpcs, read_via_splice, "%d, %p, %u", "%d",
                 fd, buf, count, out.retval);
    RETVAL_INT(read_via_splice, out.retval);
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
rpc_write_via_splice(rcf_rpc_server *rpcs,
                     int fd, const void *buf, size_t count)
{
    tarpc_write_via_splice_in  in;
    tarpc_write_via_splice_out out;

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

    rcf_rpc_call(rpcs, "write_via_splice", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(write_via_splice, out.retval);
    TAPI_RPC_LOG(rpcs, write_via_splice, "%d, %p, %u", "%d",
                 fd, buf, count, out.retval);
    RETVAL_INT(write_via_splice, out.retval);
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

int
rpc_pwrite(rcf_rpc_server *rpcs, int fd, const void *buf, size_t count,
           off_t offset)
{
    tarpc_pwrite_in  in;
    tarpc_pwrite_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pwrite, -1);
    }

    in.fd = fd;
    in.len = count;
    in.offset = offset;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = count;
        in.buf.buf_val = (uint8_t *)buf;
    }

    rcf_rpc_call(rpcs, "pwrite", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(pwrite, out.retval);
    TAPI_RPC_LOG(rpcs, pwrite, "%d, %p, %u, %jd", "%d",
                 fd, buf, count, (intmax_t)offset, out.retval);
    RETVAL_INT(pwrite, out.retval);
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
    RETVAL_INT64(lseek, out.retval);
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
    te_string str = TE_STRING_INIT_STATIC(1024);

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

        te_iovec_rpc2tarpc(iov, iovec_arr, riovcnt);
    }
    te_iovec_rpc2str_append(&str, iov, riovcnt);

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
                 fd, str.ptr, iovcnt, out.retval);
    RETVAL_INT(readv, out.retval);
}

int
rpc_preadv(rcf_rpc_server *rpcs, int fd, const struct rpc_iovec *iov,
           size_t iovcnt, off_t offset)
{
    te_string str = TE_STRING_INIT_STATIC(1024);

    tarpc_preadv_in  in;
    tarpc_preadv_out out;

    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    size_t i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(preadv, -1);
    }

    if (iovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(preadv, -1);
    }

    in.fd = fd;
    in.count = iovcnt;
    in.offset = offset;

    if (iov != NULL)
    {
        in.vector.vector_len = iovcnt;
        in.vector.vector_val = iovec_arr;

        te_iovec_rpc2tarpc(iov, iovec_arr, iovcnt);
    }
    te_iovec_rpc2str_append(&str, iov, iovcnt);

    rcf_rpc_call(rpcs, "preadv", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) &&
        iov != NULL && out.vector.vector_val != NULL)
    {
        for (i = 0; i < iovcnt; i++)
        {
            size_t in_iov_len  = iov[i].iov_len;
            size_t out_iov_len = out.vector.vector_val[i].iov_len;

            if (in_iov_len != out_iov_len)
            {
                ERROR("%s: in_iov_len(%zu) != out_iov_len(%zu)",
                      __FUNCTION__, in_iov_len, out_iov_len);
            }

            if ((iov[i].iov_base != NULL) &&
                (out.vector.vector_val[i].iov_base.iov_base_val != NULL))
            {
                memcpy(iov[i].iov_base,
                       out.vector.vector_val[i].iov_base.iov_base_val,
                       iov[i].iov_rlen);
            }
        }
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(preadv, out.retval);
    TAPI_RPC_LOG(rpcs, preadv, "%d, %s, %zu, %jd", "%d",
                 fd, str.ptr, iovcnt, (intmax_t)offset, out.retval);
    RETVAL_INT(preadv, out.retval);
}

int
rpc_writev(rcf_rpc_server *rpcs,
           int fd, const struct rpc_iovec *iov, size_t iovcnt)
{
    te_string str = TE_STRING_INIT_STATIC(1024);

    tarpc_writev_in  in;
    tarpc_writev_out out;

    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

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

        te_iovec_rpc2tarpc(iov, iovec_arr, iovcnt);
    }
    te_iovec_rpc2str_append(&str, iov, iovcnt);

    in.fd = fd;
    in.count = iovcnt;

    rcf_rpc_call(rpcs, "writev", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(writev, out.retval);
    TAPI_RPC_LOG(rpcs, writev, "%d, %s, %u", "%d",
                 fd, str.ptr, iovcnt, out.retval);
    RETVAL_INT(writev, out.retval);
}

int
rpc_pwritev(rcf_rpc_server *rpcs, int fd, const struct rpc_iovec *iov,
            size_t iovcnt, tarpc_off_t offset)
{
    te_string str = TE_STRING_INIT_STATIC(1024);

    tarpc_pwritev_in  in;
    tarpc_pwritev_out out;

    struct tarpc_iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pwritev, -1);
    }

    if (iovcnt > RCF_RPC_MAX_IOVEC)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(pwritev, -1);
    }

    if (iov != NULL)
    {
        in.vector.vector_val = iovec_arr;
        in.vector.vector_len = iovcnt;

        te_iovec_rpc2tarpc(iov, iovec_arr, iovcnt);
    }
    te_iovec_rpc2str_append(&str, iov, iovcnt);

    in.fd = fd;
    in.count = iovcnt;
    in.offset = offset;

    rcf_rpc_call(rpcs, "pwritev", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(pwritev, out.retval);
    TAPI_RPC_LOG(rpcs, pwritev, "%d, %s, %zu, %jd", "%d",
                 fd, str.ptr, iovcnt, (intmax_t)offset, out.retval);
    RETVAL_INT(pwritev, out.retval);
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

    if (rpcs->op == RCF_RPC_WAIT)
        TAPI_RPC_LOG(rpcs, fd_set_new, "", "0x%x", out.retval);
    else
    {
        if (TAPI_RPC_NAMESPACE_CHECK(rpcs, out.retval, RPC_TYPE_NS_FD_SET))
            RETVAL_RPC_PTR(fd_set_new, RPC_NULL);

        TAPI_RPC_LOG(rpcs, fd_set_new, "", RPC_PTR_FMT,
                     RPC_PTR_VAL(out.retval));
    }
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

    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, set, RPC_TYPE_NS_FD_SET))
        RETVAL_VOID(fd_set_delete);
    in.set = (tarpc_fd_set)set;

    rcf_rpc_call(rpcs, "fd_set_delete", &in, &out);

    TAPI_RPC_LOG(rpcs, fd_set_delete, RPC_PTR_FMT, "",
                 RPC_PTR_VAL(set));
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

    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, set, RPC_TYPE_NS_FD_SET))
        RETVAL_VOID(do_fd_zero);
    in.set = (tarpc_fd_set)set;

    rcf_rpc_call(rpcs, "do_fd_zero", &in, &out);

    TAPI_RPC_LOG(rpcs, do_fd_zero, RPC_PTR_FMT, "",
                 RPC_PTR_VAL(set));
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

    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, set, RPC_TYPE_NS_FD_SET))
        RETVAL_VOID(do_fd_set);
    in.set = (tarpc_fd_set)set;
    in.fd = fd;

    rcf_rpc_call(rpcs, "do_fd_set", &in, &out);

    TAPI_RPC_LOG(rpcs, do_fd_set, "%d, " RPC_PTR_FMT, "",
                 fd, RPC_PTR_VAL(set));
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

    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, set, RPC_TYPE_NS_FD_SET))
        RETVAL_VOID(do_fd_clr);
    in.set = (tarpc_fd_set)set;
    in.fd = fd;

    rcf_rpc_call(rpcs, "do_fd_clr", &in, &out);

    TAPI_RPC_LOG(rpcs, do_fd_clr, "%d, " RPC_PTR_FMT, "",
                 fd, RPC_PTR_VAL(set));
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
    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, set, RPC_TYPE_NS_FD_SET))
        RETVAL_INT(do_fd_isset, 0);
    in.set = (tarpc_fd_set)set;
    in.fd = fd;

    rcf_rpc_call(rpcs, "do_fd_isset", &in, &out);

    CHECK_RETVAL_VAR(do_fd_isset, out.retval,
                     (out.retval != 0 && out.retval != 1), -1);
    TAPI_RPC_LOG(rpcs, do_fd_isset, "%d, " RPC_PTR_FMT, "%d",
                 fd, RPC_PTR_VAL(set), out.retval);
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

    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, readfds, RPC_TYPE_NS_FD_SET))
        RETVAL_INT(select, -1);
    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, writefds, RPC_TYPE_NS_FD_SET))
        RETVAL_INT(select, -1);
    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, exceptfds, RPC_TYPE_NS_FD_SET))
        RETVAL_INT(select, -1);

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
    TAPI_RPC_LOG(rpcs, select, "%d, " RPC_PTR_FMT ", " RPC_PTR_FMT ", "
                 RPC_PTR_FMT ", %s (%s)", "%d",
                 n, RPC_PTR_VAL(readfds), RPC_PTR_VAL(writefds),
                 RPC_PTR_VAL(exceptfds), tarpc_timeval2str(timeout_in_ptr),
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

    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, readfds, RPC_TYPE_NS_FD_SET))
        RETVAL_INT(pselect, -1);
    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, writefds, RPC_TYPE_NS_FD_SET))
        RETVAL_INT(pselect, -1);
    if (TAPI_RPC_NAMESPACE_CHECK(rpcs, exceptfds, RPC_TYPE_NS_FD_SET))
        RETVAL_INT(pselect, -1);

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

    if (rpcs->last_op != RCF_RPC_CALL && timeout != NULL &&
        out.timeout.timeout_val != NULL && RPC_IS_CALL_OK(rpcs))
    {
        timeout->tv_sec = out.timeout.timeout_val[0].tv_sec;
        timeout->tv_nsec = out.timeout.timeout_val[0].tv_nsec;
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(pselect, out.retval);
    TAPI_RPC_LOG(rpcs, pselect, "%d, " RPC_PTR_FMT ", " RPC_PTR_FMT ", "
                 RPC_PTR_FMT ", %s, 0x%x", "%d",
                 n, RPC_PTR_VAL(readfds), RPC_PTR_VAL(writefds),
                 RPC_PTR_VAL(exceptfds), tarpc_timespec2str(timeout),
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
            const char* returned_events = poll_event_rpc2str(ufds[i].revents);
            te_bool need_mark = (ufds[i].revents != 0);

            rc = snprintf(buf, buflen, "{%d,%s,%s%s}",
                          ufds[i].fd,
                          poll_event_rpc2str(ufds[i].events),
                          returned_events,
                          need_mark ? " (RETURNED)" : "");
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

    in.chk_func = TEST_BEHAVIOUR(use_chk_funcs);

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
    TAPI_RPC_LOG(rpcs, poll, "%p%s, %u, %d, chk_func=%s", "%d",
                 ufds, str_buf_2, nfds, timeout,
                 (in.chk_func ? "TRUE" : "FALSE"),
                 out.retval);
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

    in.chk_func = TEST_BEHAVIOUR(use_chk_funcs);

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

    rcf_rpc_call(rpcs, "ppoll", &in, &out);

    if (rpcs->last_op != RCF_RPC_CALL && timeout != NULL &&
        out.timeout.timeout_val != NULL && RPC_IS_CALL_OK(rpcs))
    {
        timeout->tv_sec = out.timeout.timeout_val[0].tv_sec;
        timeout->tv_nsec = out.timeout.timeout_val[0].tv_nsec;
    }

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
    TAPI_RPC_LOG(rpcs, ppoll, "%p%s, %u, %s, 0x%x, chk_func=%s", "%d",
                 ufds, str_buf_2, nfds, tarpc_timespec2str(timeout),
                 (unsigned)sigmask, (in.chk_func ? "TRUE" : "FALSE"),
                 out.retval);
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
    TAPI_RPC_LOG(rpcs, epoll_pwait, "%d, %p, %d, %d, 0x%x", "%d %s",
                 epfd, events, maxevents, timeout, (unsigned)sigmask,
                 out.retval, str_buf_1);
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

    if (path != NULL)
        free(in.path.path_val);

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

    if (path != NULL)
        free(in.path.path_val);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(open64, out.fd);
    TAPI_RPC_LOG(rpcs, open64, "%s, %s, %s", "%d",
                 path, fcntl_flags_rpc2str(flags),
                 file_mode_flags_rpc2str(mode), out.fd);
    RETVAL_INT(open64, out.fd);
}


int
rpc_fcntl(rcf_rpc_server *rpcs, int fd,
            int cmd, ...)
{
    tarpc_fcntl_in  in;
    tarpc_fcntl_out out;

    fcntl_request   req;
    va_list         ap;
    int            *arg;
    char            req_val[128];

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&req_val, 0, sizeof(req_val));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(fcntl, -1);
    }

    va_start(ap, cmd);
    arg = va_arg(ap, int *);
    va_end(ap);

    in.fd = fd;
    in.cmd = cmd;

    if (cmd != RPC_F_GETOWN_EX || cmd != RPC_F_SETOWN_EX || arg != NULL)
    {
        memset(&req, 0, sizeof(req));
        in.arg.arg_val = &req;
        in.arg.arg_len = 1;
    }

    switch (cmd)
    {
        case RPC_F_GETOWN_EX:
        case RPC_F_SETOWN_EX:
            if (arg != NULL)
            {
                in.arg.arg_val[0].type = FCNTL_F_OWNER_EX;
                in.arg.arg_val[0].fcntl_request_u.req_f_owner_ex.type =
                    ((struct rpc_f_owner_ex *)arg)->type;
                in.arg.arg_val[0].fcntl_request_u.req_f_owner_ex.pid =
                    ((struct rpc_f_owner_ex *)arg)->pid;
            }
            break;
        default:
            in.arg.arg_val[0].type = FCNTL_INT;
            in.arg.arg_val[0].fcntl_request_u.req_int = (long)arg;
            break;
    }

    rcf_rpc_call(rpcs, "fcntl", &in, &out);

    if (out.arg.arg_val != NULL)
    {
        assert((cmd != RPC_F_GETOWN_EX && cmd != RPC_F_SETOWN_EX) ||
               arg != NULL);

        switch (in.arg.arg_val[0].type)
        {
            case FCNTL_F_OWNER_EX:
                ((struct rpc_f_owner_ex *)arg)->type =
                    out.arg.arg_val[0].fcntl_request_u.req_f_owner_ex.type;
                ((struct rpc_f_owner_ex *)arg)->pid =
                    out.arg.arg_val[0].fcntl_request_u.req_f_owner_ex.pid;

                snprintf(req_val, sizeof(req_val), ", {%d, %d}",
                         ((struct rpc_f_owner_ex *)arg)->type,
                         ((struct rpc_f_owner_ex *)arg)->pid);
                break;
            default:
                if (cmd != RPC_F_GETFD && cmd != RPC_F_GETFL &&
                    cmd != RPC_F_GETSIG && cmd != RPC_F_GETPIPE_SZ)
                    snprintf(req_val, sizeof(req_val), ", %ld", (long)arg);
                break;
        }
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(fcntl, out.retval);
    TAPI_RPC_LOG(rpcs, fcntl, "%d, %s%s", "%d",
                 fd, fcntl_rpc2str(cmd), req_val, out.retval);
    RETVAL_INT(fcntl, out.retval);
}

void
rpc_exit(rcf_rpc_server *rpcs, int status)
{
    tarpc_exit_in  in;
    tarpc_exit_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(exit);
    }

    in.status = status;

    rcf_rpc_call(rpcs, "exit", &in, &out);

    TAPI_RPC_LOG(rpcs, exit, "%d", "(void)", status);

    if (TE_RC_GET_ERROR(RPC_ERRNO(rpcs)) == TE_ERPCDEAD)
    {
        RING("RPC server %s is dead as a result of exit() call",
             rpcs->name);
    }
    else
        RETVAL_VOID(exit);
}

void
rpc__exit(rcf_rpc_server *rpcs, int status)
{
    tarpc__exit_in  in;
    tarpc__exit_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(_exit);
    }

    in.status = status;

    rcf_rpc_call(rpcs, "_exit", &in, &out);

    TAPI_RPC_LOG(rpcs, _exit, "%d", "(void)", status);

    if (TE_RC_GET_ERROR(RPC_ERRNO(rpcs)) == TE_ERPCDEAD)
    {
        RING("RPC server %s is dead as a result of exit() call",
             rpcs->name);
    }
    else
        RETVAL_VOID(_exit);
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

/* See description in the tapi_rpc_unistd.h */
int
rpc_pthread_cancel(rcf_rpc_server *rpcs, tarpc_pthread_t tid)
{
    tarpc_pthread_cancel_in  in;
    tarpc_pthread_cancel_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pthread_cancel, -1);
    }

    in.tid = tid;

    rcf_rpc_call(rpcs, "pthread_cancel", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(pthread_cancel, out.retval);
    TAPI_RPC_LOG(rpcs, pthread_cancel, "%llu", "%d",
                 (unsigned long long int)tid, out.retval);
    RETVAL_INT(pthread_cancel, out.retval);
}

/* See description in the tapi_rpc_unistd.h */
int
rpc_pthread_setcancelstate(rcf_rpc_server *rpcs,
                           rpc_pthread_cancelstate state,
                           rpc_pthread_cancelstate *oldstate)
{
    tarpc_pthread_setcancelstate_in   in;
    tarpc_pthread_setcancelstate_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pthread_setcancelstate, -1);
    }

    in.state = state;

    rcf_rpc_call(rpcs, "pthread_setcancelstate", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(pthread_setcancelstate, out.retval);

    if (out.retval == 0 && oldstate != NULL)
        *oldstate = out.oldstate;

    TAPI_RPC_LOG(rpcs, pthread_setcancelstate, "new: %s, old: %s", "%d",
                 pthread_cancelstate_rpc2str(state),
                 pthread_cancelstate_rpc2str(out.oldstate), out.retval);
    RETVAL_INT(pthread_setcancelstate, out.retval);
}

/* See description in the tapi_rpc_unistd.h */
int
rpc_pthread_setcanceltype(rcf_rpc_server *rpcs,
                          rpc_pthread_canceltype type,
                          rpc_pthread_canceltype *oldtype)
{
    tarpc_pthread_setcanceltype_in   in;
    tarpc_pthread_setcanceltype_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pthread_setcanceltype, -1);
    }

    in.type = type;

    rcf_rpc_call(rpcs, "pthread_setcanceltype", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(pthread_setcanceltype, out.retval);

    if (out.retval == 0 && oldtype != NULL)
        *oldtype = out.oldtype;

    TAPI_RPC_LOG(rpcs, pthread_setcanceltype, "new: %s, old: %s", "%d",
                 pthread_canceltype_rpc2str(type),
                 pthread_canceltype_rpc2str(out.oldtype), out.retval);
    RETVAL_INT(pthread_setcanceltype, out.retval);
}

/* See description in the tapi_rpc_unistd.h */
int
rpc_pthread_join(rcf_rpc_server *rpcs, tarpc_pthread_t tid, uint64_t *retval)
{
    tarpc_pthread_join_in   in;
    tarpc_pthread_join_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pthread_join, -1);
    }

    in.tid = tid;

    rcf_rpc_call(rpcs, "pthread_join", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(pthread_join, out.retval);

    if ((out.retval == 0) && (retval != NULL))
        *retval = out.ret;

    TAPI_RPC_LOG(rpcs, pthread_join, "%llu, %" TE_PRINTF_64 "u", "%d",
                 (unsigned long long int)tid, out.ret, out.retval);
    RETVAL_INT(pthread_join, out.retval);
}

tarpc_pid_t
rpc_gettid(rcf_rpc_server *rpcs)
{
    tarpc_call_gettid_in  in;
    tarpc_call_gettid_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(call_gettid, -1);
    }

    rcf_rpc_call(rpcs, "call_gettid", &in, &out);

    TAPI_RPC_LOG(rpcs, call_gettid, "", "%d",
                 out.retval);
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

    if (path != NULL)
        free(in.path.path_val);

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
#define GET_STR(_dst, _field)                                   \
        do {                                                    \
            te_strlcpy(buf->_dst, out.buf._field._field##_val,  \
                       sizeof(buf->_dst));                      \
        } while(0)

        GET_STR(sysname, sysname);
        GET_STR(nodename, nodename);
        GET_STR(release, release);
        GET_STR(version, osversion);
        GET_STR(machine, machine);
#undef GET_STR
    }

    TAPI_RPC_LOG(rpcs, uname, "", "%d", out.retval);
    RETVAL_INT(uname, out.retval);
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
 * Get address in the TA address space by its ID.
 *
 * @param rpcs    RPC server handle
 * @param id      Address ID
 *
 * @return  Value of address in the TA address space 
 */
uint64_t
rpc_get_addr_by_id(rcf_rpc_server *rpcs, rpc_ptr id)
{
    tarpc_get_addr_by_id_in     in;
    tarpc_get_addr_by_id_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(get_addr_by_id, 0);
    }

    in.id = id;

    rcf_rpc_call(rpcs, "get_addr_by_id", &in, &out);

    TAPI_RPC_LOG(rpcs, get_addr_by_id, "%d", "%llu",
                 id, out.retval);
    RETVAL_PTR64(malloc, out.retval);
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

int64_t
rpc_sysconf(rcf_rpc_server *rpcs, rpc_sysconf_name name)
{
    tarpc_sysconf_in  in;
    tarpc_sysconf_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sysconf, -1);
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.name = name;

    rcf_rpc_call(rpcs, "sysconf", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sysconf, out.retval);
    TAPI_RPC_LOG(rpcs, sysconf, "%s", "%lld",
                 sysconf_name_rpc2str(name),
                 (long long int)out.retval);
    RETVAL_INT64(sysconf, out.retval);
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

int
rpc_stat_func(rcf_rpc_server *rpcs,
              const char *path,
              rpc_stat *buf)
{
    tarpc_te_stat_in  in;
    tarpc_te_stat_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(te_stat, -1);
    }

    if (buf == NULL)
    {
        ERROR("%s(): stat buffer pointer", __FUNCTION__);
        RETVAL_INT(te_stat, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)strdup(path);
    }

    rcf_rpc_call(rpcs, "te_stat", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
        memcpy(buf, &out.buf, sizeof(out.buf));

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(stat, out.retval);
    TAPI_RPC_LOG(rpcs, stat, "%s",
                 "%d { atime %llu, ctime %llu, mtime %llu}",
                 path, out.retval,
                 out.buf.te_atime, out.buf.te_ctime, out.buf.te_mtime);
    RETVAL_INT(te_stat, out.retval);
}

int
rpc_link(rcf_rpc_server *rpcs,
         const char *path1, const char *path2)
{
    tarpc_link_in  in;
    tarpc_link_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(link, -1);
    }

    if (path1 != NULL)
    {
        in.path1.path1_len = strlen(path1) + 1;
        in.path1.path1_val = (char *)strdup(path1);
    }
    if (path2 != NULL)
    {
        in.path2.path2_len = strlen(path2) + 1;
        in.path2.path2_val = (char *)strdup(path2);
    }

    rcf_rpc_call(rpcs, "link", &in, &out);

    if (path1 != NULL)
        free(in.path1.path1_val);
    if (path2 != NULL)
        free(in.path2.path2_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(link, out.retval);
    TAPI_RPC_LOG(rpcs, link, "%s, %s", "%d", path1, path2, out.retval);
    RETVAL_INT(link, out.retval);
}

int
rpc_symlink(rcf_rpc_server *rpcs,
            const char *path1, const char *path2)
{
    tarpc_symlink_in  in;
    tarpc_symlink_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(symlink, -1);
    }

    if (path1 != NULL)
    {
        in.path1.path1_len = strlen(path1) + 1;
        in.path1.path1_val = (char *)strdup(path1);
    }
    if (path2 != NULL)
    {
        in.path2.path2_len = strlen(path2) + 1;
        in.path2.path2_val = (char *)strdup(path2);
    }

    rcf_rpc_call(rpcs, "symlink", &in, &out);

    if (path1 != NULL)
        free(in.path1.path1_val);
    if (path2 != NULL)
        free(in.path2.path2_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(symlink, out.retval);
    TAPI_RPC_LOG(rpcs, symlink, "%s, %s", "%d", path1, path2, out.retval);
    RETVAL_INT(symlink, out.retval);
}

int
rpc_unlink(rcf_rpc_server *rpcs, const char *path)
{
    tarpc_unlink_in  in;
    tarpc_unlink_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(unlink, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)strdup(path);
    }

    rcf_rpc_call(rpcs, "unlink", &in, &out);

    if (path != NULL)
        free(in.path.path_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(unlink, out.retval);
    TAPI_RPC_LOG(rpcs, unlink, "%s", "%d", path, out.retval);
    RETVAL_INT(unlink, out.retval);
}

int
rpc_rename(rcf_rpc_server *rpcs,
           const char *path_old, const char *path_new)
{
    tarpc_rename_in  in;
    tarpc_rename_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(rename, -1);
    }

    if (path_old != NULL)
    {
        in.path_old.path_old_len = strlen(path_old) + 1;
        in.path_old.path_old_val = (char *)strdup(path_old);
    }
    if (path_new != NULL)
    {
        in.path_new.path_new_len = strlen(path_new) + 1;
        in.path_new.path_new_val = (char *)strdup(path_new);
    }

    rcf_rpc_call(rpcs, "rename", &in, &out);

    if (path_old != NULL)
        free(in.path_old.path_old_val);
    if (path_new != NULL)
        free(in.path_new.path_new_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(rename, out.retval);
    TAPI_RPC_LOG(rpcs, rename, "%s, %s", "%d",
                 path_old, path_new, out.retval);
    RETVAL_INT(rename, out.retval);
}

int
rpc_mkdir(rcf_rpc_server *rpcs, const char *path, rpc_file_mode_flags mode)
{
    tarpc_mkdir_in  in;
    tarpc_mkdir_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(mkdir, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)strdup(path);
    }
    in.mode  = mode;

    rcf_rpc_call(rpcs, "mkdir", &in, &out);

    if (path != NULL)
        free(in.path.path_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(mkdir, out.retval);
    TAPI_RPC_LOG(rpcs, mkdir, "%s, %s", "%d",
                 path, file_mode_flags_rpc2str(mode), out.retval);
    RETVAL_INT(mkdir, out.retval);

}

int
rpc_mkdirp(rcf_rpc_server *rpcs, const char *path, rpc_file_mode_flags mode)
{
    tarpc_mkdir_in  in;
    tarpc_mkdir_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(mkdirp, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = strdup(path);
    }
    in.mode  = mode;

    rcf_rpc_call(rpcs, "mkdirp", &in, &out);

    if (path != NULL)
        free(in.path.path_val);

    /* mkdirp() on the agent may legitimately clear errno */
    out.common.errno_changed = FALSE;
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(mkdirp, out.retval);
    TAPI_RPC_LOG(rpcs, mkdirp, "%s, %s", "%d",
                 path, file_mode_flags_rpc2str(mode), out.retval);
    RETVAL_INT(mkdirp, out.retval);

}

int
rpc_rmdir(rcf_rpc_server *rpcs, const char *path)
{
    tarpc_rmdir_in  in;
    tarpc_rmdir_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(rmdir, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)strdup(path);
    }

    rcf_rpc_call(rpcs, "rmdir", &in, &out);

    if (path != NULL)
        free(in.path.path_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(rmdir, out.retval);
    TAPI_RPC_LOG(rpcs, rmdir, "%s", "%d", path, out.retval);
    RETVAL_INT(rmdir, out.retval);
}

int
rpc_fstatvfs(rcf_rpc_server *rpcs, int fd, tarpc_statvfs *buf)
{
    tarpc_fstatvfs_in  in;
    tarpc_fstatvfs_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(fstatvfs, -1);
    }

    if (buf == NULL)
    {
        ERROR("%s(): stat buffer pointer", __FUNCTION__);
        RETVAL_INT(fstatvfs, -1);
    }

    in.fd = fd;
    in.buf = *buf;

    rcf_rpc_call(rpcs, "fstatvfs", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
        memcpy(buf, &out.buf, sizeof(out.buf));

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(fstatvfs, out.retval);
    TAPI_RPC_LOG(rpcs, fstatvfs, "%d",
                 "%d {BLK: %u, TOTAL: %llu, FREE: %llu}", 
                 fd, out.retval,
                 out.buf.f_bsize,
                 out.buf.f_blocks,
                 out.buf.f_bfree);
    RETVAL_INT(fstatvfs, out.retval);
}

int
rpc_statvfs(rcf_rpc_server *rpcs, const char *path, tarpc_statvfs *buf)
{
    tarpc_statvfs_in  in;
    tarpc_statvfs_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(statvfs, -1);
    }

    if (buf == NULL)
    {
        ERROR("%s(): stat buffer pointer", __FUNCTION__);
        RETVAL_INT(statvfs, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)strdup(path);
    }
    in.buf = *buf;

    rcf_rpc_call(rpcs, "statvfs", &in, &out);

    if (path != NULL)
        free(in.path.path_val);

    if (RPC_IS_CALL_OK(rpcs))
        memcpy(buf, &out.buf, sizeof(out.buf));

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(statvfs, out.retval);
    TAPI_RPC_LOG(rpcs, statvfs, "%s",
                 "%d {BLK: %u, TOTAL: %llu, FREE: %llu}",
                 path, out.retval,
                 out.buf.f_bsize,
                 out.buf.f_blocks,
                 out.buf.f_bfree);
    RETVAL_INT(statvfs, out.retval);
}

int
rpc_gethostname(rcf_rpc_server *rpcs, char *name, size_t len)
{
    tarpc_gethostname_in  in;
    tarpc_gethostname_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(gethostname, -1);
    }

    if (name != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.name.name_len = len;
        in.name.name_val = name;
    }
    in.len = len;

    rcf_rpc_call(rpcs, "gethostname", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT &&
        name != NULL && out.name.name_val != NULL)
        memcpy(name, out.name.name_val, out.name.name_len);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(gethostname, out.retval);
    TAPI_RPC_LOG(rpcs, gethostname, "%s, %d", "%d",
                 name, len, out.retval);
    RETVAL_INT(gethostname, out.retval);
}

int
rpc_chroot(rcf_rpc_server *rpcs, char *path)
{
    tarpc_chroot_in  in;
    tarpc_chroot_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(chroot, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = strdup(path);
    }

    rcf_rpc_call(rpcs, "chroot", &in, &out);

    if (path != NULL)
        free(in.path.path_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(chroot, out.retval);
    TAPI_RPC_LOG(rpcs, chroot, "%s", "%d",
                 path, out.retval);
    RETVAL_INT(chroot, out.retval);
}

int
rpc_copy_ta_libs(rcf_rpc_server *rpcs, char *path)
{
    tarpc_copy_ta_libs_in  in;
    tarpc_copy_ta_libs_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(copy_ta_libs, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = strdup(path);
    }

    rcf_rpc_call(rpcs, "copy_ta_libs", &in, &out);

    if (path != NULL)
        free(in.path.path_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(copy_ta_libs, out.retval);
    TAPI_RPC_LOG(rpcs, copy_ta_libs, "", "%d",
                 out.retval);
    RETVAL_INT(copy_ta_libs, out.retval);
}

int
rpc_rm_ta_libs(rcf_rpc_server *rpcs, char *path)
{
    tarpc_rm_ta_libs_in  in;
    tarpc_rm_ta_libs_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(rm_ta_libs, -1);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = strdup(path);
    }

    rcf_rpc_call(rpcs, "rm_ta_libs", &in, &out);

    if (path != NULL)
        free(in.path.path_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(rm_ta_libs, out.retval);
    TAPI_RPC_LOG(rpcs, rm_ta_libs, "", "%d",
                 out.retval);
    RETVAL_INT(rm_ta_libs, out.retval);
}

/**
 * Convert NULL terminated array to iovec array
 * 
 * @param arr   NULL terminated array
 * @param o_iov Location for iovec array
 *              Note! Memory of the array should be freed
 * 
 * @return Length of the array with NULL argument
 */
static size_t
unistd_arr_null_to_iov(char *const *arr, struct tarpc_iovec **o_iov)
{
    size_t              i;
    char *const        *ptr;
    struct tarpc_iovec *iov;
    size_t              len;

    if (arr == NULL)
    {
        *o_iov = NULL;
        return 0;
    }

    for (i = 0, ptr = arr; *ptr != NULL; i++, ptr++)
        ;

    len = i + 1;

    iov = calloc(len, sizeof(*iov));

    for (i = 0, ptr = arr; i < len; i++, ptr++)
    {
        iov[i].iov_base.iov_base_val = (uint8_t *)*ptr;
        if (*ptr != NULL)
            iov[i].iov_len = iov[i].iov_base.iov_base_len =
                             strlen(*ptr) + 1;
        else
            iov[i].iov_len = iov[i].iov_base.iov_base_len = 0;
    }

    *o_iov = iov;

    return len;
}

/* See description in the tapi_rpc_unistd.h */
int
rpc_execve_gen(rcf_rpc_server *rpcs, const char *filename,
               char *const argv[], char *const envp[])
{
    tarpc_execve_gen_in   in;
    tarpc_execve_gen_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(execve_gen, -1);
    }

    in.argv.argv_len = unistd_arr_null_to_iov(argv,
        (struct tarpc_iovec **)&in.argv.argv_val);
    in.envp.envp_len = unistd_arr_null_to_iov(envp,
        (struct tarpc_iovec **)&in.envp.envp_val);
    in.filename = (char *)filename;

    rcf_rpc_call(rpcs, "execve_gen", &in, &out);

    free(in.argv.argv_val);
    free(in.envp.envp_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(execve_gen, out.retval);
    TAPI_RPC_LOG(rpcs, execve_gen, "", "%d", out.retval);

    rcf_rpc_free_result(&out, (xdrproc_t)xdr_tarpc_execve_gen_out);
    return out.retval;
}

/* See description in the tapi_rpc_unistd.h */
void
rpc_make_iov(rpc_iovec *iov, size_t iovcnt, size_t min, size_t max)
{
    size_t i;

    for (i = 0; i < iovcnt; i++)
    {
        iov[i].iov_base = te_make_buf(min, max, &iov[i].iov_rlen);
        iov[i].iov_len = iov[i].iov_rlen;
    }
}

/* See description in the tapi_rpc_unistd.h */
void
rpc_release_iov(rpc_iovec *iov, size_t iovcnt)
{
    size_t i;

    if (iov == NULL)
        return;

    for (i = 0; i < iovcnt; i++)
        free(iov[i].iov_base);
    memset(iov, 0, sizeof(*iov) * iovcnt);
}

/* See description in the tapi_rpc_unistd.h */
void
rpc_alloc_iov(rpc_iovec **iov, size_t iovcnt, size_t min, size_t max)
{
    *iov = tapi_calloc(iovcnt, sizeof(rpc_iovec));
    rpc_make_iov(*iov, iovcnt, min, max);
}

/* See description in the tapi_rpc_unistd.h */
void
rpc_free_iov(rpc_iovec *iov, size_t iovcnt)
{
    rpc_release_iov(iov, iovcnt);
    free(iov);
}

/* See description in the tapi_rpc_unistd.h */
uint8_t *
rpc_iov_append2dbuf(const rpc_iovec *iov, size_t iovcnt, te_dbuf *buf)
{
    size_t i;

    for (i = 0; i < iovcnt; i++)
    {
        te_dbuf_append(buf, iov[i].iov_base, iov[i].iov_len);
    }

    return buf->ptr;
}

/* See description in the tapi_rpc_unistd.h */
uint8_t *
rpc_iov2dbuf(const rpc_iovec *iov, size_t iovcnt, te_dbuf *buf)
{
    te_dbuf_free(buf);

    return rpc_iov_append2dbuf(iov, iovcnt, buf);
}

/* See description in the tapi_rpc_unistd.h */
size_t
rpc_iov_data_len(const rpc_iovec *iov, size_t iovcnt)
{
    size_t total_len = 0;
    size_t i;

    for (i = 0; i < iovcnt; i++)
    {
        total_len += iov[i].iov_len;
    }

    return total_len;
}

/* See description in tapi_rpc_unistd.h */
void
rpc_iovec_cmp_strict(rpc_iovec *iov1, rpc_iovec *iov2, size_t iovcnt)
{
    size_t i;

    for (i = 0; i < iovcnt; i++)
    {
        if (iov1[i].iov_len != iov2[i].iov_len)
        {
            ERROR("Wrong data length %"TE_PRINTF_SIZE_T"u instead of "
                  "%"TE_PRINTF_SIZE_T"u", iov2[i].iov_len, iov1[i].iov_len);
            TEST_VERDICT("One of buffers has incorrect length");
        }

        if (memcmp(iov2[i].iov_base, iov1[i].iov_base,
                   iov1[i].iov_len) != 0)
            TEST_VERDICT("One of buffers is corrupted");
    }
}

/* See description in the tapi_rpc_unistd.h */
te_errno
tapi_rpc_read_fd_to_te_string(rcf_rpc_server *rpcs,
                              int             fd,
                              te_string      *testr)
{
    assert(testr != NULL);

    te_string_reset(testr);
    return tapi_rpc_append_fd_to_te_string(rpcs, fd, testr);
}

/* See description in the tapi_rpc_unistd.h */
te_errno
tapi_rpc_append_fd_to_te_string(rcf_rpc_server *rpcs,
                                int             fd,
                                te_string      *testr)
{
    char     tmp_buf[1024];
    int      received;
    te_errno rc = 0;

    assert(testr != NULL);

    while (TRUE)
    {
        received = rpc_read(rpcs, fd, tmp_buf, sizeof(tmp_buf) - 1);

        if (received == 0)  /* EOF */
            break;

        if (received < 0)
        {
            ERROR("%s:%d: Failed to read from fd(%d): %r",
                  __FUNCTION__, __LINE__, fd, RPC_ERRNO(rpcs));
            rc = RPC_ERRNO(rpcs);
            break;
        }

        tmp_buf[received] = '\0';
        rc = te_string_append(testr, "%s", tmp_buf);
        if (rc != 0)
            break;
    };

    return rc;
}
