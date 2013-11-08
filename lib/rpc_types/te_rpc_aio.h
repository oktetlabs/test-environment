/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from aio.h.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_AIO_H__
#define __TE_RPC_AIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Incorrect address of the callback function should be used */
#define AIO_WRONG_CALLBACK      "aio_wrong_callback"


/** TA-independent operation code for lio_listio function */
typedef enum rpc_lio_opcode {
    RPC_LIO_READ,
    RPC_LIO_WRITE,
    RPC_LIO_NOP,
    RPC_LIO_UNKNOWN
} rpc_lio_opcode; 

/** Convert RPC lio_listio opcode to string */
extern const char * lio_opcode_rpc2str(rpc_lio_opcode opcode);

/** Convert RPC lio_listio opcode to native one */
extern int lio_opcode_rpc2h(rpc_lio_opcode opcode);

/** Convert native lio_listio opcode to RPC one */
extern rpc_lio_opcode lio_opcode_h2rpc(int opcode);


/** TA-independent modes for lio_listio function */
typedef enum rpc_lio_mode {
    RPC_LIO_WAIT,
    RPC_LIO_NOWAIT,
    RPC_LIO_MODE_UNKNOWN
} rpc_lio_mode; 

/** Convert RPC lio_listio mode to string */
extern const char * lio_mode_rpc2str(rpc_lio_mode mode);

/** Convert RPC lio_listio option to native one */
extern int lio_mode_rpc2h(rpc_lio_mode mode);

/** Convert native lio_listio mode to RPC one */
extern rpc_lio_mode lio_mode_h2rpc(int mode);


/** TA-independent return values for aio_cancel function */
typedef enum rpc_aio_cancel_retval {
    RPC_AIO_CANCELED,
    RPC_AIO_NOTCANCELED,
    RPC_AIO_ALLDONE,
    RPC_AIO_UNKNOWN
} rpc_aio_cancel_retval; 

/** Convert RPC aio_cancel return to string */
extern const char * aio_cancel_retval_rpc2str(rpc_aio_cancel_retval ret);

/** Convert RPC aio_cancel return value to native one */
extern int aio_cancel_retval_rpc2h(rpc_aio_cancel_retval ret);

/** Convert native aio_cancel return to RPC one */
extern rpc_aio_cancel_retval aio_cancel_retval_h2rpc(int ret);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_AIO_H__ */
