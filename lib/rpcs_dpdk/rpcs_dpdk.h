/** @file
 * @brief RPC for DPDK
 *
 * Definitions necessary for RPC implementation
 *
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
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_LIB_RPCS_DPDK_H__
#define __TE_LIB_RPCS_DPDK_H__

#include "te_rpc_errno.h"

#define RPC_TYPE_NS_RTE_MEMPOOL "rte_mempool"
#define RPC_TYPE_NS_RTE_MBUF    "rte_mbuf"

/**
 * Translate negative errno from host to RPC.
 */
static inline void
neg_errno_h2rpc(int *retval)
{
    if (*retval < 0)
        *retval = -errno_h2rpc(-*retval);
}

#define RPC_RTE_ETH_NAME_MAX_LEN 32

#define RPC_RSS_HASH_KEY_LEN_DEF 40

#endif /* __TE_LIB_RPCS_DPDK_H__ */
