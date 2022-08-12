/** @file
 * @brief RPC client API for DPDK mempool library
 *
 * Implementation of RPC client API for DPDK mempool library functions
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#include "te_config.h"

#include "tapi_rpc_internal.h"
#include "tapi_rpc_rte_mempool.h"
#include "rpcc_dpdk.h"
#include "tapi_test_log.h"
#include "tapi_mem.h"

#include "tarpc.h"

rpc_rte_mempool_p
rpc_rte_mempool_lookup(rcf_rpc_server *rpcs,
                       const char     *name)
{
    tarpc_rte_mempool_lookup_in  in;
    tarpc_rte_mempool_lookup_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.name = tapi_strdup(name);

    rcf_rpc_call(rpcs, "rte_mempool_lookup", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_mempool_lookup, out.retval);

    TAPI_RPC_LOG(rpcs, rte_mempool_lookup,
                 "%s", RPC_PTR_FMT,
                 in.name, RPC_PTR_VAL(out.retval));

    free(in.name);

    RETVAL_RPC_PTR(rte_mempool_lookup, out.retval);
}

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
