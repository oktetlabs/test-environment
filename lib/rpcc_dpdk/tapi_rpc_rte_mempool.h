/** @file
 * @brief RPC client API for DPDK mempool library
 *
 * Definition of RPC client API for DPDK mempool library functions
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in
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
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_RTE_MEMPOOL_H__
#define __TE_TAPI_RPC_RTE_MEMPOOL_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_rte_mempool TAPI for RTE MEMPOOL API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * @b rte_mempool_in_use_count() RPC
 *
 * @param mp            RTE mempool pointer
 *
 * @return The number of elements which have been allocated from the mempool
 */
extern unsigned int rpc_rte_mempool_in_use_count(rcf_rpc_server     *rpcs,
                                                 rpc_rte_mempool_p   mp);

/**
 * @b rte_mempool_free() RPC
 *
 * @param mp            Mempool to be freed
 */
extern void rpc_rte_mempool_free(rcf_rpc_server     *rpcs,
                                 rpc_rte_mempool_p   mp);

/**
 * @b rte_mempool_free_all() RPC
 * The function destroys ALL mempools by means of
 * @b rte_mempool_walk() usage at RPC server side
 */
extern void rpc_rte_mempool_free_all(rcf_rpc_server *rpcs);

/**@} <!-- END te_lib_rpc_rte_mempool --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_MEMPOOL_H__ */
