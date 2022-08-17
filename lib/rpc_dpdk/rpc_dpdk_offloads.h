/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief DPDK RPC offloads helper functions API
 *
 * API to handle DPDK RPC related operations with offloads
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __RPC_DPDK_OFFLOADS_H__
#define __RPC_DPDK_OFFLOADS_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rpc_dpdk_offload_t {
    unsigned long long bit;
    const char *name;
} rpc_dpdk_offload_t;

/* Array of all possible TA RPC Tx offloads */
extern const rpc_dpdk_offload_t rpc_dpdk_tx_offloads[];
/* Number of entries in the Tx offloads array */
extern const unsigned int rpc_dpdk_tx_offloads_num;

/* Array of all possible TA RPC Rx offloads */
extern const rpc_dpdk_offload_t rpc_dpdk_rx_offloads[];
/* Number of entries in the Rx offloads array */
extern const unsigned int rpc_dpdk_rx_offloads_num;

/**
 * Get the string representation of a Tx offload by TA RPC offload bit.
 *
 * @param bit   Tx offload bit number
 *
 * @return  Tx offload name
 */
const char * rpc_dpdk_offloads_tx_get_name(unsigned long long bit);

/**
 * Get the string representation of a Rx offload by TA RPC offload bit.
 *
 * @param bit   Rx offload bit number
 *
 * @return  Rx offload name
 */
const char * rpc_dpdk_offloads_rx_get_name(unsigned long long bit);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __RPC_DPDK_OFFLOADS_H__ */
