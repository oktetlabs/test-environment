/** @file
 * @brief RPC client API for DPDK Ethernet Device API
 *
 * RPC client API for DPDK Ethernet Device API functions.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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

#include "te_config.h"

#include "log_bufs.h"
#include "tapi_mem.h"
#include "tapi_rpc_internal.h"
#include "tapi_test_log.h"

#include "tarpc.h"

#include "rpcc_dpdk.h"


static const char *
tarpc_rte_eth_rx_offloads2str(te_log_buf *tlbp, uint32_t rx_offloads)
{
    const struct te_log_buf_bit2str rx_offloads2str[] = {
#define TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(_bit) \
        { TARPC_RTE_DEV_RX_OFFLOAD_##_bit##_BIT, #_bit }
        TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(VLAN_STRIP),
        TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(IPV4_CKSUM),
        TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(UDP_CKSUM),
        TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(TCP_CKSUM),
        TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(TCP_LRO),
        TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(QINQ_STRIP),
        TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR(OUTER_IPV4_CKSUM),
#undef TARPC_RTE_DEV_RX_OFFLOAD_BIT2STR
        { 0, NULL }
    };

    return te_bit_mask2log_buf(tlbp, rx_offloads, rx_offloads2str);
}

static const char *
tarpc_rte_eth_tx_offloads2str(te_log_buf *tlbp, uint32_t tx_offloads)
{
    const struct te_log_buf_bit2str tx_offloads2str[] = {
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
#undef TARPC_RTE_DEV_TX_OFFLOAD_BIT2STR
        { 0, NULL }
    };

    return te_bit_mask2log_buf(tlbp, tx_offloads, tx_offloads2str);
}

static const char *
tarpc_rte_eth_dev_flow_types2str(te_log_buf *tlbp, uint64_t rss_flow_types)
{
    const struct te_log_buf_bit2str rss_flow_types2str[] = {
#define TARPC_RTE_ETH_FLOW_TYPE2STR(_bit) \
        { TARPC_RTE_ETH_FLOW_##_bit, #_bit }
        TARPC_RTE_ETH_FLOW_TYPE2STR(IPV4),
        TARPC_RTE_ETH_FLOW_TYPE2STR(FRAG_IPV4),
        TARPC_RTE_ETH_FLOW_TYPE2STR(NONFRAG_IPV4_TCP),
        TARPC_RTE_ETH_FLOW_TYPE2STR(NONFRAG_IPV4_UDP),
        TARPC_RTE_ETH_FLOW_TYPE2STR(NONFRAG_IPV4_SCTP),
        TARPC_RTE_ETH_FLOW_TYPE2STR(NONFRAG_IPV4_OTHER),
        TARPC_RTE_ETH_FLOW_TYPE2STR(IPV6),
        TARPC_RTE_ETH_FLOW_TYPE2STR(FRAG_IPV6),
        TARPC_RTE_ETH_FLOW_TYPE2STR(NONFRAG_IPV6_TCP),
        TARPC_RTE_ETH_FLOW_TYPE2STR(NONFRAG_IPV6_UDP),
        TARPC_RTE_ETH_FLOW_TYPE2STR(NONFRAG_IPV6_SCTP),
        TARPC_RTE_ETH_FLOW_TYPE2STR(NONFRAG_IPV6_OTHER),
        TARPC_RTE_ETH_FLOW_TYPE2STR(L2_PAYLOAD),
        TARPC_RTE_ETH_FLOW_TYPE2STR(IPV6_EX),
        TARPC_RTE_ETH_FLOW_TYPE2STR(IPV6_TCP_EX),
        TARPC_RTE_ETH_FLOW_TYPE2STR(IPV6_UDP_EX),
#undef TARPC_RTE_ETH_FLOW_BIT2STR
        { 0, NULL }
    };

    return te_bit_mask2log_buf(tlbp, rss_flow_types, rss_flow_types2str);
}

static const char *
tarpc_rte_eth_thresh2str(te_log_buf *tlbp,
                         const struct tarpc_rte_eth_thresh *thresh)
{
    te_log_buf_append(tlbp, "{ pthresh=%u, hthresh=%u, wthresh=%u }",
                      thresh->pthresh, thresh->hthresh, thresh->wthresh);
    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_rxconf2str(te_log_buf *tlbp,
                         const struct tarpc_rte_eth_rxconf *rxconf)
{
    if (rxconf == NULL)
    {
        te_log_buf_append(tlbp, "(null)");
        return te_log_buf_get(tlbp);
    }

    te_log_buf_append(tlbp, "{ rx_thresh=");
    tarpc_rte_eth_thresh2str(tlbp, &rxconf->rx_thresh);
    te_log_buf_append(tlbp, ", rx_free_thresh=%u, rx_drop_en=%u, "
                      "rx_deferred_start=%u }",
                      rxconf->rx_free_thresh, rxconf->rx_drop_en,
                      rxconf->rx_deferred_start);
    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_txq_flags2str(te_log_buf *tlbp, uint32_t txq_flags)
{
    const struct te_log_buf_bit2str txq_flags2str[] = {
#define TARPC_RTE_ETH_TXQ_FLAGS_BIT2STR(_bit) \
        { TARPC_RTE_ETH_TXQ_FLAGS_##_bit##_BIT, #_bit }
        TARPC_RTE_ETH_TXQ_FLAGS_BIT2STR(NOMULTSEGS),
        TARPC_RTE_ETH_TXQ_FLAGS_BIT2STR(NOREFCOUNT),
        TARPC_RTE_ETH_TXQ_FLAGS_BIT2STR(NOMULTMEMP),
        TARPC_RTE_ETH_TXQ_FLAGS_BIT2STR(NOVLANOFFL),
        TARPC_RTE_ETH_TXQ_FLAGS_BIT2STR(NOXSUMSCTP),
        TARPC_RTE_ETH_TXQ_FLAGS_BIT2STR(NOXSUMUDP),
        TARPC_RTE_ETH_TXQ_FLAGS_BIT2STR(NOXSUMTCP),
#undef TARPC_RTE_ETH_TXQ_FLAGS_BIT2STR
        { 0, NULL }
    };

    return te_bit_mask2log_buf(tlbp, txq_flags, txq_flags2str);
}

static const char *
tarpc_rte_eth_txconf2str(te_log_buf *tlbp,
                         const struct tarpc_rte_eth_txconf *txconf)
{
    if (txconf == NULL)
    {
        te_log_buf_append(tlbp, "(null)");
        return te_log_buf_get(tlbp);
    }

    te_log_buf_append(tlbp, "{ tx_thresh=");
    tarpc_rte_eth_thresh2str(tlbp, &txconf->tx_thresh);
    te_log_buf_append(tlbp, ", tx_rs_thresh=%u, tx_free_thresh=%u",
                      txconf->tx_rs_thresh, txconf->tx_free_thresh);
    te_log_buf_append(tlbp, ", txq_flags=");
    tarpc_rte_eth_txq_flags2str(tlbp, txconf->txq_flags);
    te_log_buf_append(tlbp, ", tx_deferred_start=%u }",
                      txconf->tx_deferred_start);
    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_dev_desc_lim2str(te_log_buf *tlbp,
                               const struct tarpc_rte_eth_desc_lim *desc_lim)
{
    te_log_buf_append(tlbp, "{ nb_max=%u, nb_min=%u, nb_align=%u }",
                      desc_lim->nb_max, desc_lim->nb_min, desc_lim->nb_align);
    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_speeds2str(te_log_buf *tlbp, uint32_t speeds)
{
    const struct te_log_buf_bit2str speeds2str[] = {
#define TARPC_RTE_ETH_LINK_SPEED_BIT2STR(_bit) \
        { TARPC_RTE_ETH_LINK_SPEED_##_bit, #_bit }
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(FIXED),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(10M_HD),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(10M),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(100M_HD),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(100M),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(1G),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(2_5G),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(5G),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(10G),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(20G),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(25G),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(40G),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(50G),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(56G),
        TARPC_RTE_ETH_LINK_SPEED_BIT2STR(100G),
#undef TARPC_RTE_ETH_LINK_SPEED_BIT2STR
        { 0, NULL }
    };

    return te_bit_mask2log_buf(tlbp, speeds, speeds2str);
}

static const char *
tarpc_rte_eth_dev_info2str(te_log_buf *tlbp,
                           const struct tarpc_rte_eth_dev_info *dev_info)
{
    te_log_buf_append(tlbp, "{ ");

    te_log_buf_append(tlbp, "driver_name=%s, if_index=%u, "
                      "min_rx_bufsize=%u, max_rx_pktlen=%u, "
                      "max_rx_queues=%u, max_tx_queues=%u, "
                      "max_mac_addrs=%u, max_hash_mac_addrs=%u, "
                      "max_vfs=%u, max_vmdq_pools=%u",
                      dev_info->driver_name, dev_info->if_index,
                      dev_info->min_rx_bufsize, dev_info->max_rx_pktlen,
                      dev_info->max_rx_queues, dev_info->max_tx_queues,
                      dev_info->max_mac_addrs, dev_info->max_hash_mac_addrs,
                      dev_info->max_vfs, dev_info->max_vmdq_pools);

    te_log_buf_append(tlbp, ", rx_offload_capa=");
    tarpc_rte_eth_rx_offloads2str(tlbp, dev_info->rx_offload_capa);
    te_log_buf_append(tlbp, ", tx_offload_capa=");
    tarpc_rte_eth_tx_offloads2str(tlbp, dev_info->tx_offload_capa);

    te_log_buf_append(tlbp, ", reta_size=%u, hash_key_size=%u, "
                      "flow_type_rss_offloads=",
                      dev_info->reta_size, dev_info->hash_key_size);
    tarpc_rte_eth_dev_flow_types2str(tlbp, dev_info->flow_type_rss_offloads);

    te_log_buf_append(tlbp, ", default_rxconf=");
    tarpc_rte_eth_rxconf2str(tlbp, &dev_info->default_rxconf);
    te_log_buf_append(tlbp, ", default_txconf=");
    tarpc_rte_eth_txconf2str(tlbp, &dev_info->default_txconf);

    te_log_buf_append(tlbp, ", vmdq_queue_base=%u, vmdq_queue_num=%u, "
                      "vmdq_pool_base=%u",
                      dev_info->vmdq_queue_base, dev_info->vmdq_queue_num,
                      dev_info->vmdq_pool_base);

    te_log_buf_append(tlbp, ", rx_desc_lim=");
    tarpc_rte_eth_dev_desc_lim2str(tlbp, &dev_info->rx_desc_lim);
    te_log_buf_append(tlbp, ", tx_desc_lim=");
    tarpc_rte_eth_dev_desc_lim2str(tlbp, &dev_info->tx_desc_lim);

    te_log_buf_append(tlbp, ", speed_capa=");
    tarpc_rte_eth_speeds2str(tlbp, dev_info->speed_capa);

    te_log_buf_append(tlbp, " }");
    return te_log_buf_get(tlbp);
}

void
rpc_rte_eth_dev_info_get(rcf_rpc_server *rpcs, uint8_t port_id,
                         struct tarpc_rte_eth_dev_info *dev_info)
{
    tarpc_rte_eth_dev_info_get_in   in;
    tarpc_rte_eth_dev_info_get_out  out;
    te_log_buf                     *tlbp;

    if (dev_info == NULL)
        TEST_FAIL("Invalid %s() dev_info argument", __func__);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_info_get", &in, &out);

    *dev_info = out.dev_info;
    dev_info->driver_name = tapi_strdup(dev_info->driver_name);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_info_get, "%u", "dev_info=%s",
                 in.port_id, tarpc_rte_eth_dev_info2str(tlbp, dev_info));
    te_log_buf_free(tlbp);

    RETVAL_VOID(rte_eth_dev_info_get);
}


static const char *
te_log_buf_append_octet_string(te_log_buf *tlbp,
                               const uint8_t *buf, size_t len)
{
    size_t i;

    for (i = 0; i < len; ++i)
        te_log_buf_append(tlbp, "%.2x", buf[i]);

    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_rx_mq_mode2str(te_log_buf *tlbp,
                             enum tarpc_rte_eth_rx_mq_mode mq_mode)
{
    const char *mode;

    switch (mq_mode)
    {
        case TARPC_ETH_MQ_RX_NONE:
            mode = "NONE";
            break;
        case TARPC_ETH_MQ_RX_RSS:
            mode = "RSS";
            break;
        case TARPC_ETH_MQ_RX_DCB:
            mode = "DCB";
            break;
        case TARPC_ETH_MQ_RX_DCB_RSS:
            mode = "DBC+RSS";
            break;
        case TARPC_ETH_MQ_RX_VMDQ_ONLY:
            mode = "VMDQ";
            break;
        case TARPC_ETH_MQ_RX_VMDQ_RSS:
            mode = "VMDQ+RSS";
            break;
        case TARPC_ETH_MQ_RX_VMDQ_DCB:
            mode = "VMDQ+DCB";
            break;
        case TARPC_ETH_MQ_RX_VMDQ_DCB_RSS:
            mode = "VMDQ+DCB+RSS";
            break;
        default:
            mode = "<UNKNOWN>";
            break;
    }
    te_log_buf_append(tlbp, "mq_mode=%s", mode);

    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_rxmode_flags2str(te_log_buf *tlbp, uint16_t flags)
{
    const struct te_log_buf_bit2str rxmode_flags2str[] = {
#define TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(_bit) \
        { TARPC_RTE_ETH_RXMODE_##_bit##_BIT, #_bit }
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(HEADER_SPLIT),
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(HW_IP_CHECKSUM),
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(HW_VLAN_FILTER),
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(HW_VLAN_STRIP),
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(HW_VLAN_EXTEND),
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(JUMBO_FRAME),
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(HW_STRIP_CRC),
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(ENABLE_SCATTER),
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(ENABLE_LRO),
#undef TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR
        { 0, NULL }
    };

    return te_bit_mask2log_buf(tlbp, flags, rxmode_flags2str);
}

static const char *
tarpc_rte_eth_rxmode2str(te_log_buf *tlbp,
                         const struct tarpc_rte_eth_rxmode *rxconf)
{
    te_log_buf_append(tlbp, "{ ");

    tarpc_rte_eth_rx_mq_mode2str(tlbp, rxconf->mq_mode);
    te_log_buf_append(tlbp, ", max_rx_pkt_len=%u", rxconf->max_rx_pkt_len);
    te_log_buf_append(tlbp, ", split_hdr_size=%u, flags=",
                      rxconf->split_hdr_size);
    tarpc_rte_eth_rxmode_flags2str(tlbp, rxconf->flags);

    te_log_buf_append(tlbp, " }");
    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_tx_mq_mode2str(te_log_buf *tlbp,
                             enum tarpc_rte_eth_tx_mq_mode mq_mode)
{
    const char *mode;

    switch (mq_mode)
    {
        case TARPC_ETH_MQ_TX_NONE:
            mode = "NONE";
            break;
        case TARPC_ETH_MQ_TX_DCB:
            mode = "DCB";
            break;
        case TARPC_ETH_MQ_TX_VMDQ_DCB:
            mode = "VMDQ_DCB";
            break;
        case TARPC_ETH_MQ_TX_VMDQ_ONLY:
            mode = "VMDQ_ONLY";
            break;
        default:
            mode = "<UNKNOWN>";
            break;
    }
    te_log_buf_append(tlbp, "mq_mode=%s", mode);

    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_txmode_flags2str(te_log_buf *tlbp, uint16_t flags)
{
    const struct te_log_buf_bit2str txmode_flags2str[] = {
#define TARPC_RTE_DEV_TXMODE_FLAG_BIT2STR(_bit) \
        { TARPC_RTE_ETH_TXMODE_##_bit##_BIT, #_bit }
        TARPC_RTE_DEV_TXMODE_FLAG_BIT2STR(HW_VLAN_REJECT_TAGGED),
        TARPC_RTE_DEV_TXMODE_FLAG_BIT2STR(HW_VLAN_REJECT_UNTAGGED),
        TARPC_RTE_DEV_TXMODE_FLAG_BIT2STR(HW_VLAN_INSERT_PVID),
#undef TARPC_RTE_DEV_TXMODE_FLAG_BIT2STR
        { 0, NULL }
    };

    return te_bit_mask2log_buf(tlbp, flags, txmode_flags2str);
}

static const char *
tarpc_rte_eth_txmode2str(te_log_buf *tlbp,
                         const struct tarpc_rte_eth_txmode *txconf)
{
    te_log_buf_append(tlbp, "{ ");

    tarpc_rte_eth_tx_mq_mode2str(tlbp, txconf->mq_mode);
    te_log_buf_append(tlbp, ", pvid=%u, flags=", txconf->pvid);
    tarpc_rte_eth_txmode_flags2str(tlbp, txconf->flags);

    te_log_buf_append(tlbp, " }");
    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_rss_conf2str(te_log_buf *tlbp,
                           const struct tarpc_rte_eth_rss_conf *rss_conf)
{
    te_log_buf_append(tlbp, "{");

    te_log_buf_append(tlbp, "rss_key=");
    te_log_buf_append_octet_string(tlbp,
                                   rss_conf->rss_key.rss_key_val,
                                   rss_conf->rss_key.rss_key_len);
    te_log_buf_append(tlbp, ", rss_key_len=%u",
                     (unsigned long long)rss_conf->rss_key_len);
    te_log_buf_append(tlbp, ", rss_hf=%#llx",
                     (unsigned long long)rss_conf->rss_hf);

    te_log_buf_append(tlbp, "}");
    return te_log_buf_get(tlbp);
}

static const char *
taprc_rte_eth_rx_adv_conf2str(te_log_buf *tlbp,
    const struct tarpc_rte_eth_rx_adv_conf *rx_conf_adv)
{
    te_log_buf_append(tlbp, "{ rss_conf=");

    tarpc_rte_eth_rss_conf2str(tlbp, &rx_conf_adv->rss_conf);

    te_log_buf_append(tlbp, " }");
    return te_log_buf_get(tlbp);
}


static const char *
tarpc_rte_intr_conf2str(te_log_buf *tlbp,
                        const struct tarpc_rte_intr_conf *intr_conf)
{
    te_log_buf_append(tlbp, "{ lsc=%u, rxq=%u }",
                      intr_conf->lsc, intr_conf->rxq);

    return te_log_buf_get(tlbp);
}


static const char *
tarpc_rte_eth_conf2str(te_log_buf *tlbp,
                       const struct tarpc_rte_eth_conf *eth_conf)
{
    if (eth_conf == NULL)
    {
        te_log_buf_append(tlbp, "(null)");
        return te_log_buf_get(tlbp);
    }

    te_log_buf_append(tlbp, "{ ");

    te_log_buf_append(tlbp, "link_speeds=%#x, rxmode=", eth_conf->link_speeds);
    tarpc_rte_eth_rxmode2str(tlbp, &eth_conf->rxmode);
    te_log_buf_append(tlbp, ", txmode=");
    tarpc_rte_eth_txmode2str(tlbp, &eth_conf->txmode);
    te_log_buf_append(tlbp, ", lbpk_mode=%#x, rx_conf_adv=",
                      eth_conf->lpbk_mode);
    taprc_rte_eth_rx_adv_conf2str(tlbp, &eth_conf->rx_adv_conf);
    te_log_buf_append(tlbp, ", dcb_cap_en=%u, intr_conf=",
                      eth_conf->dcb_capability_en);
    tarpc_rte_intr_conf2str(tlbp, &eth_conf->intr_conf);

    te_log_buf_append(tlbp, " }");
    return te_log_buf_get(tlbp);
}

int
rpc_rte_eth_dev_configure(rcf_rpc_server *rpcs,
                          uint8_t port_id,
                          uint16_t nb_rx_queue,
                          uint16_t nb_tx_queue,
                          const struct tarpc_rte_eth_conf *eth_conf)
{
    tarpc_rte_eth_dev_configure_in      in;
    tarpc_rte_eth_dev_configure_out     out;
    te_log_buf                         *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.nb_rx_queue = nb_rx_queue;
    in.nb_tx_queue = nb_tx_queue;
    if (eth_conf != NULL)
    {
        in.eth_conf.eth_conf_val = tapi_memdup(eth_conf, sizeof(*eth_conf));
        in.eth_conf.eth_conf_len = 1;
    }

    rcf_rpc_call(rpcs, "rte_eth_dev_configure", &in, &out);

    free(in.eth_conf.eth_conf_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_configure, out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_configure, "%u, %u, %u, %s",
                 NEG_ERRNO_FMT,
                 in.port_id, in.nb_rx_queue, in.nb_tx_queue,
                 tarpc_rte_eth_conf2str(tlbp, eth_conf),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_INT(rte_eth_dev_configure, out.retval);
}

void
rpc_rte_eth_dev_close(rcf_rpc_server *rpcs, uint8_t port_id)
{
    tarpc_rte_eth_dev_close_in   in;
    tarpc_rte_eth_dev_close_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_close", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_close, "%hhu", "", in.port_id);
    RETVAL_VOID(rte_eth_dev_close);
}

int
rpc_rte_eth_dev_start(rcf_rpc_server *rpcs, uint8_t port_id)
{
    tarpc_rte_eth_dev_start_in   in;
    tarpc_rte_eth_dev_start_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_start", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_start, out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_start, "%hhu", NEG_ERRNO_FMT,
                 in.port_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_start, out.retval);
}

void
rpc_rte_eth_dev_stop(rcf_rpc_server *rpcs, uint8_t port_id)
{
    tarpc_rte_eth_dev_stop_in   in;
    tarpc_rte_eth_dev_stop_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_stop", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_stop, "%hhu", "", in.port_id);
    RETVAL_VOID(rte_eth_dev_stop);
}
