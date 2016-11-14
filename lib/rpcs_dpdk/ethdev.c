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
#include "rte_eth_ctrl.h"

#include "logger_api.h"

#include "rpc_server.h"
#include "rpcs_dpdk.h"


#define CASE_TARPC2RTE(_rte) \
    case TARPC_##_rte: *rte = (_rte); break

#define CASE_RTE2TARPC(_rte) \
    case (_rte): *rpc = TARPC_##_rte; break

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


static te_bool
tarpc_eth_link_speeds2rte(uint32_t rpc, uint32_t *rte)
{
    /* TODO Do real mapping */
    *rte = rpc;
    return 1;
}

static int
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
            return 0;
    }
    return 1;
}

static int
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
#undef TARPC_RTE_ETH_RXMODE_BIT2MEMBER
    return (rxmode_flags == 0);
}

static int
tarpc_eth_rxmode2rte(const struct tarpc_rte_eth_rxmode *rpc,
                     struct rte_eth_rxmode *rte)
{
    int ret = 1;

    ret &= tarpc_eth_rx_mq_mode2rte(rpc->mq_mode, &rte->mq_mode);
    rte->max_rx_pkt_len = rpc->max_rx_pkt_len;
    rte->split_hdr_size = rpc->split_hdr_size;
    ret &= tarpc_eth_rxmode_flags2rte(rpc->flags, rte);
    return ret;
}

static int
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
            return 0;
    }
    return 1;
}

static int
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
    return (txmode_flags == 0);
}

static int
tarpc_eth_txmode2rte(const struct tarpc_rte_eth_txmode *rpc,
                     struct rte_eth_txmode *rte)
{
    int ret = 1;

    ret &= tarpc_eth_tx_mq_mode2rte(rpc->mq_mode, &rte->mq_mode);
    rte->pvid = rpc->pvid;
    ret &= tarpc_eth_txmode_flags2rte(rpc->flags, rte);
    return ret;
}

static int
tarpc_eth_rss_conf2rte(const struct tarpc_rte_eth_rss_conf *rpc,
                       struct rte_eth_rss_conf *rte)
{
    int ret = 1;

    /* TODO Ideally it should be validated that it is not changed */
    rte->rss_key = rpc->rss_key.rss_key_val;
    rte->rss_key_len = rpc->rss_key_len;
    /* TODO Do real mapping */
    rte->rss_hf = rpc->rss_hf;
    return ret;
}

static int
tarpc_eth_rx_adv_conf2rte(const struct tarpc_rte_eth_rx_adv_conf *rpc,
                          struct rte_eth_conf *rte)
{
    int ret = 1;

    ret &= tarpc_eth_rss_conf2rte(&rpc->rss_conf, &rte->rx_adv_conf.rss_conf);
    return ret;
}

static int
tarpc_intr_conf2rte(const struct tarpc_rte_intr_conf *rpc,
                    struct rte_intr_conf *rte)
{
    rte->lsc = rpc->lsc;
    rte->rxq = rpc->rxq;
    return 1;
}

static int
tarpc_eth_conf2rte(const struct tarpc_rte_eth_conf *rpc,
                   struct rte_eth_conf *rte)
{
    int ret = 1;

    memset(rte, 0, sizeof(*rte));

    ret &= tarpc_eth_link_speeds2rte(rpc->link_speeds, &rte->link_speeds);
    ret &= tarpc_eth_rxmode2rte(&rpc->rxmode, &rte->rxmode);
    ret &= tarpc_eth_txmode2rte(&rpc->txmode, &rte->txmode);
    rte->lpbk_mode = rpc->lpbk_mode;
    ret &= tarpc_eth_rx_adv_conf2rte(&rpc->rx_adv_conf, rte);
    rte->dcb_capability_en = rpc->dcb_capability_en;
    ret &= tarpc_intr_conf2rte(&rpc->intr_conf, &rte->intr_conf);

    return ret;
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

TARPC_FUNC(rte_eth_dev_configure, {},
{
    struct rte_eth_conf eth_conf;
    struct rte_eth_conf *eth_conf_p = &eth_conf;

    if (in->eth_conf.eth_conf_len == 0)
    {
        eth_conf_p = NULL;
    }
    else if (!tarpc_eth_conf2rte(in->eth_conf.eth_conf_val, &eth_conf))
    {
        out->common._errno = TE_RC(TE_RPCS, TE_EINVAL);
        out->retval = -out->common._errno;
        goto done;
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

TARPC_FUNC(rte_eth_dev_start, {},
{
    MAKE_CALL(out->retval = func(in->port_id));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_stop, {},
{
    MAKE_CALL(func(in->port_id));
})

static int
tarpc_eth_thresh2rte(const struct tarpc_rte_eth_thresh *rpc,
                     struct rte_eth_thresh *rte)
{
    memset(rte, 0, sizeof(*rte));

    rte->pthresh = rpc->pthresh;
    rte->hthresh = rpc->hthresh;
    rte->wthresh = rpc->wthresh;

    return 1;
}

static int
tarpc_eth_txq_flags2rte(uint32_t rpc, uint32_t *rte)
{
    *rte = 0;

#define RPC_DEV_TXQ_FLAG2STR(_bit) \
    do {                                                            \
        uint32_t flag = (1 << TARPC_RTE_ETH_TXQ_FLAGS_##_bit##_BIT);\
                                                                    \
        if (rpc & flag)                                             \
        {                                                           \
            rpc &= ~flag;                                           \
            *rte |= ETH_TXQ_FLAGS_##_bit;                           \
        }                                                           \
    } while (0)
    RPC_DEV_TXQ_FLAG2STR(NOMULTSEGS);
    RPC_DEV_TXQ_FLAG2STR(NOREFCOUNT);
    RPC_DEV_TXQ_FLAG2STR(NOMULTMEMP);
    RPC_DEV_TXQ_FLAG2STR(NOVLANOFFL);
    RPC_DEV_TXQ_FLAG2STR(NOXSUMSCTP);
    RPC_DEV_TXQ_FLAG2STR(NOXSUMUDP);
    RPC_DEV_TXQ_FLAG2STR(NOXSUMTCP);
#undef RPC_DEV_TXQ_FLAG2STR

    return (rpc == 0);
}

static int
tarpc_eth_txconf2rte(const struct tarpc_rte_eth_txconf *rpc,
                     struct rte_eth_txconf *rte)
{
    int ret = 1;

    memset(rte, 0, sizeof(*rte));

    ret &= tarpc_eth_thresh2rte(&rpc->tx_thresh, &rte->tx_thresh);
    rte->tx_rs_thresh = rpc->tx_rs_thresh;
    rte->tx_free_thresh = rpc->tx_free_thresh;
    ret &= tarpc_eth_txq_flags2rte(rpc->txq_flags, &rte->txq_flags);
    rte->tx_deferred_start = rpc->tx_deferred_start;

    return ret;
}

TARPC_FUNC(rte_eth_tx_queue_setup, {},
{
    struct rte_eth_txconf eth_txconf;
    struct rte_eth_txconf *eth_txconf_p = &eth_txconf;

    if (in->tx_conf.tx_conf_len == 0)
    {
        eth_txconf_p = NULL;
    }
    else if (!tarpc_eth_txconf2rte(in->tx_conf.tx_conf_val, &eth_txconf))
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

static int
tarpc_eth_rxconf2rte(const struct tarpc_rte_eth_rxconf *rpc,
                     struct rte_eth_rxconf *rte)
{
    int ret = 1;

    memset(rte, 0, sizeof(*rte));

    ret &= tarpc_eth_thresh2rte(&rpc->rx_thresh, &rte->rx_thresh);
    rte->rx_free_thresh = rpc->rx_free_thresh;
    rte->rx_drop_en = rpc->rx_drop_en;
    rte->rx_deferred_start = rpc->rx_deferred_start;

    return ret;
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
    else if (!tarpc_eth_rxconf2rte(in->rx_conf.rx_conf_val, &eth_rxconf))
    {
        out->retval = -TE_RC(TE_RPCS, TE_EINVAL);
        goto done;
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

TARPC_FUNC_STATIC(rte_eth_tx_burst, {},
{
    struct rte_mbuf **tx_pkts;
    uint16_t i;

    if (in->tx_pkts.tx_pkts_len != 0)
        tx_pkts = (struct rte_mbuf **)calloc(in->tx_pkts.tx_pkts_len,
                                             sizeof(*tx_pkts));
    else
        tx_pkts = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        for (i = 0; i < in->tx_pkts.tx_pkts_len; i++)
            tx_pkts[i] = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->tx_pkts.tx_pkts_val[i],
                                                      ns);
    });


    MAKE_CALL(out->retval = func(in->port_id,
                                 in->queue_id,
                                 tx_pkts,
                                 in->tx_pkts.tx_pkts_len));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        for (i = 0; i < MIN(in->tx_pkts.tx_pkts_len, out->retval); i++)
            RCF_PCH_MEM_INDEX_FREE(in->tx_pkts.tx_pkts_val[i], ns);
    });
})

TARPC_FUNC_STATIC(rte_eth_rx_burst, {},
{
    struct rte_mbuf **rx_pkts;
    uint16_t i;

    rx_pkts = (struct rte_mbuf **)calloc(in->nb_pkts, sizeof(*rx_pkts));

    MAKE_CALL(out->rx_pkts.rx_pkts_len = func(in->port_id,
                                              in->queue_id,
                                              rx_pkts,
                                              in->nb_pkts));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        out->rx_pkts.rx_pkts_val = calloc(out->rx_pkts.rx_pkts_len,
                                          sizeof(tarpc_rte_mbuf));
        for (i = 0; i < MIN(in->nb_pkts, out->rx_pkts.rx_pkts_len); i++)
            out->rx_pkts.rx_pkts_val[i] = RCF_PCH_MEM_INDEX_ALLOC(rx_pkts[i], ns);
    });
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
    MAKE_CALL(func(in->port_id));
})

TARPC_FUNC(rte_eth_promiscuous_disable, {},
{
    MAKE_CALL(func(in->port_id));
})

TARPC_FUNC(rte_eth_promiscuous_get, {},
{
    MAKE_CALL(out->retval = func(in->port_id));
})

TARPC_FUNC(rte_eth_allmulticast_enable, {},
{
    MAKE_CALL(func(in->port_id));
})

TARPC_FUNC(rte_eth_allmulticast_disable, {},
{
    MAKE_CALL(func(in->port_id));
})

TARPC_FUNC(rte_eth_allmulticast_get, {},
{
    MAKE_CALL(out->retval = func(in->port_id));
})

TARPC_FUNC(rte_eth_dev_get_mtu,{},
{
    uint16_t mtu;
    te_bool is_mtu_null;

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

static int
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
            return 0;
    }
    return 1;
}

TARPC_FUNC(rte_eth_dev_set_vlan_ether_type, {},
{
    enum rte_vlan_type vlan_type;

    if (!tarpc_vlan_type2rte(in->vlan_type, &vlan_type))
    {
        out->retval = -TE_RC(TE_RPCS, TE_EINVAL);
        goto done;
    }

    MAKE_CALL(out->retval = func(in->port_id, vlan_type, in->tag_type));
    neg_errno_h2rpc(&out->retval);

done:
    ;
})

static int
tarpc_eth_vlan_offload_mask2rte(uint16_t rpc, uint16_t *rte)
{
#define TARPC_RTE_ETH_VLAN_OFFLOAD_BIT2RTE_BIT(_bit) \
    do {                                                            \
        uint16_t flag = (1 << TARPC_ETH_VLAN_##_bit##_OFFLOAD_BIT); \
                                                                    \
        if (rpc & flag)                                             \
        {                                                           \
            rpc &= ~flag;                                           \
            *rte |= ETH_VLAN_##_bit##_OFFLOAD;                      \
        }                                                           \
    } while (0)

    *rte = 0;

    TARPC_RTE_ETH_VLAN_OFFLOAD_BIT2RTE_BIT(STRIP);
    TARPC_RTE_ETH_VLAN_OFFLOAD_BIT2RTE_BIT(FILTER);
    TARPC_RTE_ETH_VLAN_OFFLOAD_BIT2RTE_BIT(EXTEND);
#undef TARPC_RTE_ETH_VLAN_OFFLOAD_BIT2RTE_BIT
    return (rpc == 0);
}

TARPC_FUNC(rte_eth_dev_set_vlan_offload, {},
{
    uint16_t rte_vlan_offload_mask;

    if (!tarpc_eth_vlan_offload_mask2rte(in->offload_mask, &rte_vlan_offload_mask))
    {
        out->retval = -TE_RC(TE_RPCS, TE_EINVAL);
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

TARPC_FUNC_STATIC(rte_eth_rx_descriptor_done, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->queue_id, in->offset));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC_STATIC(rte_eth_rx_queue_count, {},
{
    MAKE_CALL(out->retval = func(in->port_id, in->queue_id));
    neg_errno_h2rpc(&out->retval);
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
    struct ether_addr mac_addr;
    struct ether_addr *mac_addr_p = &mac_addr;

    if (out->mac_addr.mac_addr_len == 0)
        mac_addr_p = NULL;

    MAKE_CALL(func(in->port_id, mac_addr_p));

    if (mac_addr_p != NULL)
        memcpy(out->mac_addr.mac_addr_val[0].addr_bytes,
               mac_addr_p->addr_bytes,
               sizeof(out->mac_addr.mac_addr_val[0].addr_bytes));
})

static int
rte_eth_vlan_offload_mask2tarpc(int rte, uint16_t *rpc)
{
#define RTE_ETH_VLAN_OFFLOAD_BIT2TARPC_BIT(_bit) \
    do {                                                            \
        uint16_t flag = ETH_VLAN_##_bit##_OFFLOAD;                  \
                                                                    \
        if (rte & flag)                                             \
        {                                                           \
            rte &= ~flag;                                           \
            *rpc |= (1 << TARPC_ETH_VLAN_##_bit##_OFFLOAD_BIT);     \
        }                                                           \
    } while (0)
    RTE_ETH_VLAN_OFFLOAD_BIT2TARPC_BIT(STRIP);
    RTE_ETH_VLAN_OFFLOAD_BIT2TARPC_BIT(FILTER);
    RTE_ETH_VLAN_OFFLOAD_BIT2TARPC_BIT(EXTEND);
#undef RTE_ETH_VLAN_OFFLOAD_BIT2TARPC_BIT
    return (rte == 0);
}

TARPC_FUNC(rte_eth_dev_get_vlan_offload, {},
{
    uint16_t mask = 0;

    MAKE_CALL(out->retval = func(in->port_id));

    if (out->retval < 0)
    {
        neg_errno_h2rpc(&out->retval);
    }
    else if (!rte_eth_vlan_offload_mask2tarpc(out->retval, &mask))
    {
        out->retval = -TE_RC(TE_RPCS, TE_EINVAL);
    }
    else
    {
        out->retval = mask;
    }
})

TARPC_FUNC(rte_eth_dev_default_mac_addr_set, {},
{
    struct ether_addr mac_addr;
    struct ether_addr *mac_addr_p = &mac_addr;

    if (in->mac_addr.mac_addr_len == 0)
        mac_addr_p = NULL;
    else
        memcpy(mac_addr_p->addr_bytes, in->mac_addr.mac_addr_val[0].addr_bytes,
               sizeof(mac_addr_p->addr_bytes));

    MAKE_CALL(func(in->port_id, mac_addr_p));

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

TARPC_FUNC(rte_eth_dev_count, {},
{
    MAKE_CALL(out->retval = func());
})

TARPC_FUNC(rte_eth_dev_detach,{},
{
    out->devname.devname_val = malloc(RPC_RTE_ETH_NAME_MAX_LEN);

    MAKE_CALL(out->retval = func(in->port_id, out->devname.devname_val));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_rss_reta_query,{},
{
    struct rte_eth_rss_reta_entry64 *reta_conf_p;
    unsigned                         cur_group;

    if (in->reta_conf.reta_conf_len == 0)
        reta_conf_p = NULL;
    else
    {
        reta_conf_p = calloc(in->reta_conf.reta_conf_len, sizeof(*reta_conf_p));

        for (cur_group = 0; cur_group < in->reta_conf.reta_conf_len; cur_group++)
        {
            memset(&reta_conf_p[cur_group], 0, sizeof(*reta_conf_p));
            reta_conf_p[cur_group].mask = in->reta_conf.reta_conf_val[cur_group].mask;
        }
    }

    MAKE_CALL(out->retval = func(in->port_id, reta_conf_p, in->reta_size));
    neg_errno_h2rpc(&out->retval);

    if (reta_conf_p != NULL && out->retval == 0)
    {
        out->reta_conf.reta_conf_len = in->reta_conf.reta_conf_len;
        out->reta_conf.reta_conf_val = calloc(out->reta_conf.reta_conf_len,
                                              sizeof(*out->reta_conf.reta_conf_val));

        for (cur_group = 0;  cur_group < out->reta_conf.reta_conf_len; cur_group++)
            memcpy(&out->reta_conf.reta_conf_val[cur_group],
                   &reta_conf_p[cur_group],
                   sizeof(out->reta_conf.reta_conf_val[cur_group]));
    }
})

TARPC_FUNC(rte_eth_dev_rss_reta_update,{},
{
    struct rte_eth_rss_reta_entry64  reta_conf;
    struct rte_eth_rss_reta_entry64 *reta_conf_p;

    if (in->reta_conf.reta_conf_len == 0)
        reta_conf_p = NULL;
    else
    {
        reta_conf_p = &reta_conf;
        reta_conf_p->mask = in->reta_conf.reta_conf_val->mask;

        memcpy(reta_conf_p, in->reta_conf.reta_conf_val, sizeof(*reta_conf_p));
    }

    MAKE_CALL(out->retval = func(in->port_id, reta_conf_p, in->reta_size));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eth_dev_rss_hash_conf_get,{},
{
    struct rte_eth_rss_conf  rss_conf;
    struct rte_eth_rss_conf *rss_conf_p;

    rss_conf_p = &rss_conf;
    rss_conf_p->rss_key = malloc(RPC_RSS_HASH_KEY_LEN_DEF);

    out->rss_conf.rss_key.rss_key_len = RPC_RSS_HASH_KEY_LEN_DEF;
    out->rss_conf.rss_key.rss_key_val = malloc(
        out->rss_conf.rss_key.rss_key_len);

    MAKE_CALL(out->retval = func(in->port_id, rss_conf_p));
    neg_errno_h2rpc(&out->retval);

    if (out->retval == 0)
    {
        out->rss_conf.rss_key_len = rss_conf_p->rss_key_len;

        memcpy(out->rss_conf.rss_key.rss_key_val,
               rss_conf_p->rss_key,
               out->rss_conf.rss_key.rss_key_len);

        out->rss_conf.rss_hf = rss_conf_p->rss_hf;
    }
})

static int
tarpc_rte_eth_fc_mode2rpc(const enum rte_eth_rx_mq_mode rte,
                          enum tarpc_rte_eth_fc_mode *rpc)
{
    switch (rte)
    {
        CASE_RTE2TARPC(RTE_FC_NONE);
        CASE_RTE2TARPC(RTE_FC_RX_PAUSE);
        CASE_RTE2TARPC(RTE_FC_TX_PAUSE);
        CASE_RTE2TARPC(RTE_FC_FULL);
        default:
            return 0;
    }

    return 1;
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


static int
tarpc_rte_filter_type2rte(const enum tarpc_rte_filter_type rpc,
                          enum rte_filter_type *rte)
{
    switch (rpc)
    {
        CASE_TARPC2RTE(RTE_ETH_FILTER_NONE);
        CASE_TARPC2RTE(RTE_ETH_FILTER_MACVLAN);
        CASE_TARPC2RTE(RTE_ETH_FILTER_ETHERTYPE);
        CASE_TARPC2RTE(RTE_ETH_FILTER_FLEXIBLE);
        CASE_TARPC2RTE(RTE_ETH_FILTER_SYN);
        CASE_TARPC2RTE(RTE_ETH_FILTER_NTUPLE);
        CASE_TARPC2RTE(RTE_ETH_FILTER_TUNNEL);
        CASE_TARPC2RTE(RTE_ETH_FILTER_FDIR);
        CASE_TARPC2RTE(RTE_ETH_FILTER_HASH);
        CASE_TARPC2RTE(RTE_ETH_FILTER_L2_TUNNEL);
        CASE_TARPC2RTE(RTE_ETH_FILTER_MAX);
        default:
            return 0;
    }

    return 1;
}

TARPC_FUNC(rte_eth_dev_filter_supported,{},
{
    enum rte_filter_type filter_type;

    if (!tarpc_rte_filter_type2rte(in->filter_type,
                                   &filter_type))
    {
        out->common._errno = TE_RC(TE_RPCS, TE_EINVAL);
        out->retval = -out->common._errno;
        goto done;
    }

    MAKE_CALL(out->retval = func(in->port_id, filter_type));
    neg_errno_h2rpc(&out->retval);

done:
    ;
})
