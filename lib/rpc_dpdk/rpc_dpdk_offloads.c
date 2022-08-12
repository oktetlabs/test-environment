/** @file
 * @brief DPDK RPC offloads helper functions API
 *
 * API to handle DPDK RPC related operations with offloads (implementation)
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 */

#include "te_config.h"

#include "rpc_dpdk_offloads.h"
#include "te_defs.h"
#include "tarpc.h"

const rpc_dpdk_offload_t rpc_dpdk_tx_offloads[] = {
#define TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(_bit) \
    { TARPC_RTE_DEV_TX_OFFLOAD_##_bit##_BIT, #_bit }
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(VLAN_INSERT),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(IPV4_CKSUM),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(UDP_CKSUM),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(TCP_CKSUM),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(SCTP_CKSUM),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(TCP_TSO),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(UDP_TSO),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(OUTER_IPV4_CKSUM),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(QINQ_INSERT),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(VXLAN_TNL_TSO),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(GRE_TNL_TSO),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(IPIP_TNL_TSO),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(GENEVE_TNL_TSO),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(MACSEC_INSERT),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(MT_LOCKFREE),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(MULTI_SEGS),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(MBUF_FAST_FREE),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(SECURITY),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(UDP_TNL_TSO),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(IP_TNL_TSO),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(OUTER_UDP_CKSUM),
    TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR(MATCH_METADATA),
#undef TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR
};

const unsigned int rpc_dpdk_tx_offloads_num =
        TE_ARRAY_LEN(rpc_dpdk_tx_offloads);

const rpc_dpdk_offload_t rpc_dpdk_rx_offloads[] = {
#define TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(_bit) \
    { TARPC_RTE_DEV_RX_OFFLOAD_##_bit##_BIT, #_bit }
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(VLAN_STRIP),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(IPV4_CKSUM),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(UDP_CKSUM),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(TCP_CKSUM),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(TCP_LRO),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(QINQ_STRIP),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(OUTER_IPV4_CKSUM),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(MACSEC_STRIP),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(HEADER_SPLIT),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(VLAN_FILTER),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(VLAN_EXTEND),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(JUMBO_FRAME),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(CRC_STRIP),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(SCATTER),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(TIMESTAMP),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(SECURITY),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(KEEP_CRC),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(SCTP_CKSUM),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(OUTER_UDP_CKSUM),
    TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(RSS_HASH),
#undef TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR
};

const unsigned int rpc_dpdk_rx_offloads_num =
        TE_ARRAY_LEN(rpc_dpdk_rx_offloads);

const char *
rpc_dpdk_offloads_tx_get_name(unsigned long long bit)
{
    unsigned int i;

    for (i = 0; i < rpc_dpdk_tx_offloads_num; i++)
    {
        if (rpc_dpdk_tx_offloads[i].bit == bit)
            return rpc_dpdk_tx_offloads[i].name;
    }

    return NULL;
}

const char *
rpc_dpdk_offloads_rx_get_name(unsigned long long bit)
{
    unsigned int i;

    for (i = 0; i < rpc_dpdk_rx_offloads_num; i++)
    {
        if (rpc_dpdk_rx_offloads[i].bit == bit)
            return rpc_dpdk_rx_offloads[i].name;
    }

    return NULL;
}
