/** @file
 * @brief RPC client API for DPDK mbuf library
 *
 * RPC client API for DPDK mbuf library functions
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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

#include "te_config.h"

#include "tapi_rpc_internal.h"
#include "tapi_rpc_rte.h"
#include "tapi_mem.h"

#include "tarpc.h"


rpc_rte_mempool_p
rpc_rte_pktmbuf_pool_create(rcf_rpc_server *rpcs,
                            const char *name, uint32_t n, uint32_t cache_size,
                            uint16_t priv_size, uint16_t data_room_size,
                            int socket_id)
{
    tarpc_rte_pktmbuf_pool_create_in    in;
    tarpc_rte_pktmbuf_pool_create_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.name = tapi_strdup(name);
    in.n = n;
    in.cache_size = cache_size;
    in.priv_size = priv_size;
    in.data_room_size = data_room_size;
    in.socket_id = socket_id;

    rcf_rpc_call(rpcs, "rte_pktmbuf_pool_create", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_pktmbuf_pool_create, out.retval);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_pool_create,
                 "%s, %u, %u, %hu, %hu, %d", RPC_PTR_FMT,
                 in.name, in.n, in.cache_size, in.priv_size, in.data_room_size,
                 in.socket_id, RPC_PTR_VAL(out.retval));

    free(in.name);

    RETVAL_RPC_PTR(rte_pktmbuf_pool_create, out.retval);
}

rpc_rte_mbuf_p
rpc_rte_pktmbuf_alloc(rcf_rpc_server *rpcs,
                      rpc_rte_mempool_p mp)
{
    tarpc_rte_pktmbuf_alloc_in  in;
    tarpc_rte_pktmbuf_alloc_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.mp = (tarpc_rte_mempool)mp;

    rcf_rpc_call(rpcs, "rte_pktmbuf_alloc", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_pktmbuf_alloc, out.retval);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_alloc, RPC_PTR_FMT, RPC_PTR_FMT,
                 RPC_PTR_VAL(in.mp), RPC_PTR_VAL(out.retval));

    RETVAL_RPC_PTR(rte_pktmbuf_alloc, out.retval);
}
