/** @file
 * @brief Test API - RPC
 *
 * TAPI for auxilury remote socket calls implementation
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

#include "tapi_rpc_internal.h"
#include "tapi_rpc_unistd.h"


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
 * Convert 'struct timeval' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv    - pointer to 'struct timeval'
 *
 * @return null-terminated string
 */
const char *
timeval2str(const struct timeval *tv)
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
        snprintf(ptr, BUF_SIZE, "NULL");
    }
    else
    {
        snprintf(ptr, BUF_SIZE, "{%ld,%ld}", tv->tv_sec, tv->tv_usec);
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
        strcpy(buf, "NULL");
    }
    else
    {
        sprintf(buf, "{%ld,%ld}", tv->tv_sec, tv->tv_nsec);
    }
    return buf;
}


/**
 * Set dynamic library name to be used for additional name resolution.
 *
 * @param rpcs          Existing RPC server handle
 * @param libname       Name of the dynamic library or NULL
 *
 * @return Status code
 */
int
rpc_setlibname(rcf_rpc_server *rpcs, const char *libname)
{
    tarpc_setlibname_in  in;
    tarpc_setlibname_out out;

    if (rpcs == NULL)
    {
        return TE_RC(TE_RCF, EINVAL);
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.libname.libname_len = (libname == NULL) ? 0 : (strlen(libname) + 1);
    in.libname.libname_val = (char *)libname;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, _setlibname,
                 &in,  (xdrproc_t)xdr_tarpc_setlibname_in,
                 &out, (xdrproc_t)xdr_tarpc_setlibname_out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(setlibname, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s) setlibname(%s) -> %d (%s)",
                 rpcs->ta, rpcs->name, libname ? : "(NULL)",
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT_ZERO_OR_MINUS_ONE(setlibname, out.retval);
}

int
rpc_send_traffic(rcf_rpc_server *rpcs, int num,
                 int *s, const void *buf, size_t len,
                 int flags,
                 struct sockaddr *to, socklen_t tolen)
{
    rcf_rpc_op             op;
    tarpc_send_traffic_in  in;
    tarpc_send_traffic_out out;
    
    int i;
    int             *ss    = NULL;
    struct sockaddr *addrs = NULL;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }
    if (s == NULL || to == NULL)
    {
        ERROR("%s(): Invalid pointers to sockets and addresses",
              __FUNCTION__);
        return -1;
    }
    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    /* Num */
    in.num = num;
    
    /* Sockets */
    ss = (int *)calloc(num, sizeof(int));
    in.fd.fd_val = (tarpc_int *)ss;
    if (in.fd.fd_val == NULL)
    {
        ERROR("%s(): Memory allocation failure", __FUNCTION__);
        return -1;
    }
    in.fd.fd_len = num;
    for (i = 0; i < num; i++)
        in.fd.fd_val[i] = *(s + i);

    /* Length */
    in.len = len;
    
    /* Adresses */
    addrs = (struct sockaddr *)calloc(num, sizeof(struct sockaddr));
    in.to.to_val = (struct tarpc_sa *)addrs;
    if (in.to.to_val == NULL)
    {
        ERROR("%s(): Memory allocation failure", __FUNCTION__);
        return -1;
    }
    in.to.to_len = num;
    for (i = 0; i < num; i++)
    {    
        if ((to + i) != NULL && rpcs->op != RCF_RPC_WAIT)
        {
            if (tolen >= SA_COMMON_LEN)
            {
                in.to.to_val[i].sa_family = 
                    addr_family_h2rpc((to + i)->sa_family);
                in.to.to_val[i].sa_data.sa_data_len = tolen - SA_COMMON_LEN;
                in.to.to_val[i].sa_data.sa_data_val = 
                    (uint8_t *)((to + i)->sa_data);
            }
            else
            {
                in.to.to_val[i].sa_family = RPC_AF_UNSPEC;
                in.to.to_val[i].sa_data.sa_data_len = 0;
                /* Any no-NULL pointer is suitable here */
                in.to.to_val[i].sa_data.sa_data_val = (uint8_t *)(to + i);
            }
        }
    }
    
    in.tolen = tolen;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = len;
        in.buf.buf_val = (char *)buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, _send_traffic,
                 &in,  (xdrproc_t)xdr_tarpc_send_traffic_in,
                 &out, (xdrproc_t)xdr_tarpc_send_traffic_out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(send_traffic, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: send_traffic ->%d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    if (RPC_IS_CALL_OK(rpcs))
    {
        for (i = 0; i < num; i++)
        {
            RING("send_traffic to %s - done", sockaddr2str(to + i));
        }
    }

    free(addrs);
    free(ss);

    RETVAL_VAL(send_traffic, out.retval);
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

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.s = s;
    in.size_min = size_min;
    in.size_max = size_max;
    in.size_rnd_once = size_rnd_once;
    in.delay_min = delay_min;
    in.delay_max = delay_max;
    in.delay_rnd_once = delay_rnd_once;
    in.time2run = time2run;
    in.ignore_err = ignore_err;

    rcf_rpc_call(rpcs, _simple_sender,
                 &in,  (xdrproc_t)xdr_tarpc_simple_sender_in,
                 &out, (xdrproc_t)xdr_tarpc_simple_sender_out);

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

    RETVAL_INT_ZERO_OR_MINUS_ONE(simple_sender, out.retval);
}

/**
 * Simple receiver.
 *
 * @param rpcs            RPC server
 * @param s                 a socket to be user for receiving
 * @param received          location for number of received bytes
 *
 * @return number of received bytes or -1 in the case of failure
 */
int
rpc_simple_receiver(rcf_rpc_server *rpcs,
                    int s, uint64_t *received)
{
    rcf_rpc_op                op;
    tarpc_simple_receiver_in  in;
    tarpc_simple_receiver_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.s = s;

    rcf_rpc_call(rpcs, _simple_receiver,
                 &in,  (xdrproc_t)xdr_tarpc_simple_receiver_in,
                 &out, (xdrproc_t)xdr_tarpc_simple_receiver_out);

    if (out.retval == 0)
        *received = out.bytes;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(simple_receiver, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: simple_receiver(%d) -> %d (%s) "
                 "received=%u", rpcs->ta, rpcs->name, rpcop2str(op), s,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 (unsigned long)*received);

    RETVAL_INT_ZERO_OR_MINUS_ONE(simple_receiver, out.retval);
}


/* See description in tapi_rpcsock.h */
int
rpc_iomux_flooder(rcf_rpc_server *rpcs,
                  int *sndrs, int sndnum, int *rcvrs, int rcvnum,
                  int bulkszs, int time2run, int iomux, te_bool rx_nonblock,
                  uint64_t *tx_stat, uint64_t *rx_stat)
{
    rcf_rpc_op        op;
    tarpc_flooder_in  in;
    tarpc_flooder_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

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
    in.iomux = iomux;
    in.rx_nonblock = rx_nonblock;
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

    rcf_rpc_call(rpcs, _flooder,
                 &in,  (xdrproc_t)xdr_tarpc_flooder_in,
                 &out, (xdrproc_t)xdr_tarpc_flooder_out);

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
                 "flooder(%p, %d, %p, %d, %d, %d, %d, %p, %p) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), rcvrs,
                 rcvnum, sndrs, sndnum, bulkszs, time2run, iomux,
                 tx_stat, rx_stat, out.retval,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT_ZERO_OR_MINUS_ONE(flooder, out.retval);
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

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }
    if (sockets == NULL)
    {
        rpcs->_errno = RPC_EINVAL;
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.sockets.sockets_val = sockets;
    in.sockets.sockets_len = socknum;
    in.time2run = time2run;
    in.iomux = iomux;
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

    rcf_rpc_call(rpcs, _echoer,
                 &in,  (xdrproc_t)xdr_tarpc_echoer_in,
                 &out, (xdrproc_t)xdr_tarpc_echoer_out);

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

    RETVAL_INT_ZERO_OR_MINUS_ONE(echoer, out.retval);
}

ssize_t
rpc_sendfile(rcf_rpc_server *rpcs, int out_fd, int in_fd,
             off_t *offset, size_t count)
{
    rcf_rpc_op         op;
    off_t              start = (offset != NULL) ? *offset : 0;
    tarpc_sendfile_in  in;
    tarpc_sendfile_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }
    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.out_fd = out_fd;
    in.in_fd = in_fd;
    in.count = count;
    if (offset != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.offset.offset_len = 1;
        in.offset.offset_val = (tarpc_off_t *)offset;
    }

    rcf_rpc_call(rpcs, _sendfile,
                 &in,  (xdrproc_t)xdr_tarpc_sendfile_in,
                 &out, (xdrproc_t)xdr_tarpc_sendfile_out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (offset != NULL && out.offset.offset_val != NULL)
            *offset = out.offset.offset_val[0];
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendfile, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: "
                 "sendfile(%d, %d, %p(%d), %u) -> %d (%s) offset=%d",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 out_fd, in_fd, offset, start, count,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)),
                 (offset != NULL) ? *offset : 0);

    RETVAL_VAL(sendfile, out.retval);
}


/* See description in tapi_rpcsock.h */
ssize_t
rpc_socket_to_file(rcf_rpc_server *rpcs, int sock,
                   const char *path, long timeout)
{
    rcf_rpc_op               op;
    tarpc_socket_to_file_in  in;
    tarpc_socket_to_file_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.sock = sock;
    in.timeout = timeout;
    if (path != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.path.path_len = strlen(path);
        in.path.path_val = (char *)path;
    }

    rcf_rpc_call(rpcs, _socket_to_file,
                 &in, (xdrproc_t)xdr_tarpc_socket_to_file_in,
                 &out, (xdrproc_t)xdr_tarpc_socket_to_file_out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(socket_to_file, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: socket_to_file(%d, %s, %u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 sock, path, timeout,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VAL(socket_to_file, out.retval);
}

int
rpc_ftp_open(rcf_rpc_server *rpcs,
             char *uri, te_bool rdonly, te_bool passive, int offset,
             int *sock)
{
    tarpc_ftp_open_in  in;
    tarpc_ftp_open_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

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

    rcf_rpc_call(rpcs, _ftp_open,
                 &in,  (xdrproc_t)xdr_tarpc_ftp_open_in,
                 &out, (xdrproc_t)xdr_tarpc_ftp_open_out);

    if (RPC_IS_CALL_OK(rpcs) && sock != NULL)
        *sock = out.sock;

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(ftp_open, out.fd);

    TAPI_RPC_LOG("RPC (%s,%s): ftp_open(%s, %s, %s, %d, %p) -> %d (%s)",
                 rpcs->ta, rpcs->name, uri, rdonly ? "get" : "put",
                 passive ? "passive": "active", offset, sock,
                 out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VAL(ftp_open, out.fd);
}

int
rpc_many_send(rcf_rpc_server *rpcs, int sock,
              const int *vector, int nops, uint64_t *sent)
{
    rcf_rpc_op          op;
    tarpc_many_send_in  in;
    tarpc_many_send_out out;

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    in.sock = sock;

    if (vector != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.vector.vector_len = nops;
        in.vector.vector_val = (tarpc_int *)vector;
    }

    rcf_rpc_call(rpcs, _many_send,
                 &in,  (xdrproc_t)xdr_tarpc_many_send_in,
                 &out, (xdrproc_t)xdr_tarpc_many_send_out);
    if (out.retval == 0)
        *sent = out.bytes;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(many_send, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: many_send(%d, %u, %p) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 sock, nops, vector,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT_ZERO_OR_MINUS_ONE(many_send, out.retval);
}

int
rpc_overfill_buffers(rcf_rpc_server *rpcs, int sock, uint64_t *sent)
{
    rcf_rpc_op                 op;
    tarpc_overfill_buffers_in  in;
    tarpc_overfill_buffers_out out;

    op = rpcs->op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    in.sock = sock;

    rcf_rpc_call(rpcs, _overfill_buffers,
                 &in,  (xdrproc_t)xdr_tarpc_overfill_buffers_in,
                 &out, (xdrproc_t)xdr_tarpc_overfill_buffers_out);
    if (out.retval == 0)
        *sent = out.bytes;
    
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(overfill_buffers, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: overfill_buffers(%d) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 sock,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT_ZERO_OR_MINUS_ONE(overfill_buffers, out.retval);
}
