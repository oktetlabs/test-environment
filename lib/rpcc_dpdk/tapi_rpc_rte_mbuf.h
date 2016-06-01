/** @file
 * @brief RPC client API for DPDK mbuf library
 *
 * RPC client API for DPDK mbuf library functions
 *
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
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_RTE_MBUF_H__
#define __TE_TAPI_RPC_RTE_MBUF_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_rte_mbuf TAPI for RTE MBUF API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * @b rte_pktmbuf_pool_create() RPC
 *
 * @param name            The name of the mbuf pool
 * @param n               The number of elements in the mbuf pool
 * @param cache_size      Size of the per-core object cache
 * @param priv_size       Size of application private are between the rte_mbuf
 *                        structure and the data buffer
 * @param data_room_size  Size of data buffer in each mbuf, including
 *                        RTE_PKTMBUF_HEADROOM
 * @param socket_id       The socket identifier where the memory should
 *                        be allocated
 *
 * @return RTE mempool pointer on success; jumps out when pointer is @c NULL
 */
extern rpc_rte_mempool_p rpc_rte_pktmbuf_pool_create(rcf_rpc_server *rpcs,
                                                     const char *name,
                                                     uint32_t n,
                                                     uint32_t cache_size,
                                                     uint16_t priv_size,
                                                     uint16_t data_room_size,
                                                     int socket_id);

/**@} <!-- END te_lib_rpc_rte_mbuf --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_MBUF_H__ */
