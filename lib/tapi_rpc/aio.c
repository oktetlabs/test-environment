/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of asynchronous input/output
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

#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "tapi_rpc_internal.h"
#include "tapi_rpc_aio.h"


/**
 * Asynchronous read test procedure.
 *
 * @param rpcs            RPC server
 * @param s                 a socket to be user for receiving
 * @param signum            signal to be used as notification event
 * @param timeout           timeout for blocking on select() in seconds
 * @param buf               buffer for received data
 * @param buflen            buffer length to be passed to the read
 * @param rlen              real buffer length
 * @param diag              location for diagnostic message
 * @param diag_len          maximum length of the diagnostic message
 *
 * @return number of received bytes or -1 in the case of failure
 */
int
rpc_aio_read_test(rcf_rpc_server *rpcs,
                  int s, rpc_signum signum, int timeout,
                  char *buf, int buflen, int rlen,
                  char *diag, int diag_len)
{
    tarpc_aio_read_test_in  in;
    tarpc_aio_read_test_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.s = s;
    in.signum = signum;
    in.t = timeout;
    in.buf.buf_val = buf;
    in.buf.buf_len = rlen;
    in.buflen = buflen;
    in.diag.diag_val = diag;
    in.diag.diag_len = diag_len;

    rcf_rpc_call(rpcs, _aio_read_test,
                 &in,  (xdrproc_t)xdr_tarpc_aio_read_test_in,
                 &out, (xdrproc_t)xdr_tarpc_aio_read_test_out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);

        memcpy(diag, out.diag.diag_val, out.diag.diag_len);
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(aio_read_test, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): "
                 "aio_read_test(%d, %s, %d, %p, %d, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 s, signum_rpc2str(signum), timeout, buf, buflen, rlen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VAL(aio_read_test, out.retval);
}

/**
 * Asynchronous error processing test procedure.
 *
 * @param rpcs            RPC server
 * @param diag              location for diagnostic message
 * @param diag_len          maximum length of the diagnostic message
 *
 * @return number of received bytes or -1 in the case of failure
 */
int
rpc_aio_error_test(rcf_rpc_server *rpcs,
                   char *diag, int diag_len)
{
    tarpc_aio_error_test_in  in;
    tarpc_aio_error_test_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.diag.diag_val = diag;
    in.diag.diag_len = diag_len;

    rcf_rpc_call(rpcs, _aio_error_test,
                 &in,  (xdrproc_t)xdr_tarpc_aio_error_test_in,
                 &out, (xdrproc_t)xdr_tarpc_aio_error_test_out);

    if (RPC_IS_CALL_OK(rpcs))
        memcpy(diag, out.diag.diag_val, out.diag.diag_len);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(aio_error_test, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): "
                 "aio_error_test() -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT_ZERO_OR_MINUS_ONE(aio_error_test, out.retval);
}

/*
 * Asynchronous write test procedure.
 *
 * @param rpcs            RPC server
 * @param s                 a socket to be user for receiving
 * @param signum            signal to be used as notification event
 * @param buf               buffer for data to be sent
 * @param buflen            data length
 * @param diag              location for diagnostic message
 * @param diag_len          maximum length of the diagnostic message
 *
 * @return number of sent bytes or -1 in the case of failure
 */
int
rpc_aio_write_test(rcf_rpc_server *rpcs,
                   int s, rpc_signum signum,
                   char *buf, int buflen,
                   char *diag, int diag_len)
{
    tarpc_aio_write_test_in  in;
    tarpc_aio_write_test_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.s = s;
    in.signum = signum;
    in.buf.buf_val = buf;
    in.buf.buf_len = buflen;
    in.diag.diag_val = diag;
    in.diag.diag_len = diag_len;

    rcf_rpc_call(rpcs, _aio_write_test,
                 &in,  (xdrproc_t)xdr_tarpc_aio_write_test_in,
                 &out, (xdrproc_t)xdr_tarpc_aio_write_test_out);

    if (RPC_IS_CALL_OK(rpcs))
        memcpy(diag, out.diag.diag_val, out.diag.diag_len);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(aio_write_test, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): "
                 "aio_write_test(%d, %s, %p, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 s, signum_rpc2str(signum), buf, buflen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VAL(aio_write_test, out.retval);
}

/**
 * Susending on asynchronous events test procedure.
 *
 * @param rpcs            RPC server
 * @param s                 a socket to be user for receiving
 * @param s_aux             dummy socket
 * @param signum            signal to be used as notification event
 * @param timeout           timeout for blocking on ai_suspend() in seconds
 * @param buf               buffer for received data
 * @param buflen            buffer length to be passed to the read
 * @param diag              location for diagnostic message
 * @param diag_len          maximum length of the diagnostic message
 *
 * @return number of received bytes or -1 in the case of failure
 */
int
rpc_aio_suspend_test(rcf_rpc_server *rpcs,
                     int s, int s_aux, rpc_signum signum,
                     int timeout, char *buf, int buflen,
                     char *diag, int diag_len)
{
    tarpc_aio_suspend_test_in  in;
    tarpc_aio_suspend_test_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.s = s;
    in.s_aux = s_aux;
    in.signum = signum;
    in.t = timeout;
    in.buf.buf_val = buf;
    in.buf.buf_len = buflen;
    in.diag.diag_val = diag;
    in.diag.diag_len = diag_len;

    rcf_rpc_call(rpcs, _aio_suspend_test,
                 &in,  (xdrproc_t)xdr_tarpc_aio_suspend_test_in,
                 &out, (xdrproc_t)xdr_tarpc_aio_suspend_test_out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);

        memcpy(diag, out.diag.diag_val, out.diag.diag_len);
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(aio_suspend_test, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): "
                 "aio_suspend_test(%d, %d, %s, %d, %p, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 s, s_aux, signum_rpc2str(signum), timeout, buf, buflen,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VAL(aio_suspend_test, out.retval);
}

