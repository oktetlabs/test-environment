/** @file
 * @brief RPC for DPDK MEMPOOL
 *
 * RPC routines implementation to call DPDK (rte_mempool_*) functions
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC DPDK MEMPOOL"

#include "te_config.h"

#include "rte_config.h"
#include "rte_mempool.h"

#include "rpc_server.h"
#include "rpcs_dpdk.h"
#include "te_errno.h"

TARPC_FUNC(rte_mempool_lookup, {},
{
    struct rte_mempool *mp;

    MAKE_CALL(mp = func(in->name));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        out->retval = RCF_PCH_MEM_INDEX_ALLOC(mp, ns);
    });
}
)

TARPC_FUNC(rte_mempool_in_use_count, {},
{
    struct rte_mempool *mp = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        mp = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->mp, ns);
    });

    MAKE_CALL(out->retval = func(mp));
}
)

static void
rte_mempool_free_iterator(struct rte_mempool *mp,
                          void               *arg)
{
    UNUSED(arg);

    rte_mempool_free(mp);
}

TARPC_FUNC_STANDALONE(rte_mempool_free, {},
{
    if (in->free_all)
    {
        MAKE_CALL(rte_mempool_walk(rte_mempool_free_iterator, NULL));
    }
    else
    {
        struct rte_mempool *mp = NULL;

        RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
            mp = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->mp, ns);
        });

        MAKE_CALL(rte_mempool_free(mp));
    }
}
)
