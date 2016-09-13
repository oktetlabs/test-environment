/** @file
 * @brief RPC client API for DPDK ring library
 *
 * RPC client API for DPDK ring library functions (implementation)
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
#include "tapi_rpc_rte_ring.h"
#include "tapi_mem.h"
#include "log_bufs.h"
#include "rpcc_dpdk.h"
#include "tapi_test_log.h"

#include "tarpc.h"

rpc_rte_ring_p
rpc_rte_ring_create(rcf_rpc_server *rpcs,
                    const char     *name,
                    unsigned        count,
                    int             socket_id,
                    unsigned        flags)
{
    tarpc_rte_ring_create_in    in;
    tarpc_rte_ring_create_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.name = tapi_strdup(name);
    in.count = count;
    in.socket_id = socket_id;
    in.flags = flags;

    rcf_rpc_call(rpcs, "rte_ring_create", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_ring_create, out.retval);

    TAPI_RPC_LOG(rpcs, rte_ring_create, "%s, %u, %d", RPC_PTR_FMT,
                 in.name, in.count, in.socket_id, RPC_PTR_VAL(out.retval));

    free(in.name);

    RETVAL_RPC_PTR(rte_ring_create, out.retval);
}

void
rpc_rte_ring_free(rcf_rpc_server *rpcs,
                       rpc_rte_ring_p  ring)
{
    tarpc_rte_ring_free_in  in;
    tarpc_rte_ring_free_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.ring = (tarpc_rte_ring)ring;

    rcf_rpc_call(rpcs, "rte_ring_free", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_ring_free, RPC_PTR_FMT, "", RPC_PTR_VAL(in.ring));

    RETVAL_VOID(rte_ring_free);
}
