/** @file
 * @brief RPC client API for helper DPDK functions
 *
 * RPC client API for helper DPDK functions
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_DPDK_H__
#define __TE_TAPI_RPC_DPDK_H__

#include "rcf_rpc.h"

#include "rpc_dpdk_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_dpdk TAPI for helper DPDK functions
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * Find DPDK representor ports.
 *
 * @param[out] n_rep            Number of found representors
 * @param[out] rep_port_ids     Port ids of the found representors
 *
 * @return Status code
 */
extern int rpc_dpdk_find_representors(rcf_rpc_server *rpcs, unsigned int *n_rep,
                                      uint16_t **rep_port_ids);

/**@} <!-- END te_lib_rpc_dpdk --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_DPDK_H__ */