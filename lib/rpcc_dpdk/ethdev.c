/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC client API for DPDK Ethernet Device API
 *
 * RPC client API for DPDK Ethernet Device API functions.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include "log_bufs.h"
#include "tapi_mem.h"
#include "tapi_rpc_internal.h"
#include "tapi_test_log.h"

#include "tarpc.h"

#include "tapi_rpc_rte_ethdev.h"
#include "tapi_rpc_rte_mbuf.h"
#include "rpcc_dpdk.h"
#include "rpc_dpdk_offloads.h"
#include "te_str.h"


static const char *
tarpc_rte_eth_stats2str(te_log_buf                 *tlbp,
                        struct tarpc_rte_eth_stats *stats)
{
    te_log_buf_append(tlbp, "{ ipackets = %llu, opackets = %llu, "
                      "ibytes = %llu, obytes = %llu, imissed = %llu, "
                      "ierrors = %llu, oerrors = %llu, rx_nombuf = %llu }",
                      stats->ipackets, stats->opackets, stats->ibytes,
                      stats->obytes, stats->imissed, stats->ierrors,
                      stats->oerrors, stats->rx_nombuf);

    return te_log_buf_get(tlbp);
}

int
rpc_rte_eth_stats_get(rcf_rpc_server             *rpcs,
                      uint16_t                    port_id,
                      struct tarpc_rte_eth_stats *stats)
{
    struct tarpc_rte_eth_stats_get_in   in;
    struct tarpc_rte_eth_stats_get_out  out;
    te_log_buf                         *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (stats == NULL)
        TEST_FAIL("Invalid %s() 'stats' argument", __func__);

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_stats_get", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_stats_get, out.retval);

    *stats = out.stats;

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_stats_get, "%hu",
                 "stats = %s", in.port_id,
                 tarpc_rte_eth_stats2str(tlbp, stats));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_stats_get, out.retval);
}

static const char *
tarpc_rte_eth_rx_offloads2str(te_log_buf *tlbp, uint64_t rx_offloads)
{
    struct te_log_buf_bit2str *rx_offloads2str;
    const char *result;
    unsigned int i;

    /* Two more for unsupported and NULL */
    rx_offloads2str = tapi_calloc(rpc_dpdk_rx_offloads_num + 2,
                                  sizeof(*rx_offloads2str));

    for (i = 0; i < rpc_dpdk_rx_offloads_num; i++)
    {
        rx_offloads2str[i].bit = rpc_dpdk_rx_offloads[i].bit;
        rx_offloads2str[i].str = rpc_dpdk_rx_offloads[i].name;
    }

    rx_offloads2str[rpc_dpdk_rx_offloads_num].bit =
            TARPC_RTE_DEV_RX_OFFLOAD__UNSUPPORTED_BIT;
    rx_offloads2str[rpc_dpdk_rx_offloads_num].str = "_UNSUPPORTED";

    result = te_bit_mask2log_buf(tlbp, rx_offloads, rx_offloads2str);

    free(rx_offloads2str);

    return result;
}

static const char *
tarpc_rte_eth_tx_offloads2str(te_log_buf *tlbp, uint64_t tx_offloads)
{
    struct te_log_buf_bit2str *tx_offloads2str;
    const char *result;
    unsigned int i;

    /* Two more for unsupported and NULL */
    tx_offloads2str = tapi_calloc(rpc_dpdk_tx_offloads_num + 2,
                                  sizeof(*tx_offloads2str));

    for (i = 0; i < rpc_dpdk_tx_offloads_num; i++)
    {
        tx_offloads2str[i].bit = rpc_dpdk_tx_offloads[i].bit;
        tx_offloads2str[i].str = rpc_dpdk_tx_offloads[i].name;
    }

    tx_offloads2str[rpc_dpdk_tx_offloads_num].bit =
            TARPC_RTE_DEV_TX_OFFLOAD__UNSUPPORTED_BIT;
    tx_offloads2str[rpc_dpdk_tx_offloads_num].str = "_UNSUPPORTED";

    result = te_bit_mask2log_buf(tlbp, tx_offloads, tx_offloads2str);

    free(tx_offloads2str);

    return result;
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

    te_log_buf_append(tlbp, "{ ");

    te_log_buf_append(tlbp, "rx_thresh=");
    tarpc_rte_eth_thresh2str(tlbp, &rxconf->rx_thresh);
    te_log_buf_append(tlbp, ", rx_free_thresh=%u, rx_drop_en=%u, "
                      "rx_deferred_start=%u",
                      rxconf->rx_free_thresh, rxconf->rx_drop_en,
                      rxconf->rx_deferred_start);
    te_log_buf_append(tlbp, ", offloads=");
    tarpc_rte_eth_rx_offloads2str(tlbp, rxconf->offloads);

    te_log_buf_append(tlbp, " }");

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
        TARPC_RTE_ETH_TXQ_FLAGS_BIT2STR(IGNORE),
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

    te_log_buf_append(tlbp, "{ ");

    te_log_buf_append(tlbp, "tx_thresh=");
    tarpc_rte_eth_thresh2str(tlbp, &txconf->tx_thresh);
    te_log_buf_append(tlbp, ", tx_rs_thresh=%u, tx_free_thresh=%u",
                      txconf->tx_rs_thresh, txconf->tx_free_thresh);
    te_log_buf_append(tlbp, ", txq_flags=");
    tarpc_rte_eth_txq_flags2str(tlbp, txconf->txq_flags);
    te_log_buf_append(tlbp, ", tx_deferred_start=%u",
                      txconf->tx_deferred_start);
    te_log_buf_append(tlbp, ", offloads=");
    tarpc_rte_eth_tx_offloads2str(tlbp, txconf->offloads);

    te_log_buf_append(tlbp, " }");

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

static const struct te_log_buf_bit2str tapi_rpc_rte_eth_speeds2str[] = {
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

uint32_t
tapi_rpc_rte_eth_link_speeds_str2val(const char *str)
{
    size_t i = 0;

    for (i = 0; tapi_rpc_rte_eth_speeds2str[i].str != NULL; ++i)
        if (strcmp(str, tapi_rpc_rte_eth_speeds2str[i].str) == 0)
            return 1u << tapi_rpc_rte_eth_speeds2str[i].bit;

    return 0;
}

static const char *
tarpc_rte_eth_speeds2str(te_log_buf *tlbp, uint32_t speeds)
{
    return te_bit_mask2log_buf(tlbp, speeds, tapi_rpc_rte_eth_speeds2str);
}

static const char *
tarpc_rte_eth_dev_portconf2str(te_log_buf *tlbp,
                               const struct tarpc_rte_eth_dev_portconf *portconf)
{
    if (portconf == NULL)
    {
        te_log_buf_append(tlbp, "(null)");
        return te_log_buf_get(tlbp);
    }

    te_log_buf_append(tlbp, "{ burst_size=%u, ring_size=%u, nb_queues=%u }",
                      portconf->burst_size, portconf->ring_size,
                      portconf->nb_queues);

    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_dev_capa2str(te_log_buf *tlbp, uint64_t capa)
{
    const struct te_log_buf_bit2str capa2str[] = {
#define TARPC_RTE_ETH_DEV_CAPA_BIT2STR(_bit) \
        {TARPC_RTE_ETH_DEV_CAPA_##_bit##_BIT, #_bit }
        TARPC_RTE_ETH_DEV_CAPA_BIT2STR(RUNTIME_RX_QUEUE_SETUP),
        TARPC_RTE_ETH_DEV_CAPA_BIT2STR(RUNTIME_TX_QUEUE_SETUP),
        TARPC_RTE_ETH_DEV_CAPA_BIT2STR(_UNSUPPORTED),
        TARPC_RTE_ETH_DEV_CAPA_BIT2STR(_UNKNOWN),
#undef TARPC_RTE_ETH_DEV_CAPA_BIT2STR
        { 0, NULL }
    };

    return te_bit_mask2log_buf(tlbp, capa, capa2str);
}

static const char *
tarpc_rte_eth_dev_info2str(te_log_buf *tlbp,
                           const struct tarpc_rte_eth_dev_info *dev_info)
{
    te_log_buf_append(tlbp, "{ ");

    te_log_buf_append(tlbp, "driver_name=%s, if_index=%u, "
                      "min_mtu=%u, max_mtu=%u, "
                      "min_rx_bufsize=%u, max_rx_pktlen=%u, "
                      "max_rx_queues=%u, max_tx_queues=%u, "
                      "max_mac_addrs=%u, max_hash_mac_addrs=%u, "
                      "max_vfs=%u, max_vmdq_pools=%u",
                      dev_info->driver_name, dev_info->if_index,
                      dev_info->min_mtu, dev_info->max_mtu,
                      dev_info->min_rx_bufsize, dev_info->max_rx_pktlen,
                      dev_info->max_rx_queues, dev_info->max_tx_queues,
                      dev_info->max_mac_addrs, dev_info->max_hash_mac_addrs,
                      dev_info->max_vfs, dev_info->max_vmdq_pools);

    te_log_buf_append(tlbp, ", rx_queue_offload_capa=");
    tarpc_rte_eth_rx_offloads2str(tlbp, dev_info->rx_queue_offload_capa);
    te_log_buf_append(tlbp, ", rx_offload_capa=");
    tarpc_rte_eth_rx_offloads2str(tlbp, dev_info->rx_offload_capa);
    te_log_buf_append(tlbp, ", tx_queue_offload_capa=");
    tarpc_rte_eth_tx_offloads2str(tlbp, dev_info->tx_queue_offload_capa);
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

    te_log_buf_append(tlbp, ", nb_rx_queues=");
    tarpc_rte_eth_speeds2str(tlbp, dev_info->nb_rx_queues);
    te_log_buf_append(tlbp, ", nb_tx_queues=");
    tarpc_rte_eth_speeds2str(tlbp, dev_info->nb_tx_queues);

    te_log_buf_append(tlbp, ", default_rxportconf=");
    tarpc_rte_eth_dev_portconf2str(tlbp, &dev_info->default_rxportconf);
    te_log_buf_append(tlbp, ", default_txportconf=");
    tarpc_rte_eth_dev_portconf2str(tlbp, &dev_info->default_txportconf);

    te_log_buf_append(tlbp, ", dev_capa=");
    tarpc_rte_eth_dev_capa2str(tlbp, dev_info->dev_capa);

    te_log_buf_append(tlbp, " }");
    return te_log_buf_get(tlbp);
}

void
rpc_rte_eth_dev_info_get(rcf_rpc_server *rpcs, uint16_t port_id,
                         struct tarpc_rte_eth_dev_info *dev_info)
{
    tarpc_rte_eth_dev_info_get_in   in;
    tarpc_rte_eth_dev_info_get_out  out;
    te_log_buf                     *tlbp = NULL;

    if (dev_info == NULL)
        TEST_FAIL("Invalid %s() dev_info argument", __func__);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_info_get", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        *dev_info = out.dev_info;
        dev_info->driver_name = tapi_strdup(dev_info->driver_name);

        tlbp = te_log_buf_alloc();
    }

    TAPI_RPC_LOG(rpcs, rte_eth_dev_info_get, "%u", "dev_info=%s",
                 in.port_id,
                 RPC_IS_CALL_OK(rpcs) ?
                    tarpc_rte_eth_dev_info2str(tlbp, dev_info) : "N/A");

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
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(HW_TIMESTAMP),
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(SECURITY),
        TARPC_RTE_DEV_RXMODE_FLAG_BIT2STR(IGNORE_OFFLOAD_BITFIELD),
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
    te_log_buf_append(tlbp, ", mtu=%u", rxconf->mtu);
    te_log_buf_append(tlbp, ", split_hdr_size=%u", rxconf->split_hdr_size);
    te_log_buf_append(tlbp, ", offloads=");
    tarpc_rte_eth_rx_offloads2str(tlbp, rxconf->offloads);
    te_log_buf_append(tlbp, ", flags=");
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
    te_log_buf_append(tlbp, ", offloads=");
    tarpc_rte_eth_tx_offloads2str(tlbp, txconf->offloads);
    te_log_buf_append(tlbp, ", pvid=%u, flags=", txconf->pvid);
    tarpc_rte_eth_txmode_flags2str(tlbp, txconf->flags);

    te_log_buf_append(tlbp, " }");
    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rss_hash_protos2str(te_log_buf *tlbp, tarpc_rss_hash_protos_t protos)
{
    te_bool             added = FALSE;

#define TARPC_RSS_HASH_PROTO2STR(_proto)                                     \
    do {                                                                     \
        tarpc_rss_hash_protos_t proto = 1ULL << TARPC_RTE_ETH_FLOW_##_proto; \
                                                                             \
        if ((protos & proto) == proto)                                       \
        {                                                                    \
            te_log_buf_append(tlbp, "%s%s", added ? "|" : "", #_proto);      \
            added = TRUE;                                                    \
            protos &= ~(1ULL << TARPC_RTE_ETH_FLOW_##_proto);                \
        }                                                                    \
    } while (0)
    TARPC_RSS_HASH_PROTO2STR(IPV4);
    TARPC_RSS_HASH_PROTO2STR(FRAG_IPV4);
    TARPC_RSS_HASH_PROTO2STR(NONFRAG_IPV4_TCP);
    TARPC_RSS_HASH_PROTO2STR(NONFRAG_IPV4_UDP);
    TARPC_RSS_HASH_PROTO2STR(NONFRAG_IPV4_SCTP);
    TARPC_RSS_HASH_PROTO2STR(NONFRAG_IPV4_OTHER);
    TARPC_RSS_HASH_PROTO2STR(IPV6);
    TARPC_RSS_HASH_PROTO2STR(FRAG_IPV6);
    TARPC_RSS_HASH_PROTO2STR(NONFRAG_IPV6_TCP);
    TARPC_RSS_HASH_PROTO2STR(NONFRAG_IPV6_UDP);
    TARPC_RSS_HASH_PROTO2STR(NONFRAG_IPV6_SCTP);
    TARPC_RSS_HASH_PROTO2STR(NONFRAG_IPV6_OTHER);
    TARPC_RSS_HASH_PROTO2STR(L2_PAYLOAD);
    TARPC_RSS_HASH_PROTO2STR(IPV6_EX);
    TARPC_RSS_HASH_PROTO2STR(IPV6_TCP_EX);
    TARPC_RSS_HASH_PROTO2STR(IPV6_UDP_EX);
    TARPC_RSS_HASH_PROTO2STR(PORT);
    TARPC_RSS_HASH_PROTO2STR(VXLAN);
    TARPC_RSS_HASH_PROTO2STR(GENEVE);
    TARPC_RSS_HASH_PROTO2STR(NVGRE);
#undef TARPC_RSS_HASH_PROTO2STR

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
    te_log_buf_append(tlbp, ", rss_hf=");
    tarpc_rss_hash_protos2str(tlbp, rss_conf->rss_hf);

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
                          uint16_t port_id,
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

    RETVAL_ZERO_INT(rte_eth_dev_configure, out.retval);
}

void
rpc_rte_eth_dev_close(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_dev_close_in   in;
    tarpc_rte_eth_dev_close_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_close", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_close, "%hu", "", in.port_id);
    RETVAL_VOID(rte_eth_dev_close);
}

int
rpc_rte_eth_dev_reset(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_dev_reset_in   in;
    tarpc_rte_eth_dev_reset_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_reset", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_reset, out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_reset, "%hu", NEG_ERRNO_FMT,
                 in.port_id, NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_eth_dev_reset, out.retval);
}

int
rpc_rte_eth_dev_start(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_dev_start_in   in;
    tarpc_rte_eth_dev_start_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_start", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_start, out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_start, "%hu", NEG_ERRNO_FMT,
                 in.port_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_start, out.retval);
}

void
rpc_rte_eth_dev_stop(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_dev_stop_in   in;
    tarpc_rte_eth_dev_stop_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_stop", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_stop, "%hu", "", in.port_id);
    RETVAL_VOID(rte_eth_dev_stop);
}

int
rpc_rte_eth_tx_queue_setup(rcf_rpc_server *rpcs,
                           uint16_t port_id,
                           uint16_t tx_queue_id,
                           uint16_t nb_tx_desc,
                           unsigned int socket_id,
                           struct tarpc_rte_eth_txconf *tx_conf)
{
    tarpc_rte_eth_tx_queue_setup_in     in;
    tarpc_rte_eth_tx_queue_setup_out    out;
    te_log_buf                         *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.tx_queue_id = tx_queue_id;
    in.nb_tx_desc = nb_tx_desc;
    in.socket_id = socket_id;
    if (tx_conf != NULL)
    {
        in.tx_conf.tx_conf_val = tapi_memdup(tx_conf, sizeof(*tx_conf));
        in.tx_conf.tx_conf_len = 1;
    }

    rcf_rpc_call(rpcs, "rte_eth_tx_queue_setup", &in, &out);

    free(in.tx_conf.tx_conf_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_tx_queue_setup, out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_tx_queue_setup,
                 "%hu, %hu, %hu, %u, %s", NEG_ERRNO_FMT,
                 in.port_id, in.tx_queue_id, in.nb_tx_desc, in.socket_id,
                 tarpc_rte_eth_txconf2str(tlbp, tx_conf),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_tx_queue_setup, out.retval);
}

int
rpc_rte_eth_rx_queue_setup(rcf_rpc_server *rpcs,
                           uint16_t port_id,
                           uint16_t rx_queue_id,
                           uint16_t nb_rx_desc,
                           unsigned int socket_id,
                           struct tarpc_rte_eth_rxconf *rx_conf,
                           rpc_rte_mempool_p mp)
{
    tarpc_rte_eth_rx_queue_setup_in     in;
    tarpc_rte_eth_rx_queue_setup_out    out;
    te_log_buf                         *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.rx_queue_id = rx_queue_id;
    in.nb_rx_desc = nb_rx_desc;
    in.socket_id = socket_id;
    in.mp = (tarpc_rte_mempool)mp;
    if (rx_conf != NULL)
    {
        in.rx_conf.rx_conf_val = tapi_memdup(rx_conf, sizeof(*rx_conf));
        in.rx_conf.rx_conf_len = 1;
    }

    rcf_rpc_call(rpcs, "rte_eth_rx_queue_setup", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_rx_queue_setup, out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_rx_queue_setup,
                 "%hu, %hu, %hu, %u, %s, " RPC_PTR_FMT, NEG_ERRNO_FMT,
                 in.port_id, in.rx_queue_id, in.nb_rx_desc, in.socket_id,
                 tarpc_rte_eth_rxconf2str(tlbp, rx_conf), RPC_PTR_VAL(mp),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_rx_queue_setup, out.retval);
}

int
rpc_rte_eth_dev_rx_intr_enable(rcf_rpc_server *rpcs,
                               uint16_t port_id,
                               uint16_t queue_id)
{
    tarpc_rte_eth_dev_rx_intr_enable_in     in;
    tarpc_rte_eth_dev_rx_intr_enable_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_rx_intr_enable", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_rx_intr_enable,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_rx_intr_enable, "%hu, %hu", NEG_ERRNO_FMT,
                 in.port_id, in.queue_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_rx_intr_enable, out.retval);
}

int
rpc_rte_eth_dev_rx_intr_disable(rcf_rpc_server *rpcs,
                                uint16_t port_id,
                                uint16_t queue_id)
{
    tarpc_rte_eth_dev_rx_intr_disable_in     in;
    tarpc_rte_eth_dev_rx_intr_disable_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_rx_intr_disable", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_rx_intr_disable,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_rx_intr_disable, "%hu, %hu", NEG_ERRNO_FMT,
                 in.port_id, in.queue_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_rx_intr_disable, out.retval);
}

static const char *
tarpc_rte_intr_op2str(te_log_buf *tlbp, enum tarpc_rte_intr_op op)
{
    const char *str;

    switch (op)
    {
#define CASE_TARPC_RTE_INTR2STR(_op) \
        case TARPC_RTE_INTR_##_op: str = "RTE_INTR_" #_op; break

        CASE_TARPC_RTE_INTR2STR(EVENT_ADD);
        CASE_TARPC_RTE_INTR2STR(EVENT_DEL);
#undef CASE_TARPC_RTE_INTR2STR
        default:
            str = "<UNKNOWN>";
            break;
    }
    te_log_buf_append(tlbp, "%s", str);

    return te_log_buf_get(tlbp);
}

int
rpc_rte_eth_dev_rx_intr_ctl_q(rcf_rpc_server *rpcs,
                              uint16_t port_id,
                              uint16_t queue_id,
                              int epfd,
                              enum tarpc_rte_intr_op op,
                              uint64_t data)
{
    tarpc_rte_eth_dev_rx_intr_ctl_q_in      in;
    tarpc_rte_eth_dev_rx_intr_ctl_q_out     out;
    te_log_buf                             *tlbp;


    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;
    in.epfd = epfd;
    in.op = op;
    in.data = data;

    rcf_rpc_call(rpcs, "rte_eth_dev_rx_intr_ctl_q", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_rx_intr_ctl_q,
                                          out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_rx_intr_ctl_q, "%hu, %hu, %d, %s, %" PRIu64,
                 NEG_ERRNO_FMT, in.port_id, in.queue_id,
                 in.epfd, tarpc_rte_intr_op2str(tlbp, in.op),
                 in.data, NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_dev_rx_intr_ctl_q, out.retval);
}

uint16_t
rpc_rte_eth_tx_burst(rcf_rpc_server *rpcs,
                     uint16_t port_id,
                     uint16_t queue_id,
                     rpc_rte_mbuf_p *tx_pkts,
                     uint16_t nb_pkts)
{
    tarpc_rte_eth_tx_burst_in     in;
    tarpc_rte_eth_tx_burst_out    out;
    te_log_buf                   *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;
    if (tx_pkts != NULL)
    {
        in.tx_pkts.tx_pkts_len = nb_pkts;
        in.tx_pkts.tx_pkts_val = (tarpc_rte_mbuf *)tapi_memdup(tx_pkts, nb_pkts *
                                                               sizeof(*tx_pkts));
    }

    rcf_rpc_call(rpcs, "rte_eth_tx_burst", &in, &out);

    CHECK_RETVAL_VAR_ERR_COND(rte_eth_tx_burst, out.retval,
                              out.retval > in.tx_pkts.tx_pkts_len,
                              out.retval,
                              out.retval > in.tx_pkts.tx_pkts_len);

    free(in.tx_pkts.tx_pkts_val);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_tx_burst, "%hu, %hu, %s, %hu", "%hu",
                 in.port_id, in.queue_id,
                 rpc_rte_mbufs2str(tlbp, tx_pkts, in.tx_pkts.tx_pkts_len,
                                   rpcs),
                 in.tx_pkts.tx_pkts_len, out.retval);
    te_log_buf_free(tlbp);

    TAPI_RPC_OUT(rte_eth_tx_burst, out.retval > in.tx_pkts.tx_pkts_len);

    return out.retval;
}

uint16_t
rpc_rte_eth_tx_prepare(rcf_rpc_server *rpcs,
                       uint16_t        port_id,
                       uint16_t        queue_id,
                       rpc_rte_mbuf_p *tx_pkts,
                       uint16_t        nb_pkts)
{
    tarpc_rte_eth_tx_prepare_in   in;
    tarpc_rte_eth_tx_prepare_out  out;
    te_log_buf                   *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;
    if (tx_pkts != NULL)
    {
        in.tx_pkts.tx_pkts_len = nb_pkts;
        in.tx_pkts.tx_pkts_val =
            (tarpc_rte_mbuf *)tapi_memdup(tx_pkts, nb_pkts * sizeof(*tx_pkts));
    }

    rcf_rpc_call(rpcs, "rte_eth_tx_prepare", &in, &out);

    CHECK_RETVAL_VAR_ERR_COND(rte_eth_tx_prepare, out.retval,
                              out.retval > in.tx_pkts.tx_pkts_len,
                              out.retval,
                              out.retval > in.tx_pkts.tx_pkts_len);

    free(in.tx_pkts.tx_pkts_val);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_tx_prepare, "%hu, %hu, %s, %hu", "%hu",
                 in.port_id, in.queue_id,
                 rpc_rte_mbufs2str(tlbp, tx_pkts, in.tx_pkts.tx_pkts_len,
                                   rpcs),
                 in.tx_pkts.tx_pkts_len, out.retval);
    te_log_buf_free(tlbp);

    TAPI_RPC_OUT(rte_eth_tx_prepare, out.retval > in.tx_pkts.tx_pkts_len);

    return out.retval;
}

uint16_t
rpc_rte_eth_rx_burst(rcf_rpc_server *rpcs,
                     uint16_t port_id,
                     uint16_t queue_id,
                     rpc_rte_mbuf_p *rx_pkts,
                     uint16_t nb_pkts)
{
    tarpc_rte_eth_rx_burst_in     in;
    tarpc_rte_eth_rx_burst_out    out;
    te_log_buf                   *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;
    in.nb_pkts = nb_pkts;

    rcf_rpc_call(rpcs, "rte_eth_rx_burst", &in, &out);

    CHECK_RETVAL_VAR_ERR_COND(rte_eth_rx_burst, out.rx_pkts.rx_pkts_len,
                              out.rx_pkts.rx_pkts_len > in.nb_pkts,
                              out.rx_pkts.rx_pkts_len,
                              out.rx_pkts.rx_pkts_len > in.nb_pkts);

    memcpy(rx_pkts, out.rx_pkts.rx_pkts_val,
           MIN(nb_pkts, out.rx_pkts.rx_pkts_len) * sizeof(*rx_pkts));

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_rx_burst, "%hu, %hu, %hu", "%hu %s",
                 in.port_id, in.queue_id, in.nb_pkts, out.rx_pkts.rx_pkts_len,
                 rpc_rte_mbufs2str(tlbp, rx_pkts, out.rx_pkts.rx_pkts_len,
                                   rpcs));
    te_log_buf_free(tlbp);

    TAPI_RPC_OUT(rte_eth_rx_burst, out.rx_pkts.rx_pkts_len > in.nb_pkts);

    return out.rx_pkts.rx_pkts_len;
}

int
rpc_rte_eth_dev_set_link_up(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_dev_set_link_up_in   in;
    tarpc_rte_eth_dev_set_link_up_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_set_link_up", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_set_link_up, out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_set_link_up, "%hu", NEG_ERRNO_FMT,
                 in.port_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_set_link_up, out.retval);
}

int
rpc_rte_eth_dev_set_link_down(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_dev_set_link_down_in   in;
    tarpc_rte_eth_dev_set_link_down_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_set_link_down", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_set_link_down,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_set_link_down, "%hu", NEG_ERRNO_FMT,
                 in.port_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_set_link_down, out.retval);
}

int
rpc_rte_eth_promiscuous_enable(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_promiscuous_enable_in   in;
    tarpc_rte_eth_promiscuous_enable_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_promiscuous_enable", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_promiscuous_enable,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_promiscuous_enable, "%hu", NEG_ERRNO_FMT,
                 in.port_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_promiscuous_enable, out.retval);
}

int
rpc_rte_eth_promiscuous_disable(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_promiscuous_disable_in   in;
    tarpc_rte_eth_promiscuous_disable_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_promiscuous_disable", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_promiscuous_disable,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_promiscuous_disable, "%hu", NEG_ERRNO_FMT,
                 in.port_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_promiscuous_disable, out.retval);
}

int
rpc_rte_eth_promiscuous_get(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_promiscuous_get_in   in;
    tarpc_rte_eth_promiscuous_get_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_promiscuous_get", &in, &out);

    CHECK_RETVAL_VAR(rte_eth_promiscuous_get, out.retval, ((out.retval != -1)
                     && (out.retval != 0) && (out.retval != 1)), -1);

    TAPI_RPC_LOG(rpcs, rte_eth_promiscuous_get, "%hu", "%d",
                 in.port_id, out.retval);

    TAPI_RPC_OUT(rte_eth_promiscuous_get, ((out.retval != 0) &&
                 (out.retval != 1)));

    return out.retval;
}

int
rpc_rte_eth_allmulticast_enable(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_allmulticast_enable_in   in;
    tarpc_rte_eth_allmulticast_enable_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_allmulticast_enable", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_allmulticast_enable,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_allmulticast_enable, "%hu", NEG_ERRNO_FMT,
                 in.port_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_allmulticast_enable, out.retval);
}

int
rpc_rte_eth_allmulticast_disable(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_allmulticast_disable_in   in;
    tarpc_rte_eth_allmulticast_disable_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_allmulticast_disable", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_allmulticast_disable,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_allmulticast_disable, "%hu", NEG_ERRNO_FMT,
                 in.port_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_allmulticast_disable, out.retval);
}

int
rpc_rte_eth_allmulticast_get(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_allmulticast_get_in   in;
    tarpc_rte_eth_allmulticast_get_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_allmulticast_get", &in, &out);

    CHECK_RETVAL_VAR(rte_eth_allmulticast_get, out.retval, ((out.retval != -1)
                     && (out.retval != 0) && (out.retval != 1)), -1);

    TAPI_RPC_LOG(rpcs, rte_eth_allmulticast_get, "%hu", "%d",
                 in.port_id, out.retval);

    TAPI_RPC_OUT(rte_eth_allmulticast_get, ((out.retval != 0) &&
                 (out.retval != 1)));

    return out.retval;
}

int
rpc_rte_eth_dev_get_mtu(rcf_rpc_server *rpcs, uint16_t port_id, uint16_t *mtu)
{
    tarpc_rte_eth_dev_get_mtu_in   in;
    tarpc_rte_eth_dev_get_mtu_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    if (mtu != NULL)
    {
        in.mtu.mtu_len = 1;
        in.mtu.mtu_val = malloc(sizeof(*mtu));
    }

    rcf_rpc_call(rpcs, "rte_eth_dev_get_mtu", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_get_mtu,
                                          out.retval);

    if (RPC_IS_CALL_OK(rpcs))
        *mtu = out.mtu;

    TAPI_RPC_LOG(rpcs, rte_eth_dev_get_mtu, "%hu, %p",
                 NEG_ERRNO_FMT " mtu=%hu", port_id, mtu,
                 NEG_ERRNO_ARGS(out.retval), out.mtu);

    RETVAL_ZERO_INT(rte_eth_dev_get_mtu, out.retval);
}

int
rpc_rte_eth_dev_set_mtu(rcf_rpc_server *rpcs, uint16_t port_id,
                        uint16_t mtu)
{
    tarpc_rte_eth_dev_set_mtu_in   in;
    tarpc_rte_eth_dev_set_mtu_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.mtu = mtu;

    rcf_rpc_call(rpcs, "rte_eth_dev_set_mtu", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_set_mtu,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_set_mtu, "%hu, %hu", NEG_ERRNO_FMT,
                 in.port_id, in.mtu, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_set_mtu, out.retval);
}

int
rpc_rte_eth_dev_vlan_filter(rcf_rpc_server *rpcs, uint16_t port_id,
                            uint16_t vlan_id, int on)
{
    tarpc_rte_eth_dev_vlan_filter_in   in;
    tarpc_rte_eth_dev_vlan_filter_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.vlan_id = vlan_id;
    in.on = on;

    rcf_rpc_call(rpcs, "rte_eth_dev_vlan_filter", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_vlan_filter,
                                         out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_vlan_filter,
                 "%hu, %hu, %d", NEG_ERRNO_FMT,
                 in.port_id, in.vlan_id, in.on,
                 NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_eth_dev_vlan_filter, out.retval);
}

int
rpc_rte_eth_dev_set_vlan_strip_on_queue(rcf_rpc_server *rpcs, uint16_t port_id,
                                        uint16_t rx_queue_id, int on)
{
    tarpc_rte_eth_dev_set_vlan_strip_on_queue_in   in;
    tarpc_rte_eth_dev_set_vlan_strip_on_queue_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.rx_queue_id = rx_queue_id;
    in.on = on;

    rcf_rpc_call(rpcs, "rte_eth_dev_set_vlan_strip_on_queue", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_set_vlan_strip_on_queue,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_set_vlan_strip_on_queue,
                 "%hu, %hu, %d", NEG_ERRNO_FMT,
                 in.port_id, in.rx_queue_id, in.on,
                 NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_eth_dev_set_vlan_strip_on_queue, out.retval);
}

static const char *
tarpc_rte_vlan_type2str(enum tarpc_rte_vlan_type vlan_type)
{
    const char *type;

    switch (vlan_type)
    {
        case TARPC_ETH_VLAN_TYPE_UNKNOWN:
            type = "UNKNOWN";
            break;
        case TARPC_ETH_VLAN_TYPE_INNER:
            type = "INNER";
            break;
        case TARPC_ETH_VLAN_TYPE_OUTER:
            type = "OUTER";
            break;
        case TARPC_ETH_VLAN_TYPE_MAX:
            type = "MAX";
            break;
        default:
            type = "<UNKNOWN>";
            break;
    }

    return type;
}

int
rpc_rte_eth_dev_set_vlan_ether_type(rcf_rpc_server *rpcs, uint16_t port_id,
                                    enum tarpc_rte_vlan_type vlan_type,
                                    uint16_t tag_type)
{
    tarpc_rte_eth_dev_set_vlan_ether_type_in   in;
    tarpc_rte_eth_dev_set_vlan_ether_type_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.vlan_type = vlan_type;
    in.tag_type = tag_type;

    rcf_rpc_call(rpcs, "rte_eth_dev_set_vlan_ether_type", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_set_vlan_strip_on_queue,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_set_vlan_ether_type,
                 "%hu, %s, %hu", NEG_ERRNO_FMT,
                 in.port_id, tarpc_rte_vlan_type2str(in.vlan_type),
                 in.tag_type, NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_eth_dev_set_vlan_ether_type, out.retval);
}

static const char *
tarpc_rte_eth_vlan_offload_mask2str(te_log_buf *tlbp, uint16_t offload_mask)
{
    const struct te_log_buf_bit2str vlan_offload_mask2str[] = {
#define TARPC_RTE_DEV_VLAN_OFFLOAD_FLAG_BIT2STR(_bit) \
        { TARPC_ETH_VLAN_##_bit##_OFFLOAD_BIT, #_bit }
        TARPC_RTE_DEV_VLAN_OFFLOAD_FLAG_BIT2STR(STRIP),
        TARPC_RTE_DEV_VLAN_OFFLOAD_FLAG_BIT2STR(FILTER),
        TARPC_RTE_DEV_VLAN_OFFLOAD_FLAG_BIT2STR(EXTEND),
#undef TARPC_RTE_DEV_VLAN_OFFLOAD_FLAG_BIT2STR
        { 0, NULL }
    };

    return te_bit_mask2log_buf(tlbp, offload_mask, vlan_offload_mask2str);
}

int
rpc_rte_eth_dev_set_vlan_offload(rcf_rpc_server *rpcs, uint16_t port_id,
                                 tarpc_int offload_mask)
{
    tarpc_rte_eth_dev_set_vlan_offload_in   in;
    tarpc_rte_eth_dev_set_vlan_offload_out  out;
    te_log_buf                             *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.offload_mask = offload_mask;

    rcf_rpc_call(rpcs, "rte_eth_dev_set_vlan_offload", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_set_vlan_offload,
                                          out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_set_vlan_offload,
                 "%hu, %s", NEG_ERRNO_FMT,
                 in.port_id,
                 tarpc_rte_eth_vlan_offload_mask2str(tlbp, in.offload_mask),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_dev_set_vlan_offload, out.retval);
}

static uint16_t
tarpc_rte_eth_vlan_offload_mask_valid(uint16_t offload_mask)
{
#define CHECK_TARPC_RTE_ETH_VLAN_OFFLOAD_BIT(_bit) \
    do {                                                            \
        uint16_t flag = (1 << TARPC_ETH_VLAN_##_bit##_OFFLOAD_BIT); \
        offload_mask &= ~flag;                                      \
    } while (0)
    CHECK_TARPC_RTE_ETH_VLAN_OFFLOAD_BIT(STRIP);
    CHECK_TARPC_RTE_ETH_VLAN_OFFLOAD_BIT(FILTER);
    CHECK_TARPC_RTE_ETH_VLAN_OFFLOAD_BIT(EXTEND);
#undef CHECK_TARPC_RTE_ETH_VLAN_OFFLOAD_BIT
    return offload_mask;
}

int
rpc_rte_eth_dev_get_vlan_offload(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_dev_get_vlan_offload_in   in;
    tarpc_rte_eth_dev_get_vlan_offload_out  out;
    te_log_buf                             *tlbp;
    int                                     check_mask;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_get_vlan_offload", &in, &out);

    check_mask = tarpc_rte_eth_vlan_offload_mask_valid(out.retval);

    CHECK_RETVAL_VAR_ERR_COND(rte_eth_dev_get_vlan_offload, out.retval,
                              out.retval >= 0 && check_mask != 0,
                              RETVAL_ECORRUPTED, out.retval < 0);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_get_vlan_offload,
                 "%hu", "%s",
                 in.port_id,
                 (out.retval < 0) ? neg_errno_rpc2str(out.retval) :
                     tarpc_rte_eth_vlan_offload_mask2str(tlbp, out.retval));
    te_log_buf_free(tlbp);

    RETVAL_INT(rte_eth_dev_get_vlan_offload, out.retval);
}

int
rpc_rte_eth_dev_set_vlan_pvid(rcf_rpc_server *rpcs, uint16_t port_id,
                              uint16_t pvid, int on)
{
    tarpc_rte_eth_dev_set_vlan_pvid_in   in;
    tarpc_rte_eth_dev_set_vlan_pvid_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.pvid = pvid;
    in.on = on;

    rcf_rpc_call(rpcs, "rte_eth_dev_set_vlan_pvid", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_set_vlan_pvid,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_set_vlan_pvid,
                 "%hu, %hu, %d", NEG_ERRNO_FMT,
                 in.port_id, in.pvid, in.on, NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_eth_dev_set_vlan_pvid, out.retval);
}

int
rpc_rte_eth_rx_queue_count(rcf_rpc_server *rpcs, uint16_t port_id,
                           uint16_t queue_id)
{
    tarpc_rte_eth_rx_queue_count_in   in;
    tarpc_rte_eth_rx_queue_count_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;

    rcf_rpc_call(rpcs, "rte_eth_rx_queue_count", &in, &out);

    /* Number of descriptors is 16bit */
    CHECK_RETVAL_VAR_ERR_COND(rte_eth_rx_queue_count, out.retval,
                              (out.retval > UINT16_MAX), RETVAL_ECORRUPTED,
                              (out.retval < 0));

    TAPI_RPC_LOG(rpcs, rte_eth_rx_queue_count,
                 "%hu, %hu", NEG_ERRNO_FMT,
                 in.port_id, in.queue_id, NEG_ERRNO_ARGS(out.retval));

    RETVAL_INT(rte_eth_rx_queue_count, out.retval);
}

int
rpc_rte_eth_rx_descriptor_status(rcf_rpc_server *rpcs,
                                 uint16_t        port_id,
                                 uint16_t        queue_id,
                                 uint16_t        offset)
{
    char *status_str = NULL;

    tarpc_rte_eth_rx_descriptor_status_in  in;
    tarpc_rte_eth_rx_descriptor_status_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;
    in.offset = offset;

    rcf_rpc_call(rpcs, "rte_eth_rx_descriptor_status", &in, &out);

    switch (out.retval)
    {
        case TARPC_RTE_ETH_RX_DESC_AVAIL:
            status_str = "AVAIL";
            break;
        case TARPC_RTE_ETH_RX_DESC_DONE:
            status_str = "DONE";
            break;
        case TARPC_RTE_ETH_RX_DESC_UNAVAIL:
            status_str = "UNAVAIL";
            break;
        default:
            break;
    }

    if (out.retval < 0)
    {
        TAPI_RPC_LOG(rpcs, rte_eth_rx_descriptor_status,
                     "%hu, %hu, %hu", NEG_ERRNO_FMT,
                     in.port_id, in.queue_id, in.offset,
                     NEG_ERRNO_ARGS(out.retval));
    }
    else if (status_str != NULL)
    {
        TAPI_RPC_LOG(rpcs, rte_eth_rx_descriptor_status,
                     "%hu, %hu, %hu", "%s",
                     in.port_id, in.queue_id, in.offset, status_str);
    }
    else
    {
        TAPI_RPC_OUT(rte_eth_rx_descriptor_status, TRUE);
    }

    RETVAL_INT(rte_eth_rx_descriptor_status, out.retval);
}

int
rpc_rte_eth_tx_descriptor_status(rcf_rpc_server *rpcs,
                                 uint16_t        port_id,
                                 uint16_t        queue_id,
                                 uint16_t        offset)
{
    char *status_str = NULL;

    tarpc_rte_eth_tx_descriptor_status_in  in;
    tarpc_rte_eth_tx_descriptor_status_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;
    in.offset = offset;

    rcf_rpc_call(rpcs, "rte_eth_tx_descriptor_status", &in, &out);

    switch (out.retval)
    {
        case TARPC_RTE_ETH_TX_DESC_FULL:
            status_str = "FULL";
            break;
        case TARPC_RTE_ETH_TX_DESC_DONE:
            status_str = "DONE";
            break;
        case TARPC_RTE_ETH_TX_DESC_UNAVAIL:
            status_str = "UNAVAIL";
            break;
        default:
            break;
    }

    if (out.retval < 0)
    {
        TAPI_RPC_LOG(rpcs, rte_eth_tx_descriptor_status,
                     "%hu, %hu, %hu", NEG_ERRNO_FMT,
                     in.port_id, in.queue_id, in.offset,
                     NEG_ERRNO_ARGS(out.retval));
    }
    else if (status_str != NULL)
    {
        TAPI_RPC_LOG(rpcs, rte_eth_tx_descriptor_status,
                     "%hu, %hu, %hu", "%s",
                     in.port_id, in.queue_id, in.offset, status_str);
    }
    else
    {
        TAPI_RPC_OUT(rte_eth_tx_descriptor_status, TRUE);
    }

    RETVAL_INT(rte_eth_tx_descriptor_status, out.retval);
}

int
rpc_rte_eth_dev_socket_id(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_dev_socket_id_in   in;
    tarpc_rte_eth_dev_socket_id_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_socket_id", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(rte_eth_dev_socket_id, out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_socket_id, "%hu", "%d",
                 in.port_id, out.retval);

    TAPI_RPC_OUT(rte_eth_dev_socket_id, out.retval < -1);

    return (out.retval);
}

int
rpc_rte_eth_dev_is_valid_port(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_dev_is_valid_port_in   in;
    tarpc_rte_eth_dev_is_valid_port_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_is_valid_port", &in, &out);

    CHECK_RETVAL_VAR(rte_eth_dev_is_valid_port, out.retval, ((out.retval != 1)
                     && (out.retval != 0)), -1);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_is_valid_port, "%hu", "%d",
                 in.port_id, out.retval);
    RETVAL_INT(rte_eth_dev_is_valid_port, out.retval);
}

int
rpc_rte_eth_dev_rx_queue_start(rcf_rpc_server *rpcs, uint16_t port_id,
                               uint16_t queue_id)
{
    tarpc_rte_eth_dev_rx_queue_start_in   in;
    tarpc_rte_eth_dev_rx_queue_start_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_rx_queue_start", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_rx_queue_start,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_rx_queue_start, "%hu, %hu", NEG_ERRNO_FMT,
                 in.port_id, in.queue_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_rx_queue_start, out.retval);
}

int
rpc_rte_eth_dev_rx_queue_stop(rcf_rpc_server *rpcs, uint16_t port_id,
                              uint16_t queue_id)
{
    tarpc_rte_eth_dev_rx_queue_stop_in   in;
    tarpc_rte_eth_dev_rx_queue_stop_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_rx_queue_stop", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_rx_queue_stop,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_rx_queue_stop, "%hu, %hu", NEG_ERRNO_FMT,
                 in.port_id, in.queue_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_rx_queue_stop, out.retval);
}

int
rpc_rte_eth_dev_tx_queue_start(rcf_rpc_server *rpcs, uint16_t port_id,
                               uint16_t queue_id)
{
    tarpc_rte_eth_dev_tx_queue_start_in   in;
    tarpc_rte_eth_dev_tx_queue_start_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_tx_queue_start", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_tx_queue_start,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_tx_queue_start, "%hu, %hu", NEG_ERRNO_FMT,
                 in.port_id, in.queue_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_tx_queue_start, out.retval);
}

int
rpc_rte_eth_dev_tx_queue_stop(rcf_rpc_server *rpcs, uint16_t port_id,
                              uint16_t queue_id)
{
    tarpc_rte_eth_dev_tx_queue_stop_in   in;
    tarpc_rte_eth_dev_tx_queue_stop_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_tx_queue_stop", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_tx_queue_stop,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_tx_queue_stop, "%hu, %hu", NEG_ERRNO_FMT,
                 in.port_id, in.queue_id, NEG_ERRNO_ARGS(out.retval));
    RETVAL_ZERO_INT(rte_eth_dev_tx_queue_stop, out.retval);
}

void
rpc_rte_eth_macaddr_get(rcf_rpc_server *rpcs, uint16_t port_id,
                        struct tarpc_ether_addr *mac_addr)
{
    tarpc_rte_eth_macaddr_get_in   in;
    tarpc_rte_eth_macaddr_get_out  out;
    te_log_buf                    *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    if (mac_addr != NULL)
    {
        in.mac_addr.mac_addr_len = 1;
        in.mac_addr.mac_addr_val = malloc(sizeof(*mac_addr));
    }

    rcf_rpc_call(rpcs, "rte_eth_macaddr_get", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && out.mac_addr.mac_addr_len != 0)
        memcpy(mac_addr->addr_bytes, out.mac_addr.mac_addr_val[0].addr_bytes,
               sizeof(out.mac_addr.mac_addr_val[0].addr_bytes));

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_macaddr_get, "%hu, %p", "%s",
                 in.port_id, mac_addr, te_ether_addr2log_buf(
                    tlbp, (uint8_t *)&out.mac_addr.mac_addr_val[0].addr_bytes));
    te_log_buf_free(tlbp);

    RETVAL_VOID(rte_eth_macaddr_get);
}

int
rpc_rte_eth_dev_default_mac_addr_set(rcf_rpc_server *rpcs, uint16_t port_id,
                                     struct tarpc_ether_addr *mac_addr)
{
    tarpc_rte_eth_dev_default_mac_addr_set_in   in;
    tarpc_rte_eth_dev_default_mac_addr_set_out  out;
    te_log_buf                                 *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    if (mac_addr != NULL)
    {
        in.mac_addr.mac_addr_len = 1;
        in.mac_addr.mac_addr_val = tapi_memdup(mac_addr, sizeof(*mac_addr));
    }

    rcf_rpc_call(rpcs, "rte_eth_dev_default_mac_addr_set", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_default_mac_addr_set,
                                          out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_default_mac_addr_set, "%hu, %s", NEG_ERRNO_FMT,
                 in.port_id, te_ether_addr2log_buf(
                    tlbp, (uint8_t *)&in.mac_addr.mac_addr_val[0].addr_bytes),
                    NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_dev_default_mac_addr_set, out.retval);
}

static const char *
tarpc_rte_eth_rxq_info2str(te_log_buf *tlbp,
                           const struct tarpc_rte_eth_rxq_info *qinfo,
                           rcf_rpc_server *rpcs)
{
    te_log_buf_append(tlbp, "{ ");

    te_log_buf_append(tlbp, "mp=", RPC_PTR_FMT, RPC_PTR_VAL(qinfo->mp));

    te_log_buf_append(tlbp, ", conf=");
    tarpc_rte_eth_rxconf2str(tlbp, &qinfo->conf);

    te_log_buf_append(tlbp, ", scattered_rx=%hu, nb_desc=%hu",
                      qinfo->scattered_rx, qinfo->nb_desc);

    te_log_buf_append(tlbp, " }");
    return te_log_buf_get(tlbp);
}

int
rpc_rte_eth_rx_queue_info_get(rcf_rpc_server *rpcs, uint16_t port_id,
                              uint16_t queue_id,
                              struct tarpc_rte_eth_rxq_info *qinfo)
{
    tarpc_rte_eth_rx_queue_info_get_in   in;
    tarpc_rte_eth_rx_queue_info_get_out  out;
    te_log_buf                          *tlbp;

    if (qinfo == NULL)
        TEST_FAIL("Invalid %s() qinfo argument", __func__);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;

    rcf_rpc_call(rpcs, "rte_eth_rx_queue_info_get", &in, &out);

    *qinfo = out.qinfo;

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_rx_queue_info_get, "%hu, %hu", "qinfo=%s, %s",
                 in.port_id, in.queue_id,
                 tarpc_rte_eth_rxq_info2str(tlbp, qinfo, rpcs),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_rx_queue_info_get, out.retval);
}

static const char *
tarpc_rte_eth_txq_info2str(te_log_buf *tlbp,
                           const struct tarpc_rte_eth_txq_info *qinfo)
{
    te_log_buf_append(tlbp, "{ ");

    te_log_buf_append(tlbp, ", conf=");
    tarpc_rte_eth_txconf2str(tlbp, &qinfo->conf);

    te_log_buf_append(tlbp, ", nb_desc=%hu", qinfo->nb_desc);

    te_log_buf_append(tlbp, " }");
    return te_log_buf_get(tlbp);
}

int
rpc_rte_eth_tx_queue_info_get(rcf_rpc_server *rpcs, uint16_t port_id,
                              uint16_t queue_id,
                              struct tarpc_rte_eth_txq_info *qinfo)
{
    tarpc_rte_eth_tx_queue_info_get_in   in;
    tarpc_rte_eth_tx_queue_info_get_out  out;
    te_log_buf                          *tlbp;

    if (qinfo == NULL)
        TEST_FAIL("Invalid %s() qinfo argument", __func__);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.queue_id = queue_id;

    rcf_rpc_call(rpcs, "rte_eth_tx_queue_info_get", &in, &out);

    *qinfo = out.qinfo;

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_tx_queue_info_get, "%hu, %hu", "qinfo=%s, %s",
                 in.port_id, in.queue_id,
                 tarpc_rte_eth_txq_info2str(tlbp, qinfo),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_tx_queue_info_get, out.retval);
}

static const char *
tarpc_rte_reta_conf2str(te_log_buf *tlbp,
                        const struct tarpc_rte_eth_rss_reta_entry64 *reta_conf,
                        uint16_t reta_size)
{
    unsigned cur_group, cur_entry;

    if (reta_conf == NULL)
    {
        te_log_buf_append(tlbp, "(null)");
        return te_log_buf_get(tlbp);
    }

    te_log_buf_append(tlbp, "reta_conf={");

    for (cur_group = 0;
         cur_group < TE_DIV_ROUND_UP(reta_size, RPC_RTE_RETA_GROUP_SIZE);
         cur_group++)
    {
        te_log_buf_append(tlbp, " mask=%llx", reta_conf[cur_group].mask);
        te_log_buf_append(tlbp, ", reta=");

        for (cur_entry = 0; cur_entry < RPC_RTE_RETA_GROUP_SIZE; cur_entry++)
            te_log_buf_append(tlbp, " %hu", reta_conf[cur_group].reta[cur_entry]);

    }

    te_log_buf_append(tlbp, " }");

    return te_log_buf_get(tlbp);
}

int
rpc_rte_eth_dev_rss_reta_query(rcf_rpc_server *rpcs, uint16_t port_id,
                               struct tarpc_rte_eth_rss_reta_entry64 *reta_conf,
                               uint16_t reta_size)
{
    tarpc_rte_eth_dev_rss_reta_query_in   in;
    tarpc_rte_eth_dev_rss_reta_query_out  out;
    te_log_buf                           *tlbp;
    unsigned                              cur_group;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (reta_conf != NULL)
    {
        in.reta_conf.reta_conf_len =
            TE_DIV_ROUND_UP(reta_size, RPC_RTE_RETA_GROUP_SIZE);
        in.reta_conf.reta_conf_val = calloc(in.reta_conf.reta_conf_len,
                                            sizeof(*reta_conf));
        for (cur_group = 0; cur_group < in.reta_conf.reta_conf_len; cur_group++)
            in.reta_conf.reta_conf_val[cur_group].mask = reta_conf[cur_group].mask;
    }

    in.port_id = port_id;
    in.reta_size = reta_size;

    rcf_rpc_call(rpcs, "rte_eth_dev_rss_reta_query", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_rss_reta_query,
                                          out.retval);

    if (out.retval == 0)
        for (cur_group = 0; cur_group < out.reta_conf.reta_conf_len; cur_group ++)
            memcpy(&reta_conf[cur_group], &out.reta_conf.reta_conf_val[cur_group],
                   sizeof(*reta_conf));

    tlbp = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, rte_eth_dev_rss_reta_query,
                 "%hu, %p, %hu", NEG_ERRNO_FMT ", %s",
                 in.port_id, reta_conf, reta_size,
                 NEG_ERRNO_ARGS(out.retval),
                 tarpc_rte_reta_conf2str(tlbp,
                     (out.reta_conf.reta_conf_len == 0) ? NULL :
                      out.reta_conf.reta_conf_val, reta_size));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_dev_rss_reta_query, out.retval);
}

int
rpc_rte_eth_dev_rss_hash_conf_get(rcf_rpc_server *rpcs, uint16_t port_id,
                                  struct tarpc_rte_eth_rss_conf *rss_conf)
{
    tarpc_rte_eth_dev_rss_hash_conf_get_in   in;
    tarpc_rte_eth_dev_rss_hash_conf_get_out  out;
    te_log_buf                              *rss_conf_in;
    te_log_buf                              *rss_conf_out;

    rss_conf_in = te_log_buf_alloc();
    if (rss_conf != NULL)
        tarpc_rte_eth_rss_conf2str(rss_conf_in, rss_conf);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.rss_conf.rss_conf_len = rss_conf != NULL ? 1 : 0;
    in.rss_conf.rss_conf_val = rss_conf;

    rcf_rpc_call(rpcs, "rte_eth_dev_rss_hash_conf_get", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_rss_hash_conf_get,
                                          out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rss_conf != NULL)
    {
        const struct tarpc_rte_eth_rss_conf *rss_conf_out =
            out.rss_conf.rss_conf_val;
        void *rss_key_buf;

        if (rss_conf_out == NULL ||
            ((rss_conf->rss_key.rss_key_val == NULL) !=
             (rss_conf_out->rss_key.rss_key_val == NULL)) ||
            (rss_conf->rss_key.rss_key_len !=
             rss_conf_out->rss_key.rss_key_len))
        {
            ERROR("%s(): unexpected RSS configuration result", __FUNCTION__);
            te_log_buf_free(rss_conf_in);
            RETVAL_ZERO_INT(rte_eth_dev_rss_hash_conf_get,
                            -TE_RC(TE_TAPI, TE_EFAULT));
        }

        /* Temporaryly save RSS key buffer poiner provided by caller */
        rss_key_buf = rss_conf->rss_key.rss_key_val;
        /* Copy result */
        *rss_conf = *rss_conf_out;
        /* Restore RSS key buffer provided by caller and copy result in it */
        rss_conf->rss_key.rss_key_val = rss_key_buf;
        if (rss_key_buf != NULL)
            memcpy(rss_key_buf, rss_conf_out->rss_key.rss_key_val,
                   rss_conf->rss_key.rss_key_len);
    }

    rss_conf_out = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, rte_eth_dev_rss_hash_conf_get,
                 "%hu, %p (%s)", NEG_ERRNO_FMT ", %s",
                 in.port_id, rss_conf, te_log_buf_get(rss_conf_in),
                 NEG_ERRNO_ARGS(out.retval),
                 (out.retval != 0) ? "n/a" :
                 (rss_conf == NULL) ? "NULL" :
                 tarpc_rte_eth_rss_conf2str(rss_conf_out, rss_conf));

    te_log_buf_free(rss_conf_in);
    te_log_buf_free(rss_conf_out);

    RETVAL_ZERO_INT(rte_eth_dev_rss_hash_conf_get, out.retval);
}

static const char *
tarpc_rte_eth_fc_mode2str(te_log_buf *tlbp,
                          enum tarpc_rte_eth_fc_mode fc_mode)
{
    const char *mode;

    switch (fc_mode)
    {
        case TARPC_RTE_FC_NONE:
            mode = "NONE";
            break;
        case TARPC_RTE_FC_RX_PAUSE:
            mode = "RX_PAUSE";
            break;
        case TARPC_RTE_FC_TX_PAUSE:
            mode = "TX_PAUSE";
            break;
        case TARPC_RTE_FC_FULL:
            mode = "FULL";
            break;
        default:
            mode = "<UNKNOWN>";
            break;
    }

    te_log_buf_append(tlbp, "mode=%s", mode);

    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_eth_fc_conf2str(te_log_buf *tlbp,
                          const struct tarpc_rte_eth_fc_conf *fc_conf)
{
    te_log_buf_append(tlbp, "{");

    te_log_buf_append(tlbp, "high_water=%lu, low_water=%lu"
                      ", pause_time=%hu, send_xon=%hu, ",
                      fc_conf->high_water, fc_conf->low_water,
                      fc_conf->pause_time, fc_conf->send_xon);

    tarpc_rte_eth_fc_mode2str(tlbp, fc_conf->mode);

    te_log_buf_append(tlbp, ", mac_ctrl_frame_fwd=%hhu, autoneg=%hhu",
                      fc_conf->mac_ctrl_frame_fwd, fc_conf->autoneg);
    te_log_buf_append(tlbp, "}");

    return te_log_buf_get(tlbp);
}

int rpc_rte_eth_dev_flow_ctrl_get(rcf_rpc_server *rpcs, uint16_t port_id,
                                  struct tarpc_rte_eth_fc_conf *fc_conf)
{
    tarpc_rte_eth_dev_flow_ctrl_get_in   in;
    tarpc_rte_eth_dev_flow_ctrl_get_out  out;
    te_log_buf                          *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_flow_ctrl_get", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_flow_ctrl_get,
                                          out.retval);

    if (out.retval == 0)
        *fc_conf = out.fc_conf;

    tlbp = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, rte_eth_dev_flow_ctrl_get,
                 "%hu, %p", NEG_ERRNO_FMT ", %s",
                 in.port_id, fc_conf, NEG_ERRNO_ARGS(out.retval),
                 (fc_conf == NULL) ? "<NULL>" :
                 tarpc_rte_eth_fc_conf2str(tlbp, fc_conf));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_dev_flow_ctrl_get, out.retval);
}

int
rpc_rte_eth_xstats_get_names(rcf_rpc_server *rpcs, uint16_t port_id,
                             struct tarpc_rte_eth_xstat_name *xstats_names,
                             unsigned int size)
{
    tarpc_rte_eth_xstats_get_names_in   in;
    tarpc_rte_eth_xstats_get_names_out  out;
    te_log_buf                         *tlbp;
    int                                 i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (size != 0 && xstats_names == NULL)
    {
        ERROR("%s(): No array of xstats names, but size is greater than 0",
              __FUNCTION__);
        RETVAL_ZERO_INT(rte_eth_xstats_get_names, -1);
    }

    in.port_id = port_id;
    in.size = size;

    rcf_rpc_call(rpcs, "rte_eth_xstats_get_names", &in, &out);

    CHECK_RETVAL_VAR_ERR_COND(rte_eth_xstats_get_names,
                              out.retval, FALSE,
                              -TE_RC(TE_TAPI, TE_ECORRUPTED),
                              (out.retval < 0));

    tlbp = te_log_buf_alloc();
    te_log_buf_append(tlbp, "{");
    if (xstats_names != NULL && out.retval > 0 &&
        (unsigned int)out.retval <= size)
    {
        for (i = 0; i < out.retval; i++)
        {
            strncpy(xstats_names[i].name,
                    out.xstats_names.xstats_names_val[i].name,
                    TARPC_RTE_ETH_XSTATS_NAME_SIZE);

            te_log_buf_append(tlbp, "%s%d=%s", (i == 0) ? "" : ", ", i,
                              xstats_names[i].name);
        }
    }
    te_log_buf_append(tlbp, "}");

    TAPI_RPC_LOG(rpcs, rte_eth_xstats_get_names,
                 "%hu, %u", NEG_ERRNO_FMT " xstats_names=%s", in.port_id,
                 size, NEG_ERRNO_ARGS(out.retval), te_log_buf_get(tlbp));
    te_log_buf_free(tlbp);

    RETVAL_INT(rte_eth_xstats_get_names, out.retval);
}

int
rpc_rte_eth_xstats_get(rcf_rpc_server *rpcs, uint16_t port_id,
                       struct tarpc_rte_eth_xstat *xstats,
                       unsigned int n)
{
    tarpc_rte_eth_xstats_get_in     in;
    tarpc_rte_eth_xstats_get_out    out;
    te_log_buf                     *tlbp;
    int                             i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (n != 0 && xstats == NULL)
    {
        ERROR("%s(): No array of xstats, but size is greater than 0",
              __FUNCTION__);
        RETVAL_ZERO_INT(rte_eth_xstats_get, -1);
    }

    in.port_id = port_id;
    in.n = n;

    rcf_rpc_call(rpcs, "rte_eth_xstats_get", &in, &out);

    CHECK_RETVAL_VAR_ERR_COND(rpc_rte_eth_xstats_get,
                              out.retval, FALSE,
                              -TE_RC(TE_TAPI, TE_ECORRUPTED),
                              (out.retval < 0));

    tlbp = te_log_buf_alloc();
    te_log_buf_append(tlbp, "{ ");
    if ((xstats != NULL) && (out.retval > 0) &&
        ((unsigned int)out.retval <= n))
    {
        for (i = 0; i < out.retval; ++i)
        {
            xstats[i] = out.xstats.xstats_val[i];

            te_log_buf_append(tlbp, "%s%" PRIu64 ":%" PRIu64,
                              (i == 0) ? "" : ", ",
                              xstats[i].id, xstats[i].value);
        }
    }
    te_log_buf_append(tlbp, " }");

    TAPI_RPC_LOG(rpcs, rte_eth_xstats_get,
                 "%hu, %u", NEG_ERRNO_FMT " xstats = %s", in.port_id, n,
                 NEG_ERRNO_ARGS(out.retval), te_log_buf_get(tlbp));
    te_log_buf_free(tlbp);

    RETVAL_INT(rte_eth_xstats_get, out.retval);
}

void
rpc_rte_eth_xstats_reset(rcf_rpc_server *rpcs, uint16_t port_id)
{
    tarpc_rte_eth_xstats_reset_in   in;
    tarpc_rte_eth_xstats_reset_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_xstats_reset", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_eth_xstats_reset, "%hu", "", in.port_id);
    RETVAL_VOID(rte_eth_xstats_reset);
}

static const char *
tarpc_log_array_uint64(te_log_buf   *tlbp,
                       uint64_t     *items,
                       unsigned int  nb_items)
{
    unsigned int i;

    te_log_buf_append(tlbp, "{");
    for (i = 0; (items != NULL) && (i < nb_items); ++i)
    {
        te_log_buf_append(tlbp, " %" PRIu64 "%s", items[i],
                          ((i + 1) != nb_items) ? "," : " ");
    }
    te_log_buf_append(tlbp, "}");

    return te_log_buf_get(tlbp);
}

static const char *
tarpc_log_kv_array_uint64(te_log_buf   *tlbp,
                          uint64_t     *keys,
                          uint64_t     *values,
                          unsigned int  nb_items)
{
    unsigned int i;

    te_log_buf_append(tlbp, "{");
    for (i = 0; (values != NULL) && (i < nb_items); ++i)
    {
        te_log_buf_append(tlbp, " %" PRIu64 ":%" PRIu64 "%s",
                          (keys != NULL) ? keys[i] : i, values[i],
                          ((i + 1) != nb_items) ? "," : " ");
    }
    te_log_buf_append(tlbp, "}");

    return te_log_buf_get(tlbp);
}

int
rpc_rte_eth_xstats_get_by_id(rcf_rpc_server *rpcs,
                             uint16_t        port_id,
                             uint64_t       *ids,
                             uint64_t       *values,
                             unsigned int    n)
{
    tarpc_rte_eth_xstats_get_by_id_in   in;
    tarpc_rte_eth_xstats_get_by_id_out  out;
    te_log_buf                         *tlbp_ids;
    te_log_buf                         *tlbp_values;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.ids.ids_val = ((ids != NULL) && (n > 0)) ?
                     tapi_memdup(ids, sizeof(*ids) * n) : NULL;
    in.ids.ids_len = (in.ids.ids_val != NULL) ? n : 0;
    in.n = n;

    rcf_rpc_call(rpcs, "rte_eth_xstats_get_by_id", &in, &out);
    CHECK_RETVAL_VAR_ERR_COND(rte_eth_xstats_get_by_id, out.retval, FALSE,
                              -TE_RC(TE_TAPI, TE_ECORRUPTED), (out.retval < 0));

    tlbp_ids = te_log_buf_alloc();
    tlbp_values = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_xstats_get_by_id, "%hu, %s, %p, %u",
                 NEG_ERRNO_FMT " %s", in.port_id,
                 tarpc_log_array_uint64(tlbp_ids, ids, n), values, in.n,
                 NEG_ERRNO_ARGS(out.retval),
                 tarpc_log_kv_array_uint64(tlbp_values, ids,
                                           out.values.values_val,
                                           out.values.values_len));
    te_log_buf_free(tlbp_values);
    te_log_buf_free(tlbp_ids);

    if ((values != NULL) &&
        (out.values.values_val != NULL) && (out.values.values_len > 0))
    {
        memcpy(values, out.values.values_val,
               sizeof(*values) * out.values.values_len);
    }

    RETVAL_INT(rte_eth_xstats_get_by_id, out.retval);
}

static const char *
tarpc_rte_eth_dump_xstat_names(te_log_buf                      *tlbp,
                               uint64_t                        *ids,
                               struct tarpc_rte_eth_xstat_name *xstat_names,
                               unsigned int                     nb_xstat_names)
{
    unsigned int i;

    te_log_buf_append(tlbp, "{");
    for (i = 0; (xstat_names != NULL) && (i < nb_xstat_names); ++i)
    {
        te_log_buf_append(tlbp, " %" PRIu64 ":%s%s",
                          (ids != NULL) ? ids[i] : i, xstat_names[i].name,
                          ((i + 1) != nb_xstat_names) ? "," : " ");
    }
    te_log_buf_append(tlbp, "}");

    return te_log_buf_get(tlbp);
}

int
rpc_rte_eth_xstats_get_names_by_id(rcf_rpc_server                  *rpcs,
                                   uint16_t                         port_id,
                                   struct tarpc_rte_eth_xstat_name *xstat_names,
                                   unsigned int                     size,
                                   uint64_t                        *ids)
{
    tarpc_rte_eth_xstats_get_names_by_id_in   in;
    tarpc_rte_eth_xstats_get_names_by_id_out  out;
    te_log_buf                               *tlbp_ids;
    te_log_buf                               *tlbp_xstat_names;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.ids.ids_val = ((ids != NULL) && (size > 0)) ?
                     tapi_memdup(ids, sizeof(*ids) * size) : NULL;
    in.ids.ids_len = (in.ids.ids_val != NULL) ? size : 0;
    in.size = size;

    rcf_rpc_call(rpcs, "rte_eth_xstats_get_names_by_id", &in, &out);
    CHECK_RETVAL_VAR_ERR_COND(rte_eth_xstats_get_names_by_id, out.retval, FALSE,
                              -TE_RC(TE_TAPI, TE_ECORRUPTED), (out.retval < 0));

    tlbp_ids = te_log_buf_alloc();
    tlbp_xstat_names = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_xstats_get_names_by_id, "%hu, %p, %u, %s",
                 NEG_ERRNO_FMT " %s", in.port_id, xstat_names,
                 in.size, tarpc_log_array_uint64(tlbp_ids, ids, size),
                 NEG_ERRNO_ARGS(out.retval),
                 tarpc_rte_eth_dump_xstat_names(tlbp_xstat_names, ids,
                                                out.xstat_names.xstat_names_val,
                                                out.xstat_names.xstat_names_len));
    te_log_buf_free(tlbp_xstat_names);
    te_log_buf_free(tlbp_ids);

    if ((xstat_names != NULL) && (out.xstat_names.xstat_names_val != NULL) &&
        (out.xstat_names.xstat_names_len > 0))
    {
        memcpy(xstat_names, out.xstat_names.xstat_names_val,
               sizeof(*xstat_names) * out.xstat_names.xstat_names_len);
    }

    RETVAL_INT(rte_eth_xstats_get_names_by_id, out.retval);
}

int
rpc_rte_eth_dev_rss_hash_update(rcf_rpc_server *rpcs, uint16_t port_id,
                                struct tarpc_rte_eth_rss_conf *rss_conf)
{
    tarpc_rte_eth_dev_rss_hash_update_in   in;
    tarpc_rte_eth_dev_rss_hash_update_out  out;
    te_log_buf                            *tlbp;
    te_errno                               rc = 0;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rss_conf == NULL)
    {
        rc = TE_RC(TE_RPC, TE_EINVAL);
        goto check_ret;
    }

    if (rss_conf->rss_key_len != 0)
    {
        in.rss_conf.rss_key.rss_key_val = tapi_memdup(
            rss_conf->rss_key.rss_key_val, rss_conf->rss_key_len);

        in.rss_conf.rss_key.rss_key_len = rss_conf->rss_key_len;
    }

    in.rss_conf.rss_key_len = rss_conf->rss_key_len;
    in.rss_conf.rss_hf = rss_conf->rss_hf;
    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_rss_hash_update", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_rss_hash_update,
                                          out.retval);
    tlbp = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, rte_eth_dev_rss_hash_update,
                 "%hu, %s", NEG_ERRNO_FMT,
                 in.port_id, (rss_conf == NULL) ? "NULL" :
                 tarpc_rte_eth_rss_conf2str(tlbp, rss_conf),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

check_ret:
    RETVAL_ZERO_INT(rte_eth_dev_rss_hash_update,
                    (rc) ? -rc : out.retval);
}

int
rpc_rte_eth_dev_rss_reta_update(rcf_rpc_server *rpcs, uint16_t port_id,
                               struct tarpc_rte_eth_rss_reta_entry64 *reta_conf,
                               uint16_t reta_size)
{
    tarpc_rte_eth_dev_rss_reta_update_in   in;
    tarpc_rte_eth_dev_rss_reta_update_out  out;
    te_log_buf                            *tlbp;
    te_errno                               rc = 0;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (reta_conf != NULL)
    {
        in.reta_conf.reta_conf_len =
            TE_DIV_ROUND_UP(reta_size, RPC_RTE_RETA_GROUP_SIZE);
        in.reta_conf.reta_conf_val = calloc(in.reta_conf.reta_conf_len,
                                            sizeof(*reta_conf));
        if (in.reta_conf.reta_conf_val == NULL)
        {
            rc = TE_RC(TE_RPC, TE_ENOMEM);

            goto check_ret;
        }

        memcpy(in.reta_conf.reta_conf_val, reta_conf,
               in.reta_conf.reta_conf_len * sizeof(*reta_conf));
    }

    in.port_id = port_id;
    in.reta_size = reta_size;

    rcf_rpc_call(rpcs, "rte_eth_dev_rss_reta_update", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_rss_reta_update,
                                          out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_rss_reta_update,
                 "%hu, %p, %hu", NEG_ERRNO_FMT,
                 in.port_id, reta_conf, reta_size,
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

check_ret:
    RETVAL_ZERO_INT(rte_eth_dev_rss_reta_update,
                    (rc) ? -rc : out.retval);
}

static const char *
tarpc_rte_eth_link2str(te_log_buf                 *tlbp,
                       struct tarpc_rte_eth_link *eth_link)
{
    te_log_buf_append(
        tlbp, "{ link_speed = %llu, link_duplex = %s, "
        "link_autoneg = %s, link_status = %s }", eth_link->link_speed,
        (eth_link->link_duplex == 0) ? "HALF_DUPLEX" : "FULL_DUPLEX",
        (eth_link->link_autoneg == 0) ? "FIXED" : "AUTONEG",
        (eth_link->link_status == 0) ? "DOWN" : "UP");

    return te_log_buf_get(tlbp);
}

void
rpc_rte_eth_link_get_nowait(rcf_rpc_server *rpcs, uint16_t port_id,
                            struct tarpc_rte_eth_link *eth_link)
{
    tarpc_rte_eth_link_get_nowait_in   in;
    tarpc_rte_eth_link_get_nowait_out  out;
    te_log_buf                        *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (eth_link == NULL)
        TEST_FAIL("Invalid %s() 'eth_link' argument", __func__);

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_link_get_nowait", &in, &out);

    *eth_link = out.eth_link;

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_link_get_nowait, "%hu",
                 "eth_link = %s", in.port_id,
                 tarpc_rte_eth_link2str(tlbp, eth_link));
    te_log_buf_free(tlbp);

    RETVAL_VOID(rte_eth_link_get_nowait);
}

void
rpc_rte_eth_link_get(rcf_rpc_server *rpcs,
                     uint16_t port_id,
                     struct tarpc_rte_eth_link *eth_link)
{
    tarpc_rte_eth_link_get_in   in;
    tarpc_rte_eth_link_get_out  out;
    te_log_buf                 *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (eth_link == NULL)
        TEST_FAIL("Invalid %s() 'eth_link' argument", __func__);

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_link_get", &in, &out);

    *eth_link = out.eth_link;

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_link_get, "%hu",
                 "eth_link = %s", in.port_id,
                 tarpc_rte_eth_link2str(tlbp, eth_link));
    te_log_buf_free(tlbp);

    RETVAL_VOID(rte_eth_link_get);
}

int
rpc_dpdk_eth_await_link_up(rcf_rpc_server *rpcs,
                           uint16_t        port_id,
                           unsigned int    nb_attempts,
                           unsigned int    wait_int_ms,
                           unsigned int    after_up_ms)
{
    tarpc_dpdk_eth_await_link_up_in  in;
    tarpc_dpdk_eth_await_link_up_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.nb_attempts = nb_attempts;
    in.wait_int_ms = wait_int_ms;
    in.after_up_ms = after_up_ms;

    rcf_rpc_call(rpcs, "dpdk_eth_await_link_up", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(dpdk_eth_await_link_up, out.retval);

    TAPI_RPC_LOG(rpcs, dpdk_eth_await_link_up,
                 "%hu, nb_attempts = %u, wait_int_ms = %u, after_up_ms = %u",
                 NEG_ERRNO_FMT, in.port_id, in.nb_attempts, in.wait_int_ms,
                 in.after_up_ms, NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(dpdk_eth_await_link_up, out.retval);
}

int rpc_rte_eth_dev_flow_ctrl_set(rcf_rpc_server *rpcs, uint16_t port_id,
                                  struct tarpc_rte_eth_fc_conf *fc_conf)
{
    tarpc_rte_eth_dev_flow_ctrl_set_in   in;
    tarpc_rte_eth_dev_flow_ctrl_set_out  out;
    te_log_buf                          *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (fc_conf == NULL)
    {
        ERROR("%s(): No flow control parameters", __FUNCTION__);
        RETVAL_ZERO_INT(rte_eth_dev_flow_ctrl_set, -1);
    }

    in.port_id = port_id;
    in.fc_conf = *fc_conf;

    rcf_rpc_call(rpcs, "rte_eth_dev_flow_ctrl_set", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_flow_ctrl_set,
                                          out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_flow_ctrl_set, "%hu, %s",
                 NEG_ERRNO_FMT, in.port_id,
                 tarpc_rte_eth_fc_conf2str(tlbp, fc_conf),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_dev_flow_ctrl_set, out.retval);
}

static void
tapi_rpc_rte_packet_type_mask2str(te_log_buf *tlbp,
                                  uint32_t    ptype_mask)
{
#define CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(_l, _t) \
    case TARPC_RTE_PTYPE_##_l##_##_t << TARPC_RTE_PTYPE_##_l##_OFFSET: \
        te_log_buf_append(tlbp, "%s_%s", #_l, #_t);                    \
        break

    switch (ptype_mask) {
        case 0:
            te_log_buf_append(tlbp, "NONE");
            break;
        case TARPC_RTE_PTYPE_L2_MASK:
            te_log_buf_append(tlbp, "L2_ALL");
            break;
        case TARPC_RTE_PTYPE_L3_MASK:
            te_log_buf_append(tlbp, "L3_ALL");
            break;
        case TARPC_RTE_PTYPE_L4_MASK:
            te_log_buf_append(tlbp, "L4_ALL");
            break;
        case TARPC_RTE_PTYPE_TUNNEL_MASK:
            te_log_buf_append(tlbp, "TUNNEL_ALL");
            break;
        case TARPC_RTE_PTYPE_INNER_L2_MASK:
            te_log_buf_append(tlbp, "INNER_L2_ALL");
            break;
        case TARPC_RTE_PTYPE_INNER_L3_MASK:
            te_log_buf_append(tlbp, "INNER_L3_ALL");
            break;
        case TARPC_RTE_PTYPE_INNER_L4_MASK:
            te_log_buf_append(tlbp, "INNER_L4_ALL");
            break;

        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L2, ETHER);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L2, ETHER_TIMESYNC);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L2, ETHER_ARP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L2, ETHER_LLDP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L2, ETHER_NSH);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L2, ETHER_VLAN);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L2, ETHER_QINQ);

        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L3, IPV4);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L3, IPV4_EXT);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L3, IPV6);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L3, IPV4_EXT_UNKNOWN);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L3, IPV6_EXT);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L3, IPV6_EXT_UNKNOWN);

        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L4, TCP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L4, UDP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L4, FRAG);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L4, SCTP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L4, ICMP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(L4, NONFRAG);

        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(TUNNEL, IP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(TUNNEL, GRE);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(TUNNEL, VXLAN);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(TUNNEL, NVGRE);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(TUNNEL, GENEVE);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(TUNNEL, GRENAT);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(TUNNEL, GTPC);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(TUNNEL, GTPU);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(TUNNEL, ESP);

        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L2, ETHER);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L2, ETHER_VLAN);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L2, ETHER_QINQ);

        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L3, IPV4);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L3, IPV4_EXT);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L3, IPV6);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L3, IPV4_EXT_UNKNOWN);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L3, IPV6_EXT);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L3, IPV6_EXT_UNKNOWN);

        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L4, TCP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L4, UDP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L4, FRAG);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L4, SCTP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L4, ICMP);
        CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR(INNER_L4, NONFRAG);
#undef CASE_TARPC_RTE_PKTMBUF_PTYPE_MASK2STR

        default:
            te_log_buf_append(tlbp, "UNKNOWN_TYPE");
            break;
    }
}

static void
tarpc_rte_packet_type_mask_arg2str(te_log_buf *tlbp,
                                   uint32_t    pm)
{
    tapi_rpc_rte_packet_type_mask2str(tlbp, pm & TARPC_RTE_PTYPE_L2_MASK);
    te_log_buf_append(tlbp, " | ");
    tapi_rpc_rte_packet_type_mask2str(tlbp, pm & TARPC_RTE_PTYPE_L3_MASK);
    te_log_buf_append(tlbp, " | ");
    tapi_rpc_rte_packet_type_mask2str(tlbp, pm & TARPC_RTE_PTYPE_L4_MASK);
    te_log_buf_append(tlbp, " | ");
    tapi_rpc_rte_packet_type_mask2str(tlbp, pm & TARPC_RTE_PTYPE_TUNNEL_MASK);
    te_log_buf_append(tlbp, " | ");
    tapi_rpc_rte_packet_type_mask2str(tlbp, pm & TARPC_RTE_PTYPE_INNER_L2_MASK);
    te_log_buf_append(tlbp, " | ");
    tapi_rpc_rte_packet_type_mask2str(tlbp, pm & TARPC_RTE_PTYPE_INNER_L3_MASK);
    te_log_buf_append(tlbp, " | ");
    tapi_rpc_rte_packet_type_mask2str(tlbp, pm & TARPC_RTE_PTYPE_INNER_L4_MASK);
}

static void
tarpc_rte_supported_ptypes2str(te_log_buf *tlbp, uint32_t *ptypes, int num)
{
    int i;

    te_log_buf_append(tlbp, ": ");
    tapi_rpc_rte_packet_type_mask2str(tlbp, ptypes[0]);
    for (i = 1; i < num; i++)
    {
        te_log_buf_append(tlbp, " | ");
        tapi_rpc_rte_packet_type_mask2str(tlbp, ptypes[i]);
    }
}

int
rpc_rte_eth_dev_get_supported_ptypes(rcf_rpc_server *rpcs, uint16_t port_id,
                                     uint32_t ptype_mask, uint32_t *ptypes,
                                     int num)
{
    tarpc_rte_eth_dev_get_supported_ptypes_in   in;
    tarpc_rte_eth_dev_get_supported_ptypes_out  out;
    te_log_buf                                 *tlbp_arg;
    te_log_buf                                 *tlbp_ret;
    int                                         i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if ((num != 0) && (ptypes == NULL))
    {
        ERROR("%s(): No array of ptypes, but num is greater than 0", __FUNCTION__);
        RETVAL_ZERO_INT(rte_eth_dev_get_supported_ptypes, -1);
    }

    in.port_id = port_id;
    in.ptype_mask = ptype_mask;
    in.num = num;

    rcf_rpc_call(rpcs, "rte_eth_dev_get_supported_ptypes", &in, &out);

    CHECK_RETVAL_VAR_ERR_COND(rte_eth_dev_get_supported_ptypes,
                              out.retval, FALSE,
                              -TE_RC(TE_TAPI, TE_ECORRUPTED),
                              (out.retval < 0));
    tlbp_arg = te_log_buf_alloc();
    tlbp_ret = te_log_buf_alloc();

    tarpc_rte_packet_type_mask_arg2str(tlbp_arg, ptype_mask);

    if (ptypes != NULL && out.retval > 0)
    {
        for (i = 0; i < MIN(num, out.retval); i++)
            ptypes[i] = out.ptypes.ptypes_val[i];

        tarpc_rte_supported_ptypes2str(tlbp_ret, ptypes, MIN(num, out.retval));
    }

    TAPI_RPC_LOG(rpcs, rpc_rte_eth_dev_get_supported_ptypes,
                 "%hu, %s", "%s%s", in.port_id, te_log_buf_get(tlbp_arg),
                 NEG_ERRNO_ARGS(out.retval), te_log_buf_get(tlbp_ret));
    te_log_buf_free(tlbp_arg);
    te_log_buf_free(tlbp_ret);

    RETVAL_INT(rte_eth_dev_get_supported_ptypes, out.retval);
}

static const char *
tarpc_ether_addr_list2str(te_log_buf              *tlbp,
                          struct tarpc_ether_addr *mc_addr_set,
                          uint32_t                 nb_mc_addr)
{
    unsigned int i;

    te_log_buf_append(tlbp, "{");
    for (i = 0; i < nb_mc_addr; i++)
    {
        if (i > 0)
            te_log_buf_append(tlbp, ", ");
        te_ether_addr2log_buf(tlbp, (uint8_t *)&mc_addr_set[i].addr_bytes);
    }
    te_log_buf_append(tlbp, "}");

    return te_log_buf_get(tlbp);
}

int
rpc_rte_eth_dev_set_mc_addr_list(rcf_rpc_server *rpcs,
                                 uint16_t port_id,
                                 struct tarpc_ether_addr *mc_addr_set,
                                 uint32_t nb_mc_addr)
{
    tarpc_rte_eth_dev_set_mc_addr_list_in       in;
    tarpc_rte_eth_dev_set_mc_addr_list_out      out;
    te_log_buf                                 *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (nb_mc_addr != 0 && mc_addr_set == NULL)
    {
        ERROR("%s(): No mc_addr_set, but size is greater than 0",
              __FUNCTION__);
        RETVAL_ZERO_INT(rte_eth_dev_set_mc_addr_list, -1);
    }

    in.port_id = port_id;
    in.mc_addr_set.mc_addr_set_val = mc_addr_set;
    in.mc_addr_set.mc_addr_set_len = nb_mc_addr;

    rcf_rpc_call(rpcs, "rte_eth_dev_set_mc_addr_list", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_set_mc_addr_list,
                                          out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_set_mc_addr_list,
                 "%hu, %s", NEG_ERRNO_FMT, in.port_id,
                 tarpc_ether_addr_list2str(tlbp, mc_addr_set, nb_mc_addr),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_eth_dev_set_mc_addr_list, out.retval);
}

int
rpc_rte_eth_dev_fw_version_get(rcf_rpc_server *rpcs,
                               uint16_t        port_id,
                               char           *fw_version,
                               size_t          fw_size)
{
    tarpc_rte_eth_dev_fw_version_get_in    in;
    tarpc_rte_eth_dev_fw_version_get_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if ((fw_version == NULL) || (fw_size == 0))
    {
        ERROR("%s(): no buffer specified", __FUNCTION__);
        RETVAL_ZERO_INT(rte_eth_dev_fw_version_get, -EINVAL);
    }

    in.port_id = port_id;
    in.fw_version.fw_version_val = tapi_memdup(fw_version, fw_size);
    in.fw_version.fw_version_len = fw_size;

    rcf_rpc_call(rpcs, "rte_eth_dev_fw_version_get", &in, &out);

    memcpy(fw_version, out.fw_version.fw_version_val, fw_size);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_fw_version_get,
                 "%hu, %#x, %u", NEG_ERRNO_FMT "; fw_version: %s%s%s",
                 in.port_id, in.fw_version.fw_version_val,
                 in.fw_version.fw_version_len, NEG_ERRNO_ARGS(out.retval),
                 (out.retval == 0) ? out.fw_version.fw_version_val : "N/A",
                 (out.retval > 0) ? " (truncated data)" : "",
                 (out.retval < 0) ? " (error occurred)" : "");

    RETVAL_INT(rte_eth_dev_fw_version_get, out.retval);
}

const char *
tarpc_rte_eth_tunnel_type2str(enum tarpc_rte_eth_tunnel_type tunnel_type)
{
    const char *type;

    switch (tunnel_type)
    {
        case TARPC_RTE_TUNNEL_TYPE_NONE:
            type = "NONE";
            break;
        case TARPC_RTE_TUNNEL_TYPE_VXLAN:
            type = "VXLAN";
            break;
        case TARPC_RTE_TUNNEL_TYPE_GENEVE:
            type = "GENEVE";
            break;
        case TARPC_RTE_TUNNEL_TYPE_TEREDO:
            type = "TEREDO";
            break;
        case TARPC_RTE_TUNNEL_TYPE_NVGRE:
            type = "NVGRE";
            break;
        case TARPC_RTE_TUNNEL_TYPE_IP_IN_GRE:
            type = "IP_IN_GRE";
            break;
        case TARPC_RTE_L2_TUNNEL_TYPE_E_TAG:
            type = "L2_E_TAG";
            break;
        case TARPC_RTE_TUNNEL_TYPE_MAX:
            type = "MAX";
            break;
        default:
            type = "<UNKNOWN>";
            break;
    }

    return type;
}

int
rpc_rte_eth_dev_udp_tunnel_port_add(rcf_rpc_server                  *rpcs,
                                    uint16_t                         port_id,
                                    struct tarpc_rte_eth_udp_tunnel *tunnel_udp)
{
    tarpc_rte_eth_dev_udp_tunnel_port_add_in  in;
    tarpc_rte_eth_dev_udp_tunnel_port_add_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (tunnel_udp == NULL)
    {
        ERROR("%s(): no tunnel configuration specified", __FUNCTION__);
        RETVAL_ZERO_INT(rte_eth_dev_udp_tunnel_port_add, -EINVAL);
    }

    in.port_id = port_id;
    memcpy(&in.tunnel_udp, tunnel_udp, sizeof(in.tunnel_udp));

    rcf_rpc_call(rpcs, "rte_eth_dev_udp_tunnel_port_add", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_udp_tunnel_port_add,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_udp_tunnel_port_add,
                 "%hu, udp_port = %" PRIu16 ", prot_type = %s",
                 NEG_ERRNO_FMT, in.port_id, in.tunnel_udp.udp_port,
                 tarpc_rte_eth_tunnel_type2str(in.tunnel_udp.prot_type),
                 NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_eth_dev_udp_tunnel_port_add, out.retval);
}

int
rpc_rte_eth_dev_udp_tunnel_port_delete(
                                rcf_rpc_server                  *rpcs,
                                uint16_t                         port_id,
                                struct tarpc_rte_eth_udp_tunnel *tunnel_udp)
{
    tarpc_rte_eth_dev_udp_tunnel_port_delete_in  in;
    tarpc_rte_eth_dev_udp_tunnel_port_delete_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (tunnel_udp == NULL)
    {
        ERROR("%s(): no tunnel configuration specified", __FUNCTION__);
        RETVAL_ZERO_INT(rte_eth_dev_udp_tunnel_port_delete, -EINVAL);
    }

    in.port_id = port_id;
    memcpy(&in.tunnel_udp, tunnel_udp, sizeof(in.tunnel_udp));

    rcf_rpc_call(rpcs, "rte_eth_dev_udp_tunnel_port_delete", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_udp_tunnel_port_delete,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_udp_tunnel_port_delete,
                 "%hu, udp_port = %" PRIu16 ", prot_type = %s",
                 NEG_ERRNO_FMT, in.port_id, in.tunnel_udp.udp_port,
                 tarpc_rte_eth_tunnel_type2str(in.tunnel_udp.prot_type),
                 NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_eth_dev_udp_tunnel_port_delete, out.retval);
}

int
rpc_rte_eth_dev_get_port_by_name(rcf_rpc_server *rpcs,
                                 const char     *name,
                                 uint16_t       *port_id)
{
    tarpc_rte_eth_dev_get_port_by_name_in  in;
    tarpc_rte_eth_dev_get_port_by_name_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.name = tapi_strdup(name);

    rcf_rpc_call(rpcs, "rte_eth_dev_get_port_by_name", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_get_port_by_name,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_get_port_by_name,
                 "name='%s'", "port_id=%d; " NEG_ERRNO_FMT,
                 in.name, (out.retval == 0) ? out.port_id : -1,
                 NEG_ERRNO_ARGS(out.retval));

    free(in.name);

    if (out.retval == 0)
        *port_id = out.port_id;

    RETVAL_ZERO_INT(rte_eth_dev_get_port_by_name, out.retval);
}

int
rpc_rte_eth_dev_get_name_by_port(rcf_rpc_server *rpcs,
                                 uint16_t        port_id,
                                 char           *name)
{
    tarpc_rte_eth_dev_get_name_by_port_in  in;
    tarpc_rte_eth_dev_get_name_by_port_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_eth_dev_get_name_by_port", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_eth_dev_get_name_by_port,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, rte_eth_dev_get_name_by_port,
                 "port_id=%d", "name='%s'; " NEG_ERRNO_FMT,
                 in.port_id, (out.retval == 0) ? out.name : "",
                 NEG_ERRNO_ARGS(out.retval));

    if (out.retval == 0 && name != NULL && out.name != NULL)
        te_strlcpy(name, out.name, RPC_RTE_ETH_NAME_MAX_LEN);

    RETVAL_ZERO_INT(rte_eth_dev_get_name_by_port, out.retval);
}

char *
rpc_rte_eth_dev_rx_offload_name(rcf_rpc_server *rpcs,
                                uint64_t        offload)
{
    tarpc_rte_eth_dev_rx_offload_name_in   in;
    tarpc_rte_eth_dev_rx_offload_name_out  out;
    te_log_buf                            *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.offload = offload;

    rcf_rpc_call(rpcs, "rte_eth_dev_rx_offload_name", &in, &out);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_rx_offload_name, "offload=%s", "name='%s'",
                 tarpc_rte_eth_rx_offloads2str(tlbp, in.offload),
                 (out.retval != NULL) ? out.retval : "UNKNOWN");
    te_log_buf_free(tlbp);

    if (out.retval != NULL)
        return tapi_strdup(out.retval);
    else
        return NULL;
}

char *
rpc_rte_eth_dev_tx_offload_name(rcf_rpc_server *rpcs,
                                uint64_t        offload)
{
    tarpc_rte_eth_dev_tx_offload_name_in   in;
    tarpc_rte_eth_dev_tx_offload_name_out  out;
    te_log_buf                            *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.offload = offload;

    rcf_rpc_call(rpcs, "rte_eth_dev_tx_offload_name", &in, &out);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_eth_dev_tx_offload_name, "offload=%s", "name='%s'",
                 tarpc_rte_eth_tx_offloads2str(tlbp, in.offload),
                 (out.retval != NULL) ? out.retval : "UNKNOWN");
    te_log_buf_free(tlbp);

    if (out.retval != NULL)
        return tapi_strdup(out.retval);
    else
        return NULL;
}

static const char *
tarpc_rte_eth_rx_metadata_bits2str(te_log_buf      *lb,
                                   const uint64_t  *bits)
{
    const struct te_log_buf_bit2str  bit2str[] = {

#define TARPC_RTE_ETH_RX_METADATA_BIT2STR(_bit)           \
        { TARPC_RTE_ETH_RX_METADATA_##_bit##_BIT, #_bit }

        TARPC_RTE_ETH_RX_METADATA_BIT2STR(USER_FLAG),
        TARPC_RTE_ETH_RX_METADATA_BIT2STR(USER_MARK),
        TARPC_RTE_ETH_RX_METADATA_BIT2STR(TUNNEL_ID),

#undef TARPC_RTE_ETH_RX_METADATA_BIT2STR

        { 0, NULL }
    };

    return te_bit_mask2log_buf(lb, *bits, bit2str);
}

int
rpc_rte_eth_rx_metadata_negotiate(rcf_rpc_server  *rpcs,
                                  uint16_t         port_id,
                                  uint64_t        *features)
{
    tarpc_rte_eth_rx_metadata_negotiate_out   out = {};
    tarpc_rte_eth_rx_metadata_negotiate_in    in = {};
    te_log_buf                               *lb_out;
    te_log_buf                               *lb_in;

    in.port_id = port_id;

    TAPI_RPC_SET_IN_ARG_IF_PTR_NOT_NULL(features);

    rcf_rpc_call(rpcs, "rte_eth_rx_metadata_negotiate", &in, &out);

    TAPI_RPC_CHECK_OUT_ARG_SINGLE_PTR(rte_eth_rx_metadata_negotiate, features);

    lb_out = te_log_buf_alloc();
    lb_in = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, rte_eth_rx_metadata_negotiate,
                 "port_id=%" PRIu16 ", features=%s",
                 "features=%s; " NEG_ERRNO_FMT, in.port_id,
                 TAPI_RPC_LOG_ARG_TO_STR(in, features, lb_in,
                                         tarpc_rte_eth_rx_metadata_bits2str),
                 TAPI_RPC_LOG_ARG_TO_STR(out, features, lb_out,
                                         tarpc_rte_eth_rx_metadata_bits2str),
                 NEG_ERRNO_ARGS(out.retval));

    te_log_buf_free(lb_out);
    te_log_buf_free(lb_in);

    TAPI_RPC_COPY_OUT_ARG_IF_PTR_NOT_NULL(features);

    RETVAL_ZERO_INT(rte_eth_rx_metadata_negotiate, out.retval);
}
