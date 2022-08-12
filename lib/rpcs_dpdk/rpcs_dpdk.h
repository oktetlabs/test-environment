/** @file
 * @brief RPC for DPDK
 *
 * Definitions necessary for RPC implementation
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_LIB_RPCS_DPDK_H__
#define __TE_LIB_RPCS_DPDK_H__

#include "te_rpc_errno.h"
#include "rpc_dpdk_defs.h"

#define RPC_TYPE_NS_RTE_MEMPOOL "rte_mempool"
#define RPC_TYPE_NS_RTE_MBUF    "rte_mbuf"
#define RPC_TYPE_NS_RTE_RING    "rte_ring"
#define RPC_TYPE_NS_RTE_FLOW    "rte_flow"

/**
 * Translate negative errno from host to RPC.
 */
static inline void
neg_errno_h2rpc(int *retval)
{
    if (*retval < 0)
        *retval = -errno_h2rpc(-*retval);
}

#endif /* __TE_LIB_RPCS_DPDK_H__ */
