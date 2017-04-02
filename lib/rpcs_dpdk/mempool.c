/** @file
 * @brief RPC for DPDK MEMPOOL
 *
 * RPC routines implementation to call DPDK (rte_mempool_*) functions
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC DPDK MEMPOOL"

#include "te_config.h"
#ifdef HAVE_PACKAGE_H
#include "package.h"
#endif

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
