/** @file
 * @brief Test API - RPC
 *
 * TAPI for auxilury remote socket calls implementation
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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

#include <stdio.h>
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
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include "te_defs.h"
#include "te_printf.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_misc.h"
#include "tapi_rpc_winsock2.h"


/* See description in tapi_rpc_misc.h */
tarpc_ssize_t
rpc_get_sizeof(rcf_rpc_server *rpcs, const char *type_name)
{
    struct tarpc_get_sizeof_in  in;
    struct tarpc_get_sizeof_out out;
    int                         rc;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(get_sizeof, -1);
    }

    if (type_name == NULL)
    {
        ERROR("%s(): NULL type name", __FUNCTION__);
        RETVAL_INT(get_sizeof, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.typename = strdup(type_name);

    rcf_rpc_call(rpcs, "get_sizeof", &in, &out);

    free(in.typename);

    rc = out.size;

    TAPI_RPC_LOG("RPC (%s,%s): get_sizeof(%s) -> %d",
                 rpcs->ta, rpcs->name, type_name, rc);

    RETVAL_INT(get_sizeof, rc);
}

/* See description in tapi_rpc_misc.h */
rpc_ptr
rpc_get_addrof(rcf_rpc_server *rpcs, const char *name)
{
    struct tarpc_get_addrof_in  in;
    struct tarpc_get_addrof_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(get_addrof, RPC_NULL);
    }

    if (name == NULL)
    {
        ERROR("%s(): NULL type name", __FUNCTION__);
        RETVAL_RPC_PTR(get_addrof, RPC_NULL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.name = strdup(name);

    rcf_rpc_call(rpcs, "get_addrof", &in, &out);

    free(in.name);

    TAPI_RPC_LOG("RPC (%s,%s): get_addrof(%s) -> %u",
                 rpcs->ta, rpcs->name, name, out.addr);

    RETVAL_RPC_PTR(get_addrof, out.addr);
}

/* See description in tapi_rpc_misc.h */
uint64_t
rpc_get_var(rcf_rpc_server *rpcs, const char *name, tarpc_size_t size)
{
    struct tarpc_get_var_in  in;
    struct tarpc_get_var_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL || name == NULL ||
        !(size == 1 || size == 2 || size == 4 || size == 8))
    {
        ERROR("%s(): Invalid parameter is provided", __FUNCTION__);
        TAPI_JMP_DO(TE_EFAIL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.name = strdup(name);
    in.size = size;

    rcf_rpc_call(rpcs, "get_var", &in, &out);

    free(in.name);

    CHECK_RETVAL_VAR_IS_BOOL(get_var, out.found);

    TAPI_RPC_LOG("RPC (%s,%s): get_var(%s, %u) -> %llu%s",
                 rpcs->ta, rpcs->name, name, size, out.found ? out.val : 0,
                 out.found ? "" : " (not found)");

    TAPI_RPC_OUT(get_var, !out.found);

    return out.val;
}

/* See description in tapi_rpc_misc.h */
void
rpc_set_var(rcf_rpc_server *rpcs, const char *name,
            tarpc_size_t size, uint64_t val)
{
    struct tarpc_set_var_in  in;
    struct tarpc_set_var_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL || name == NULL ||
        !(size == 1 || size == 2 || size == 4 || size == 8))
    {
        ERROR("%s(): Invalid parameter is provided", __FUNCTION__);
        TAPI_JMP_DO(TE_EFAIL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.name = strdup(name);
    in.size = size;
    in.val = val;

    rcf_rpc_call(rpcs, "set_var", &in, &out);

    free(in.name);

    CHECK_RETVAL_VAR_IS_BOOL(get_var, out.found);

    TAPI_RPC_LOG("RPC (%s,%s): set_var(%s, %u, %llu) -> %s",
                 rpcs->ta, rpcs->name, name, size, in.val,
                 out.found ? "OK" : "not found");

    TAPI_RPC_OUT(set_var, !out.found);
}


/**
 * Convert I/O vector to array.
 *
 * @param len      total data length
 * @param v        I/O vector
 * @param cnt      number of elements in vector
 *
 * @return Pointer to allocated array or NULL.
 */
static uint8_t *
rpc_iovec_to_array(size_t len, const struct rpc_iovec *v, size_t cnt)
{
    uint8_t *array = malloc(len);
    uint8_t *p     = array;
    size_t   i;

    if (array == NULL)
    {
        ERROR("Allocation of %u bytes failure", len);
        return NULL;
    }

    for (i = 0; i < cnt && len > 0; ++i)
    {
        size_t copylen = (v[i].iov_len < len) ? v[i].iov_len : len;

        memcpy(p, v[i].iov_base, copylen);
        p += copylen;
        len -= copylen;
    }
    if (len != 0)
    {
        ERROR("I/O vector total length is less than length by elements");
        free(array);
        return NULL;
    }
    return array;
}

/* See description in tapi_rpcsock.h */
int
rpc_iovec_cmp(size_t v1len, const struct rpc_iovec *v1, size_t v1cnt,
              size_t v2len, const struct rpc_iovec *v2, size_t v2cnt)
{
    int      result;
    uint8_t *array1 = NULL;
    uint8_t *array2 = NULL;

    if (v1len != v2len)
        return -1;

    array1 = rpc_iovec_to_array(v1len, v1, v1cnt);
    array2 = rpc_iovec_to_array(v2len, v2, v2cnt);
    if (array1 == NULL || array2 == NULL)
    {
        result = -1;
    }
    else
    {
        result = (memcmp(array1, array2, v1len) == 0) ? 0 : -1;
    }

    free(array1);
    free(array2);

    return result;
}


/**
 * Convert 'struct tarpc_timeval' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv        Pointer to 'struct tarpc_timeval'
 *
 * @return NUL-terminated string
 */
const char *
tarpc_timeval2str(const struct tarpc_timeval *tv)
{

/* Number of buffers used in the function */
#define N_BUFS 10
#define BUF_SIZE 32

    static char buf[N_BUFS][BUF_SIZE];
    static char (*cur_buf)[BUF_SIZE] = (char (*)[BUF_SIZE])buf[0];

    char *ptr;

    /*
     * Firt time the function is called we start from the second buffer, but
     * then after a turn we'll use all N_BUFS buffer.
     */
    if (cur_buf == (char (*)[BUF_SIZE])buf[N_BUFS - 1])
        cur_buf = (char (*)[BUF_SIZE])buf[0];
    else
        cur_buf++;

    ptr = *cur_buf;

    if (tv == NULL)
    {
        snprintf(ptr, BUF_SIZE, "(nil)");
    }
    else
    {
        snprintf(ptr, BUF_SIZE, "{%ld,%ld}",
                 (long)tv->tv_sec, (long)tv->tv_usec);
    }

#undef BUF_SIZE
#undef N_BUFS

    return ptr;
}

/**
 * Convert 'struct timespec' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv    - pointer to 'struct timespec'
 *
 * @return null-terminated string
 */
const char *
timespec2str(const struct timespec *tv)
{
    static char buf[32];

    if (tv == NULL)
    {
        strcpy(buf, "(nil)");
    }
    else
    {
        sprintf(buf, "{%ld,%ld}", tv->tv_sec, tv->tv_nsec);
    }
    return buf;
}


/**
 * Simple sender.
 *
 * @param rpcs            RPC server
 * @param s                 a socket to be user for sending
 * @param size_min          minimum size of the message in bytes
 * @param size_max          maximum size of the message in bytes
 * @param size_rnd_once     if true, random size should be calculated
 *                          only once and used for all messages;
 *                          if false, random size is calculated for
 *                          each message
 * @param delay_min         minimum delay between messages in microseconds
 * @param delay_max         maximum delay between messages in microseconds
 * @param delay_rnd_once    if true, random delay should be calculated
 *                          only once and used for all messages;
 *                          if false, random delay is calculated for
 *                          each message
 * @param time2run          how long run (in seconds)
 * @param sent              location for number of sent bytes
 * @param ignore_err        Ignore errors while run
 *
 * @return Number of sent bytes or -1 in the case of failure if
 *         ignore_err set to FALSE or number of sent bytes or 0 if
 *         ignore_err set to TRUE
 */
int
rpc_simple_sender(rcf_rpc_server *rpcs,
                  int s, int size_min, int size_max,
                  int size_rnd_once, int delay_min, int delay_max,
                  int delay_rnd_once, int time2run, uint64_t *sent,
                  int ignore_err)
{
    rcf_rpc_op              op;
    tarpc_simple_sender_in  in;
    tarpc_simple_sender_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(simple_sender, -1);
    }

    op = rpcs->op;

    in.s = s;
    in.size_min = size_min;
    in.size_max = size_max;
    in.size_rnd_once = size_rnd_once;
    in.delay_min = delay_min;
    in.delay_max = delay_max;
    in.delay_rnd_once = delay_rnd_once;
    in.time2run = time2run;
    in.ignore_err = ignore_err;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }

    rcf_rpc_call(rpcs, "simple_sender", &in, &out);

    if (out.retval == 0)
        *sent = out.bytes;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(simple_sender, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "simple_sender(%d, %d, %d, %d, %d, %d, %d, %d, %d) -> "
                 "%d (%s) %u", rpcs->ta, rpcs->name, rpcop2str(op),
                 s, size_min, size_max, size_rnd_once,
                 delay_min, delay_max, delay_rnd_once,
                 time2run, ignore_err,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 (unsigned int)*sent);

    RETVAL_INT(simple_sender, out.retval);
}

/**
 * Simple receiver.
 *
 * @param rpcs            RPC server
 * @param s               a socket to be user for receiving
 * @param received        location for number of received bytes
 *
 * @return number of received bytes or -1 in the case of failure
 */
int
rpc_simple_receiver(rcf_rpc_server *rpcs,
                    int s, uint32_t time2run, uint64_t *received)
{
    rcf_rpc_op                op;
    tarpc_simple_receiver_in  in;
    tarpc_simple_receiver_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(simple_receiver, -1);
    }

    op = rpcs->op;

    in.s = s;
    in.time2run = time2run;
    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }

    rcf_rpc_call(rpcs, "simple_receiver", &in, &out);

    if (out.retval == 0)
        *received = out.bytes;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(simple_receiver, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: simple_receiver(%d, %d) -> %d (%s) "
                 "received=%u", rpcs->ta, rpcs->name, rpcop2str(op), s,
                 time2run, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 (unsigned long)*received);

    RETVAL_INT(simple_receiver, out.retval);
}


/* See description in tapi_rpc_misc.h */
int
rpc_recv_verify(rcf_rpc_server *rpcs, int s,
                const char *gen_data_fname, uint64_t start)
{
    rcf_rpc_op            op;
    tarpc_recv_verify_in  in;
    tarpc_recv_verify_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recv_verify, -1);
    }
    if (gen_data_fname == NULL)
    {
        ERROR("%s(): NULL function name", __FUNCTION__);
        RETVAL_INT(recv_verify, -1);
    }

    RING("%s(): fname %s", __FUNCTION__, gen_data_fname);

    op = rpcs->op;

    in.s = s;
    in.start = start;

#if 1
    if (op != RCF_RPC_WAIT)
    {
        in.fname.fname_len = strlen(gen_data_fname) + 1;
        in.fname.fname_val = strdup(gen_data_fname);
    }
#endif

    rcf_rpc_call(rpcs, "recv_verify", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: recv_verify(%d, %d) -> %d (%s) ",
                 rpcs->ta, rpcs->name, rpcop2str(op), s,
                 (uint32_t)start, out.retval,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(recv_verify, out.retval);
}


/* See description in tapi_rpcsock.h */
int
rpc_iomux_flooder(rcf_rpc_server *rpcs,
                  int *sndrs, int sndnum, int *rcvrs, int rcvnum,
                  int bulkszs, int time2run, int time2wait, int iomux,
                  te_bool rx_nonblock, uint64_t *tx_stat, uint64_t *rx_stat)
{
    rcf_rpc_op        op;
    tarpc_flooder_in  in;
    tarpc_flooder_out out;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(flooder, -1);
    }

    op = rpcs->op;

    if (sndrs != NULL)
    {
        in.sndrs.sndrs_val = sndrs;
        in.sndrs.sndrs_len = sndnum;
    }
    if (rcvrs != NULL)
    {
        in.rcvrs.rcvrs_val = rcvrs;
        in.rcvrs.rcvrs_len = rcvnum;
    }
    in.bulkszs = bulkszs;
    in.time2run = time2run;
    in.time2wait = time2wait;
    in.iomux = iomux;
    in.rx_nonblock = rx_nonblock;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }

    if (tx_stat != NULL)
    {
        in.tx_stat.tx_stat_val = tx_stat;
        in.tx_stat.tx_stat_len = sndnum;
    }
    if (rx_stat != NULL)
    {
        in.rx_stat.rx_stat_val = rx_stat;
        in.rx_stat.rx_stat_len = rcvnum;
    }
    rcf_rpc_call(rpcs, "flooder", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (tx_stat != NULL)
            memcpy(tx_stat, out.tx_stat.tx_stat_val,
                   out.tx_stat.tx_stat_len * sizeof(tx_stat[0]));
        if (rx_stat != NULL)
            memcpy(rx_stat, out.rx_stat.rx_stat_val,
                   out.rx_stat.rx_stat_len * sizeof(rx_stat[0]));
    }
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(flooder, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "flooder(%p, %d, %p, %d, %d, %d, %d, %d, %p, %p) "
                 "-> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), rcvrs,
                 rcvnum, sndrs, sndnum, bulkszs, time2run, time2wait,
                 iomux,
                 tx_stat, rx_stat, out.retval,
                 errno_rpc2str(RPC_ERRNO(rpcs)));
    RETVAL_INT(flooder, out.retval);
}

/* See description in tapi_rpcsock.h */
int
rpc_iomux_echoer(rcf_rpc_server *rpcs,
                 int *sockets, int socknum, int time2run, int iomux,
                 uint64_t *tx_stat, uint64_t *rx_stat)
{
    rcf_rpc_op       op;
    tarpc_echoer_in  in;
    tarpc_echoer_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(echoer, -1);
    }
    if (sockets == NULL)
    {
        rpcs->_errno = RPC_EINVAL;
        RETVAL_INT(echoer, -1);
    }

    op = rpcs->op;


    in.sockets.sockets_val = sockets;
    in.sockets.sockets_len = socknum;
    in.time2run = time2run;
    in.iomux = iomux;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }

    if (tx_stat != NULL)
    {
        in.tx_stat.tx_stat_val = tx_stat;
        in.tx_stat.tx_stat_len = socknum;
    }
    if (rx_stat != NULL)
    {
        in.rx_stat.rx_stat_val = rx_stat;
        in.rx_stat.rx_stat_len = socknum;
    }

    rcf_rpc_call(rpcs, "echoer", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (tx_stat != NULL)
            memcpy(tx_stat, out.tx_stat.tx_stat_val,
                   out.tx_stat.tx_stat_len * sizeof(tx_stat[0]));
        if (rx_stat != NULL)
            memcpy(rx_stat, out.rx_stat.rx_stat_val,
                   out.rx_stat.rx_stat_len * sizeof(rx_stat[0]));
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(echoer, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: echoer(%p, %d, %d, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 sockets, socknum, time2run, iomux,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(echoer, out.retval);
}

ssize_t
rpc_sendfile(rcf_rpc_server *rpcs, int out_fd, int in_fd,
             tarpc_off_t *offset, size_t count)
{
    rcf_rpc_op         op;
    tarpc_off_t        start = (offset != NULL) ? *offset : 0;
    tarpc_sendfile_in  in;
    tarpc_sendfile_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendfile, -1);
    }
    op = rpcs->op;

    in.out_fd = out_fd;
    in.in_fd = in_fd;
    in.count = count;
    if (offset != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.offset.offset_len = 1;
        in.offset.offset_val = offset;
    }

    rcf_rpc_call(rpcs, "sendfile", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (offset != NULL && out.offset.offset_val != NULL)
            *offset = out.offset.offset_val[0];
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendfile, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "sendfile(%d, %d, %p(%d), %u) -> %d (%s) offset=%d",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 out_fd, in_fd, offset, (int)start, (unsigned)count,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 (offset != NULL) ? (int)*offset : 0);

    RETVAL_INT(sendfile, out.retval);
}


/* See description in tapi_rpcsock.h */
ssize_t
rpc_socket_to_file(rcf_rpc_server *rpcs, int sock,
                   const char *path, long timeout)
{
    rcf_rpc_op               op;
    tarpc_socket_to_file_in  in;
    tarpc_socket_to_file_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(socket_to_file, -1);
    }

    op = rpcs->op;

    in.sock = sock;
    in.timeout = timeout;
    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(timeout + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }
    if (path != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)path;
    }

    rcf_rpc_call(rpcs, "socket_to_file", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(socket_to_file, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: socket_to_file(%d, %s, %u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 sock, path, timeout,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(socket_to_file, out.retval);
}

int
rpc_ftp_open(rcf_rpc_server *rpcs,
             char *uri, te_bool rdonly, te_bool passive, int offset,
             int *sock)
{
    tarpc_ftp_open_in  in;
    tarpc_ftp_open_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ftp_open, -1);
    }

    in.uri.uri_val = uri;
    in.uri.uri_len = strlen(uri) + 1;
    in.rdonly = rdonly;
    in.passive = passive;
    in.offset = offset;
    if (sock == NULL)
        in.sock.sock_len = 0;
    else
    {
        in.sock.sock_val = (tarpc_int *)sock;
        in.sock.sock_len = 1;
    }

    rcf_rpc_call(rpcs, "ftp_open", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && sock != NULL)
        *sock = out.sock;

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(ftp_open, out.fd);

    TAPI_RPC_LOG("RPC (%s,%s): ftp_open(%s, %s, %s, %d, %p) -> %d (%s)",
                 rpcs->ta, rpcs->name, uri, rdonly ? "get" : "put",
                 passive ? "passive": "active", offset, sock,
                 out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(ftp_open, out.fd);
}

int
rpc_ftp_close(rcf_rpc_server *rpcs, int sock)
{
    tarpc_ftp_close_in  in;
    tarpc_ftp_close_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ftp_close, -1);
    }

    in.sock = sock;

    rcf_rpc_call(rpcs, "ftp_close", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(ftp_close, out.ret);

    TAPI_RPC_LOG("RPC (%s,%s): ftp_close(%d) -> %d (%s)",
                 rpcs->ta, rpcs->name, sock,
                 out.ret, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(ftp_open, out.ret);
}

int
rpc_overfill_buffers(rcf_rpc_server *rpcs, int sock, uint64_t *sent)
{
    rcf_rpc_op                 op;
    tarpc_overfill_buffers_in  in;
    tarpc_overfill_buffers_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(overfill_buffers, -1);
    }
    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.sock = sock;

    rcf_rpc_call(rpcs, "overfill_buffers", &in, &out);

    if ((out.retval == 0) && (sent != NULL))
        *sent = out.bytes;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(overfill_buffers, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: overfill_buffers(%d) -> %d (%s) sent=%d",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 sock,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 (sent != NULL) ? (int)(*sent) : -1);

    RETVAL_INT(overfill_buffers, out.retval);
}

/**
 * Copy the data from src_buf to the dst_buf buffer located at TA.
 */
void
rpc_set_buf(rcf_rpc_server *rpcs, const uint8_t *src_buf,
            size_t len, rpc_ptr dst_buf, rpc_ptr offset)
{
    tarpc_set_buf_in  in;
    tarpc_set_buf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(set_buf);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.src_buf.src_buf_val = (char *)src_buf;
    if (src_buf != NULL)
        in.src_buf.src_buf_len = len;
    else
        in.src_buf.src_buf_len = 0;

    in.dst_buf = (tarpc_ptr)dst_buf;
    in.offset = (tarpc_ptr)offset;

    rcf_rpc_call(rpcs, "set_buf", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): set_buf() -> (%s)",
                 rpcs->ta, rpcs->name,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(set_buf);
}

/**
 * Copy the data from the src_buf buffer located at TA to dst_buf buffer.
 */
void
rpc_get_buf(rcf_rpc_server *rpcs, rpc_ptr src_buf,
            rpc_ptr offset, size_t len, uint8_t *dst_buf)
{
    tarpc_get_buf_in  in;
    tarpc_get_buf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(get_buf);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.src_buf = (tarpc_ptr)src_buf;
    in.offset = (tarpc_ptr)offset;
    in.len = len;

    rcf_rpc_call(rpcs, "get_buf", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): get_buf() -> (%s)",
                 rpcs->ta, rpcs->name,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    if ((out.dst_buf.dst_buf_len != 0) && (out.dst_buf.dst_buf_val != NULL))
        memcpy(dst_buf, out.dst_buf.dst_buf_val, out.dst_buf.dst_buf_len);

    RETVAL_VOID(get_buf);
}

/**
 * Fill buffer by the pattern
 */
void
rpc_set_buf_pattern(rcf_rpc_server *rpcs, int pattern,
                    size_t len, rpc_ptr dst_buf, rpc_ptr offset)
{
    tarpc_set_buf_pattern_in  in;
    tarpc_set_buf_pattern_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(set_buf_pattern);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.dst_buf = (tarpc_ptr)dst_buf;
    in.offset = (tarpc_ptr)offset;
    in.pattern = pattern;
    in.len = len;

    rcf_rpc_call(rpcs, "set_buf_pattern", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): set_buf_pattern() -> (%s)",
                 rpcs->ta, rpcs->name,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(set_buf_pattern);
}


void
rpc_vm_trasher(rcf_rpc_server *rpcs, te_bool start)
{
    tarpc_vm_trasher_in  in;
    tarpc_vm_trasher_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(vm_trasher);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.start = start;

    rcf_rpc_call(rpcs, "vm_trasher", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: vm_trasher(%s) -> (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(rpcs->op),
                 start == FALSE ? "stop" : "start",
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(vm_trasher);
}

void
rpc_create_child_process_socket(rcf_rpc_server *pco_father, int father_s,
                                rpc_socket_domain domain,
                                rpc_socket_type sock_type,
                                rcf_rpc_server **pco_child, int *child_s)
{
    pid_t                   process_id;
    char                    info[512];
    int                     info_len = sizeof(info);
    char                    process_name[12];
    static int              counter = 1;

    sprintf(process_name, "pco_child%d", counter);

    if (rpc_is_winsock2(pco_father))
    {
        rcf_rpc_server_create(pco_father->ta, process_name, pco_child);
        process_id = rpc_getpid(*pco_child);
        rpc_wsa_duplicate_socket(pco_father, father_s, process_id,
                                 info, &info_len);
        *child_s = rpc_wsa_socket(*pco_child, domain, sock_type,
                                  RPC_PROTO_DEF, info, info_len, 0);
    }
    else
    {
        rcf_rpc_server_fork(pco_father, process_name, pco_child);
        *child_s = father_s;
    }
    counter++;
}

/**
 * Get readability (there are data to read) or writability (it is allowed
 * to write) of a particular socket.
 *
 * @param answer     answer location
 * @param rpcs       RPC server handle
 * @param s          socket to be checked
 * @param timeout    timeout in seconds
 * @param type       type of checking: "READ" or "WRITE"
 *
 * @return status code
 */
int
tapi_rpc_get_rw_ability(te_bool *answer, rcf_rpc_server *rpcs,
                        int s, int timeout, char *type)
{
    tarpc_timeval   tv = { 0 , 0 };
    rpc_fd_set_p    fds = RPC_NULL;
    int             rc = -1;
    int             result = -1;

    fds = rpc_fd_set_new(rpcs);
    rpc_do_fd_zero(rpcs, fds);
    rpc_do_fd_set(rpcs, s, fds);

    tv.tv_sec = timeout;

    if (type[0] == 'R')
        rc = rpc_select(rpcs, s + 1, fds, RPC_NULL, RPC_NULL, &tv);
    else
        rc = rpc_select(rpcs, s + 1, RPC_NULL, fds, RPC_NULL, &tv);

    *answer = (rc == 1);
    result = 0;

    rpc_fd_set_delete(rpcs, fds);

    return result;
}
