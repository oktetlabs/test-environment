/** @file
 * @brief RPC for TAD RTE ring
 *
 * RPC routines implementation to use RTE ring API
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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

#define TE_LGR_USER     "RPC RTE ring"

#include "te_config.h"
#ifdef HAVE_PACKAGE_H
#include "package.h"
#endif

#include "rte_config.h"
#include "rte_ring.h"

#include "logger_api.h"

#include "rpc_server.h"
#include "rpcs_dpdk.h"
#include "te_errno.h"

static int
tarpc_rte_ring_flags2rte(unsigned rpc_flags, unsigned *rte_flags)
{
    uint32_t    rte_tmp = 0;

    if ((rpc_flags & (1U << TARPC_RTE_RING_F_SP_ENQ)) != 0)
        rte_tmp |= RING_F_SP_ENQ;

    if ((rpc_flags & (1U << TARPC_RTE_RING_F_SC_DEQ)) != 0)
        rte_tmp |= RING_F_SC_DEQ;

    *rte_flags = rte_tmp;

    return (1);
}

TARPC_FUNC_STATIC(rte_ring_create, {},
{
    struct rte_ring    *ring = NULL;
    unsigned            flags;

    if (tarpc_rte_ring_flags2rte(in->flags, &flags) != 1)
        goto out;

    MAKE_CALL(ring = func(in->name, in->count, in->socket_id, flags));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_RING, {
        out->retval = RCF_PCH_MEM_INDEX_ALLOC(ring, ns);
    });

out:
    ;
}
)

TARPC_FUNC_STATIC(rte_ring_free, {},
{
    struct rte_ring    *ring = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_RING, {
        ring = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->ring, ns);
    });

    MAKE_CALL(func(ring));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_RING, {
        RCF_PCH_MEM_INDEX_FREE(in->ring, ns);
    });
}
)

TARPC_FUNC_STANDALONE(rte_ring_enqueue_mbuf, {},
{
    struct rte_ring    *ring = NULL;
    struct rte_mbuf    *m = NULL;
    te_errno            err;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_RING, {
        ring = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->ring, ns);
    });

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(err = rte_ring_enqueue(ring, (void *)m));

    neg_errno_h2rpc(&err);

    out->retval = -err;
}
)

TARPC_FUNC_STANDALONE(rte_ring_dequeue_mbuf, {},
{
    struct rte_ring    *ring = NULL;
    struct rte_mbuf    *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_RING, {
        ring = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->ring, ns);
    });

    MAKE_CALL((void)rte_ring_dequeue(ring, (void **)&m));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        out->retval = RCF_PCH_MEM_INDEX_ALLOC(m, ns);
    });
}
)
