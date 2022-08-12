/** @file
 * @brief RPC client API for DPDK mempool library
 *
 * Definition of RPC client API for DPDK mempool library functions
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TAPI_RPC_RTE_MEMPOOL_H__
#define __TE_TAPI_RPC_RTE_MEMPOOL_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_rte_mempool TAPI for RTE MEMPOOL API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * @b rte_mempool_lookup() RPC
 *
 * @param name          RTE mempool name
 *
 * @return RTE mempool pointer on success; jumps out when pointer is @c NULL
 */
extern rpc_rte_mempool_p rpc_rte_mempool_lookup(rcf_rpc_server *rpcs,
                                                const char     *name);

/**
 * @b rte_mempool_in_use_count() RPC
 *
 * @param mp            RTE mempool pointer
 *
 * @return The number of elements which have been allocated from the mempool
 */
extern unsigned int rpc_rte_mempool_in_use_count(rcf_rpc_server     *rpcs,
                                                 rpc_rte_mempool_p   mp);

/**
 * @b rte_mempool_free() RPC
 *
 * @param mp            Mempool to be freed
 */
extern void rpc_rte_mempool_free(rcf_rpc_server     *rpcs,
                                 rpc_rte_mempool_p   mp);

/**
 * @b rte_mempool_free_all() RPC
 * The function destroys ALL mempools by means of
 * @b rte_mempool_walk() usage at RPC server side
 */
extern void rpc_rte_mempool_free_all(rcf_rpc_server *rpcs);

/**@} <!-- END te_lib_rpc_rte_mempool --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_MEMPOOL_H__ */
