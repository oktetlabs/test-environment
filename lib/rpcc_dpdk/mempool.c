/** @file
 * @brief RPC client API for DPDK mempool library
 *
 * Implementation of RPC client API for DPDK mempool library functions
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
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#include "te_config.h"

#include "tapi_rpc_internal.h"
#include "tapi_rpc_rte_mempool.h"
#include "rpcc_dpdk.h"
#include "tapi_test_log.h"

#include "tarpc.h"

unsigned int
rpc_rte_mempool_in_use_count(rcf_rpc_server     *rpcs,
                             rpc_rte_mempool_p   mp)
{
    tarpc_rte_mempool_in_use_count_in  in;
    tarpc_rte_mempool_in_use_count_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.mp = (tarpc_rte_mempool)mp;

    rcf_rpc_call(rpcs, "rte_mempool_in_use_count", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_mempool_in_use_count, RPC_PTR_FMT, "%u",
                 RPC_PTR_VAL(in.mp), out.retval);

    TAPI_RPC_OUT(rte_mempool_in_use_count, FALSE);

    return (out.retval);
}

static void
rpc_rte_mempool_free_custom(rcf_rpc_server     *rpcs,
                            te_bool             free_all,
                            rpc_rte_mempool_p   mp)
{
    tarpc_rte_mempool_free_in  in;
    tarpc_rte_mempool_free_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.free_all = (tarpc_bool)free_all;
    in.mp = (tarpc_rte_mempool)mp;

    rcf_rpc_call(rpcs, "rte_mempool_free", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_mempool_free,
                 RPC_PTR_FMT "%s", "",
                 RPC_PTR_VAL(in.mp), (free_all) ? "(ALL)" : "");

    RETVAL_VOID(rte_mempool_free);
}

void
rpc_rte_mempool_free(rcf_rpc_server     *rpcs,
                     rpc_rte_mempool_p   mp)
{
    rpc_rte_mempool_free_custom(rpcs, FALSE, mp);
}

void
rpc_rte_mempool_free_all(rcf_rpc_server *rpcs)
{
    rpc_rte_mempool_free_custom(rpcs, TRUE, RPC_NULL);
}
