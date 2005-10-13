/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of asynchronous input/output
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

#ifndef __TE_TAPI_RPC_AIO_H__
#define __TE_TAPI_RPC_AIO_H__

#include "rcf_rpc.h"
#include "te_rpc_aio.h"

/**
 * Allocate a AIO control block.
 *
 * @param rpcs   RPC server handle
 *
 * @return AIO control block address, otherwise @b NULL is returned on error
 */
extern rpc_aiocb_p rpc_create_aiocb(rcf_rpc_server *rpcs);

/**
 * Destroy a specified AIO control block.
 *
 * @param rpcs     RPC server handle
 * @param cb       control block to be deleted
 */
extern void rpc_delete_aiocb(rcf_rpc_server *rpcs, rpc_aiocb_p cb);

/**
 * Delete AIO control block in cleanup part of the test. 
 *
 * @param _rpcs     RPC server handle
 * @param _cb       AIO control block
 */
#define CLEANUP_RPC_DELETE_AIOCB(_rpcs, _cb) \
    do {                                                \
        if ((_cb != RPC_NULL) >= 0 && (_rpcs) != NULL)  \
        {                                               \
            RPC_AWAIT_IUT_ERROR(_rpcs);                 \
            rpc_delete_aiocb(_rpcs, _cb);               \
            if (!RPC_IS_CALL_OK(_rpcs))                 \
                MACRO_TEST_ERROR;                       \
            _cb = RPC_NULL;                             \
        }                                               \
    } while (0)


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
extern void rpc_fill_aiocb(rcf_rpc_server *rpcs, rpc_aiocb_p cb, 
                           int fildes, rpc_lio_opcode opcode,
                           int reqprio, rpc_ptr buf, size_t nbytes,
                           const tarpc_sigevent *sigevent);

/**
 * Request asynchronous read operation.
 *
 * @param rpcs     RPC server handle
 * @param cb       control block 
 *
 * @return 0 (success) or -1 (failure)
 */
extern int rpc_aio_read(rcf_rpc_server *rpcs, rpc_aiocb_p cb);

/**
 * Request asynchronous write operation.
 *
 * @param rpcs     RPC server handle
 * @param cb       control block 
 *
 * @return 0 (success) or -1 (failure)
 */
extern int rpc_aio_write(rcf_rpc_server *rpcs, rpc_aiocb_p cb);

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
extern ssize_t rpc_aio_return(rcf_rpc_server *rpcs, rpc_aiocb_p cb);

/**
 * Get status of the asynchronous I/O request.
 *
 * @param rpcs     RPC server handle
 * @param cb       control block 
 *
 * @return OS-independent error code
 */
extern int rpc_aio_error(rcf_rpc_server *rpcs, rpc_aiocb_p cb);

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
extern int rpc_aio_cancel(rcf_rpc_server *rpcs, int fd, rpc_aiocb_p cb);

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
extern int rpc_aio_fsync(rcf_rpc_server *rpcs, 
                         rpc_fcntl_flags op, rpc_aiocb_p cb);

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
extern int rpc_aio_suspend(rcf_rpc_server *rpcs, const rpc_aiocb_p *cblist,
                           int n, const struct timespec *timeout);

/**
 * Initiate a list of I/O requests with a single function call.
 *
 * @param rpcs     RPC server handle
 * @param mode     if RPC_LIO_WAIT, return after completion of all requests;
 *                 if RPC_LIO_NOWAIT, requrn after requests queueing
 * @param cblist   list of control blocks corresponding to AIO requests
 * @param nent     number of elements in cblist
 * @param sigevent notification mode description
 *
 * @return 0 (success) or -1 (failure)
 */
extern int rpc_lio_listio(rcf_rpc_server *rpcs, 
                          rpc_lio_mode mode, const rpc_aiocb_p *cblist,
                          int nent, const tarpc_sigevent *sigevent);

#endif /* !__TE_TAPI_RPC_AIO_H__ */
