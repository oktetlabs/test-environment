/** @file
 * @brief RPC for DPDK Ethernet Devices API
 *
 * RPC routines implementation to call DPDK (rte_eth_*) functions.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC rte_eth_dev"

#include "te_config.h"
#ifdef HAVE_PACKAGE_H
#include "package.h"
#endif

#include "rte_config.h"
#include "rte_ethdev.h"

#include "logger_api.h"

#include "unix_internal.h"
#include "tarpc_server.h"
#include "rpcs_dpdk.h"


static uint32_t
tarpc_rte_rx_offloads2rpc(uint32_t rte)
{
    uint32_t    rpc = 0;

#define RTE_DEV_RX_OFFLOAD2RPC(_bit) \
    do {                                                            \
        uint32_t flag = DEV_RX_OFFLOAD_##_bit;                      \
                                                                    \
        if (rte & flag)                                             \
        {                                                           \
            rte &= ~flag;                                           \
            rpc |= (1 << TARPC_RTE_DEV_RX_OFFLOAD_##_bit##_BIT);    \
        }                                                           \
    } while (0)
    RTE_DEV_RX_OFFLOAD2RPC(VLAN_STRIP);
    RTE_DEV_RX_OFFLOAD2RPC(IPV4_CKSUM);
    RTE_DEV_RX_OFFLOAD2RPC(UDP_CKSUM);
    RTE_DEV_RX_OFFLOAD2RPC(TCP_CKSUM);
    RTE_DEV_RX_OFFLOAD2RPC(TCP_LRO);
    RTE_DEV_RX_OFFLOAD2RPC(QINQ_STRIP);
    RTE_DEV_RX_OFFLOAD2RPC(OUTER_IPV4_CKSUM);
#undef RTE_DEV_RX_OFFLOAD2RPC
    if (rte != 0)
        rpc = (1 << TARPC_RTE_DEV_RX_OFFLOAD__UNKNOWN_BIT);
    return rpc;
}

static uint32_t
tarpc_rte_tx_offloads2rpc(uint32_t rte)
{
    uint32_t    rpc = 0;

#define RTE_DEV_TX_OFFLOAD2RPC(_bit) \
    do {                                                            \
        uint32_t flag = DEV_TX_OFFLOAD_##_bit;                      \
                                                                    \
        if (rte & flag)                                             \
        {                                                           \
            rte &= ~flag;                                           \
            rpc |= (1 << TARPC_RTE_DEV_TX_OFFLOAD_##_bit##_BIT);    \
        }                                                           \
    } while (0)
    RTE_DEV_TX_OFFLOAD2RPC(VLAN_INSERT);
    RTE_DEV_TX_OFFLOAD2RPC(IPV4_CKSUM);
    RTE_DEV_TX_OFFLOAD2RPC(UDP_CKSUM);
    RTE_DEV_TX_OFFLOAD2RPC(TCP_CKSUM);
    RTE_DEV_TX_OFFLOAD2RPC(SCTP_CKSUM);
    RTE_DEV_TX_OFFLOAD2RPC(TCP_TSO);
    RTE_DEV_TX_OFFLOAD2RPC(UDP_TSO);
    RTE_DEV_TX_OFFLOAD2RPC(OUTER_IPV4_CKSUM);
    RTE_DEV_TX_OFFLOAD2RPC(QINQ_INSERT);
#undef RTE_DEV_TX_OFFLOAD2RPC
    if (rte != 0)
        rpc = (1 << TARPC_RTE_DEV_TX_OFFLOAD__UNKNOWN_BIT);
    return rpc;
}

static uint64_t
tarpc_rte_eth_rss_flow_types2rpc(uint64_t rte)
{
    uint64_t    rpc = 0;

#define RTE_ETH_FLOW_TYPE2RPC(_bit) \
    do {                                                \
        uint64_t flag = ETH_RSS_##_bit;                 \
                                                        \
        if (rte & flag)                                 \
        {                                               \
            rte &= ~flag;                               \
            rpc |= (1 << TARPC_RTE_ETH_FLOW_##_bit);    \
        }                                               \
    } while (0)
    RTE_ETH_FLOW_TYPE2RPC(IPV4);
    RTE_ETH_FLOW_TYPE2RPC(FRAG_IPV4);
    RTE_ETH_FLOW_TYPE2RPC(NONFRAG_IPV4_TCP);
    RTE_ETH_FLOW_TYPE2RPC(NONFRAG_IPV4_UDP);
    RTE_ETH_FLOW_TYPE2RPC(NONFRAG_IPV4_SCTP);
    RTE_ETH_FLOW_TYPE2RPC(NONFRAG_IPV4_OTHER);
    RTE_ETH_FLOW_TYPE2RPC(IPV6);
    RTE_ETH_FLOW_TYPE2RPC(FRAG_IPV6);
    RTE_ETH_FLOW_TYPE2RPC(NONFRAG_IPV6_TCP);
    RTE_ETH_FLOW_TYPE2RPC(NONFRAG_IPV6_UDP);
    RTE_ETH_FLOW_TYPE2RPC(NONFRAG_IPV6_SCTP);
    RTE_ETH_FLOW_TYPE2RPC(NONFRAG_IPV6_OTHER);
    RTE_ETH_FLOW_TYPE2RPC(L2_PAYLOAD);
    RTE_ETH_FLOW_TYPE2RPC(IPV6_EX);
    RTE_ETH_FLOW_TYPE2RPC(IPV6_TCP_EX);
    RTE_ETH_FLOW_TYPE2RPC(IPV6_UDP_EX);
#undef RTE_ETH_FLOW_TYPE2RPC
    if (rte != 0)
        rpc = (1 << TARPC_RTE_ETH_FLOW__UNKNOWN);
    return rpc;
}

static void
tarpc_rte_eth_thresh2rpc(const struct rte_eth_thresh *rte,
                         struct tarpc_rte_eth_thresh *rpc)
{
    rpc->pthresh = rte->pthresh;
    rpc->hthresh = rte->hthresh;
    rpc->wthresh = rte->wthresh;
}

static void
tarpc_rte_eth_rxconf2rpc(const struct rte_eth_rxconf *rte,
                         struct tarpc_rte_eth_rxconf *rpc)
{
    tarpc_rte_eth_thresh2rpc(&rte->rx_thresh, &rpc->rx_thresh);
    rpc->rx_free_thresh = rte->rx_free_thresh;
    rpc->rx_drop_en = rte->rx_drop_en;
    rpc->rx_deferred_start = rte->rx_deferred_start;
}

static uint64_t
tarpc_rte_eth_txq_flags2rpc(uint32_t rte)
{
    uint32_t    rpc = 0;

#define RTE_DEV_TXQ_FLAG2STR(_bit) \
    do {                                                        \
        uint32_t flag = ETH_TXQ_FLAGS_##_bit;                   \
                                                                \
        if (rte & flag)                                         \
        {                                                       \
            rte &= ~flag;                                       \
            rpc |= (1 << TARPC_RTE_ETH_TXQ_FLAGS_##_bit##_BIT); \
        }                                                       \
    } while (0)
    RTE_DEV_TXQ_FLAG2STR(NOMULTSEGS);
    RTE_DEV_TXQ_FLAG2STR(NOREFCOUNT);
    RTE_DEV_TXQ_FLAG2STR(NOMULTMEMP);
    RTE_DEV_TXQ_FLAG2STR(NOVLANOFFL);
    RTE_DEV_TXQ_FLAG2STR(NOXSUMSCTP);
    RTE_DEV_TXQ_FLAG2STR(NOXSUMUDP);
    RTE_DEV_TXQ_FLAG2STR(NOXSUMTCP);
#undef RTE_DEV_TXQ_FLAG2STR
    if (rte != 0)
        rpc = (1 << TARPC_RTE_ETH_TXQ_FLAGS__UNKNOWN_BIT);
    return rpc;
}

static void
tarpc_rte_eth_txconf2rpc(const struct rte_eth_txconf *rte,
                         struct tarpc_rte_eth_txconf *rpc)
{
    tarpc_rte_eth_thresh2rpc(&rte->tx_thresh, &rpc->tx_thresh);
    rpc->tx_rs_thresh = rte->tx_rs_thresh;
    rpc->tx_free_thresh = rte->tx_free_thresh;
    rpc->txq_flags = tarpc_rte_eth_txq_flags2rpc(rte->txq_flags);
    rpc->tx_deferred_start = rte->tx_deferred_start;
}

static void
tarpc_rte_eth_desc_lim2rpc(const struct rte_eth_desc_lim *rte,
                           struct tarpc_rte_eth_desc_lim *rpc)
{
    rpc->nb_max = rte->nb_max;
    rpc->nb_min = rte->nb_min;
    rpc->nb_align = rte->nb_align;
}

static uint32_t
tarpc_rte_eth_link_speeds2rpc(uint32_t rte)
{
    uint32_t    rpc = 0;

#define RTE_ETH_LINK_SPEED2RPC(_bit) \
    do {                                                    \
        uint32_t flag = ETH_LINK_SPEED_##_bit;              \
                                                            \
        if (rte & flag)                                     \
        {                                                   \
            rte &= ~flag;                                   \
            rpc |= (1 << TARPC_RTE_ETH_LINK_SPEED_##_bit);  \
        }                                                   \
    } while (0)
    RTE_ETH_LINK_SPEED2RPC(FIXED);
    RTE_ETH_LINK_SPEED2RPC(10M_HD);
    RTE_ETH_LINK_SPEED2RPC(10M);
    RTE_ETH_LINK_SPEED2RPC(100M_HD);
    RTE_ETH_LINK_SPEED2RPC(100M);
    RTE_ETH_LINK_SPEED2RPC(1G);
    RTE_ETH_LINK_SPEED2RPC(2_5G);
    RTE_ETH_LINK_SPEED2RPC(5G);
    RTE_ETH_LINK_SPEED2RPC(10G);
    RTE_ETH_LINK_SPEED2RPC(20G);
    RTE_ETH_LINK_SPEED2RPC(25G);
    RTE_ETH_LINK_SPEED2RPC(40G);
    RTE_ETH_LINK_SPEED2RPC(50G);
    RTE_ETH_LINK_SPEED2RPC(56G);
    RTE_ETH_LINK_SPEED2RPC(100G);
#undef RTE_ETH_LINK_SPEED2RPC
    if (rte != 0)
        rpc = (1 << TARPC_RTE_ETH_LINK_SPEED__UNKNOWN);
    return rpc;
}

TARPC_FUNC(rte_eth_dev_info_get, {},
{
    struct rte_eth_dev_info dev_info;

    MAKE_CALL(func(in->port_id, &dev_info));

    /* pci_dev is not mapped/returned */
    out->dev_info.driver_name = strdup(dev_info.driver_name);
    out->dev_info.if_index = dev_info.if_index;
    out->dev_info.min_rx_bufsize = dev_info.min_rx_bufsize;
    out->dev_info.max_rx_pktlen = dev_info.max_rx_pktlen;
    out->dev_info.max_rx_queues = dev_info.max_rx_queues;
    out->dev_info.max_tx_queues = dev_info.max_tx_queues;
    out->dev_info.max_mac_addrs = dev_info.max_mac_addrs;
    out->dev_info.max_hash_mac_addrs = dev_info.max_hash_mac_addrs;
    out->dev_info.max_vfs = dev_info.max_vfs;
    out->dev_info.max_vmdq_pools = dev_info.max_vmdq_pools;
    out->dev_info.rx_offload_capa =
        tarpc_rte_rx_offloads2rpc(dev_info.rx_offload_capa);
    out->dev_info.tx_offload_capa =
        tarpc_rte_tx_offloads2rpc(dev_info.tx_offload_capa);
    out->dev_info.reta_size = dev_info.reta_size;
    out->dev_info.hash_key_size = dev_info.hash_key_size;
    out->dev_info.flow_type_rss_offloads =
        tarpc_rte_eth_rss_flow_types2rpc(dev_info.flow_type_rss_offloads);
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
        tarpc_rte_eth_link_speeds2rpc(dev_info.speed_capa);
})
