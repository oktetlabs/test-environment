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
#include "te_rpc_fcntl.h"
#include "tapi_rpc_aio.h"
#include "tapi_rpc_misc.h"

/**
 * Allocate a AIO control block.
 *
 * @param rpcs   RPC server handle
 *
 * @return AIO control block address, otherwise @b NULL is returned on error
 */
rpc_aiocb_p 
rpc_create_aiocb(rcf_rpc_server *rpcs)
{
    tarpc_create_aiocb_in  in;
    tarpc_create_aiocb_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(create_aiocb, RPC_NULL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    rcf_rpc_call(rpcs, "create_aiocb", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): create_aiocb() -> %u (%s)",
                 rpcs->ta, rpcs->name,
                 out.cb, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(create_aiocb, out.cb);
}

/**
 * Destroy a specified AIO control block.
 *
 * @param rpcs     RPC server handle
 * @param cb       control block to be deleted
 */
void 
rpc_delete_aiocb(rcf_rpc_server *rpcs, rpc_aiocb_p cb)
{
    tarpc_delete_aiocb_in  in;
    tarpc_delete_aiocb_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(delete_aiocb);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.cb = (tarpc_aiocb_t)cb;

    rcf_rpc_call(rpcs, "delete_aiocb", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): delete_aiocb(%u) -> (%s)",
                 rpcs->ta, rpcs->name,
                 cb, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(delete_aiocb);
}

/**
 * Fill a specified AIO control block.
 *
 * @param rpcs     RPC server handle
 * @param cb       control block to be updated
 * @param fildes   file descriptor
 * @param opcode   lio_listio operation code
 * @param reqprio  request priority
 * @param buf      pre-allocated memory buffer
 * @param nbytes   buffer length
 * @param sigevent notification mode description
 */
void 
rpc_fill_aiocb(rcf_rpc_server *rpcs, rpc_aiocb_p cb, 
               int fildes, rpc_lio_opcode opcode,
               int reqprio, rpc_ptr buf, size_t nbytes,
               const tarpc_sigevent *sigevent)
{
    tarpc_fill_aiocb_in  in;
    tarpc_fill_aiocb_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(fill_aiocb);
    }
    
    if (sigevent == NULL)
    {
        rpcs->_errno = TE_EINVAL;
        ERROR("NULL pointer to sigevent is passed to rpc_fill_aiocb()");
        return;
    }
    
    in.sigevent = *sigevent;
    in.sigevent.function = strdup(in.sigevent.function == NULL ? "" 
                                  : in.sigevent.function);
    
    if (in.sigevent.function == NULL)
    {
        rpcs->_errno = TE_ENOMEM;
        ERROR("Out of memory!");
        return;
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.cb = (tarpc_aiocb_t)cb;
    in.fildes = fildes;
    in.lio_opcode = opcode;
    in.reqprio = reqprio;
    in.buf = buf;
    in.nbytes = nbytes;

    rcf_rpc_call(rpcs, "fill_aiocb", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): fill_aiocb(%u, %d, %s, %d, %u, %u, %s) "
                 "-> (%s)",
                 rpcs->ta, rpcs->name,
                 cb, fildes, lio_opcode_rpc2str(opcode), 
                 reqprio, buf, nbytes, tarpc_sigevent2str(sigevent),
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_VOID(fill_aiocb);
}

/**
 * Request asynchronous read operation.
 *
 * @param rpcs     RPC server handle
 * @param cb       control block 
 *
 * @return 0 (success) or -1 (failure)
 */
int 
rpc_aio_read(rcf_rpc_server *rpcs, rpc_aiocb_p cb)
{
    tarpc_aio_read_in  in;
    tarpc_aio_read_out out;

    rcf_rpc_op op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(aio_read, -1);
    }
    
    op = rpcs->op;

    in.cb = (tarpc_aiocb_t)cb;

    rcf_rpc_call(rpcs, "aio_read", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(aio_read, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: aio_read(%u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), cb, 
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_ZERO_INT(aio_read, out.retval);
}

/**
 * Request asynchronous write operation.
 *
 * @param rpcs     RPC server handle
 * @param cb       control block 
 *
 * @return 0 (success) or -1 (failure)
 */
int 
rpc_aio_write(rcf_rpc_server *rpcs, rpc_aiocb_p cb)
{
    tarpc_aio_write_in  in;
    tarpc_aio_write_out out;

    rcf_rpc_op op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(aio_write, -1);
    }

    op = rpcs->op;

    in.cb = (tarpc_aiocb_t)cb;

    rcf_rpc_call(rpcs, "aio_write", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(aio_write, out.retval);
    
    TAPI_RPC_LOG("RPC (%s,%s)%s: aio_write(%u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), cb, 
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_ZERO_INT(aio_write, out.retval);
}

/**
 * Retrieve final return status for asynchronous I/O request.
 *
 * @param rpcs     RPC server handle
 * @param cb       control block 
 *
 * @return Return status of AIO request
 *
 * @note The function converting OS errno to OS-independent one is also 
 * applied to value returned by aio_return() on RPC server. The result of 
 * the conversion is stored as errno in RPC server structure. This is 
 * necessary to obtain correct aio_return() result when it is called for 
 * failre request.
 */
ssize_t 
rpc_aio_return(rcf_rpc_server *rpcs, rpc_aiocb_p cb)
{
    tarpc_aio_return_in  in;
    tarpc_aio_return_out out;

    rcf_rpc_op op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(aio_return, -1);
    }

    op = rpcs->op;

    in.cb = (tarpc_aiocb_t)cb;

    rcf_rpc_call(rpcs, "aio_return", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(aio_return, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s)%s: aio_return(%u) -> %u (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), cb, 
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(aio_return, out.retval);
}

/**
 * Get status of the asynchronous I/O request.
 *
 * @param rpcs     RPC server handle
 * @param cb       control block 
 *
 * @return OS-independent error code
 */
int 
rpc_aio_error(rcf_rpc_server *rpcs, rpc_aiocb_p cb)
{
    tarpc_aio_error_in  in;
    tarpc_aio_error_out out;

    rcf_rpc_op op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(aio_error, 0);
    }

    op = rpcs->op;

    in.cb = (tarpc_aiocb_t)cb;

    rcf_rpc_call(rpcs, "aio_error", &in, &out);

    CHECK_RETVAL_VAR(aio_error, out.retval, out.retval < 0, 
                     TE_RC(TE_RPC, TE_EFAIL));

    TAPI_RPC_LOG("RPC (%s,%s)%s: aio_error(%u) -> %s (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), cb, 
                 errno_rpc2str(out.retval), 
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(aio_error, out.retval);
}

/**
 * Cancel asynchronous I/O request(s) corresponding to the file descriptor.
 *
 * @param rpcs     RPC server handle
 * @param fd       file descriptor
 * @param cb       control block or NULL (in this case all AIO requests
 *                 are cancelled)
 *
 * @return Status code
 * @retval AIO_CANCELED         all requests are successfully cancelled
 * @retval AIO_NOTCANCELED      at least one request is not cancelled
 * @retval AIO_ALLDONE          all requests are completed before this call
 * @retval -1                   error occured
 */
int 
rpc_aio_cancel(rcf_rpc_server *rpcs, int fd, rpc_aiocb_p cb)
{
    tarpc_aio_cancel_in  in;
    tarpc_aio_cancel_out out;

    rcf_rpc_op op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(aio_cancel, -1);
    }

    op = rpcs->op;

    in.cb = (tarpc_aiocb_t)cb;
    in.fd = fd;

    rcf_rpc_call(rpcs, "aio_cancel", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(aio_cancel, out.retval);
    
    TAPI_RPC_LOG("RPC (%s,%s)%s: aio_cancel(%u, %d) -> %s (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), cb, fd, 
                 aio_cancel_retval_rpc2str(out.retval),
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(aio_cancel, out.retval);
}

/**
 * Do a sync on all outstanding asynchronous I/O operations associated 
 * with cb->aio_fildes.
 *
 * @param rpcs     RPC server handle
 * @param op       operation (RPC_O_SYNC or RPC_O_DSYNC)
 * @param cb       control block with file descriptor and
 *                 notification mode description
 *
 * @return 0 (success) or -1 (failure)
 */
int 
rpc_aio_fsync(rcf_rpc_server *rpcs, 
              rpc_fcntl_flags op, rpc_aiocb_p cb)
{
    tarpc_aio_fsync_in  in;
    tarpc_aio_fsync_out out;

    rcf_rpc_op rpc_op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(aio_fsync, -1);
    }

    rpc_op = rpcs->op;

    in.cb = (tarpc_aiocb_t)cb;
    in.op = op;

    rcf_rpc_call(rpcs, "aio_fsync", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(aio_fsync, out.retval);
    
    TAPI_RPC_LOG("RPC (%s,%s)%s: aio_fsync(%s, %u) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(rpc_op),
                 fcntl_flags_rpc2str(op), cb, 
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_ZERO_INT(aio_fsync, out.retval);
}
              
/**
 * Suspend the calling process until at least one of the asynchronous I/O 
 * requests in the list cblist  of  length n have  completed, a signal is 
 * delivered, or timeout is not NULL and the time interval it indicates 
 * has passed.
 *
 * @param rpcs     RPC server handle
 * @param cblist   list of control blocks corresponding to AIO requests
 * @param n        number of elements in cblist
 * @param timeout  timeout of NULL
 *
 * @return 0 (success) or -1 (failure)
 */
int 
rpc_aio_suspend(rcf_rpc_server *rpcs, const rpc_aiocb_p *cblist,
                int n, const struct timespec *timeout)
{
    tarpc_aio_suspend_in  in;
    tarpc_aio_suspend_out out;
    struct tarpc_timespec tv;

    rcf_rpc_op op;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(aio_suspend, -1);
    }

    op = rpcs->op;

    in.cb.cb_val = (tarpc_aiocb_t *)cblist;
    in.cb.cb_len = cblist == NULL ? 0 : n;
    in.n = n;

    if (timeout != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        tv.tv_sec = timeout->tv_sec;
        tv.tv_nsec = timeout->tv_nsec;
        in.timeout.timeout_len = 1;
        in.timeout.timeout_val = &tv;
    }

    rcf_rpc_call(rpcs, "aio_suspend", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(aio_suspend, out.retval);
    
    TAPI_RPC_LOG("RPC (%s,%s)%s: aio_suspend(0x%X, %d, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 cblist, n, 
                 timespec2str(timeout),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_ZERO_INT(aio_suspend, out.retval);
}
                
/**
 * Initiate a list of I/O requests with a single function call.
 *
 * @param rpcs     RPC server handle
 * @param mode     if RPC_LIO_WAIT, return after completion of all requests;
 *                 if RPC_LI_NOWAIT, requrn after requests queueing
 * @param cblist   list of control blocks corresponding to AIO requests
 * @param nent     number of elements in cblist
 * @param sigevent notification mode description
 *
 * @return 0 (success) or -1 (failure)
 */
int 
rpc_lio_listio(rcf_rpc_server *rpcs, 
               rpc_lio_mode mode, const rpc_aiocb_p *cblist,
               int nent, const tarpc_sigevent *sigevent)
{
    tarpc_lio_listio_in  in;
    tarpc_lio_listio_out out;

    rcf_rpc_op op;
    
    tarpc_sigevent ev;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(lio_listio, -1);
    }

    op = rpcs->op;

    in.cb.cb_val = (tarpc_aiocb_t *)cblist;
    in.cb.cb_len = cblist == NULL ? 0 : nent;
    in.nent = nent;
    in.mode = mode;
    
    memset(&ev, 0, sizeof(ev));
    if (sigevent != NULL)
    {
        in.sig.sig_len = 1;
        in.sig.sig_val = &ev;
        ev = *sigevent;
        ev.function = strdup(ev.function == NULL ? "" : ev.function);
        if (ev.function == NULL)
        {
            ERROR("Out of memory!");
            RETVAL_INT(lio_listio, -1);
        }
    }

    rcf_rpc_call(rpcs, "lio_listio", &in, &out);
    
    free(ev.function);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(lio_listio, out.retval);
    
    TAPI_RPC_LOG("RPC (%s,%s)%s: lio_listio(%s, 0x%X, %d, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 lio_mode_rpc2str(mode), 
                 cblist, nent, tarpc_sigevent2str(sigevent),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_ZERO_INT(lio_listio, out.retval);
}
