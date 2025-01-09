/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC for DPDK Ethernet Devices API
 *
 * RPC routines implementation to call DPDK (rte_eth_*) functions.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "RPC rte_eth_dev"

#include "te_config.h"

#include "te_alloc.h"
#include "te_errno.h"
#include "te_str.h"
#include "te_enum.h"

#include "rte_config.h"
#include "rte_ethdev.h"
#include "rte_eth_ctrl.h"
#include "rte_version.h"

#include "logger_api.h"

#include "rpc_server.h"
#include "rpcs_dpdk.h"


#ifndef HAVE_STRUCT_RTE_ETHER_ADDR
#define rte_ether_addr ether_addr
#endif /* ! HAVE_STRUCT_RTE_ETHER_ADDR */

#define CASE_TARPC2RTE(_rte) \
    case TARPC_##_rte: *rte = RTE_##_rte; break

static const te_enum_bitmask_conv rx_offloads_map[] = {
#define RX_OFFLOAD_BIT_MAP(_bit) {                                     \
    .bits_from = UINT64_C(1) << TARPC_RTE_ETH_RX_OFFLOAD_##_bit##_BIT, \
    .bits_to = RTE_ETH_RX_OFFLOAD_##_bit                               \
}
#ifdef RTE_ETH_RX_OFFLOAD_VLAN_STRIP
    RX_OFFLOAD_BIT_MAP(VLAN_STRIP),
#endif /* RTE_ETH_RX_OFFLOAD_VLAN_STRIP */
#ifdef RTE_ETH_RX_OFFLOAD_IPV4_CKSUM
    RX_OFFLOAD_BIT_MAP(IPV4_CKSUM),
#endif /* RTE_ETH_RX_OFFLOAD_IPV4_CKSUM */
#ifdef RTE_ETH_RX_OFFLOAD_UDP_CKSUM
    RX_OFFLOAD_BIT_MAP(UDP_CKSUM),
#endif /* RTE_ETH_RX_OFFLOAD_UDP_CKSUM */
#ifdef RTE_ETH_RX_OFFLOAD_TCP_CKSUM
    RX_OFFLOAD_BIT_MAP(TCP_CKSUM),
#endif /* RTE_ETH_RX_OFFLOAD_TCP_CKSUM */
#ifdef RTE_ETH_RX_OFFLOAD_TCP_LRO
    RX_OFFLOAD_BIT_MAP(TCP_LRO),
#endif /* RTE_ETH_RX_OFFLOAD_TCP_LRO */
#ifdef RTE_ETH_RX_OFFLOAD_QINQ_STRIP
    RX_OFFLOAD_BIT_MAP(QINQ_STRIP),
#endif /* RTE_ETH_RX_OFFLOAD_QINQ_STRIP */
#ifdef RTE_ETH_RX_OFFLOAD_OUTER_IPV4_CKSUM
    RX_OFFLOAD_BIT_MAP(OUTER_IPV4_CKSUM),
#endif /* RTE_ETH_RX_OFFLOAD_OUTER_IPV4_CKSUM */
#ifdef RTE_ETH_RX_OFFLOAD_MACSEC_STRIP
    RX_OFFLOAD_BIT_MAP(MACSEC_STRIP),
#endif /* RTE_ETH_RX_OFFLOAD_MACSEC_STRIP */
#ifdef RTE_ETH_RX_OFFLOAD_HEADER_SPLIT
    RX_OFFLOAD_BIT_MAP(HEADER_SPLIT),
#endif /* RTE_ETH_RX_OFFLOAD_HEADER_SPLIT */
#ifdef RTE_ETH_RX_OFFLOAD_VLAN_FILTER
    RX_OFFLOAD_BIT_MAP(VLAN_FILTER),
#endif /* RTE_ETH_RX_OFFLOAD_VLAN_FILTER */
#ifdef RTE_ETH_RX_OFFLOAD_VLAN_EXTEND
    RX_OFFLOAD_BIT_MAP(VLAN_EXTEND),
#endif /* RTE_ETH_RX_OFFLOAD_VLAN_EXTEND */
#ifdef RTE_ETH_RX_OFFLOAD_JUMBO_FRAME
    RX_OFFLOAD_BIT_MAP(JUMBO_FRAME),
#endif /* RTE_ETH_RX_OFFLOAD_JUMBO_FRAME */
#ifdef RTE_ETH_RX_OFFLOAD_CRC_STRIP
    RX_OFFLOAD_BIT_MAP(CRC_STRIP),
#endif /* RTE_ETH_RX_OFFLOAD_CRC_STRIP */
#ifdef RTE_ETH_RX_OFFLOAD_SCATTER
    RX_OFFLOAD_BIT_MAP(SCATTER),
#endif /* RTE_ETH_RX_OFFLOAD_SCATTER */
#ifdef RTE_ETH_RX_OFFLOAD_TIMESTAMP
    RX_OFFLOAD_BIT_MAP(TIMESTAMP),
#endif /* RTE_ETH_RX_OFFLOAD_TIMESTAMP */
#ifdef RTE_ETH_RX_OFFLOAD_SECURITY
    RX_OFFLOAD_BIT_MAP(SECURITY),
#endif /* RTE_ETH_RX_OFFLOAD_SECURITY */
#ifdef RTE_ETH_RX_OFFLOAD_KEEP_CRC
    RX_OFFLOAD_BIT_MAP(KEEP_CRC),
#endif /* RTE_ETH_RX_OFFLOAD_KEEP_CRC */
#ifdef RTE_ETH_RX_OFFLOAD_SCTP_CKSUM
    RX_OFFLOAD_BIT_MAP(SCTP_CKSUM),
#endif /* RTE_ETH_RX_OFFLOAD_SCTP_CKSUM */
#ifdef RTE_ETH_RX_OFFLOAD_OUTER_UDP_CKSUM
    RX_OFFLOAD_BIT_MAP(OUTER_UDP_CKSUM),
#endif /* RTE_ETH_RX_OFFLOAD_OUTER_UDP_CKSUM */
#ifdef RTE_ETH_RX_OFFLOAD_RSS_HASH
    RX_OFFLOAD_BIT_MAP(RSS_HASH),
#endif /* RTE_ETH_RX_OFFLOAD_RSS_HASH */
#ifdef RTE_ETH_RX_OFFLOAD_BUFFER_SPLIT
    RX_OFFLOAD_BIT_MAP(BUFFER_SPLIT),
#endif /* RTE_ETH_RX_OFFLOAD_BUFFER_SPLIT */
#undef RX_OFFLOAD_BIT_MAP
    TE_ENUM_BITMASK_CONV_END
};

static const te_enum_bitmask_conv tx_offloads_map[] = {
#define TX_OFFLOAD_BIT_MAP(_bit) {                                     \
    .bits_from = UINT64_C(1) << TARPC_RTE_ETH_TX_OFFLOAD_##_bit##_BIT, \
    .bits_to = RTE_ETH_TX_OFFLOAD_##_bit                               \
}
#ifdef RTE_ETH_TX_OFFLOAD_VLAN_INSERT
    TX_OFFLOAD_BIT_MAP(VLAN_INSERT),
#endif /* RTE_ETH_TX_OFFLOAD_VLAN_INSERT */
#ifdef RTE_ETH_TX_OFFLOAD_IPV4_CKSUM
    TX_OFFLOAD_BIT_MAP(IPV4_CKSUM),
#endif /* RTE_ETH_TX_OFFLOAD_IPV4_CKSUM */
#ifdef RTE_ETH_TX_OFFLOAD_UDP_CKSUM
    TX_OFFLOAD_BIT_MAP(UDP_CKSUM),
#endif /* RTE_ETH_TX_OFFLOAD_UDP_CKSUM */
#ifdef RTE_ETH_TX_OFFLOAD_TCP_CKSUM
    TX_OFFLOAD_BIT_MAP(TCP_CKSUM),
#endif /* RTE_ETH_TX_OFFLOAD_TCP_CKSUM */
#ifdef RTE_ETH_TX_OFFLOAD_SCTP_CKSUM
    TX_OFFLOAD_BIT_MAP(SCTP_CKSUM),
#endif /* RTE_ETH_TX_OFFLOAD_SCTP_CKSUM */
#ifdef RTE_ETH_TX_OFFLOAD_TCP_TSO
    TX_OFFLOAD_BIT_MAP(TCP_TSO),
#endif /* RTE_ETH_TX_OFFLOAD_TCP_TSO */
#ifdef RTE_ETH_TX_OFFLOAD_UDP_TSO
    TX_OFFLOAD_BIT_MAP(UDP_TSO),
#endif /* RTE_ETH_TX_OFFLOAD_UDP_TSO*/
#ifdef RTE_ETH_TX_OFFLOAD_OUTER_IPV4_CKSUM
    TX_OFFLOAD_BIT_MAP(OUTER_IPV4_CKSUM),
#endif /* RTE_ETH_TX_OFFLOAD_OUTER_IPV4_CKSUM */
#ifdef RTE_ETH_TX_OFFLOAD_QINQ_INSERT
    TX_OFFLOAD_BIT_MAP(QINQ_INSERT),
#endif /* RTE_ETH_TX_OFFLOAD_QINQ_INSERT */
#ifdef RTE_ETH_TX_OFFLOAD_VXLAN_TNL_TSO
    TX_OFFLOAD_BIT_MAP(VXLAN_TNL_TSO),
#endif /* RTE_ETH_TX_OFFLOAD_VXLAN_TNL_TSO */
#ifdef RTE_ETH_TX_OFFLOAD_GRE_TNL_TSO
    TX_OFFLOAD_BIT_MAP(GRE_TNL_TSO),
#endif /* RTE_ETH_TX_OFFLOAD_GRE_TNL_TSO */
#ifdef RTE_ETH_TX_OFFLOAD_IPIP_TNL_TSO
    TX_OFFLOAD_BIT_MAP(IPIP_TNL_TSO),
#endif /* RTE_ETH_TX_OFFLOAD_IPIP_TNL_TSO */
#ifdef RTE_ETH_TX_OFFLOAD_GENEVE_TNL_TSO
    TX_OFFLOAD_BIT_MAP(GENEVE_TNL_TSO),
#endif /* RTE_ETH_TX_OFFLOAD_GENEVE_TNL_TSO */
#ifdef RTE_ETH_TX_OFFLOAD_MACSEC_INSERT
    TX_OFFLOAD_BIT_MAP(MACSEC_INSERT),
#endif /* RTE_ETH_TX_OFFLOAD_MACSEC_INSERT */
#ifdef RTE_ETH_TX_OFFLOAD_MT_LOCKFREE
    TX_OFFLOAD_BIT_MAP(MT_LOCKFREE),
#endif /* RTE_ETH_TX_OFFLOAD_MT_LOCKFREE */
#ifdef RTE_ETH_TX_OFFLOAD_MULTI_SEGS
    TX_OFFLOAD_BIT_MAP(MULTI_SEGS),
#endif /* RTE_ETH_TX_OFFLOAD_MULTI_SEGS */
#ifdef RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE
    TX_OFFLOAD_BIT_MAP(MBUF_FAST_FREE),
#endif /* RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE */
#ifdef RTE_ETH_TX_OFFLOAD_SECURITY
    TX_OFFLOAD_BIT_MAP(SECURITY),
#endif /* RTE_ETH_TX_OFFLOAD_SECURITY */
#ifdef RTE_ETH_TX_OFFLOAD_UDP_TNL_TSO
    TX_OFFLOAD_BIT_MAP(UDP_TNL_TSO),
#endif /* RTE_ETH_TX_OFFLOAD_UDP_TNL_TSO */
#ifdef RTE_ETH_TX_OFFLOAD_IP_TNL_TSO
    TX_OFFLOAD_BIT_MAP(IP_TNL_TSO),
#endif /* RTE_ETH_TX_OFFLOAD_IP_TNL_TSO */
#ifdef RTE_ETH_TX_OFFLOAD_OUTER_UDP_CKSUM
    TX_OFFLOAD_BIT_MAP(OUTER_UDP_CKSUM),
#endif /* RTE_ETH_TX_OFFLOAD_IP_TNL_TSO */
#ifdef RTE_ETH_TX_OFFLOAD_SEND_ON_TIMESTAMP
    TX_OFFLOAD_BIT_MAP(SEND_ON_TIMESTAMP),
#endif /* RTE_ETH_TX_OFFLOAD_SEND_ON_TIMESTAMP */
#undef TX_OFFLOAD_BIT_MAP
    TE_ENUM_BITMASK_CONV_END
};

#ifdef HAVE_STRUCT_RTE_ETH_DEV_INFO_DEV_CAPA
static const te_enum_bitmask_conv dev_capa_map[] = {
#define DEV_CAPA_BIT_MAP(_bit) {                                     \
    .bits_from = UINT64_C(1) << TARPC_RTE_ETH_DEV_CAPA_##_bit##_BIT, \
    .bits_to = RTE_ETH_DEV_CAPA_##_bit                               \
}
#ifdef RTE_ETH_DEV_CAPA_RUNTIME_RX_QUEUE_SETUP
    DEV_CAPA_BIT_MAP(RUNTIME_RX_QUEUE_SETUP),
#endif /* RTE_ETH_DEV_CAPA_RUNTIME_RX_QUEUE_SETUP */
#ifdef RTE_ETH_DEV_CAPA_RUNTIME_TX_QUEUE_SETUP
    DEV_CAPA_BIT_MAP(RUNTIME_TX_QUEUE_SETUP),
#endif /* RTE_ETH_DEV_CAPA_RUNTIME_TX_QUEUE_SETUP */
#undef DEV_CAPA_BIT_MAP
    TE_ENUM_BITMASK_CONV_END
};
#endif /* HAVE_STRUCT_RTE_ETH_DEV_INFO_DEV_CAPA */

static void
tarpc_rte_eth_thresh2rpc(const struct rte_eth_thresh *rte,
                         struct tarpc_rte_eth_thresh *rpc)
{
    rpc->pthresh = rte->pthresh;
    rpc->hthresh = rte->hthresh;
    rpc->wthresh = rte->wthresh;
}

static const te_enum_bitmask_conv rss_hf_proto_map[] = {
#define RSS_HF_PROTO_BIT_MAP(_bit) {                      \
    .bits_from = UINT64_C(1) << TARPC_RTE_ETH_RSS_##_bit, \
    .bits_to = RTE_ETH_RSS_##_bit                         \
}
    RSS_HF_PROTO_BIT_MAP(IPV4),
    RSS_HF_PROTO_BIT_MAP(FRAG_IPV4),
    RSS_HF_PROTO_BIT_MAP(NONFRAG_IPV4_TCP),
    RSS_HF_PROTO_BIT_MAP(NONFRAG_IPV4_UDP),
    RSS_HF_PROTO_BIT_MAP(NONFRAG_IPV4_SCTP),
    RSS_HF_PROTO_BIT_MAP(NONFRAG_IPV4_OTHER),
    RSS_HF_PROTO_BIT_MAP(IPV6),
    RSS_HF_PROTO_BIT_MAP(FRAG_IPV6),
    RSS_HF_PROTO_BIT_MAP(NONFRAG_IPV6_TCP),
    RSS_HF_PROTO_BIT_MAP(NONFRAG_IPV6_UDP),
    RSS_HF_PROTO_BIT_MAP(NONFRAG_IPV6_SCTP),
    RSS_HF_PROTO_BIT_MAP(NONFRAG_IPV6_OTHER),
    RSS_HF_PROTO_BIT_MAP(L2_PAYLOAD),
    RSS_HF_PROTO_BIT_MAP(IPV6_EX),
    RSS_HF_PROTO_BIT_MAP(IPV6_TCP_EX),
    RSS_HF_PROTO_BIT_MAP(IPV6_UDP_EX),
    RSS_HF_PROTO_BIT_MAP(PORT),
    RSS_HF_PROTO_BIT_MAP(VXLAN),
    RSS_HF_PROTO_BIT_MAP(GENEVE),
    RSS_HF_PROTO_BIT_MAP(NVGRE),
    RSS_HF_PROTO_BIT_MAP(GTPU),
    RSS_HF_PROTO_BIT_MAP(ETH),
    RSS_HF_PROTO_BIT_MAP(S_VLAN),
    RSS_HF_PROTO_BIT_MAP(C_VLAN),
    RSS_HF_PROTO_BIT_MAP(ESP),
    RSS_HF_PROTO_BIT_MAP(AH),
    RSS_HF_PROTO_BIT_MAP(L2TPV3),
    RSS_HF_PROTO_BIT_MAP(PFCP),
    RSS_HF_PROTO_BIT_MAP(PPPOE),
    RSS_HF_PROTO_BIT_MAP(ECPRI),
    RSS_HF_PROTO_BIT_MAP(MPLS),
    RSS_HF_PROTO_BIT_MAP(IPV4_CHKSUM),
    RSS_HF_PROTO_BIT_MAP(L4_CHKSUM),
    RSS_HF_PROTO_BIT_MAP(L2TPV2),

    RSS_HF_PROTO_BIT_MAP(L3_SRC_ONLY),
    RSS_HF_PROTO_BIT_MAP(L3_DST_ONLY),
    RSS_HF_PROTO_BIT_MAP(L4_SRC_ONLY),
    RSS_HF_PROTO_BIT_MAP(L4_DST_ONLY),
    RSS_HF_PROTO_BIT_MAP(L2_SRC_ONLY),
    RSS_HF_PROTO_BIT_MAP(L2_DST_ONLY),

    RSS_HF_PROTO_BIT_MAP(L3_PRE32),
    RSS_HF_PROTO_BIT_MAP(L3_PRE40),
    RSS_HF_PROTO_BIT_MAP(L3_PRE48),
    RSS_HF_PROTO_BIT_MAP(L3_PRE56),
    RSS_HF_PROTO_BIT_MAP(L3_PRE64),
    RSS_HF_PROTO_BIT_MAP(L3_PRE96),

    RSS_HF_PROTO_BIT_MAP(LEVEL_OUTERMOST),
    RSS_HF_PROTO_BIT_MAP(LEVEL_INNERMOST),
#undef RSS_HF_PROTO_BIT_MAP
    TE_ENUM_BITMASK_CONV_END
};

static void
tarpc_rte_eth_rxconf2rpc(const struct rte_eth_rxconf *rte,
                         struct tarpc_rte_eth_rxconf *rpc)
{
    tarpc_rte_eth_thresh2rpc(&rte->rx_thresh, &rpc->rx_thresh);
    rpc->rx_free_thresh = rte->rx_free_thresh;
    rpc->rx_drop_en = rte->rx_drop_en;
    rpc->rx_deferred_start = rte->rx_deferred_start;
#if RTE_VERSION >= RTE_VERSION_NUM(17,11,0,1)
    rpc->offloads = rpc_dpdk_bitmask64_rte2rpc(
                        rte->offloads, rx_offloads_map,
                        TARPC_RTE_ETH_RX_OFFLOAD__UNKNOWN_BIT);
#endif
}

#if RTE_VERSION < RTE_VERSION_NUM(18,8,0,0)
static const te_enum_bitmask_conv txq_flags_map[] = {
#define TXQ_FLAG_BIT_MAP(_bit) {                                      \
    .bits_from = UINT64_C(1) << TARPC_RTE_ETH_TXQ_FLAGS_##_bit##_BIT, \
    .bits_to = ETH_TXQ_FLAGS_##_bit                                   \
}
    TXQ_FLAG_BIT_MAP(NOMULTSEGS),
    TXQ_FLAG_BIT_MAP(NOREFCOUNT),
    TXQ_FLAG_BIT_MAP(NOMULTMEMP),
    TXQ_FLAG_BIT_MAP(NOVLANOFFL),
    TXQ_FLAG_BIT_MAP(NOXSUMSCTP),
    TXQ_FLAG_BIT_MAP(NOXSUMUDP),
    TXQ_FLAG_BIT_MAP(NOXSUMTCP);
#if RTE_VERSION >= RTE_VERSION_NUM(17,11,0,1)
    TXQ_FLAG_BIT_MAP(IGNORE),
#endif
#undef TXQ_FLAG_BIT_MAP
    TE_ENUM_BITMASK_CONV_END
};
#endif

static void
tarpc_rte_eth_txconf2rpc(const struct rte_eth_txconf *rte,
                         struct tarpc_rte_eth_txconf *rpc)
{
    tarpc_rte_eth_thresh2rpc(&rte->tx_thresh, &rpc->tx_thresh);
    rpc->tx_rs_thresh = rte->tx_rs_thresh;
    rpc->tx_free_thresh = rte->tx_free_thresh;
#if RTE_VERSION < RTE_VERSION_NUM(18,8,0,0)
    rpc->txq_flags = rpc_dpdk_bitmask32_rte2rpc(
                        rte->txq_flags, tx_offloads_map,
                        TARPC_RTE_ETH_TXQ_FLAGS__UNKNOWN_BIT);
#endif
    rpc->tx_deferred_start = rte->tx_deferred_start;
#if RTE_VERSION >= RTE_VERSION_NUM(17,11,0,1)
    rpc->offloads = rpc_dpdk_bitmask64_rte2rpc(
                        rte->offloads, tx_offloads_map,
                        TARPC_RTE_ETH_TX_OFFLOAD__UNKNOWN_BIT);
#endif
}

static void
tarpc_rte_eth_desc_lim2rpc(const struct rte_eth_desc_lim *rte,
                           struct tarpc_rte_eth_desc_lim *rpc)
{
    rpc->nb_max = rte->nb_max;
    rpc->nb_min = rte->nb_min;
    rpc->nb_align = rte->nb_align;
    rpc->nb_seg_max = rte->nb_seg_max;
    rpc->nb_mtu_seg_max = rte->nb_mtu_seg_max;
}

static const te_enum_bitmask_conv speed_link_map[] = {
#define LINK_SPEED_BIT_MAP(_bit) {                                     \
    .bits_from = UINT64_C(1) << TARPC_RTE_ETH_LINK_SPEED_##_bit##_BIT, \
    .bits_to = RTE_ETH_LINK_SPEED_##_bit                               \
}
    LINK_SPEED_BIT_MAP(FIXED),
    LINK_SPEED_BIT_MAP(10M_HD),
    LINK_SPEED_BIT_MAP(10M),
    LINK_SPEED_BIT_MAP(100M_HD),
    LINK_SPEED_BIT_MAP(100M),
    LINK_SPEED_BIT_MAP(1G),
    LINK_SPEED_BIT_MAP(2_5G),
    LINK_SPEED_BIT_MAP(5G),
    LINK_SPEED_BIT_MAP(10G),
    LINK_SPEED_BIT_MAP(20G),
    LINK_SPEED_BIT_MAP(25G),
    LINK_SPEED_BIT_MAP(40G),
    LINK_SPEED_BIT_MAP(50G),
    LINK_SPEED_BIT_MAP(56G),
    LINK_SPEED_BIT_MAP(100G),
#undef LINK_SPEED_BIT_MAP
    TE_ENUM_BITMASK_CONV_END
};

#ifdef HAVE_STRUCT_RTE_ETH_DEV_PORTCONF
static void
tarpc_rte_eth_dev_portconf2rpc(const struct rte_eth_dev_portconf *rte,
                               struct tarpc_rte_eth_dev_portconf *rpc)
{
    rpc->burst_size = rte->burst_size;
    rpc->ring_size = rte->ring_size;
    rpc->nb_queues = rte->nb_queues;
}
#endif /* HAVE_STRUCT_RTE_ETH_DEV_PORTCONF */

TARPC_FUNC(rte_eth_dev_info_get, {},
{
    struct rte_eth_dev_info dev_info;

    MAKE_CALL(func(in->port_id, &dev_info));

    /* pci_dev is not mapped/returned */
    out->dev_info.driver_name =
        (dev_info.driver_name == NULL) ? NULL : strdup(dev_info.driver_name);
    out->dev_info.if_index = dev_info.if_index;
    out->dev_info.min_mtu = dev_info.min_mtu;
    out->dev_info.max_mtu = dev_info.max_mtu;
    out->dev_info.min_rx_bufsize = dev_info.min_rx_bufsize;
    out->dev_info.max_rx_pktlen = dev_info.max_rx_pktlen;
    out->dev_info.max_rx_queues = dev_info.max_rx_queues;
    out->dev_info.max_tx_queues = dev_info.max_tx_queues;
    out->dev_info.max_mac_addrs = dev_info.max_mac_addrs;
    out->dev_info.max_hash_mac_addrs = dev_info.max_hash_mac_addrs;
    out->dev_info.max_vfs = dev_info.max_vfs;
    out->dev_info.max_vmdq_pools = dev_info.max_vmdq_pools;
#ifdef HAVE_STRUCT_RTE_ETH_DEV_INFO_RX_QUEUE_OFFLOAD_CAPA
    out->dev_info.rx_queue_offload_capa =
        rpc_dpdk_bitmask64_rte2rpc(dev_info.rx_queue_offload_capa,
                                   rx_offloads_map,
                                   TARPC_RTE_ETH_RX_OFFLOAD__UNKNOWN_BIT);
#else /* !HAVE_STRUCT_RTE_ETH_DEV_INFO_RX_QUEUE_OFFLOAD_CAPA */
    out->dev_info.rx_queue_offload_capa =
        (1ULL << TARPC_RTE_ETH_RX_OFFLOAD__UNSUPPORTED_BIT);
#endif /* HAVE_STRUCT_RTE_ETH_DEV_INFO_RX_QUEUE_OFFLOAD_CAPA */
    out->dev_info.rx_offload_capa =
        rpc_dpdk_bitmask64_rte2rpc(dev_info.rx_offload_capa,
                                   rx_offloads_map,
                                   TARPC_RTE_ETH_RX_OFFLOAD__UNKNOWN_BIT);
#ifdef HAVE_STRUCT_RTE_ETH_DEV_INFO_TX_QUEUE_OFFLOAD_CAPA
    out->dev_info.tx_queue_offload_capa =
        rpc_dpdk_bitmask64_rte2rpc(dev_info.tx_queue_offload_capa,
                                   tx_offloads_map,
                                   TARPC_RTE_ETH_TX_OFFLOAD__UNKNOWN_BIT);
#else /* !HAVE_STRUCT_RTE_ETH_DEV_INFO_TX_QUEUE_OFFLOAD_CAPA */
    out->dev_info.tx_queue_offload_capa =
        (1ULL << TARPC_RTE_ETH_TX_OFFLOAD__UNSUPPORTED_BIT);
#endif /* HAVE_STRUCT_RTE_ETH_DEV_INFO_TX_QUEUE_OFFLOAD_CAPA */
    out->dev_info.tx_offload_capa =
        rpc_dpdk_bitmask64_rte2rpc(dev_info.tx_offload_capa,
                                   tx_offloads_map,
                                   TARPC_RTE_ETH_TX_OFFLOAD__UNKNOWN_BIT);
    out->dev_info.reta_size = dev_info.reta_size;
    out->dev_info.hash_key_size = dev_info.hash_key_size;
    out->dev_info.flow_type_rss_offloads =
        rpc_dpdk_bitmask64_rte2rpc(dev_info.flow_type_rss_offloads,
                                   rss_hf_proto_map,
                                   TARPC_RTE_ETH_RSS__UNKNOWN);
    tarpc_rte_eth_rxconf2rpc(&dev_info.default_rxconf,
                             &out->dev_info.default_rxconf);
    tarpc_rte_eth_txconf2rpc(&dev_info.default_txconf,
                             &out->dev_info.default_txconf);
    out->dev_info.vmdq_queue_base = dev_info.vmdq_queue_base;
    out->dev_info.vmdq_queue_num = dev_info.vmdq_queue_num;
    tarpc_rte_eth_desc_lim2rpc(&dev_info.rx_desc_lim,
                               &out->dev_info.rx_desc_lim);
    tarpc_rte_eth_desc_lim2rpc(&dev_info.tx_desc_lim,
                               &out->dev_info.tx_desc_lim);
    out->dev_info.speed_capa =
        rpc_dpdk_bitmask32_rte2rpc(dev_info.speed_capa, speed_link_map,
                                   TARPC_RTE_ETH_LINK_SPEED__UNKNOWN_BIT);
    out->dev_info.nb_rx_queues = dev_info.nb_rx_queues;
    out->dev_info.nb_tx_queues = dev_info.nb_tx_queues;
#ifdef HAVE_STRUCT_RTE_ETH_DEV_INFO_DEV_CAPA
    out->dev_info.dev_capa =
        rpc_dpdk_bitmask64_rte2rpc(dev_info.dev_capa, dev_capa_map,
                                   TARPC_RTE_ETH_DEV_CAPA__UNKNOWN_BIT);
#else /* !HAVE_STRUCT_RTE_ETH_DEV_INFO_DEV_CAPA */
    out->dev_info.dev_capa =
        (1ULL << TARPC_RTE_ETH_DEV_CAPA__UNSUPPORTED_BIT);
#endif /* HAVE_STRUCT_RTE_ETH_DEV_INFO_DEV_CAPA */
#ifdef HAVE_STRUCT_RTE_ETH_DEV_PORTCONF
    tarpc_rte_eth_dev_portconf2rpc(&dev_info.default_rxportconf,
                                   &out->dev_info.default_rxportconf);
    tarpc_rte_eth_dev_portconf2rpc(&dev_info.default_txportconf,
                                   &out->dev_info.default_txportconf);
#else /* !HAVE_STRUCT_RTE_ETH_DEV_PORTCONF */
    memset(&out->dev_info.default_rxportconf, 0,
           sizeof(out->dev_info.default_rxportconf));
    memset(&out->dev_info.default_txportconf, 0,
           sizeof(out->dev_info.default_txportconf));
#endif /* HAVE_STRUCT_RTE_ETH_DEV_PORTCONF */
})

static te_errno
tarpc_eth_rx_mq_mode2rte(const enum tarpc_rte_eth_rx_mq_mode rpc,
                         enum rte_eth_rx_mq_mode *rte)
{
    switch (rpc)
    {
        CASE_TARPC2RTE(ETH_MQ_RX_NONE);
        CASE_TARPC2RTE(ETH_MQ_RX_RSS);
        CASE_TARPC2RTE(ETH_MQ_RX_DCB);
        CASE_TARPC2RTE(ETH_MQ_RX_DCB_RSS);
        CASE_TARPC2RTE(ETH_MQ_RX_VMDQ_ONLY);
        CASE_TARPC2RTE(ETH_MQ_RX_VMDQ_RSS);
        CASE_TARPC2RTE(ETH_MQ_RX_VMDQ_DCB);
        CASE_TARPC2RTE(ETH_MQ_RX_VMDQ_DCB_RSS);
        default:
            return TE_EINVAL;
    }
    return 0;
}

#if RTE_VERSION < RTE_VERSION_NUM(18,8,0,0)
static te_errno
tarpc_eth_rxmode_flags2rte(uint16_t rxmode_flags,
                           struct rte_eth_rxmode *rxmode)
{
#define TARPC_RTE_ETH_RXMODE_BIT2MEMBER(_bit, _member) \
    do {                                                            \
        uint16_t flag = (1 << TARPC_RTE_ETH_RXMODE_##_bit##_BIT);   \
                                                                    \
        if (rxmode_flags & flag)                                    \
        {                                                           \
            rxmode_flags &= ~flag;                                  \
            rxmode->_member = 1;                                    \
        }                                                           \
    } while (0)
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(HEADER_SPLIT, header_split);
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(HW_IP_CHECKSUM, hw_ip_checksum);
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(HW_VLAN_FILTER, hw_vlan_filter);
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(HW_VLAN_STRIP, hw_vlan_strip);
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(HW_VLAN_EXTEND, hw_vlan_extend);
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(JUMBO_FRAME, jumbo_frame);
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(HW_STRIP_CRC, hw_strip_crc);
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(ENABLE_SCATTER, enable_scatter);
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(ENABLE_LRO, enable_lro);
#if RTE_VERSION >= RTE_VERSION_NUM(17,11,0,1)
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(HW_TIMESTAMP, hw_timestamp);
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(IGNORE_OFFLOAD_BITFIELD,
                                    ignore_offload_bitfield);
#endif
#if RTE_VERSION >= RTE_VERSION_NUM(17,11,0,2)
    TARPC_RTE_ETH_RXMODE_BIT2MEMBER(SECURITY, security);
#endif
#undef TARPC_RTE_ETH_RXMODE_BIT2MEMBER
    return (rxmode_flags == 0) ? 0 : TE_EINVAL;
}
#endif

static te_errno
tarpc_eth_rxmode2rte(const struct tarpc_rte_eth_rxmode *rpc,
                     struct rte_eth_rxmode *rte)
{
    te_errno rc;

    rc = tarpc_eth_rx_mq_mode2rte(rpc->mq_mode, &rte->mq_mode);
    if (rc != 0)
        return rc;

    rte->mtu = rpc->mtu;
#if RTE_VERSION >= RTE_VERSION_NUM(17,11,0,1)
    rc = rpc_dpdk_bitmask64_rpc2rte(rpc->offloads, rx_offloads_map,
                                    &rte->offloads);
    if (rc != 0)
        return rc;
#endif
#if RTE_VERSION < RTE_VERSION_NUM(18,8,0,0)
    rc = tarpc_eth_rxmode_flags2rte(rpc->flags, rte);
    if (rc != 0)
        return rc;
#endif
    return 0;
}

static te_errno
tarpc_eth_tx_mq_mode2rte(const enum tarpc_rte_eth_tx_mq_mode rpc,
                         enum rte_eth_tx_mq_mode *rte)
{
    switch (rpc)
    {
        CASE_TARPC2RTE(ETH_MQ_TX_NONE);
        CASE_TARPC2RTE(ETH_MQ_TX_DCB);
        CASE_TARPC2RTE(ETH_MQ_TX_VMDQ_DCB);
        CASE_TARPC2RTE(ETH_MQ_TX_VMDQ_ONLY);
        default:
            return TE_EINVAL;
    }
    return 0;
}

static te_errno
tarpc_eth_txmode_flags2rte(uint16_t txmode_flags,
                           struct rte_eth_txmode *txmode)
{
#define TARPC_RTE_ETH_TXMODE_BIT2MEMBER(_bit, _member) \
    do {                                                            \
        uint16_t flag = (1 << TARPC_RTE_ETH_TXMODE_##_bit##_BIT);   \
                                                                    \
        if (txmode_flags & flag)                                    \
        {                                                           \
            txmode_flags &= ~flag;                                  \
            txmode->_member = 1;                                    \
        }                                                           \
    } while (0)
    TARPC_RTE_ETH_TXMODE_BIT2MEMBER(HW_VLAN_REJECT_TAGGED,
                                    hw_vlan_reject_tagged);
    TARPC_RTE_ETH_TXMODE_BIT2MEMBER(HW_VLAN_REJECT_UNTAGGED,
                                    hw_vlan_reject_untagged);
    TARPC_RTE_ETH_TXMODE_BIT2MEMBER(HW_VLAN_INSERT_PVID,
                                    hw_vlan_insert_pvid);
#undef TARPC_RTE_ETH_TXMODE_BIT2MEMBER
    return (txmode_flags == 0) ? 0 : TE_EINVAL;
}

static te_errno
tarpc_eth_txmode2rte(const struct tarpc_rte_eth_txmode *rpc,
                     struct rte_eth_txmode *rte)
{
    te_errno rc;

    rc = tarpc_eth_tx_mq_mode2rte(rpc->mq_mode, &rte->mq_mode);
    if (rc != 0)
        return rc;
#if RTE_VERSION >= RTE_VERSION_NUM(17,11,0,1)
    rc = rpc_dpdk_bitmask64_rpc2rte(rpc->offloads, tx_offloads_map,
                                    &rte->offloads);
    if (rc != 0)
        return rc;
#endif
    rte->pvid = rpc->pvid;
    rc = tarpc_eth_txmode_flags2rte(rpc->flags, rte);
    if (rc != 0)
        return rc;

    return 0;
}

static te_errno
tarpc_eth_rss_conf2rte(const struct tarpc_rte_eth_rss_conf *rpc,
                       struct rte_eth_rss_conf *rte)
{
    te_errno rc;

    /* TODO Ideally it should be validated that it is not changed */
    rte->rss_key = rpc->rss_key.rss_key_val;
    rte->rss_key_len = rpc->rss_key_len;

    rc = rpc_dpdk_bitmask64_rpc2rte(rpc->rss_hf, rss_hf_proto_map,
                                    &rte->rss_hf);
    if (rc != 0)
        return rc;

    return 0;
}

static te_errno
tarpc_eth_rx_adv_conf2rte(const struct tarpc_rte_eth_rx_adv_conf *rpc,
                          struct rte_eth_conf *rte)
{
    te_errno rc;

    rc = tarpc_eth_rss_conf2rte(&rpc->rss_conf, &rte->rx_adv_conf.rss_conf);
    if (rc != 0)
        return rc;

    return 0;
}

static te_errno
tarpc_intr_conf2rte(const struct tarpc_rte_intr_conf *rpc,
                    struct rte_intr_conf *rte)
{
    rte->lsc = rpc->lsc;
    rte->rxq = rpc->rxq;

    return 0;
}

static te_errno
tarpc_eth_conf2rte(const struct tarpc_rte_eth_conf *rpc,
                   struct rte_eth_conf *rte)
{
    te_errno rc;

    memset(rte, 0, sizeof(*rte));

    rc = rpc_dpdk_bitmask32_rpc2rte(rpc->link_speeds, speed_link_map,
                                    &rte->link_speeds);
    if (rc != 0)
        return rc;

    rc = tarpc_eth_rxmode2rte(&rpc->rxmode, &rte->rxmode);
    if (rc != 0)
        return rc;

    rc = tarpc_eth_txmode2rte(&rpc->txmode, &rte->txmode);
    if (rc != 0)
        return rc;

    rte->lpbk_mode = rpc->lpbk_mode;
    rc = tarpc_eth_rx_adv_conf2rte(&rpc->rx_adv_conf, rte);
    if (rc != 0)
        return rc;

    rte->dcb_capability_en = rpc->dcb_capability_en;
    rc = tarpc_intr_conf2rte(&rpc->intr_conf, &rte->intr_conf);
    if (rc != 0)
        return rc;

    return 0;
}

TARPC_FUNC(rte_eth_stats_get, {},
{
    struct rte_eth_stats stats;

    MAKE_CALL(out->retval = func(in->port_id, &stats));

    if (out->retval == 0)
    {
        out->stats.ipackets = stats.ipackets;
        out->stats.opackets = stats.opackets;
        out->stats.ibytes = stats.ibytes;
        out->stats.obytes = stats.obytes;
        out->stats.imissed = stats.imissed;
        out->stats.ierrors = stats.ierrors;
        out->stats.oerrors = stats.oerrors;
        out->stats.rx_nombuf = stats.rx_nombuf;
    }
})

TARPC_FUNC(rte_eth_stats_reset,{},
{
    MAKE_CALL(func(in->port_id));
})

TARPC_FUNC(rte_eth_dev_configure, {},
{
    struct rte_eth_conf eth_conf;
    struct rte_eth_conf *eth_conf_p = &eth_conf;

    if (in->eth_conf.eth_conf_len == 0)
    {
        eth_conf_p = NULL;
    }
    else
    {
        te_errno rc;

        rc = tarpc_eth_conf2rte(in->eth_conf.eth_conf_val, &eth_conf);
        if (rc != 0)
        {
            out->common._errno = TE_RC(TE_RPCS, rc);
            out->retval = -out->common._errno;
            goto done;
        }

    }

    MAKE_CALL(out->retval = func(in->port_id, in->nb_rx_queue,
                                 in->nb_tx_queue, eth_conf_p));
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

TARPC_FUNC(rte_eth_dev_close, {},
{
    MAKE_CALL(func(in->port_id));
})

TARPC_FUNC(rte_eth_dev_reset, {},
{
    MAKE_CALL(func(in->port_id));
})

TARPC_FUNC(rte_eth_dev_start, {},
{
    MAKE_CALL(out->retval = func(in->port_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_stop, {},
{
    MAKE_CALL(func(in->port_id));
})

static te_errno
tarpc_eth_thresh2rte(const struct tarpc_rte_eth_thresh *rpc,
                     struct rte_eth_thresh *rte)
{
    memset(rte, 0, sizeof(*rte));

    rte->pthresh = rpc->pthresh;
    rte->hthresh = rpc->hthresh;
    rte->wthresh = rpc->wthresh;

    return 0;
}

static te_errno
tarpc_eth_txconf2rte(const struct tarpc_rte_eth_txconf *rpc,
                     struct rte_eth_txconf *rte)
{
    te_errno rc;

    memset(rte, 0, sizeof(*rte));

    rc = tarpc_eth_thresh2rte(&rpc->tx_thresh, &rte->tx_thresh);
    if (rc != 0)
        return rc;

    rte->tx_rs_thresh = rpc->tx_rs_thresh;
    rte->tx_free_thresh = rpc->tx_free_thresh;
#if RTE_VERSION < RTE_VERSION_NUM(18,8,0,0)
    rc = rpc_dpdk_bitmask32_rpc2rte(rpc->txq_flags, txq_flags_map,
                                    &rte->txq_flags);
    if (rc != 0)
        return rc;
#endif
    rte->tx_deferred_start = rpc->tx_deferred_start;
#if RTE_VERSION >= RTE_VERSION_NUM(17,11,0,1)
    rc = rpc_dpdk_bitmask64_rpc2rte(rpc->offloads, tx_offloads_map,
                                    &rte->offloads);
    if (rc != 0)
        return rc;
#endif

    return 0;
}

TARPC_FUNC(rte_eth_tx_queue_setup, {},
{
    struct rte_eth_txconf eth_txconf;
    struct rte_eth_txconf *eth_txconf_p = &eth_txconf;

    if (in->tx_conf.tx_conf_len == 0)
    {
        eth_txconf_p = NULL;
    }
    else if (tarpc_eth_txconf2rte(in->tx_conf.tx_conf_val, &eth_txconf) != 0)
    {
        out->retval = -TE_RC(TE_RPCS, TE_EINVAL);
        goto done;
    }

    MAKE_CALL(out->retval = func(in->port_id,
                                 in->tx_queue_id,
                                 in->nb_tx_desc,
                                 in->socket_id,
                                 eth_txconf_p));
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

static te_errno
tarpc_eth_rxconf2rte(const struct tarpc_rte_eth_rxconf *rpc,
                     struct rte_eth_rxconf *rte)
{
    te_errno rc;

    memset(rte, 0, sizeof(*rte));

    rc = tarpc_eth_thresh2rte(&rpc->rx_thresh, &rte->rx_thresh);
    if (rc != 0)
        return rc;

    rte->rx_free_thresh = rpc->rx_free_thresh;
    rte->rx_drop_en = rpc->rx_drop_en;
    rte->rx_deferred_start = rpc->rx_deferred_start;
#if RTE_VERSION >= RTE_VERSION_NUM(17,11,0,1)
    rpc_dpdk_bitmask64_rpc2rte(rpc->offloads, rx_offloads_map,
                               &rte->offloads);
    if (rc != 0)
        return rc;
#endif

    return 0;
}

TARPC_FUNC(rte_eth_rx_queue_setup, {},
{
    struct rte_mempool *mp = NULL;
    struct rte_eth_rxconf eth_rxconf;
    struct rte_eth_rxconf *eth_rxconf_p = &eth_rxconf;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        mp = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->mp, ns);
    });

    if (in->rx_conf.rx_conf_len == 0)
    {
        eth_rxconf_p = NULL;
    }
    else
    {
        te_errno rc;

        rc = tarpc_eth_rxconf2rte(in->rx_conf.rx_conf_val, &eth_rxconf);
        if (rc != 0)
        {
            out->retval = -TE_RC(TE_RPCS, TE_EINVAL);
            goto done;
        }
    }

    MAKE_CALL(out->retval = func(in->port_id,
                                 in->rx_queue_id,
                                 in->nb_rx_desc,
                                 in->socket_id,
                                 eth_rxconf_p,
                                 mp));
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

TARPC_FUNC(rte_eth_dev_rx_intr_enable, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->queue_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_rx_intr_disable, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->queue_id));
    neg_errno_h2rpc(&out->retval);
})

static te_errno
tarpc_intr_op2rte(const enum tarpc_rte_intr_op rpc,
                  int *rte)
{
    switch (rpc)
    {
        case TARPC_RTE_INTR_EVENT_ADD:
            *rte = RTE_INTR_EVENT_ADD;
            break;
        case TARPC_RTE_INTR_EVENT_DEL:
            *rte = RTE_INTR_EVENT_DEL;
            break;
        default:
            return TE_EINVAL;
    }
    return 0;
}

TARPC_FUNC(rte_eth_dev_rx_intr_ctl_q, {},
{
    int      op;
    te_errno rc;

    rc = tarpc_intr_op2rte(in->op, &op);
    if (rc != 0)
    {
        out->common._errno = TE_RC(TE_RPCS, rc);
        out->retval = -out->common._errno;
        goto done;
    }

    MAKE_CALL(out->retval = func(in->port_id, in->queue_id,
                                 in->epfd, op, (void *)in->data));
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

TARPC_FUNC_STATIC(rte_eth_tx_burst, {},
{
    uint16_t          nb_pkts = in->tx_pkts.tx_pkts_len;
    struct rte_mbuf **tx_pkts = NULL;
    uint16_t          i;

    if (nb_pkts != 0)
        tx_pkts = (struct rte_mbuf **)TE_ALLOC(nb_pkts * sizeof(*tx_pkts));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        for (i = 0; i < nb_pkts; i++)
            tx_pkts[i] = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->tx_pkts.tx_pkts_val[i],
                                                      ns);
    });


    MAKE_CALL(out->retval = func(in->port_id, in->queue_id, tx_pkts, nb_pkts));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        for (i = 0; i < MIN(nb_pkts, out->retval); i++)
            RCF_PCH_MEM_INDEX_FREE(in->tx_pkts.tx_pkts_val[i], ns);
    });

    free(tx_pkts);
})

TARPC_FUNC_STATIC(rte_eth_tx_prepare, {},
{
    uint16_t          nb_pkts = in->tx_pkts.tx_pkts_len;
    struct rte_mbuf **tx_pkts = NULL;
    uint16_t          i;

    if (nb_pkts != 0)
        tx_pkts = (struct rte_mbuf **)TE_ALLOC(nb_pkts * sizeof(*tx_pkts));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        for (i = 0; i < nb_pkts; i++)
        {
            tarpc_rte_mbuf mem_index = in->tx_pkts.tx_pkts_val[i];

            tx_pkts[i] = RCF_PCH_MEM_INDEX_MEM_TO_PTR(mem_index, ns);
        }
    });

    MAKE_CALL(out->retval = func(in->port_id, in->queue_id, tx_pkts, nb_pkts));

    free(tx_pkts);
})

TARPC_FUNC_STATIC(rte_eth_rx_burst, {},
{
    struct rte_mbuf **rx_pkts = NULL;
    uint16_t          nb_pkts_rx;
    uint16_t          i;

    if (in->nb_pkts != 0)
        rx_pkts = (struct rte_mbuf **)TE_ALLOC(in->nb_pkts * sizeof(*rx_pkts));

    MAKE_CALL(nb_pkts_rx = func(in->port_id, in->queue_id,
                                rx_pkts, in->nb_pkts));
    out->rx_pkts.rx_pkts_len = nb_pkts_rx;

    if (nb_pkts_rx != 0)
        out->rx_pkts.rx_pkts_val =
            TE_ALLOC(nb_pkts_rx * sizeof(tarpc_rte_mbuf));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        for (i = 0; i < MIN(in->nb_pkts, nb_pkts_rx); i++)
            out->rx_pkts.rx_pkts_val[i] = RCF_PCH_MEM_INDEX_ALLOC(rx_pkts[i], ns);
    });

    free(rx_pkts);
})

TARPC_FUNC(rte_eth_dev_set_link_up, {},
{
    MAKE_CALL(out->retval = func(in->port_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_set_link_down, {},
{
    MAKE_CALL(out->retval = func(in->port_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_promiscuous_enable, {},
{
#if HAVE_RTE_PROMISCUOUS_RETURN_VOID
    MAKE_CALL(func(in->port_id));
#else
    MAKE_CALL(out->retval = func(in->port_id));
    neg_errno_h2rpc(&out->retval);
#endif
})

TARPC_FUNC(rte_eth_promiscuous_disable, {},
{
#if HAVE_RTE_PROMISCUOUS_RETURN_VOID
    MAKE_CALL(func(in->port_id));
#else
    MAKE_CALL(out->retval = func(in->port_id));
    neg_errno_h2rpc(&out->retval);
#endif
})

TARPC_FUNC(rte_eth_promiscuous_get, {},
{
    MAKE_CALL(out->retval = func(in->port_id));
})

TARPC_FUNC(rte_eth_allmulticast_enable, {},
{
#if HAVE_RTE_ALLMULTICAST_RETURN_VOID
    MAKE_CALL(func(in->port_id));
#else
    MAKE_CALL(out->retval = func(in->port_id));
    neg_errno_h2rpc(&out->retval);
#endif
})

TARPC_FUNC(rte_eth_allmulticast_disable, {},
{
#if HAVE_RTE_ALLMULTICAST_RETURN_VOID
    MAKE_CALL(func(in->port_id));
#else
    MAKE_CALL(out->retval = func(in->port_id));
    neg_errno_h2rpc(&out->retval);
#endif
})

TARPC_FUNC(rte_eth_allmulticast_get, {},
{
    MAKE_CALL(out->retval = func(in->port_id));
})

TARPC_FUNC(rte_eth_dev_get_mtu,{},
{
    uint16_t mtu;
    bool is_mtu_null;

    is_mtu_null = (in->mtu.mtu_len == 0);

    MAKE_CALL(out->retval = rte_eth_dev_get_mtu(
        in->port_id, is_mtu_null ? NULL : &mtu));
    neg_errno_h2rpc(&out->retval);

    if (!is_mtu_null)
        out->mtu = mtu;

})

TARPC_FUNC(rte_eth_dev_set_mtu, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->mtu));
    neg_errno_h2rpc(&out->retval);
})


TARPC_FUNC(rte_eth_dev_vlan_filter, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->vlan_id, in->on));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_set_vlan_strip_on_queue, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->rx_queue_id, in->on));
    neg_errno_h2rpc(&out->retval);
})

static te_errno
tarpc_vlan_type2rte(const enum tarpc_rte_vlan_type rpc,
                    enum rte_vlan_type *rte)
{
    switch (rpc)
    {
        CASE_TARPC2RTE(ETH_VLAN_TYPE_UNKNOWN);
        CASE_TARPC2RTE(ETH_VLAN_TYPE_INNER);
        CASE_TARPC2RTE(ETH_VLAN_TYPE_OUTER);
        CASE_TARPC2RTE(ETH_VLAN_TYPE_MAX);
        default:
            return TE_EINVAL;
    }
    return 0;
}

TARPC_FUNC(rte_eth_dev_set_vlan_ether_type, {},
{
    enum rte_vlan_type vlan_type;
    te_errno           rc;

    rc = tarpc_vlan_type2rte(in->vlan_type, &vlan_type);
    if (rc != 0)
    {
        out->retval = -TE_RC(TE_RPCS, rc);
        goto done;
    }

    MAKE_CALL(out->retval = func(in->port_id, vlan_type, in->tag_type));
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

static const te_enum_bitmask_conv vlan_offload_map[] = {
#define VLAN_OFFLOAD_BIT_MAP(_bit) {                                 \
    .bits_from = UINT64_C(1) << TARPC_ETH_VLAN_##_bit##_OFFLOAD_BIT, \
    .bits_to = RTE_ETH_VLAN_##_bit##_OFFLOAD                         \
}
    VLAN_OFFLOAD_BIT_MAP(STRIP),
    VLAN_OFFLOAD_BIT_MAP(FILTER),
    VLAN_OFFLOAD_BIT_MAP(EXTEND),
#undef VLAN_OFFLOAD_BIT_MAP
    TE_ENUM_BITMASK_CONV_END
};

TARPC_FUNC(rte_eth_dev_set_vlan_offload, {},
{
    uint16_t rte_vlan_offload_mask;
    te_errno rc;

    rc = rpc_dpdk_bitmask16_rpc2rte(in->offload_mask, vlan_offload_map,
                                    &rte_vlan_offload_mask);
    if (rc != 0)
    {
        out->retval = -TE_RC(TE_RPCS, rc);
        goto done;
    }

    MAKE_CALL(out->retval = func(in->port_id, rte_vlan_offload_mask));
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

TARPC_FUNC(rte_eth_dev_set_vlan_pvid, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->pvid, in->on));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC_STATIC(rte_eth_rx_queue_count, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->queue_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC_STATIC(rte_eth_rx_descriptor_status, {},
{
    int ret;

    MAKE_CALL(ret = func(in->port_id, in->queue_id, in->offset));

    switch (ret)
    {
        case RTE_ETH_RX_DESC_AVAIL:
            out->retval = TARPC_RTE_ETH_RX_DESC_AVAIL;
            break;
        case RTE_ETH_RX_DESC_DONE:
            out->retval = TARPC_RTE_ETH_RX_DESC_DONE;
            break;
        case RTE_ETH_RX_DESC_UNAVAIL:
            out->retval = TARPC_RTE_ETH_RX_DESC_UNAVAIL;
            break;
        default:
            if (ret < 0)
            {
                neg_errno_h2rpc(&ret);
                out->retval = ret;
            }
            else
            {
                out->retval = TARPC_RTE_ETH_RX_DESC__UNKNOWN;
            }
            break;
    }
})

TARPC_FUNC_STATIC(rte_eth_tx_descriptor_status, {},
{
    int ret;

    MAKE_CALL(ret = func(in->port_id, in->queue_id, in->offset));

    switch (ret)
    {
        case RTE_ETH_TX_DESC_FULL:
            out->retval = TARPC_RTE_ETH_TX_DESC_FULL;
            break;
        case RTE_ETH_TX_DESC_DONE:
            out->retval = TARPC_RTE_ETH_TX_DESC_DONE;
            break;
        case RTE_ETH_TX_DESC_UNAVAIL:
            out->retval = TARPC_RTE_ETH_TX_DESC_UNAVAIL;
            break;
        default:
            if (ret < 0)
            {
                neg_errno_h2rpc(&ret);
                out->retval = ret;
            }
            else
            {
                out->retval = TARPC_RTE_ETH_TX_DESC__UNKNOWN;
            }
            break;
    }
})

TARPC_FUNC(rte_eth_dev_socket_id, {},
{
    MAKE_CALL(out->retval = func(in->port_id));
})

TARPC_FUNC(rte_eth_dev_is_valid_port, {},
{
    MAKE_CALL(out->retval = func(in->port_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_rx_queue_start, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->queue_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_rx_queue_stop, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->queue_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_tx_queue_start, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->queue_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_tx_queue_stop, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->queue_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_macaddr_get,
{
    COPY_ARG(mac_addr);
},
{
    struct rte_ether_addr mac_addr;
    struct rte_ether_addr *mac_addr_p = &mac_addr;

    if (out->mac_addr.mac_addr_len == 0)
        mac_addr_p = NULL;

    MAKE_CALL(func(in->port_id, mac_addr_p));

    if (mac_addr_p != NULL)
        memcpy(out->mac_addr.mac_addr_val[0].addr_bytes,
               mac_addr_p->addr_bytes,
               sizeof(out->mac_addr.mac_addr_val[0].addr_bytes));
})

TARPC_FUNC(rte_eth_dev_get_vlan_offload, {},
{
    uint64_t mask = 0;
    te_errno rc;

    MAKE_CALL(out->retval = func(in->port_id));

    if (out->retval < 0)
    {
        neg_errno_h2rpc(&out->retval);
    }
    else
    {
        mask = out->retval;
        rc = rpc_dpdk_bitmask64_convert(mask, vlan_offload_map, true, &mask);
        if (rc != 0)
            out->retval = -TE_RC(TE_RPCS, rc);
        else
            out->retval = mask;
    }
})

TARPC_FUNC(rte_eth_dev_default_mac_addr_set, {},
{
    struct rte_ether_addr mac_addr;
    struct rte_ether_addr *mac_addr_p = &mac_addr;

    if (in->mac_addr.mac_addr_len == 0)
        mac_addr_p = NULL;
    else
        memcpy(mac_addr_p->addr_bytes, in->mac_addr.mac_addr_val[0].addr_bytes,
               sizeof(mac_addr_p->addr_bytes));

    MAKE_CALL(out->retval = func(in->port_id, mac_addr_p));

    neg_errno_h2rpc(&out->retval);

})

TARPC_FUNC(rte_eth_rx_queue_info_get, {},
{
    struct rte_eth_rxq_info qinfo;

    MAKE_CALL(out->retval = func(in->port_id, in->queue_id, &qinfo));
    neg_errno_h2rpc(&out->retval);

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        out->qinfo.mp = RCF_PCH_MEM_INDEX_ALLOC(qinfo.mp, ns);
    });

    tarpc_rte_eth_rxconf2rpc(&qinfo.conf,
                             &out->qinfo.conf);

    out->qinfo.scattered_rx = qinfo.scattered_rx;
    out->qinfo.nb_desc = qinfo.nb_desc;
})

TARPC_FUNC(rte_eth_tx_queue_info_get, {},
{
    struct rte_eth_txq_info qinfo;

    MAKE_CALL(out->retval = func(in->port_id, in->queue_id, &qinfo));
    neg_errno_h2rpc(&out->retval);

    tarpc_rte_eth_txconf2rpc(&qinfo.conf,
                             &out->qinfo.conf);

    out->qinfo.nb_desc = qinfo.nb_desc;
})

TARPC_FUNC(rte_eth_dev_rss_reta_query,{},
{
    struct rte_eth_rss_reta_entry64 *reta_conf_p = NULL;
    unsigned int                     reta_conf_len;
    unsigned                         cur_group;

    reta_conf_len = in->reta_conf.reta_conf_len;
    if (reta_conf_len != 0)
    {
        reta_conf_p = TE_ALLOC(reta_conf_len * sizeof(*reta_conf_p));

        for (cur_group = 0; cur_group < reta_conf_len; cur_group++)
        {
            memset(&reta_conf_p[cur_group], 0, sizeof(*reta_conf_p));
            reta_conf_p[cur_group].mask = in->reta_conf.reta_conf_val[cur_group].mask;
        }
    }

    MAKE_CALL(out->retval = func(in->port_id, reta_conf_p, in->reta_size));
    neg_errno_h2rpc(&out->retval);

    if (reta_conf_p != NULL && out->retval == 0)
    {
        out->reta_conf.reta_conf_len = reta_conf_len;
        if (reta_conf_len != 0)
            out->reta_conf.reta_conf_val =
                TE_ALLOC(reta_conf_len * sizeof(*out->reta_conf.reta_conf_val));

        for (cur_group = 0;  cur_group < reta_conf_len; cur_group++)
            memcpy(&out->reta_conf.reta_conf_val[cur_group],
                   &reta_conf_p[cur_group],
                   sizeof(out->reta_conf.reta_conf_val[cur_group]));
    }

    free(reta_conf_p);
})

TARPC_FUNC(rte_eth_dev_rss_hash_conf_get,
{
    COPY_ARG(rss_conf);
},
{
    struct rte_eth_rss_conf  rss_conf;
    struct rte_eth_rss_conf *rss_conf_p;

    if (out->rss_conf.rss_conf_val != NULL)
    {
        /*
         * Buffer length provided to function must be less o equal to
         * the real buffer length to avoid memory corruption.
         */
        if (out->rss_conf.rss_conf_val->rss_key_len >
            out->rss_conf.rss_conf_val->rss_key.rss_key_len)
        {
            out->common._errno = TE_RC(TE_RPCS, TE_EINVAL);
            out->retval = -out->common._errno;
            goto done;
        }

        memset(&rss_conf, 0, sizeof(rss_conf));
        rss_conf.rss_key = out->rss_conf.rss_conf_val->rss_key.rss_key_val;
        rss_conf.rss_key_len = out->rss_conf.rss_conf_val->rss_key_len;
        /*
         * Ignore result since conversion of theoretically unused value
         * is not that important.
         */
        rpc_dpdk_bitmask64_rpc2rte(out->rss_conf.rss_conf_val->rss_hf,
                                   rss_hf_proto_map,
                                   &rss_conf.rss_hf);
        rss_conf_p = &rss_conf;
    }
    else
    {
        rss_conf_p = NULL;
    }

    MAKE_CALL(out->retval = func(in->port_id, rss_conf_p));
    neg_errno_h2rpc(&out->retval);

    if (out->retval == 0 && rss_conf_p != NULL)
    {
        /*
         * It is unexpected, but still possible that the function
         * changes rss_key pointer. We can handle NULL case gracefully
         * here.
         */
        if (rss_conf_p->rss_key == NULL)
        {
            free(out->rss_conf.rss_conf_val->rss_key.rss_key_val);
            out->rss_conf.rss_conf_val->rss_key.rss_key_val = NULL;
            out->rss_conf.rss_conf_val->rss_key.rss_key_len = 0;
        }
        else if (rss_conf_p->rss_key !=
                 out->rss_conf.rss_conf_val->rss_key.rss_key_val)
        {
            ERROR("rte_eth_dev_rss_hash_conf_get(): changed rss_key pointer in an unexpected way");
            out->retval = -TE_RC(TE_RPCS, TE_EFAULT);
        }
        out->rss_conf.rss_conf_val->rss_key_len = rss_conf_p->rss_key_len;
        out->rss_conf.rss_conf_val->rss_hf =
            rpc_dpdk_bitmask64_rte2rpc(rss_conf_p->rss_hf, rss_hf_proto_map,
                                       TARPC_RTE_ETH_RSS__UNKNOWN);
    }

done:
    ;
})

static te_errno
tarpc_rte_eth_fc_mode2rpc(const enum rte_eth_fc_mode rte,
                          enum tarpc_rte_eth_fc_mode *rpc)
{
    switch (rte)
    {
        case RTE_ETH_FC_NONE:
            *rpc = TARPC_RTE_FC_NONE;
            break;
        case RTE_ETH_FC_RX_PAUSE:
            *rpc = TARPC_RTE_FC_RX_PAUSE;
            break;
        case RTE_ETH_FC_TX_PAUSE:
            *rpc = TARPC_RTE_FC_TX_PAUSE;
            break;
        case RTE_ETH_FC_FULL:
            *rpc = TARPC_RTE_FC_FULL;
            break;
        default:
            return TE_EINVAL;
    }

    return 0;
}

static te_errno
tarpc_rpc_eth_fc_mode2rte(const enum tarpc_rte_eth_fc_mode rpc,
                          enum rte_eth_fc_mode *rte)
{
    switch (rpc)
    {
        case TARPC_RTE_FC_NONE:
            *rte = RTE_ETH_FC_NONE;
            break;
        case TARPC_RTE_FC_RX_PAUSE:
            *rte = RTE_ETH_FC_RX_PAUSE;
            break;
        case TARPC_RTE_FC_TX_PAUSE:
            *rte = RTE_ETH_FC_TX_PAUSE;
            break;
        case TARPC_RTE_FC_FULL:
            *rte = RTE_ETH_FC_FULL;
            break;
        default:
            return TE_EINVAL;
    }

    return 0;
}

TARPC_FUNC(rte_eth_dev_flow_ctrl_get,{},
{
    struct rte_eth_fc_conf  fc_conf;
    struct rte_eth_fc_conf *fc_conf_p;

    fc_conf_p = &fc_conf;

    MAKE_CALL(out->retval = func(in->port_id, fc_conf_p));
    neg_errno_h2rpc(&out->retval);

    if (out->retval == 0)
    {
        if (tarpc_rte_eth_fc_mode2rpc(fc_conf_p->mode,
            &out->fc_conf.mode))
            goto done;

        out->fc_conf.high_water = fc_conf_p->high_water;
        out->fc_conf.low_water = fc_conf_p->low_water;
        out->fc_conf.pause_time = fc_conf_p->pause_time;
        out->fc_conf.send_xon = fc_conf_p->send_xon;
        out->fc_conf.mac_ctrl_frame_fwd = fc_conf_p->mac_ctrl_frame_fwd;
        out->fc_conf.autoneg = fc_conf_p->autoneg;
    }

done:
    ;
})

TARPC_FUNC(rte_eth_dev_flow_ctrl_set,{},
{
    struct rte_eth_fc_conf  fc_conf;
    te_errno                rc;

    rc = tarpc_rpc_eth_fc_mode2rte(in->fc_conf.mode, &fc_conf.mode);
    if (rc != 0)
    {
        out->common._errno = TE_RC(TE_RPCS, rc);
        out->retval = -out->common._errno;
        goto done;
    }

    fc_conf.high_water = in->fc_conf.high_water;
    fc_conf.low_water= in->fc_conf.low_water;
    fc_conf.pause_time= in->fc_conf.pause_time;
    fc_conf.send_xon = in->fc_conf.send_xon;
    fc_conf.mac_ctrl_frame_fwd = in->fc_conf.mac_ctrl_frame_fwd;
    fc_conf.autoneg = in->fc_conf.autoneg;

    MAKE_CALL(out->retval = func(in->port_id, &fc_conf));

    neg_errno_h2rpc(&out->retval);

done:
    ;
})

TARPC_FUNC(rte_eth_xstats_get_names,{},
{
    struct rte_eth_xstat_name  *xstats_names = NULL;
    unsigned int                i;

    if (in->size != 0)
    {
        xstats_names = TE_ALLOC(in->size * sizeof(struct rte_eth_xstat_name));
        out->xstats_names.xstats_names_val =
            TE_ALLOC(in->size * sizeof(struct tarpc_rte_eth_xstat_name));
    }

    MAKE_CALL(out->retval = func(in->port_id, xstats_names, in->size));

    if (in->size != 0 && out->retval > 0 &&
        (unsigned int)out->retval <= in->size)
    {
        for (i = 0; i < in->size; i++)
            te_strlcpy(out->xstats_names.xstats_names_val[i].name,
                       xstats_names[i].name, TARPC_RTE_ETH_XSTATS_NAME_SIZE);
    }

    out->xstats_names.xstats_names_len = in->size;

    free(xstats_names);
})

TARPC_FUNC(rte_eth_xstats_get,{},
{
    struct rte_eth_xstat   *xstats = NULL;
    unsigned int            i;

    if (in->n != 0)
    {
        xstats = TE_ALLOC(in->n * sizeof(struct rte_eth_xstat));
        out->xstats.xstats_val =
            TE_ALLOC(in->n * sizeof(struct tarpc_rte_eth_xstat));
    }

    MAKE_CALL(out->retval = func(in->port_id, xstats, in->n));

    if (in->n != 0 && out->retval > 0 && (unsigned int)out->retval <= in->n)
    {
        for (i = 0; i < in->n; i++)
        {
            out->xstats.xstats_val[i].id = xstats[i].id;
            out->xstats.xstats_val[i].value = xstats[i].value;
        }
    }

    out->xstats.xstats_len = in->n;

    free(xstats);
})

TARPC_FUNC(rte_eth_xstats_reset,{},
{
    MAKE_CALL(func(in->port_id));
})

TARPC_FUNC(rte_eth_xstats_get_by_id, {},
{
    uint64_t *values = NULL;

    if (in->n > 0)
        values = TE_ALLOC(in->n * sizeof(*values));

    MAKE_CALL(out->retval = func(in->port_id, in->ids.ids_val,
                                 values, in->n));
    if ((out->retval > 0) &&
        ((unsigned int)out->retval <= in->n))
    {
        out->values.values_val = values;
        out->values.values_len = out->retval;
    }
    else
    {
        neg_errno_h2rpc(&out->retval);
        free(values);
    }

    ;
})

TARPC_FUNC(rte_eth_xstats_get_names_by_id, {},
{
    struct rte_eth_xstat_name *xstat_names = NULL;

    if (in->size > 0)
        xstat_names = TE_ALLOC(in->size * sizeof(*xstat_names));

    MAKE_CALL(out->retval = func(in->port_id, xstat_names,
                                 in->size, in->ids.ids_val));
    if ((out->retval > 0) &&
        ((unsigned int)out->retval <= in->size))
    {
        struct tarpc_rte_eth_xstat_name *xstat_names_out;
        int i;

        xstat_names_out = TE_ALLOC(out->retval * sizeof(*xstat_names_out));

        out->xstat_names.xstat_names_val = xstat_names_out;

        for (i = 0; i < out->retval; ++i)
        {
            te_strlcpy(out->xstat_names.xstat_names_val[i].name,
                       xstat_names[i].name, TARPC_RTE_ETH_XSTATS_NAME_SIZE);
        }

        out->xstat_names.xstat_names_len = out->retval;
    }

    free(xstat_names);
})

TARPC_FUNC(rte_eth_dev_rss_hash_update,{},
{
    struct rte_eth_rss_conf  rss_conf;
    struct rte_eth_rss_conf *rss_conf_p;

    rss_conf_p = &rss_conf;
    memset(rss_conf_p, 0, sizeof(*rss_conf_p));

    if (in->rss_conf.rss_key_len != 0)
        tarpc_eth_rss_conf2rte(&in->rss_conf, rss_conf_p);
    else
        rss_conf_p->rss_key = NULL;

    MAKE_CALL(out->retval = func(in->port_id, rss_conf_p));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_rss_reta_update,{},
{
    struct rte_eth_rss_reta_entry64 *reta_conf_p = NULL;
    unsigned int                     reta_conf_len;

    reta_conf_len = in->reta_conf.reta_conf_len;
    if (reta_conf_len != 0)
    {
        reta_conf_p = TE_ALLOC(reta_conf_len * sizeof(*reta_conf_p));

        memcpy(reta_conf_p, in->reta_conf.reta_conf_val,
               reta_conf_len * sizeof(*reta_conf_p));
    }

    MAKE_CALL(out->retval = func(in->port_id, reta_conf_p, in->reta_size));
    neg_errno_h2rpc(&out->retval);

    free(reta_conf_p);
})

TARPC_FUNC(rte_eth_link_get_nowait, {},
{
    struct rte_eth_link eth_link;

    MAKE_CALL(func(in->port_id, &eth_link));

    out->eth_link.link_speed = eth_link.link_speed;
    out->eth_link.link_duplex = eth_link.link_duplex;
    out->eth_link.link_autoneg = eth_link.link_autoneg;
    out->eth_link.link_status = eth_link.link_status;
})

TARPC_FUNC(rte_eth_link_get, {},
{
    struct rte_eth_link eth_link;

    MAKE_CALL(func(in->port_id, &eth_link));

    out->eth_link.link_speed = eth_link.link_speed;
    out->eth_link.link_duplex = eth_link.link_duplex;
    out->eth_link.link_autoneg = eth_link.link_autoneg;
    out->eth_link.link_status = eth_link.link_status;
})

TARPC_FUNC_STANDALONE(dpdk_eth_await_link_up, {},
{
    unsigned int i;

    for (i = 0; i < in->nb_attempts; ++i)
    {
        struct rte_eth_link eth_link;

        usleep(in->wait_int_ms * 1000);

        memset(&eth_link, 0, sizeof(eth_link));

        MAKE_CALL(rte_eth_link_get_nowait(in->port_id, &eth_link));
        if (eth_link.link_status)
        {
            out->retval = 0;

            usleep(in->after_up_ms * 1000);

            goto done;
        }
    }

    out->retval = -ETIMEDOUT;
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

static te_errno
tarpc_rte_pktmbuf_packet_type_mask2rte(uint32_t  rpc,
                                       uint32_t *rte_out)
{
    uint32_t rte = 0;

#define RTE_PKTMBUF_PTYPE_MASK2RTE(_l, _t) \
    case (TARPC_RTE_PTYPE_##_l##_##_t << TARPC_RTE_PTYPE_##_l##_OFFSET): \
        rte |= RTE_PTYPE_##_l##_##_t;                                    \
        break

    switch (rpc & TARPC_RTE_PTYPE_L2_MASK) {
        case TARPC_RTE_PTYPE_L2_MASK:
            rte |= RTE_PTYPE_L2_MASK;
            break;
        case TARPC_RTE_PTYPE_L2_UNKNOWN:
            break;
        RTE_PKTMBUF_PTYPE_MASK2RTE(L2, ETHER);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L2, ETHER_TIMESYNC);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L2, ETHER_ARP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L2, ETHER_LLDP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L2, ETHER_NSH);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L2, ETHER_VLAN);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L2, ETHER_QINQ);
        default:
            return TE_EINVAL;
    }

    switch (rpc & TARPC_RTE_PTYPE_L3_MASK) {
        case TARPC_RTE_PTYPE_L3_MASK:
            rte |= RTE_PTYPE_L3_MASK;
            break;
        case TARPC_RTE_PTYPE_L3_UNKNOWN:
            break;
        RTE_PKTMBUF_PTYPE_MASK2RTE(L3, IPV4);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L3, IPV4_EXT);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L3, IPV6);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L3, IPV4_EXT_UNKNOWN);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L3, IPV6_EXT);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L3, IPV6_EXT_UNKNOWN);
        default:
            return TE_EINVAL;
    }

    switch (rpc & TARPC_RTE_PTYPE_L4_MASK) {
        case TARPC_RTE_PTYPE_L4_MASK:
            rte |= RTE_PTYPE_L4_MASK;
            break;
        case TARPC_RTE_PTYPE_L4_UNKNOWN:
            break;
        RTE_PKTMBUF_PTYPE_MASK2RTE(L4, TCP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L4, UDP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L4, FRAG);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L4, SCTP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L4, ICMP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(L4, NONFRAG);
        default:
            return TE_EINVAL;
    }

    switch (rpc & TARPC_RTE_PTYPE_TUNNEL_MASK) {
        case TARPC_RTE_PTYPE_TUNNEL_MASK:
            rte |= RTE_PTYPE_TUNNEL_MASK;
            break;
        case TARPC_RTE_PTYPE_TUNNEL_UNKNOWN:
            break;
        RTE_PKTMBUF_PTYPE_MASK2RTE(TUNNEL, IP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(TUNNEL, GRE);
        RTE_PKTMBUF_PTYPE_MASK2RTE(TUNNEL, VXLAN);
        RTE_PKTMBUF_PTYPE_MASK2RTE(TUNNEL, NVGRE);
        RTE_PKTMBUF_PTYPE_MASK2RTE(TUNNEL, GENEVE);
        RTE_PKTMBUF_PTYPE_MASK2RTE(TUNNEL, GRENAT);
#ifdef RTE_PTYPE_TUNNEL_GTPC
        RTE_PKTMBUF_PTYPE_MASK2RTE(TUNNEL, GTPC);
#endif /* RTE_PTYPE_TUNNEL_GTPC */
#ifdef RTE_PTYPE_TUNNEL_GTPU
        RTE_PKTMBUF_PTYPE_MASK2RTE(TUNNEL, GTPU);
#endif /* RTE_PTYPE_TUNNEL_GTPU */
#ifdef RTE_PTYPE_TUNNEL_ESP
        RTE_PKTMBUF_PTYPE_MASK2RTE(TUNNEL, ESP);
#endif /* RTE_PTYPE_TUNNEL_ESP */
        default:
            return TE_EINVAL;
    }

    switch (rpc & TARPC_RTE_PTYPE_INNER_L2_MASK) {
        case TARPC_RTE_PTYPE_INNER_L2_MASK:
            rte |= RTE_PTYPE_INNER_L2_MASK;
            break;
        case TARPC_RTE_PTYPE_INNER_L2_UNKNOWN:
            break;
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L2, ETHER);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L2, ETHER_VLAN);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L2, ETHER_QINQ);
        default:
            return TE_EINVAL;
    }

    switch (rpc & TARPC_RTE_PTYPE_INNER_L3_MASK) {
        case TARPC_RTE_PTYPE_INNER_L3_MASK:
            rte |= RTE_PTYPE_INNER_L3_MASK;
            break;
        case TARPC_RTE_PTYPE_INNER_L3_UNKNOWN:
            break;
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L3, IPV4);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L3, IPV4_EXT);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L3, IPV6);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L3, IPV4_EXT_UNKNOWN);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L3, IPV6_EXT);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L3, IPV6_EXT_UNKNOWN);
        default:
            return TE_EINVAL;
    }

    switch (rpc & TARPC_RTE_PTYPE_INNER_L4_MASK) {
        case TARPC_RTE_PTYPE_INNER_L4_MASK:
            rte |= RTE_PTYPE_INNER_L4_MASK;
        case TARPC_RTE_PTYPE_INNER_L4_UNKNOWN:
            break;
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L4, TCP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L4, UDP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L4, FRAG);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L4, SCTP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L4, ICMP);
        RTE_PKTMBUF_PTYPE_MASK2RTE(INNER_L4, NONFRAG);
        default:
            return TE_EINVAL;
    }

#undef RTE_PKTMBUF_PTYPE_MASK2RTE

    *rte_out = rte;

    return (0);
}

static void
tarpc_rte_pktmbuf_packet_type2rpc_mask(uint32_t *rpc_ptype_mask)
{
    uint32_t rpc;
    uint32_t d;

    /* switch() default */
    d = 0;

    d |= (TARPC_RTE_PTYPE_L2__UNKNOWN << TARPC_RTE_PTYPE_L2_OFFSET);
    d |= (TARPC_RTE_PTYPE_L3__UNKNOWN << TARPC_RTE_PTYPE_L3_OFFSET);
    d |= (TARPC_RTE_PTYPE_L4__UNKNOWN << TARPC_RTE_PTYPE_L4_OFFSET);

    d |= (TARPC_RTE_PTYPE_TUNNEL__UNKNOWN << TARPC_RTE_PTYPE_TUNNEL_OFFSET);
    d |= (TARPC_RTE_PTYPE_INNER_L2__UNKNOWN << TARPC_RTE_PTYPE_INNER_L2_OFFSET);
    d |= (TARPC_RTE_PTYPE_INNER_L3__UNKNOWN << TARPC_RTE_PTYPE_INNER_L3_OFFSET);
    d |= (TARPC_RTE_PTYPE_INNER_L4__UNKNOWN << TARPC_RTE_PTYPE_INNER_L4_OFFSET);

#define RTE_PKTMBUF_PTYPE2RPC_MASK(_l, _t) \
    case RTE_PTYPE_##_l##_##_t:             \
        rpc = TARPC_RTE_PTYPE_##_l##_##_t << TARPC_RTE_PTYPE_##_l##_OFFSET; \
        break

    switch (*rpc_ptype_mask) {
        case 0:
            break;
        case RTE_PTYPE_L2_MASK:
            rpc = TARPC_RTE_PTYPE_L2_MASK;
            break;
        case RTE_PTYPE_L3_MASK:
            rpc = TARPC_RTE_PTYPE_L3_MASK;
            break;
        case RTE_PTYPE_L4_MASK:
            rpc = TARPC_RTE_PTYPE_L4_MASK;
            break;
        case RTE_PTYPE_TUNNEL_MASK:
            rpc = TARPC_RTE_PTYPE_TUNNEL_MASK;
            break;
        case RTE_PTYPE_INNER_L2_MASK:
            rpc = TARPC_RTE_PTYPE_INNER_L2_MASK;
            break;
        case RTE_PTYPE_INNER_L3_MASK:
            rpc = TARPC_RTE_PTYPE_INNER_L3_MASK;
            break;
        case RTE_PTYPE_INNER_L4_MASK:
            rpc = TARPC_RTE_PTYPE_INNER_L4_MASK;
            break;

        RTE_PKTMBUF_PTYPE2RPC_MASK(L2, ETHER);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L2, ETHER_TIMESYNC);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L2, ETHER_ARP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L2, ETHER_LLDP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L2, ETHER_NSH);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L2, ETHER_VLAN);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L2, ETHER_QINQ);

        RTE_PKTMBUF_PTYPE2RPC_MASK(L3, IPV4);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L3, IPV4_EXT);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L3, IPV6);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L3, IPV4_EXT_UNKNOWN);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L3, IPV6_EXT);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L3, IPV6_EXT_UNKNOWN);

        RTE_PKTMBUF_PTYPE2RPC_MASK(L4, TCP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L4, UDP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L4, FRAG);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L4, SCTP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L4, ICMP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(L4, NONFRAG);

        RTE_PKTMBUF_PTYPE2RPC_MASK(TUNNEL, IP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(TUNNEL, GRE);
        RTE_PKTMBUF_PTYPE2RPC_MASK(TUNNEL, VXLAN);
        RTE_PKTMBUF_PTYPE2RPC_MASK(TUNNEL, NVGRE);
        RTE_PKTMBUF_PTYPE2RPC_MASK(TUNNEL, GENEVE);
        RTE_PKTMBUF_PTYPE2RPC_MASK(TUNNEL, GRENAT);
#ifdef RTE_PTYPE_TUNNEL_GTPC
        RTE_PKTMBUF_PTYPE2RPC_MASK(TUNNEL, GTPC);
#endif /* RTE_PTYPE_TUNNEL_GTPC */
#ifdef RTE_PTYPE_TUNNEL_GTPU
        RTE_PKTMBUF_PTYPE2RPC_MASK(TUNNEL, GTPU);
#endif /* RTE_PTYPE_TUNNEL_GTPU */
#ifdef RTE_PTYPE_TUNNEL_ESP
        RTE_PKTMBUF_PTYPE2RPC_MASK(TUNNEL, ESP);
#endif /* RTE_PTYPE_TUNNEL_ESP */

        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L2, ETHER);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L2, ETHER_VLAN);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L2, ETHER_QINQ);

        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L3, IPV4);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L3, IPV4_EXT);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L3, IPV6);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L3, IPV4_EXT_UNKNOWN);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L3, IPV6_EXT);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L3, IPV6_EXT_UNKNOWN);

        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L4, TCP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L4, UDP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L4, FRAG);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L4, SCTP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L4, ICMP);
        RTE_PKTMBUF_PTYPE2RPC_MASK(INNER_L4, NONFRAG);

        default:
            rpc = d;
            break;
    }

#undef RTE_PKTMBUF_PTYPE2RPC_MASK

    *rpc_ptype_mask = rpc;
}

TARPC_FUNC(rte_eth_dev_get_supported_ptypes,{},
{
    int         i;
    uint32_t    ptype_mask;
    uint32_t   *ptypes = NULL;
    te_errno    rc;

    rc = tarpc_rte_pktmbuf_packet_type_mask2rte(in->ptype_mask, &ptype_mask);
    if (rc != 0)
    {
        out->common._errno = TE_RC(TE_RPCS, rc);
        out->retval = -out->common._errno;
        goto done;
    }

    if (in->num != 0)
        ptypes = TE_ALLOC(in->num * sizeof(uint32_t));

    MAKE_CALL(out->retval = func(in->port_id, ptype_mask,
                                 ptypes, in->num));

    neg_errno_h2rpc(&out->retval);

    if (ptypes != NULL && out->retval > 0)
    {
        for (i = 0; i < MIN(in->num, out->retval); i++)
            tarpc_rte_pktmbuf_packet_type2rpc_mask(ptypes + i);
    }

    out->ptypes.ptypes_val = ptypes;
    out->ptypes.ptypes_len = in->num;

done:
    ;
})

TARPC_FUNC(rte_eth_dev_set_mc_addr_list, {},
{
    struct rte_ether_addr *mc_addr_set = NULL;
    unsigned int i;

    if (in->mc_addr_set.mc_addr_set_len != 0)
    {
        mc_addr_set = TE_ALLOC(in->mc_addr_set.mc_addr_set_len *
                               sizeof(*mc_addr_set));

        for (i = 0; i < in->mc_addr_set.mc_addr_set_len; i++)
            memcpy(mc_addr_set[i].addr_bytes,
                   in->mc_addr_set.mc_addr_set_val[i].addr_bytes,
                   sizeof(mc_addr_set[i].addr_bytes));
    }

    MAKE_CALL(out->retval = func(in->port_id, mc_addr_set,
                                 in->mc_addr_set.mc_addr_set_len));

    neg_errno_h2rpc(&out->retval);

    free(mc_addr_set);
})

TARPC_FUNC(rte_eth_dev_fw_version_get,
{
    COPY_ARG_NOTNULL(fw_version);
},
{
    MAKE_CALL(out->retval = func(in->port_id,
                                 out->fw_version.fw_version_val,
                                 out->fw_version.fw_version_len));

    neg_errno_h2rpc(&out->retval);
})

static te_errno
tarpc_rte_eth_tunnel_type2rte(const enum tarpc_rte_eth_tunnel_type  rpc,
                              enum rte_eth_tunnel_type             *rte)
{
    switch (rpc)
    {
        case TARPC_RTE_TUNNEL_TYPE_NONE:
            *rte = RTE_ETH_TUNNEL_TYPE_NONE;
            break;
        case TARPC_RTE_TUNNEL_TYPE_VXLAN:
            *rte = RTE_ETH_TUNNEL_TYPE_VXLAN;
            break;
        case TARPC_RTE_TUNNEL_TYPE_GENEVE:
            *rte = RTE_ETH_TUNNEL_TYPE_GENEVE;
            break;
        case TARPC_RTE_TUNNEL_TYPE_TEREDO:
            *rte = RTE_ETH_TUNNEL_TYPE_TEREDO;
            break;
        case TARPC_RTE_TUNNEL_TYPE_NVGRE:
            *rte = RTE_ETH_TUNNEL_TYPE_NVGRE;
            break;
        case TARPC_RTE_TUNNEL_TYPE_IP_IN_GRE:
            *rte = RTE_ETH_TUNNEL_TYPE_IP_IN_GRE;
            break;
        case TARPC_RTE_L2_TUNNEL_TYPE_E_TAG:
            *rte = RTE_ETH_L2_TUNNEL_TYPE_E_TAG;
            break;
        case TARPC_RTE_TUNNEL_TYPE_MAX:
            *rte = RTE_ETH_TUNNEL_TYPE_MAX;
            break;
        default:
            return TE_EINVAL;
    }
    return 0;
}

TARPC_FUNC(rte_eth_dev_udp_tunnel_port_add, {},
{
    struct rte_eth_udp_tunnel tunnel_udp;
    enum rte_eth_tunnel_type  prot_type;
    te_errno                  rc;

    rc = tarpc_rte_eth_tunnel_type2rte(in->tunnel_udp.prot_type, &prot_type);
    if (rc != 0)
    {
        out->retval = -TE_RC(TE_RPCS, rc);
        goto done;
    }

    tunnel_udp.udp_port = in->tunnel_udp.udp_port;
    tunnel_udp.prot_type = prot_type;

    MAKE_CALL(out->retval = func(in->port_id, &tunnel_udp));
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

TARPC_FUNC(rte_eth_dev_udp_tunnel_port_delete, {},
{
    struct rte_eth_udp_tunnel tunnel_udp;
    enum rte_eth_tunnel_type  prot_type;
    te_errno                  rc;

    rc = tarpc_rte_eth_tunnel_type2rte(in->tunnel_udp.prot_type, &prot_type);
    if (rc != 0)
    {
        out->retval = -TE_RC(TE_RPCS, rc);
        goto done;
    }

    tunnel_udp.udp_port = in->tunnel_udp.udp_port;
    tunnel_udp.prot_type = prot_type;

    MAKE_CALL(out->retval = func(in->port_id, &tunnel_udp));
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

TARPC_FUNC(rte_eth_dev_get_port_by_name, {},
{
    MAKE_CALL(out->retval = func(in->name, &out->port_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_get_name_by_port, {},
{
    out->name = TE_ALLOC(RPC_RTE_ETH_NAME_MAX_LEN);

    MAKE_CALL(out->retval = func(in->port_id, out->name));

    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC_STANDALONE(rte_eth_dev_rx_offload_name, {},
{
#if RTE_VERSION >= RTE_VERSION_NUM(18,2,0,1)
    const char *name;

    MAKE_CALL(name = rte_eth_dev_rx_offload_name(in->offload));
    out->retval = (name != NULL) ? strdup(name) : NULL;
#else
    out->retval = strdup("UNKNOWN");
#endif
})

TARPC_FUNC_STANDALONE(rte_eth_dev_tx_offload_name, {},
{
#if RTE_VERSION >= RTE_VERSION_NUM(18,2,0,1)
    const char *name;

    MAKE_CALL(name = rte_eth_dev_tx_offload_name(in->offload));
    out->retval = (name != NULL) ? strdup(name) : NULL;
#else
    out->retval = strdup("UNKNOWN");
#endif
})

static const te_enum_bitmask_conv rx_metadata_map[] = {
#define RX_METADATA_BIT_MAP(_bit) {                                     \
    .bits_from = UINT64_C(1) << TARPC_RTE_ETH_RX_METADATA_##_bit##_BIT, \
    .bits_to = RTE_ETH_RX_METADATA_##_bit                               \
}
#ifdef RTE_ETH_RX_METADATA_USER_FLAG
    RX_METADATA_BIT_MAP(USER_FLAG),
#endif /* RTE_ETH_RX_METADATA_USER_FLAG */
#ifdef RTE_ETH_RX_METADATA_USER_MARK
    RX_METADATA_BIT_MAP(USER_MARK),
#endif /* RTE_ETH_RX_METADATA_USER_MARK */
#ifdef RTE_ETH_RX_METADATA_TUNNEL_ID
    RX_METADATA_BIT_MAP(TUNNEL_ID),
#endif /* RTE_ETH_RX_METADATA_TUNNEL_ID */
#undef RX_METADATA_BIT_MAP
    TE_ENUM_BITMASK_CONV_END
};

TARPC_FUNC(rte_eth_rx_metadata_negotiate,
{
    COPY_ARG(features);
},
{
    uint64_t  *features = NULL;

    CHECK_ARG_SINGLE_PTR(out, features);

    if (out->features.features_len != 0)
    {
        te_errno  rc;

        features = out->features.features_val;

        rc = rpc_dpdk_bitmask64_rpc2rte(*features, rx_metadata_map, features);
        if (rc != 0)
        {
            out->retval = -TE_RC(TE_RPCS, rc);
            *features = 0;
            goto done;
        }
    }

    MAKE_CALL(out->retval = func(in->port_id, features));

    if (out->features.features_len != 0)
    {
        *features = rpc_dpdk_bitmask64_rte2rpc(
                        *features, rx_metadata_map,
                         TARPC_RTE_ETH_RX_METADATA__UNKNOWN_BIT);
    }

    neg_errno_h2rpc(&out->retval);

done:
    ;
})

const te_enum_trn link_speed_trn[] = {
#define LINK_SPEED_MAP(_speed) {              \
    .from = TARPC_RTE_ETH_SPEED_NUM_##_speed, \
    .to = RTE_ETH_SPEED_NUM_##_speed          \
}
    LINK_SPEED_MAP(10M),
    LINK_SPEED_MAP(100M),
    LINK_SPEED_MAP(1G),
    LINK_SPEED_MAP(2_5G),
    LINK_SPEED_MAP(5G),
    LINK_SPEED_MAP(10G),
    LINK_SPEED_MAP(20G),
    LINK_SPEED_MAP(25G),
    LINK_SPEED_MAP(40G),
    LINK_SPEED_MAP(50G),
    LINK_SPEED_MAP(56G),
    LINK_SPEED_MAP(100G),
#undef LINK_SPEED
    TE_ENUM_TRN_END
};

/*
 * This map is used for translation RTE FEC capa to TARPC FEC capa.
 * From this point of view, a particular mode can be safely considered
 * a position of bit.
 */
static const te_enum_bitmask_conv fec_capa_map[] = {
#define FEC_CAPA_BIT_MAP(_bit) {                                \
    .bits_from = UINT64_C(1) << TARPC_RTE_ETH_FEC_##_bit##_BIT, \
    .bits_to = RTE_ETH_FEC_MODE_CAPA_MASK(_bit)                 \
}
    FEC_CAPA_BIT_MAP(NOFEC),
    FEC_CAPA_BIT_MAP(AUTO),
    FEC_CAPA_BIT_MAP(BASER),
    FEC_CAPA_BIT_MAP(RS),
    FEC_CAPA_BIT_MAP(LLRS),
#undef FEC_CAPA_BIT_MAP
    TE_ENUM_BITMASK_CONV_END
};

static void
tarpc_rte_speed_fec_capa2rpc(const struct rte_eth_fec_capa *rte,
                             struct tarpc_rte_eth_fec_capa *rpc)
{
    rpc->speed = te_enum_translate(link_speed_trn, (int)rte->speed, true,
                                   TARPC_RTE_ETH_SPEED_NUM_UNKNOWN);
    rpc->capa = rpc_dpdk_bitmask32_rte2rpc(rte->capa, fec_capa_map,
                                           TARPC_RTE_ETH_FEC__UNKNOWN_BIT);
}

TARPC_FUNC(rte_eth_fec_get_capability,
{
    COPY_ARG(speed_fec_capa);
},
{
    struct rte_eth_fec_capa *speed_fec_capa = NULL;
    int                      i;

    if (out->speed_fec_capa.speed_fec_capa_len != 0)
        speed_fec_capa = TE_ALLOC(in->num * sizeof(*speed_fec_capa));

    MAKE_CALL(out->retval = func(in->port_id, speed_fec_capa, in->num));

    if (speed_fec_capa != NULL && out->retval > 0)
    {
        for (i = 0; i < out->retval; i++)
        {
            tarpc_rte_speed_fec_capa2rpc(
                        &speed_fec_capa[i],
                        &out->speed_fec_capa.speed_fec_capa_val[i]);
        }
    }

    free(speed_fec_capa);
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

TARPC_FUNC(rte_eth_fec_get,
{
    COPY_ARG(fec_capa);
},
{
    uint32_t *fec_capap = NULL;

    CHECK_ARG_SINGLE_PTR(out, fec_capa);

    if (out->fec_capa.fec_capa_len != 0)
        fec_capap = out->fec_capa.fec_capa_val;

    MAKE_CALL(out->retval = func(in->port_id, fec_capap));

    if (out->fec_capa.fec_capa_len != 0)
    {
        *fec_capap = rpc_dpdk_bitmask32_rte2rpc(
                         *fec_capap, fec_capa_map,
                         TARPC_RTE_ETH_FEC__UNKNOWN_BIT);
    }

    neg_errno_h2rpc(&out->retval);

done:
    ;
})

TARPC_FUNC(rte_eth_fec_set, {},
{
    uint32_t fec_capa;
    te_errno rc;

    rc = rpc_dpdk_bitmask32_rpc2rte(in->fec_capa, fec_capa_map, &fec_capa);
    if (rc != 0)
    {
        out->common._errno = TE_RC(TE_RPCS, rc);
        out->retval = -out->common._errno;
        goto done;
    }

    MAKE_CALL(out->retval = func(in->port_id, fec_capa));

    neg_errno_h2rpc(&out->retval);

done:
    ;
})

TARPC_FUNC(rte_eth_dev_get_reg_info,
{
    COPY_ARG(info);
},
{
    struct rte_dev_reg_info *info = NULL;

    if (out->info.info_len != 0)
    {
        info = TE_ALLOC(out->info.info_len * sizeof(*info));
        info->data = out->info.info_val->data.data_val;
        info->offset = out->info.info_val->offset;
        info->length = out->info.info_val->length;
        info->width = out->info.info_val->width;
        info->version = out->info.info_val->info_version;
    }

    MAKE_CALL(out->retval = func(in->port_id, info));

    if (info != NULL)
    {
        out->info.info_val->offset = info->offset;
        out->info.info_val->length = info->length;
        out->info.info_val->width = info->width;
        out->info.info_val->info_version = info->version;
    }

    neg_errno_h2rpc(&out->retval);

done:
    free(info);
})
