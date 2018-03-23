/** @file
 * @brief RPC client API for DPDK
 *
 * RPC client API for DPDK functions.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_RTE_H__
#define __TE_TAPI_RPC_RTE_H__

#include "te_rpc_types.h"

#include "tapi_cfg_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef rpc_ptr rpc_rte_mempool_p;
typedef rpc_ptr rpc_rte_mbuf_p;
typedef rpc_ptr rpc_rte_ring_p;
typedef rpc_ptr rpc_rte_flow_attr_p;
typedef rpc_ptr rpc_rte_flow_item_p;
typedef rpc_ptr rpc_rte_flow_action_p;
typedef rpc_ptr rpc_rte_flow_p;

/**
 * Get TE_ENV_DPDK_REUSE_RPCS feature status
 *
 * @return @c TRUE, if the feature is requested;
 *         @c FALSE, if the feature is disabled
 */
static inline te_bool
dpdk_reuse_rpcs(void)
{
    cfg_val_type    val_type = CVT_STRING;
    char           *reuse = NULL;
    te_bool         is_enabled = TRUE;
    te_errno        rc;

    rc = cfg_get_instance_fmt(&val_type, &reuse, "/local:/dpdk:/reuse_rpcs:");
    if ((rc != 0) || (reuse == NULL) || (strcasecmp(reuse, "yes") != 0))
        is_enabled = FALSE;

    free(reuse);

    return is_enabled;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_H__ */
