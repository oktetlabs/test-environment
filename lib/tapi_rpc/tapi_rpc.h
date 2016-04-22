/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for all remote calls.
 *
 * Most functions have the same prototype and semantics of parameters
 * and return code as Linux implementation of Berkeley Socket API
 * functions.
 *
 * The only exception is the first parameter - RPC server handle created
 * using RCF RPC library calls.
 *
 * If NULL pointer is provided to API call, no data are read/write
 * from/to.  The pointer is just passed to RPC interface to call
 * remote function with NULL argument.
 *
 * *_gen functions provide more generic interface for test purposes.
 * Additional arguments specify real length of the provided buffers.
 * Such functionality is required to verify that called function
 * writes nothing across provided buffer boundaries.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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

#ifndef __TE_TAPI_RPC_H__
#define __TE_TAPI_RPC_H__

#include "te_defs.h"
#include "rcf_rpc.h"
#include "tapi_rpc_socket.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_ifnameindex.h"
#include "tapi_rpc_netdb.h"
#include "tapi_rpc_aio.h"
#include "tapi_rpc_winsock2.h"
#include "tapi_rpc_dlfcn.h"
#include "tapi_rpc_misc.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get readability (there are data to read) or writability (it is allowed
 * to write) of a particular socket.
 *
 * @param answer     pointer a the answer
 * @param rpcs       RPC server handle
 * @param s          socket to be checked
 * @param timeout    timeout in seconds
 * @param type       type of checking: "READ" or "WRITE"
 *
 * @return status code
 */
extern int rpc_get_rw_ability(te_bool *answer, rcf_rpc_server *rpcs,
                              int s, int timeout, char *type);

/**
 * Check data exchange on the pipe.
 *
 * @param rpcs     RPC server handle
 * @param pipefds  Pipe file descriptors
 *
 * @return @c 0 on success or @c -1 on failure
 */
extern int tapi_check_pipe(rcf_rpc_server *rpcs, int *pipefds);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_H__ */
