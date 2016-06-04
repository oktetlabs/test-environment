/** @file
 * @brief RPC for DPDK
 *
 * Definition of RPC structures and functions for DPDK (rte_*)
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

/*
 * The is in fact appended to tarpc.x.m4, so it may reuse any types
 * defined there.
 */

/** Handle of the 'rte_mempool' or 0 */
typedef tarpc_ptr    tarpc_rte_mempool;

/** Handle of the 'rte_mbuf' or 0 */
typedef tarpc_ptr    tarpc_rte_mbuf;

/* Just to make two-dimensional array of strings */
struct tarpc_string {
    string  str<>;
};

/* rte_eal_init() */
struct tarpc_rte_eal_init_in {
    struct tarpc_in_arg         common;
    tarpc_int                   argc;
    tarpc_string                argv<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_eal_init_out;


/** enum rte_proc_type_t */
enum tarpc_rte_proc_type_t {
    TARPC_RTE_PROC_AUTO,
    TARPC_RTE_PROC_PRIMARY,
    TARPC_RTE_PROC_SECONDARY,
    TARPC_RTE_PROC_INVALID,

    TARPC_RTE_PROC__UNKNOWN
};

/** rte_eal_process_type() */
struct tarpc_rte_eal_process_type_in {
    struct tarpc_in_arg         common;
};

struct tarpc_rte_eal_process_type_out {
    struct tarpc_out_arg        common;
    enum tarpc_rte_proc_type_t  retval;
};

struct tarpc_mbuf_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
};

struct tarpc_mbuf_retval_out {
    struct tarpc_out_arg    common;
    tarpc_rte_mbuf          retval;
};

/** rte_pktmbuf_pool_create() */
struct tarpc_rte_pktmbuf_pool_create_in {
    struct tarpc_in_arg     common;
    string                  name<>;
    uint32_t                n;
    uint32_t                cache_size;
    uint16_t                priv_size;
    uint16_t                data_room_size;
    tarpc_int               socket_id;
};

struct tarpc_rte_pktmbuf_pool_create_out {
    struct tarpc_out_arg    common;
    tarpc_rte_mempool       retval;
};

/** rte_pktmbuf_alloc() */
struct tarpc_rte_pktmbuf_alloc_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mempool       mp;
};

typedef struct tarpc_mbuf_retval_out tarpc_rte_pktmbuf_alloc_out;

/** rte_pktmbuf_free() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_free_in;

typedef struct tarpc_void_out tarpc_rte_pktmbuf_free_out;

/** rte_pktmbuf_append_data() */
struct tarpc_rte_pktmbuf_append_data_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    uint8_t                 buf<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_pktmbuf_append_data_out;

/** rte_pktmbuf_read_data() */
struct tarpc_rte_pktmbuf_read_data_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    tarpc_size_t            offset;
    tarpc_size_t            len;
    uint8_t                 buf<>;
};

struct tarpc_rte_pktmbuf_read_data_out {
    struct tarpc_out_arg    common;
    tarpc_ssize_t           retval;
    uint8_t                 buf<>;
};

/** rte_pktmbuf_clone() */
struct tarpc_rte_pktmbuf_clone_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    tarpc_rte_mempool       mp;
};

typedef struct tarpc_mbuf_retval_out tarpc_rte_pktmbuf_clone_out;

/** rte_pktmbuf_prepend_data() */
struct tarpc_rte_pktmbuf_prepend_data_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    uint8_t                 buf<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_pktmbuf_prepend_data_out;

/** rte_pktmbuf_get_next() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_next_in;

typedef struct tarpc_mbuf_retval_out tarpc_rte_pktmbuf_get_next_out;

/** rte_pktmbuf_get_pkt_len() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_pkt_len_in;

struct tarpc_rte_pktmbuf_get_pkt_len_out {
    struct tarpc_out_arg    common;
    uint32_t                retval;
};

/** rte_pktmbuf_alloc_bulk() */
struct tarpc_rte_pktmbuf_alloc_bulk_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mempool       mp;
    tarpc_uint              count;
};

struct tarpc_rte_pktmbuf_alloc_bulk_out {
    struct tarpc_out_arg    common;
    tarpc_rte_mbuf          bulk<>;
    tarpc_int               retval;
};

/** rte_pktmbuf_chain() */
struct tarpc_rte_pktmbuf_chain_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          head;
    tarpc_rte_mbuf          tail;
};

typedef struct tarpc_int_retval_out tarpc_rte_pktmbuf_chain_out;

/** rte_pktmbuf_get_nb_segs() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_nb_segs_in;

struct tarpc_rte_pktmbuf_get_nb_segs_out {
    struct tarpc_out_arg    common;
    uint8_t                 retval;
};

/** rte_pktmbuf_get_port() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_port_in;

struct tarpc_rte_pktmbuf_get_port_out {
    struct tarpc_out_arg    common;
    uint8_t                 retval;
};

/** rte_pktmbuf_set_port() */
struct tarpc_rte_pktmbuf_set_port_in {
    struct tarpc_out_arg    common;
    tarpc_rte_mbuf          m;
    uint8_t                 port;
};

typedef struct tarpc_void_out tarpc_rte_pktmbuf_set_port_out;

/** rte_pktmbuf_get_data_len() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_data_len_in;

struct tarpc_rte_pktmbuf_get_data_len_out {
    struct tarpc_out_arg    common;
    uint16_t                retval;
};


/*
 * rte_eth_dev API
 */

struct tarpc_rte_eth_dev_port_id_in {
    struct tarpc_in_arg             common;
    uint8_t                         port_id;
};


/** Link speeds */
enum tarpc_eth_link_speeds {
    TARPC_RTE_ETH_LINK_SPEED_FIXED = 0,
    TARPC_RTE_ETH_LINK_SPEED_10M_HD,
    TARPC_RTE_ETH_LINK_SPEED_10M,
    TARPC_RTE_ETH_LINK_SPEED_100M_HD,
    TARPC_RTE_ETH_LINK_SPEED_100M,
    TARPC_RTE_ETH_LINK_SPEED_1G,
    TARPC_RTE_ETH_LINK_SPEED_2_5G,
    TARPC_RTE_ETH_LINK_SPEED_5G,
    TARPC_RTE_ETH_LINK_SPEED_10G,
    TARPC_RTE_ETH_LINK_SPEED_20G,
    TARPC_RTE_ETH_LINK_SPEED_25G,
    TARPC_RTE_ETH_LINK_SPEED_40G,
    TARPC_RTE_ETH_LINK_SPEED_50G,
    TARPC_RTE_ETH_LINK_SPEED_56G,
    TARPC_RTE_ETH_LINK_SPEED_100G,

    TARPC_RTE_ETH_LINK_SPEED__UNKNOWN
};

/** RX offload capabilities of a device */
enum tarpc_dev_rx_offload_bits {
    TARPC_RTE_DEV_RX_OFFLOAD_VLAN_STRIP_BIT = 0,
    TARPC_RTE_DEV_RX_OFFLOAD_IPV4_CKSUM_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_UDP_CKSUM_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_TCP_CKSUM_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_TCP_LRO_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_QINQ_STRIP_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_OUTER_IPV4_CKSUM_BIT,

    TARPC_RTE_DEV_RX_OFFLOAD__UNKNOWN_BIT
};

/** TX offload capabilities of a device */
enum tarpc_dev_tx_offload_bits {
    TARPC_RTE_DEV_TX_OFFLOAD_VLAN_INSERT_BIT = 0,
    TARPC_RTE_DEV_TX_OFFLOAD_IPV4_CKSUM_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_UDP_CKSUM_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_TCP_CKSUM_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_SCTP_CKSUM_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_TCP_TSO_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_UDP_TSO_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_OUTER_IPV4_CKSUM_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_QINQ_INSERT_BIT,

    TARPC_RTE_DEV_TX_OFFLOAD__UNKNOWN_BIT
};

/** Flow types */
enum tarpc_rte_eth_flow_types {
    TARPC_RTE_ETH_FLOW_IPV4 = 0,
    TARPC_RTE_ETH_FLOW_FRAG_IPV4,
    TARPC_RTE_ETH_FLOW_NONFRAG_IPV4_TCP,
    TARPC_RTE_ETH_FLOW_NONFRAG_IPV4_UDP,
    TARPC_RTE_ETH_FLOW_NONFRAG_IPV4_SCTP,
    TARPC_RTE_ETH_FLOW_NONFRAG_IPV4_OTHER,
    TARPC_RTE_ETH_FLOW_IPV6,
    TARPC_RTE_ETH_FLOW_FRAG_IPV6,
    TARPC_RTE_ETH_FLOW_NONFRAG_IPV6_TCP,
    TARPC_RTE_ETH_FLOW_NONFRAG_IPV6_UDP,
    TARPC_RTE_ETH_FLOW_NONFRAG_IPV6_SCTP,
    TARPC_RTE_ETH_FLOW_NONFRAG_IPV6_OTHER,
    TARPC_RTE_ETH_FLOW_L2_PAYLOAD,
    TARPC_RTE_ETH_FLOW_IPV6_EX,
    TARPC_RTE_ETH_FLOW_IPV6_TCP_EX,
    TARPC_RTE_ETH_FLOW_IPV6_UDP_EX,

    TARPC_RTE_ETH_FLOW__UNKNOWN
};

/** Transmit queue flags */
enum tarpc_rte_eth_txq_flags {
    TARPC_RTE_ETH_TXQ_FLAGS_NOMULTSEGS_BIT = 0,
    TARPC_RTE_ETH_TXQ_FLAGS_NOREFCOUNT_BIT,
    TARPC_RTE_ETH_TXQ_FLAGS_NOMULTMEMP_BIT,
    TARPC_RTE_ETH_TXQ_FLAGS_NOVLANOFFL_BIT,
    TARPC_RTE_ETH_TXQ_FLAGS_NOXSUMSCTP_BIT,
    TARPC_RTE_ETH_TXQ_FLAGS_NOXSUMUDP_BIT,
    TARPC_RTE_ETH_TXQ_FLAGS_NOXSUMTCP_BIT,

    TARPC_RTE_ETH_TXQ_FLAGS__UNKNOWN_BIT
};

struct tarpc_rte_eth_thresh {
    uint8_t                         pthresh;
    uint8_t                         hthresh;
    uint8_t                         wthresh;
};

struct tarpc_rte_eth_rxconf {
    struct tarpc_rte_eth_thresh     rx_thresh;
    uint16_t                        rx_free_thresh;
    uint8_t                         rx_drop_en;
    uint8_t                         rx_deferred_start;
};

struct tarpc_rte_eth_txconf {
    struct tarpc_rte_eth_thresh     tx_thresh;
    uint16_t                        tx_rs_thresh;
    uint16_t                        tx_free_thresh;
    uint32_t                        txq_flags;
    uint8_t                         tx_deferred_start;
};

struct tarpc_rte_eth_desc_lim {
    uint16_t                        nb_max;
    uint16_t                        nb_min;
    uint16_t                        nb_align;
};

struct tarpc_rte_eth_dev_info {
    string                          driver_name<>;
    unsigned int                    if_index;
    uint32_t                        min_rx_bufsize;
    uint32_t                        max_rx_pktlen;
    uint16_t                        max_rx_queues;
    uint16_t                        max_tx_queues;
    uint32_t                        max_mac_addrs;
    uint32_t                        max_hash_mac_addrs;
    uint16_t                        max_vfs;
    uint16_t                        max_vmdq_pools;
    uint32_t                        rx_offload_capa;
    uint32_t                        tx_offload_capa;
    uint16_t                        reta_size;
    uint8_t                         hash_key_size;
    uint64_t                        flow_type_rss_offloads;
    struct tarpc_rte_eth_rxconf     default_rxconf;
    struct tarpc_rte_eth_txconf     default_txconf;
    uint16_t                        vmdq_queue_base;
    uint16_t                        vmdq_queue_num;
    uint16_t                        vmdq_pool_base;
    struct tarpc_rte_eth_desc_lim   rx_desc_lim;
    struct tarpc_rte_eth_desc_lim   tx_desc_lim;
    uint32_t                        speed_capa;
};

/** rte_eth_dev_info_get() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_info_get_in;

struct tarpc_rte_eth_dev_info_get_out {
    struct tarpc_out_arg            common;
    struct tarpc_rte_eth_dev_info   dev_info;
};


enum tarpc_rte_eth_rx_mq_mode {
    TARPC_ETH_MQ_RX_NONE = 0,
    TARPC_ETH_MQ_RX_RSS,
    TARPC_ETH_MQ_RX_DCB,
    TARPC_ETH_MQ_RX_DCB_RSS,
    TARPC_ETH_MQ_RX_VMDQ_ONLY,
    TARPC_ETH_MQ_RX_VMDQ_RSS,
    TARPC_ETH_MQ_RX_VMDQ_DCB,
    TARPC_ETH_MQ_RX_VMDQ_DCB_RSS,

    TARPC_ETH_MQ_RX__UNKNOWN
};

enum tarpc_rte_eth_rxmode_flags {
    TARPC_RTE_ETH_RXMODE_HEADER_SPLIT_BIT = 0,
    TARPC_RTE_ETH_RXMODE_HW_IP_CHECKSUM_BIT,
    TARPC_RTE_ETH_RXMODE_HW_VLAN_FILTER_BIT,
    TARPC_RTE_ETH_RXMODE_HW_VLAN_STRIP_BIT,
    TARPC_RTE_ETH_RXMODE_HW_VLAN_EXTEND_BIT,
    TARPC_RTE_ETH_RXMODE_JUMBO_FRAME_BIT,
    TARPC_RTE_ETH_RXMODE_HW_STRIP_CRC_BIT,
    TARPC_RTE_ETH_RXMODE_ENABLE_SCATTER_BIT,
    TARPC_RTE_ETH_RXMODE_ENABLE_LRO_BIT
};

struct tarpc_rte_eth_rxmode {
    enum tarpc_rte_eth_rx_mq_mode       mq_mode;
    uint32_t                            max_rx_pkt_len;
    uint16_t                            split_hdr_size;
    uint16_t                            flags;
};

enum tarpc_rte_eth_tx_mq_mode {
    TARPC_ETH_MQ_TX_NONE = 0,
    TARPC_ETH_MQ_TX_DCB,
    TARPC_ETH_MQ_TX_VMDQ_DCB,
    TARPC_ETH_MQ_TX_VMDQ_ONLY,

    TARPC_ETH_MQ_TX__UNKNOWN
};

enum tarpc_rte_eth_txmode_flags {
    TARPC_RTE_ETH_TXMODE_HW_VLAN_REJECT_TAGGED_BIT = 0,
    TARPC_RTE_ETH_TXMODE_HW_VLAN_REJECT_UNTAGGED_BIT,
    TARPC_RTE_ETH_TXMODE_HW_VLAN_INSERT_PVID_BIT
};

struct tarpc_rte_eth_txmode {
    enum tarpc_rte_eth_tx_mq_mode       mq_mode;
    uint16_t                            pvid;
    uint8_t                             flags;
};

struct tarpc_rte_eth_rss_conf {
    uint8_t                             rss_key<>;
    uint8_t                             rss_key_len;
    uint64_t                            rss_hf;
};

struct tarpc_rte_eth_rx_adv_conf {
    struct tarpc_rte_eth_rss_conf       rss_conf;
    /* vmdq_dcb_conf */
    /* dcb_rx_conf */
    /* vmdq_rx_conf */
};

struct tarpc_rte_intr_conf {
    uint16_t                            lsc;
    uint16_t                            rxq;
};

struct tarpc_rte_eth_conf {
    uint16_t                            link_speeds;
    struct tarpc_rte_eth_rxmode         rxmode;
    struct tarpc_rte_eth_txmode         txmode;
    uint32_t                            lpbk_mode;
    struct tarpc_rte_eth_rx_adv_conf    rx_adv_conf;
    /* tx_adv_conf */
    uint32_t                            dcb_capability_en;
    /* fdir_conf */
    struct tarpc_rte_intr_conf          intr_conf;
};


/** rte_eth_dev_configure() */
struct tarpc_rte_eth_dev_configure_in {
    struct tarpc_in_arg         common;
    uint8_t                     port_id;
    uint16_t                    nb_rx_queue;
    uint16_t                    nb_tx_queue;
    struct tarpc_rte_eth_conf   eth_conf<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_configure_out;

/** rte_eth_dev_close() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_close_in;

typedef struct tarpc_void_out tarpc_rte_eth_dev_close_out;

/** rte_eth_dev_start() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_start_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_start_out;

/** rte_eth_dev_stop() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_stop_in;

typedef struct tarpc_void_out tarpc_rte_eth_dev_stop_out;

/** rte_eth_tx_queue_setup() */
struct tarpc_rte_eth_tx_queue_setup_in {
    struct tarpc_in_arg         common;
    uint8_t                     port_id;
    uint16_t                    tx_queue_id;
    uint16_t                    nb_tx_desc;
    tarpc_int                   socket_id;
    struct tarpc_rte_eth_txconf tx_conf<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_tx_queue_setup_out;

/** rte_eth_rx_queue_setup() */
struct tarpc_rte_eth_rx_queue_setup_in {
    struct tarpc_in_arg         common;
    uint8_t                     port_id;
    uint16_t                    rx_queue_id;
    uint16_t                    nb_rx_desc;
    tarpc_int                   socket_id;
    struct tarpc_rte_eth_rxconf rx_conf<>;
    tarpc_rte_mempool           mp;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_rx_queue_setup_out;

/** rte_eth_tx_burst() */
struct tarpc_rte_eth_tx_burst_in {
    struct tarpc_in_arg  common;
    uint8_t              port_id;
    uint16_t             queue_id;
    tarpc_rte_mbuf       tx_pkts<>;
};

struct tarpc_rte_eth_tx_burst_out {
    struct tarpc_out_arg    common;
    uint16_t                retval;
};

/** rte_eth_rx_burst() */
struct tarpc_rte_eth_rx_burst_in {
    struct tarpc_in_arg  common;
    uint8_t              port_id;
    uint16_t             queue_id;
    uint16_t             nb_pkts;
};

struct tarpc_rte_eth_rx_burst_out {
    struct tarpc_out_arg    common;
    tarpc_rte_mbuf          rx_pkts<>;
};

/** rte_eth_dev_set_link_up() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_set_link_up_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_set_link_up_out;

/** rte_eth_dev_set_link_down() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_set_link_down_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_set_link_down_out;

/** rte_eth_promiscuous_enable() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_promiscuous_enable_in;

typedef struct tarpc_void_out tarpc_rte_eth_promiscuous_enable_out;

/** rte_eth_promiscuous_disable() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_promiscuous_disable_in;

typedef struct tarpc_void_out tarpc_rte_eth_promiscuous_disable_out;

/** rte_eth_promiscuous_get() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_promiscuous_get_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_promiscuous_get_out;

/** rte_eth_allmulticast_enable() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_allmulticast_enable_in;

typedef struct tarpc_void_out tarpc_rte_eth_allmulticast_enable_out;

/** rte_eth_allmulticast_disable() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_allmulticast_disable_in;

typedef struct tarpc_void_out tarpc_rte_eth_allmulticast_disable_out;

/** rte_eth_allmulticast_get() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_allmulticast_get_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_allmulticast_get_out;

/** rte_eth_dev_get_mtu() */
struct tarpc_rte_eth_dev_get_mtu_in {
    struct tarpc_in_arg  common;
    uint8_t              port_id;
    uint16_t             mtu<>;

};

struct tarpc_rte_eth_dev_get_mtu_out {
    struct tarpc_out_arg  common;
    tarpc_int             retval;
    uint16_t              mtu;
};

/** rte_eth_dev_set_mtu() */
struct tarpc_rte_eth_dev_set_mtu_in {
    struct tarpc_in_arg  common;
    uint8_t              port_id;
    uint16_t             mtu;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_set_mtu_out;

/** rte_eth_dev_vlan_filter() */
struct tarpc_rte_eth_dev_vlan_filter_in {
    struct tarpc_in_arg  common;
    uint8_t              port_id;
    uint16_t             vlan_id;
    tarpc_int            on;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_vlan_filter_out;

/** rte_eth_dev_set_vlan_strip_on_queue() */
struct tarpc_rte_eth_dev_set_vlan_strip_on_queue_in {
    struct tarpc_in_arg  common;
    uint8_t              port_id;
    uint16_t             rx_queue_id;
    tarpc_int            on;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_set_vlan_strip_on_queue_out;

enum tarpc_rte_vlan_type {
    TARPC_ETH_VLAN_TYPE_UNKNOWN = 0,
    TARPC_ETH_VLAN_TYPE_INNER,
    TARPC_ETH_VLAN_TYPE_OUTER,
    TARPC_ETH_VLAN_TYPE_MAX,

    ARPC_ETH_VLAN_TYPE__UNKNOWN
};

/** rte_eth_dev_set_vlan_ether_type() */
struct tarpc_rte_eth_dev_set_vlan_ether_type_in {
    struct tarpc_in_arg       common;
    uint8_t                   port_id;
    enum tarpc_rte_vlan_type  vlan_type;
    uint16_t                  tag_type;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_set_vlan_ether_type_out;

enum tarpc_rte_eth_vlan_offload_flags {
    TARPC_ETH_VLAN_STRIP_OFFLOAD_BIT = 0,
    TARPC_ETH_VLAN_FILTER_OFFLOAD_BIT,
    TARPC_ETH_VLAN_EXTEND_OFFLOAD_BIT,

    TARPC_ETH_VALN__UNKNOWN_OFFLOAD_BIT
};

/** rte_eth_dev_set_vlan_offload() */
struct tarpc_rte_eth_dev_set_vlan_offload_in {
    struct tarpc_in_arg     common;
    uint8_t                 port_id;
    tarpc_int               offload_mask;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_set_vlan_offload_out;

program dpdk
{
    version ver0
    {
        RPC_DEF(rte_eal_init)
        RPC_DEF(rte_eal_process_type)

        RPC_DEF(rte_pktmbuf_pool_create)
        RPC_DEF(rte_pktmbuf_alloc)
        RPC_DEF(rte_pktmbuf_free)
        RPC_DEF(rte_pktmbuf_append_data)
        RPC_DEF(rte_pktmbuf_read_data)
        RPC_DEF(rte_pktmbuf_clone)
        RPC_DEF(rte_pktmbuf_prepend_data)
        RPC_DEF(rte_pktmbuf_get_next)
        RPC_DEF(rte_pktmbuf_get_pkt_len)
        RPC_DEF(rte_pktmbuf_alloc_bulk)
        RPC_DEF(rte_pktmbuf_chain)
        RPC_DEF(rte_pktmbuf_get_nb_segs)
        RPC_DEF(rte_pktmbuf_get_port)
        RPC_DEF(rte_pktmbuf_set_port)
        RPC_DEF(rte_pktmbuf_get_data_len)

        RPC_DEF(rte_eth_dev_info_get)
        RPC_DEF(rte_eth_dev_configure)
        RPC_DEF(rte_eth_dev_close)
        RPC_DEF(rte_eth_dev_start)
        RPC_DEF(rte_eth_dev_stop)
        RPC_DEF(rte_eth_tx_queue_setup)
        RPC_DEF(rte_eth_rx_queue_setup)
        RPC_DEF(rte_eth_tx_burst)
        RPC_DEF(rte_eth_rx_burst)
        RPC_DEF(rte_eth_dev_set_link_up)
        RPC_DEF(rte_eth_dev_set_link_down)
        RPC_DEF(rte_eth_promiscuous_enable)
        RPC_DEF(rte_eth_promiscuous_disable)
        RPC_DEF(rte_eth_promiscuous_get)
        RPC_DEF(rte_eth_allmulticast_enable)
        RPC_DEF(rte_eth_allmulticast_disable)
        RPC_DEF(rte_eth_allmulticast_get)
        RPC_DEF(rte_eth_dev_get_mtu)
        RPC_DEF(rte_eth_dev_set_mtu)
        RPC_DEF(rte_eth_dev_vlan_filter)
        RPC_DEF(rte_eth_dev_set_vlan_strip_on_queue)
        RPC_DEF(rte_eth_dev_set_vlan_ether_type)
        RPC_DEF(rte_eth_dev_set_vlan_offload)
    } = 1;
} = 2;
