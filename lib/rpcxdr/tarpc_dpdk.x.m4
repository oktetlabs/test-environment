/** @file
 * @brief RPC for DPDK
 *
 * Definition of RPC structures and functions for DPDK (rte_*)
 *
 * Copyright (C) 2003-2018 OKTET Labs.
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

/** Handle of the 'rte_ring' or 0 */
typedef tarpc_ptr    tarpc_rte_ring;

/** Bitmask of RSS hash protocols */
typedef uint64_t     tarpc_rss_hash_protos_t;

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

/** Packet Offload Flags */
enum tarpc_pktmbuf_ol_flags {
    TARPC_PKT_RX_VLAN_PKT = 0,
    TARPC_PKT_RX_VLAN,
    TARPC_PKT_RX_VLAN_STRIPPED,
    TARPC_PKT_RX_RSS_HASH,
    TARPC_PKT_RX_FDIR,
    TARPC_PKT_RX_IP_CKSUM_BAD,
    TARPC_PKT_RX_IP_CKSUM_GOOD,
    TARPC_PKT_RX_IP_CKSUM_NONE,
    TARPC_PKT_RX_L4_CKSUM_BAD,
    TARPC_PKT_RX_L4_CKSUM_GOOD,
    TARPC_PKT_RX_L4_CKSUM_NONE,
    TARPC_PKT_RX_EIP_CKSUM_BAD,
    TARPC_PKT_RX_OVERSIZE,
    TARPC_PKT_RX_HBUF_OVERFLOW,
    TARPC_PKT_RX_RECIP_ERR,
    TARPC_PKT_RX_MAC_ERR,
    TARPC_PKT_RX_IEEE1588_PTP,
    TARPC_PKT_RX_IEEE1588_TMST,
    TARPC_PKT_RX_FDIR_ID,
    TARPC_PKT_RX_FDIR_FLX,
    TARPC_PKT_RX_QINQ_PKT,
    TARPC_PKT_RX_QINQ,
    TARPC_PKT_RX_QINQ_STRIPPED,

    TARPC_PKT_TX_QINQ_PKT = 32,
    TARPC_PKT_TX_TCP_SEG,
    TARPC_PKT_TX_IEEE1588_TMST,
    TARPC_PKT_TX_L4_NO_CKSUM,
    TARPC_PKT_TX_TCP_CKSUM,
    TARPC_PKT_TX_SCTP_CKSUM,
    TARPC_PKT_TX_UDP_CKSUM,
    TARPC_PKT_TX_L4_MASK,
    TARPC_PKT_TX_IP_CKSUM,
    TARPC_PKT_TX_IPV4,
    TARPC_PKT_TX_IPV6,
    TARPC_PKT_TX_VLAN_PKT,
    TARPC_PKT_TX_OUTER_IP_CKSUM,
    TARPC_PKT_TX_OUTER_IPV4,
    TARPC_PKT_TX_OUTER_IPV6,

    TARPC_IND_ATTACHED_MBUF = 50,
    TARPC_CTRL_MBUF_FLAG,
    TARPC_EXT_ATTACHED_MBUF,

    TARPC_PKT__UNKNOWN = 63
};

enum tarpc_pktmbuf_l2_types {
    TARPC_RTE_PTYPE_L2_UNKNOWN = 0,
    TARPC_RTE_PTYPE_L2_ETHER,
    TARPC_RTE_PTYPE_L2_ETHER_TIMESYNC,
    TARPC_RTE_PTYPE_L2_ETHER_ARP,
    TARPC_RTE_PTYPE_L2_ETHER_LLDP,
    TARPC_RTE_PTYPE_L2_ETHER_NSH,
    TARPC_RTE_PTYPE_L2_ETHER_VLAN,
    TARPC_RTE_PTYPE_L2_ETHER_QINQ,

    TARPC_RTE_PTYPE_L2__UNKNOWN
};

enum tarpc_pktmbuf_l3_types {
    TARPC_RTE_PTYPE_L3_UNKNOWN = 0,
    TARPC_RTE_PTYPE_L3_IPV4,
    TARPC_RTE_PTYPE_L3_IPV4_EXT,
    TARPC_RTE_PTYPE_L3_IPV6,
    TARPC_RTE_PTYPE_L3_IPV4_EXT_UNKNOWN,
    TARPC_RTE_PTYPE_L3_IPV6_EXT,
    TARPC_RTE_PTYPE_L3_IPV6_EXT_UNKNOWN,

    TARPC_RTE_PTYPE_L3__UNKNOWN
};

enum tarpc_pktmbuf_l4_types {
    TARPC_RTE_PTYPE_L4_UNKNOWN = 0,
    TARPC_RTE_PTYPE_L4_TCP,
    TARPC_RTE_PTYPE_L4_UDP,
    TARPC_RTE_PTYPE_L4_FRAG,
    TARPC_RTE_PTYPE_L4_SCTP,
    TARPC_RTE_PTYPE_L4_ICMP,
    TARPC_RTE_PTYPE_L4_NONFRAG,

    TARPC_RTE_PTYPE_L4__UNKNOWN
};

enum tarpc_pktmbuf_tunnel_types {
    TARPC_RTE_PTYPE_TUNNEL_UNKNOWN = 0,
    TARPC_RTE_PTYPE_TUNNEL_IP,
    TARPC_RTE_PTYPE_TUNNEL_GRE,
    TARPC_RTE_PTYPE_TUNNEL_VXLAN,
    TARPC_RTE_PTYPE_TUNNEL_NVGRE,
    TARPC_RTE_PTYPE_TUNNEL_GENEVE,
    TARPC_RTE_PTYPE_TUNNEL_GRENAT,
    TARPC_RTE_PTYPE_TUNNEL_GTPC,
    TARPC_RTE_PTYPE_TUNNEL_GTPU,
    TARPC_RTE_PTYPE_TUNNEL_ESP,

    TARPC_RTE_PTYPE_TUNNEL__UNKNOWN
};

enum tarpc_pktmbuf_inner_l2_types {
    TARPC_RTE_PTYPE_INNER_L2_UNKNOWN = 0,
    TARPC_RTE_PTYPE_INNER_L2_ETHER,
    TARPC_RTE_PTYPE_INNER_L2_ETHER_VLAN,
    TARPC_RTE_PTYPE_INNER_L2_ETHER_QINQ,

    TARPC_RTE_PTYPE_INNER_L2__UNKNOWN
};

enum tarpc_pktmbuf_inner_l3_types {
    TARPC_RTE_PTYPE_INNER_L3_UNKNOWN = 0,
    TARPC_RTE_PTYPE_INNER_L3_IPV4,
    TARPC_RTE_PTYPE_INNER_L3_IPV4_EXT,
    TARPC_RTE_PTYPE_INNER_L3_IPV6,
    TARPC_RTE_PTYPE_INNER_L3_IPV4_EXT_UNKNOWN,
    TARPC_RTE_PTYPE_INNER_L3_IPV6_EXT,
    TARPC_RTE_PTYPE_INNER_L3_IPV6_EXT_UNKNOWN,

    TARPC_RTE_PTYPE_INNER_L3__UNKNOWN
};

enum tarpc_pktmbuf_inner_l4_types {
    TARPC_RTE_PTYPE_INNER_L4_UNKNOWN = 0,
    TARPC_RTE_PTYPE_INNER_L4_TCP,
    TARPC_RTE_PTYPE_INNER_L4_UDP,
    TARPC_RTE_PTYPE_INNER_L4_FRAG,
    TARPC_RTE_PTYPE_INNER_L4_SCTP,
    TARPC_RTE_PTYPE_INNER_L4_ICMP,
    TARPC_RTE_PTYPE_INNER_L4_NONFRAG,

    TARPC_RTE_PTYPE_INNER_L4__UNKNOWN
};

/** rte_mempool_lookup() */
struct tarpc_rte_mempool_lookup_in {
    struct tarpc_in_arg     common;
    string                  name<>;
};

struct tarpc_rte_mempool_lookup_out {
    struct tarpc_out_arg    common;
    tarpc_rte_mempool       retval;
};

/** rte_mempool_in_use_count() */
struct tarpc_rte_mempool_in_use_count_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mempool       mp;
};

struct tarpc_rte_mempool_in_use_count_out {
    struct tarpc_out_arg    common;
    unsigned int            retval;
};

/** rte_mempool_free() */
struct tarpc_rte_mempool_free_in {
    struct tarpc_in_arg     common;
    tarpc_bool              free_all;
    tarpc_rte_mempool       mp;
};

typedef struct tarpc_void_out tarpc_rte_mempool_free_out;

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

/** rte_pktmbuf_pool_create_by_ops() */
struct tarpc_rte_pktmbuf_pool_create_by_ops_in {
    struct tarpc_in_arg     common;
    string                  name<>;
    uint32_t                n;
    uint32_t                cache_size;
    uint16_t                priv_size;
    uint16_t                data_room_size;
    tarpc_int               socket_id;
    string                  ops_name<>;
};

struct tarpc_rte_pktmbuf_pool_create_by_ops_out {
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
    uint16_t                retval;
};

/** rte_pktmbuf_set_port() */
struct tarpc_rte_pktmbuf_set_port_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    uint16_t                port;
};

typedef struct tarpc_void_out tarpc_rte_pktmbuf_set_port_out;

/** rte_pktmbuf_get_data_len() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_data_len_in;

struct tarpc_rte_pktmbuf_get_data_len_out {
    struct tarpc_out_arg    common;
    uint16_t                retval;
};

/** rte_pktmbuf_get_vlan_tci() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_vlan_tci_in;

struct tarpc_rte_pktmbuf_get_vlan_tci_out {
    struct tarpc_out_arg    common;
    uint16_t                retval;
};

/** rte_pktmbuf_set_vlan_tci() */
struct tarpc_rte_pktmbuf_set_vlan_tci_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    uint16_t                vlan_tci;
};

typedef struct tarpc_void_out tarpc_rte_pktmbuf_set_vlan_tci_out;

/** rte_pktmbuf_get_vlan_tci_outer() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_vlan_tci_outer_in;

struct tarpc_rte_pktmbuf_get_vlan_tci_outer_out {
    struct tarpc_out_arg    common;
    uint16_t                retval;
};

/** rte_pktmbuf_set_vlan_tci_outer() */
struct tarpc_rte_pktmbuf_set_vlan_tci_outer_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    uint16_t                vlan_tci_outer;
};

typedef struct tarpc_void_out tarpc_rte_pktmbuf_set_vlan_tci_outer_out;

/** rte_pktmbuf_get_flags() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_flags_in;

struct tarpc_rte_pktmbuf_get_flags_out {
    struct tarpc_out_arg    common;
    uint64_t                retval;
};

/** rte_pktmbuf_set_flags() */
struct tarpc_rte_pktmbuf_set_flags_in {
    struct tarpc_in_arg    common;
    tarpc_rte_mbuf         m;
    uint64_t               ol_flags;
};

typedef struct tarpc_int_retval_out tarpc_rte_pktmbuf_set_flags_out;

/** rte_pktmbuf_get_pool() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_pool_in;

struct tarpc_rte_pktmbuf_get_pool_out {
    struct tarpc_out_arg    common;
    tarpc_rte_mempool       retval;
};

/** rte_pktmbuf_headroom() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_headroom_in;

struct tarpc_rte_pktmbuf_headroom_out {
    struct tarpc_out_arg    common;
    uint16_t                retval;
};

/** rte_pktmbuf_tailroom() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_tailroom_in;

struct tarpc_rte_pktmbuf_tailroom_out {
    struct tarpc_out_arg    common;
    uint16_t                retval;
};

/** rte_pktmbuf_trim() */
struct tarpc_rte_pktmbuf_trim_in {
    struct tarpc_in_arg    common;
    tarpc_rte_mbuf         m;
    uint16_t               len;
};

typedef struct tarpc_int_retval_out tarpc_rte_pktmbuf_trim_out;

/** rte_pktmbuf_adj() */
struct tarpc_rte_pktmbuf_adj_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    uint16_t                len;
};

struct tarpc_rte_pktmbuf_adj_out {
    struct tarpc_out_arg    common;
    uint16_t                retval;
};

struct tarpc_rte_pktmbuf_packet_type {
    uint8_t l2_type;
    uint8_t l3_type;
    uint8_t l4_type;
    uint8_t tun_type;
    uint8_t inner_l2_type;
    uint8_t inner_l3_type;
    uint8_t inner_l4_type;
};

/** rte_pktmbuf_get_packet_type() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_packet_type_in;

struct tarpc_rte_pktmbuf_get_packet_type_out {
    struct tarpc_out_arg                    common;
    struct tarpc_rte_pktmbuf_packet_type    p_type;
};

/** rte_pktmbuf_set_packet_type() */
struct tarpc_rte_pktmbuf_set_packet_type_in {
    struct tarpc_in_arg                     common;
    tarpc_rte_mbuf                          m;
    struct tarpc_rte_pktmbuf_packet_type    p_type;
};

typedef struct tarpc_int_retval_out tarpc_rte_pktmbuf_set_packet_type_out;

/** rte_pktmbuf_get_rss_hash() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_rss_hash_in;

struct tarpc_rte_pktmbuf_get_rss_hash_out {
    struct tarpc_out_arg    common;
    uint32_t                retval;
};

/** rte_pktmbuf_get_fdir_id() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_fdir_id_in;

struct tarpc_rte_pktmbuf_get_fdir_id_out {
    struct tarpc_out_arg    common;
    uint32_t                retval;
};

struct tarpc_rte_pktmbuf_tx_offload {
    uint16_t l2_len; 
    uint16_t l3_len; 
    uint16_t l4_len; 
    uint16_t tso_segsz; 
    uint16_t outer_l3_len; 
    uint16_t outer_l2_len;
};

/** rte_pktmbuf_get_tx_offload() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_tx_offload_in;

struct tarpc_rte_pktmbuf_get_tx_offload_out {
    struct tarpc_out_arg                    common;
    struct tarpc_rte_pktmbuf_tx_offload     tx_offload;
};

/** rte_pktmbuf_set_tx_offload() */
struct tarpc_rte_pktmbuf_set_tx_offload_in {
    struct tarpc_in_arg                     common;
    tarpc_rte_mbuf                          m;
    struct tarpc_rte_pktmbuf_tx_offload     tx_offload;
};

typedef struct tarpc_void_out tarpc_rte_pktmbuf_set_tx_offload_out;

/** rte_pktmbuf_refcnt_update() */
struct tarpc_rte_pktmbuf_refcnt_update_in {
    struct tarpc_in_arg common;
    tarpc_rte_mbuf      m;
    int16_t             v;
};

typedef struct tarpc_void_out tarpc_rte_pktmbuf_refcnt_update_out;

struct tarpc_pktmbuf_seg_group {
    uint16_t len;  /**< Segment length */
    uint8_t  num;  /**< Number of segments */
};

/** rte_pktmbuf_redist() */
struct tarpc_rte_pktmbuf_redist_in {
    struct tarpc_in_arg             common;
    tarpc_rte_mbuf                  m;
    tarpc_rte_mempool               mp_multi<>;
    struct tarpc_pktmbuf_seg_group  seg_groups<>;
};

struct tarpc_rte_pktmbuf_redist_out {
    struct tarpc_out_arg            common;
    tarpc_rte_mbuf                  m;
    tarpc_int                       retval;
};


/*
 * RTE ring API
 */

/** RTE ring flags */
enum tarpc_rte_ring_flags {
    TARPC_RTE_RING_F_SP_ENQ = 0,
    TARPC_RTE_RING_F_SC_DEQ,

    TARPC_RTE_RING_UNKNOWN
};

/** rte_ring_create() */
struct tarpc_rte_ring_create_in {
    struct tarpc_in_arg     common;
    string                  name<>;
    unsigned                count;
    int                     socket_id;
    unsigned                flags;
};

struct tarpc_rte_ring_create_out {
    struct tarpc_out_arg    common;
    tarpc_rte_ring          retval;
};

/** rte_ring_free() */
struct tarpc_rte_ring_free_in {
    struct tarpc_in_arg     common;
    tarpc_rte_ring          ring;
};

typedef struct tarpc_void_out tarpc_rte_ring_free_out;

/** rte_ring_enqueue_mbuf() */
struct tarpc_rte_ring_enqueue_mbuf_in {
    struct tarpc_in_arg     common;
    tarpc_rte_ring          ring;
    tarpc_rte_mbuf          m;
};

typedef struct tarpc_int_retval_out tarpc_rte_ring_enqueue_mbuf_out;

/** rte_ring_dequeue_mbuf() */
struct tarpc_rte_ring_dequeue_mbuf_in {
    struct tarpc_in_arg     common;
    tarpc_rte_ring          ring;
};

typedef struct tarpc_mbuf_retval_out tarpc_rte_ring_dequeue_mbuf_out;


/*
 * rte_mbuf_layer API
 */
struct tarpc_rte_mk_mbuf_from_template_in {
    struct tarpc_in_arg     common;
    string                  template<>;
    tarpc_rte_mempool       mp;
};

struct tarpc_rte_mk_mbuf_from_template_out {
    struct tarpc_out_arg    common;
    tarpc_rte_mbuf          mbufs<>;
    tarpc_int               retval;
};

struct tarpc_rte_mbuf_match_pattern_in {
    struct tarpc_in_arg     common;
    string                  pattern<>;
    tarpc_rte_mbuf          mbufs<>;
    tarpc_bool              return_matching_pkts;
    tarpc_bool              seq_match;
};

struct tarpc_rte_mbuf_match_pattern_out {
    struct tarpc_out_arg    common;
    unsigned int            matched;
    tarpc_string            packets<>;
    tarpc_int               retval;
};

/*
 * rte_eth_dev API
 */

struct tarpc_rte_eth_stats {
    uint64_t                        ipackets;
    uint64_t                        opackets;
    uint64_t                        ibytes;
    uint64_t                        obytes;
    uint64_t                        imissed;
    uint64_t                        ierrors;
    uint64_t                        oerrors;
    uint64_t                        rx_nombuf;
};

/** rte_eth_stats_get() */
struct tarpc_rte_eth_stats_get_in {
    struct tarpc_in_arg             common;
    uint16_t                        port_id;
};

struct tarpc_rte_eth_stats_get_out {
    struct tarpc_out_arg            common;
    struct tarpc_rte_eth_stats      stats;
    tarpc_int                       retval;
};

struct tarpc_rte_eth_dev_port_id_in {
    struct tarpc_in_arg             common;
    uint16_t                        port_id;
};

struct tarpc_rte_eth_dev_port_id_queue_id_in {
    struct tarpc_in_arg             common;
    uint16_t                        port_id;
    uint16_t                        queue_id;
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
    TARPC_RTE_DEV_RX_OFFLOAD_MACSEC_STRIP_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_HEADER_SPLIT_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_VLAN_FILTER_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_VLAN_EXTEND_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_JUMBO_FRAME_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_CRC_STRIP_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_SCATTER_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_TIMESTAMP_BIT,
    TARPC_RTE_DEV_RX_OFFLOAD_SECURITY_BIT,

    TARPC_RTE_DEV_RX_OFFLOAD__UNSUPPORTED_BIT,

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
    TARPC_RTE_DEV_TX_OFFLOAD_VXLAN_TNL_TSO_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_GRE_TNL_TSO_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_IPIP_TNL_TSO_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_GENEVE_TNL_TSO_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_MACSEC_INSERT_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_MT_LOCKFREE_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_MULTI_SEGS_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_MBUF_FAST_FREE_BIT,
    TARPC_RTE_DEV_TX_OFFLOAD_SECURITY_BIT,

    TARPC_RTE_DEV_TX_OFFLOAD__UNSUPPORTED_BIT,

    TARPC_RTE_DEV_TX_OFFLOAD__UNKNOWN_BIT
};

/** Flow types */
enum tarpc_rte_eth_flow_types {
    TARPC_RTE_ETH_FLOW_RAW = 0,
    TARPC_RTE_ETH_FLOW_IPV4,
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
    TARPC_RTE_ETH_FLOW_PORT,
    TARPC_RTE_ETH_FLOW_VXLAN,
    TARPC_RTE_ETH_FLOW_GENEVE,
    TARPC_RTE_ETH_FLOW_NVGRE,
    TARPC_RTE_ETH_FLOW_MAX,

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
    uint64_t                        offloads;
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
    uint64_t                        rx_queue_offload_capa;
    uint64_t                        rx_offload_capa;
    uint64_t                        tx_queue_offload_capa;
    uint64_t                        tx_offload_capa;
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
    TARPC_RTE_ETH_RXMODE_ENABLE_LRO_BIT,
    TARPC_RTE_ETH_RXMODE_HW_TIMESTAMP_BIT,
    TARPC_RTE_ETH_RXMODE_SECURITY_BIT,
    TARPC_RTE_ETH_RXMODE_IGNORE_OFFLOAD_BITFIELD_BIT
};

struct tarpc_rte_eth_rxmode {
    enum tarpc_rte_eth_rx_mq_mode       mq_mode;
    uint32_t                            max_rx_pkt_len;
    uint16_t                            split_hdr_size;
    uint64_t                            offloads;
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
    uint64_t                            offloads;
    uint16_t                            pvid;
    uint8_t                             flags;
};

struct tarpc_rte_eth_rss_conf {
    uint8_t                             rss_key<>;
    uint8_t                             rss_key_len;
    tarpc_rss_hash_protos_t             rss_hf;
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
    uint16_t                    port_id;
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
    uint16_t                    port_id;
    uint16_t                    tx_queue_id;
    uint16_t                    nb_tx_desc;
    tarpc_int                   socket_id;
    struct tarpc_rte_eth_txconf tx_conf<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_tx_queue_setup_out;

/** rte_eth_rx_queue_setup() */
struct tarpc_rte_eth_rx_queue_setup_in {
    struct tarpc_in_arg         common;
    uint16_t                    port_id;
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
    uint16_t             port_id;
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
    uint16_t             port_id;
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
    uint16_t             port_id;
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
    uint16_t             port_id;
    uint16_t             mtu;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_set_mtu_out;

/** rte_eth_dev_vlan_filter() */
struct tarpc_rte_eth_dev_vlan_filter_in {
    struct tarpc_in_arg  common;
    uint16_t             port_id;
    uint16_t             vlan_id;
    tarpc_int            on;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_vlan_filter_out;

/** rte_eth_dev_set_vlan_strip_on_queue() */
struct tarpc_rte_eth_dev_set_vlan_strip_on_queue_in {
    struct tarpc_in_arg  common;
    uint16_t             port_id;
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
    uint16_t                  port_id;
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
    uint16_t                port_id;
    tarpc_int               offload_mask;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_set_vlan_offload_out;

/** rte_eth_dev_get_vlan_offload() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_get_vlan_offload_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_get_vlan_offload_out;

/** rte_eth_dev_set_vlan_pvid() */
struct tarpc_rte_eth_dev_set_vlan_pvid_in {
    struct tarpc_in_arg     common;
    uint16_t                port_id;
    uint16_t                pvid;
    tarpc_int               on;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_set_vlan_pvid_out;

struct tarpc_descriptor_status_in {
    struct tarpc_in_arg common;
    uint16_t            port_id;
    uint16_t            queue_id;
    uint16_t            offset;
};

/** rte_eth_rx_descriptor_done() */
typedef struct tarpc_descriptor_status_in tarpc_rte_eth_rx_descriptor_done_in;
typedef struct tarpc_int_retval_out tarpc_rte_eth_rx_descriptor_done_out;

/** rte_eth_rx_queue_count() */
struct tarpc_rte_eth_rx_queue_count_in {
    struct tarpc_in_arg     common;
    uint16_t                port_id;
    uint16_t                queue_id;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_rx_queue_count_out;

enum tarpc_rte_eth_rx_desc_status_value {
    TARPC_RTE_ETH_RX_DESC_AVAIL = 0,
    TARPC_RTE_ETH_RX_DESC_DONE,
    TARPC_RTE_ETH_RX_DESC_UNAVAIL,

    TARPC_RTE_ETH_RX_DESC__UNKNOWN
};

enum tarpc_rte_eth_tx_desc_status_value {
    TARPC_RTE_ETH_TX_DESC_FULL = 0,
    TARPC_RTE_ETH_TX_DESC_DONE,
    TARPC_RTE_ETH_TX_DESC_UNAVAIL,

    TARPC_RTE_ETH_TX_DESC__UNKNOWN
};

/** rte_eth_rx_descriptor_status() */
typedef struct tarpc_descriptor_status_in tarpc_rte_eth_rx_descriptor_status_in;
typedef struct tarpc_int_retval_out tarpc_rte_eth_rx_descriptor_status_out;

/** rte_eth_tx_descriptor_status() */
typedef struct tarpc_descriptor_status_in tarpc_rte_eth_tx_descriptor_status_in;
typedef struct tarpc_int_retval_out tarpc_rte_eth_tx_descriptor_status_out;

/** rte_eth_dev_socket_id() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_socket_id_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_socket_id_out;

/** rte_eth_dev_is_valid_port() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_is_valid_port_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_is_valid_port_out;

/** rte_eth_dev_rx_queue_start() */
typedef struct tarpc_rte_eth_dev_port_id_queue_id_in tarpc_rte_eth_dev_rx_queue_start_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_rx_queue_start_out;

/** rte_eth_dev_rx_queue_stop() */
typedef struct tarpc_rte_eth_dev_port_id_queue_id_in tarpc_rte_eth_dev_rx_queue_stop_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_rx_queue_stop_out;

/** rte_eth_dev_tx_queue_start() */
typedef struct tarpc_rte_eth_dev_port_id_queue_id_in tarpc_rte_eth_dev_tx_queue_start_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_tx_queue_start_out;

/** rte_eth_dev_tx_queue_stop() */
typedef struct tarpc_rte_eth_dev_port_id_queue_id_in tarpc_rte_eth_dev_tx_queue_stop_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_tx_queue_stop_out;

struct tarpc_ether_addr {
    uint8_t    addr_bytes[6];
};

/** rte_eth_macaddr_get() */
struct tarpc_rte_eth_macaddr_get_in {
    struct tarpc_in_arg      common;
    uint16_t                 port_id;
    struct tarpc_ether_addr  mac_addr<>;
};

struct tarpc_rte_eth_macaddr_get_out {
    struct tarpc_out_arg     common;
    struct tarpc_ether_addr  mac_addr<>;
};

/** rte_eth_dev_default_mac_addr_set() */
struct tarpc_rte_eth_dev_default_mac_addr_set_in {
    struct tarpc_in_arg      common;
    uint16_t                 port_id;
    struct tarpc_ether_addr  mac_addr<>;
};

struct tarpc_rte_eth_rxq_info {
    tarpc_rte_mempool             mp;
    struct tarpc_rte_eth_rxconf   conf;
    uint8_t                       scattered_rx;
    uint16_t                      nb_desc;
};

/** rte_eth_rx_queue_info_get() */
typedef struct tarpc_rte_eth_dev_port_id_queue_id_in tarpc_rte_eth_rx_queue_info_get_in;

struct tarpc_rte_eth_rx_queue_info_get_out {
    struct tarpc_out_arg          common;
    struct tarpc_rte_eth_rxq_info qinfo;
    tarpc_int                     retval;

};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_default_mac_addr_set_out;

struct tarpc_rte_eth_txq_info {
    struct tarpc_rte_eth_txconf   conf;
    uint16_t                      nb_desc;
};

/** rte_eth_tx_queue_info_get() */
typedef struct tarpc_rte_eth_dev_port_id_queue_id_in tarpc_rte_eth_tx_queue_info_get_in;

struct tarpc_rte_eth_tx_queue_info_get_out {
    struct tarpc_out_arg          common;
    struct tarpc_rte_eth_txq_info qinfo;
    tarpc_int                     retval;

};

/** rte_vlan_strip() */
struct tarpc_rte_vlan_strip_in {
    struct tarpc_in_arg    common;
    tarpc_rte_mbuf         m;
};

typedef struct tarpc_int_retval_out tarpc_rte_vlan_strip_out;

/** rte_eth_dev_attach() */
struct tarpc_rte_eth_dev_attach_in {
    struct tarpc_in_arg common;
    string              devargs<>;
};

struct tarpc_rte_eth_dev_attach_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    uint16_t             port_id;
};

/** rte_eth_dev_detach() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_detach_in;

struct tarpc_rte_eth_dev_detach_out {
    struct tarpc_out_arg    common;
    tarpc_int               retval;
    string                  devname<>;
};

struct tarpc_rte_eth_rss_reta_entry64 {
    uint64_t    mask;
    uint16_t    reta[64];
};

/** rte_eth_dev_rss_reta_query() */
struct tarpc_rte_eth_dev_rss_reta_query_in {
    struct tarpc_in_arg                    common;
    uint16_t                               port_id;
    uint16_t                               reta_size;
    struct tarpc_rte_eth_rss_reta_entry64  reta_conf<>;
};

struct tarpc_rte_eth_dev_rss_reta_query_out {
    struct tarpc_out_arg                   common;
    tarpc_int                              retval;
    struct tarpc_rte_eth_rss_reta_entry64  reta_conf<>;
};

/** rte_eth_dev_rss_reta_update() */
struct tarpc_rte_eth_dev_rss_reta_update_in {
    struct tarpc_in_arg                    common;
    uint16_t                               port_id;
    uint16_t                               reta_size;
    struct tarpc_rte_eth_rss_reta_entry64  reta_conf<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_rss_reta_update_out;

/** rte_eth_dev_rss_hash_conf_get() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_rss_hash_conf_get_in;

struct tarpc_rte_eth_dev_rss_hash_conf_get_out {
    struct tarpc_out_arg                   common;
    tarpc_int                              retval;
    struct tarpc_rte_eth_rss_conf          rss_conf;
};

enum tarpc_rte_eth_fc_mode {
    TARPC_RTE_FC_NONE,
    TARPC_RTE_FC_RX_PAUSE,
    TARPC_RTE_FC_TX_PAUSE,
    TARPC_RTE_FC_FULL,

    TARPC_RTE_FC__UNKNOWN
};

struct tarpc_rte_eth_fc_conf {
    uint32_t                      high_water;
    uint32_t                      low_water;
    uint16_t                      pause_time;
    uint16_t                      send_xon;
    enum tarpc_rte_eth_fc_mode    mode;
    uint8_t                       mac_ctrl_frame_fwd;
    uint8_t                       autoneg;
};

/** rte_eth_dev_flow_ctrl_get() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_flow_ctrl_get_in;

struct tarpc_rte_eth_dev_flow_ctrl_get_out {
    struct tarpc_out_arg                   common;
    tarpc_int                              retval;
    struct tarpc_rte_eth_fc_conf           fc_conf;
};

/** rte_eth_dev_flow_ctrl_set() */
struct tarpc_rte_eth_dev_flow_ctrl_set_in {
    struct tarpc_in_arg                    common;
    uint16_t                               port_id;
    struct tarpc_rte_eth_fc_conf           fc_conf;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_flow_ctrl_set_out;

enum tarpc_rte_filter_type {
    TARPC_RTE_ETH_FILTER_NONE = 0,
    TARPC_RTE_ETH_FILTER_MACVLAN,
    TARPC_RTE_ETH_FILTER_ETHERTYPE,
    TARPC_RTE_ETH_FILTER_FLEXIBLE,
    TARPC_RTE_ETH_FILTER_SYN,
    TARPC_RTE_ETH_FILTER_NTUPLE,
    TARPC_RTE_ETH_FILTER_TUNNEL,
    TARPC_RTE_ETH_FILTER_FDIR,
    TARPC_RTE_ETH_FILTER_HASH,
    TARPC_RTE_ETH_FILTER_L2_TUNNEL,
    TARPC_RTE_ETH_FILTER_MAX,

    TARPC_RTE_ETH_FILTER__UNKNOWN
};

/** rte_eth_dev_filter_supported() */
struct tarpc_rte_eth_dev_filter_supported_in {
    struct tarpc_in_arg         common;
    uint16_t                    port_id;
    enum tarpc_rte_filter_type  filter_type;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_filter_supported_out;


/**
 * Generic operations on filters
 */
enum tarpc_rte_filter_op {
    /** used to check whether the type filter is supported */
    TARPC_RTE_ETH_FILTER_NOP = 0,
    TARPC_RTE_ETH_FILTER_ADD,
    TARPC_RTE_ETH_FILTER_UPDATE,
    TARPC_RTE_ETH_FILTER_DELETE,
    TARPC_RTE_ETH_FILTER_FLUSH,
    TARPC_RTE_ETH_FILTER_GET,
    TARPC_RTE_ETH_FILTER_SET,
    TARPC_RTE_ETH_FILTER_INFO,
    TARPC_RTE_ETH_FILTER_STATS,
    TARPC_RTE_ETH_FILTER_OP_MAX,

    TARPC_RTE_ETH_FILTER_OP__UNKNOWN
};

enum tarpc_rte_ethtype_flags {
    TARPC_RTE_ETHTYPE_FLAGS_MAC_BIT = 0,
    TARPC_RTE_ETHTYPE_FLAGS_DROP_BIT,

    TARPC_RTE_ETHTYPE_FLAGS__UNKNOWN_BIT
};

struct tarpc_rte_eth_ethertype_filter {
    struct tarpc_ether_addr mac_addr;
    uint16_t                ether_type;
    uint16_t                flags;
    uint16_t                queue;
};

enum tarpc_rte_ntuple_flags {
    TARPC_RTE_NTUPLE_FLAGS_DST_IP_BIT = 0,
    TARPC_RTE_NTUPLE_FLAGS_SRC_IP_BIT,
    TARPC_RTE_NTUPLE_FLAGS_DST_PORT_BIT,
    TARPC_RTE_NTUPLE_FLAGS_SRC_PORT_BIT,
    TARPC_RTE_NTUPLE_FLAGS_PROTO_BIT,
    TARPC_RTE_NTUPLE_FLAGS_TCP_FLAG_BIT,

    TARPC_RTE_NTUPLE_FLAGS__UNKNOWN_BIT
};

enum tarpc_rte_tcp_flags {
    TARPC_RTE_TCP_URG_FLAG_BIT = 0,
    TARPC_RTE_TCP_ACK_FLAG_BIT,
    TARPC_RTE_TCP_PSH_FLAG_BIT,
    TARPC_RTE_TCP_RST_FLAG_BIT,
    TARPC_RTE_TCP_SYN_FLAG_BIT,
    TARPC_RTE_TCP_FIN_FLAG_BIT,
    TARPC_RTE_TCP_FLAG_ALL_BIT,

    TARPC_RTE_TCP__UNKNOWN_BIT
};

struct tarpc_rte_eth_ntuple_filter {
    uint16_t flags;
    uint32_t dst_ip;
    uint32_t dst_ip_mask;
    uint32_t src_ip;
    uint32_t src_ip_mask;
    uint16_t dst_port;
    uint16_t dst_port_mask;
    uint16_t src_port;
    uint16_t src_port_mask;
    uint8_t  proto;
    uint8_t  proto_mask;
    uint8_t  tcp_flags;
    uint16_t priority;
    uint16_t queue;
};

/** rte_eth_dev_filter_ctrl */
struct tarpc_rte_eth_dev_filter_ctrl_in {
    struct tarpc_in_arg         common;
    uint16_t                    port_id;
    enum tarpc_rte_filter_type  filter_type;
    enum tarpc_rte_filter_op    filter_op;
    uint8_t                     arg<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_filter_ctrl_out;

/** rte_eth_dev_rss_hash_update() */
struct tarpc_rte_eth_dev_rss_hash_update_in {
    struct tarpc_in_arg                   common;
    uint16_t                              port_id;
    struct tarpc_rte_eth_rss_conf         rss_conf;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_rss_hash_update_out;

struct tarpc_rte_eth_link {
    uint32_t   link_speed;
    uint8_t    link_duplex;
    uint8_t    link_autoneg;
    uint8_t    link_status;
};

/** rte_eth_link_get_nowait() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_link_get_nowait_in;

struct tarpc_rte_eth_link_get_nowait_out {
    struct tarpc_out_arg                  common;
    struct tarpc_rte_eth_link             eth_link;
};

/** rte_eth_link_get() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_link_get_in;

struct tarpc_rte_eth_link_get_out {
    struct tarpc_out_arg                  common;
    struct tarpc_rte_eth_link             eth_link;
};

/** rte_eth_dev_fw_version_get() */
struct tarpc_rte_eth_dev_fw_version_get_in {
    struct tarpc_in_arg     common;
    uint16_t                port_id;
    char                    fw_version<>;
};

struct tarpc_rte_eth_dev_fw_version_get_out {
    struct tarpc_out_arg    common;
    char                    fw_version<>;
    tarpc_int               retval;
};

enum tarpc_rte_eth_xstats_name_size {
    TARPC_RTE_ETH_XSTATS_NAME_SIZE = 64
};

struct tarpc_rte_eth_xstat {
    uint64_t    id;
    uint64_t    value;
};

struct tarpc_rte_eth_xstat_name {
    char    name[TARPC_RTE_ETH_XSTATS_NAME_SIZE];
};

/** rte_eth_xstats_get_names() */
struct tarpc_rte_eth_xstats_get_names_in {
    struct tarpc_in_arg                     common;
    uint16_t                                port_id;
    unsigned int                            size;
};

struct tarpc_rte_eth_xstats_get_names_out {
    struct tarpc_out_arg                    common;
    tarpc_int                               retval;
    struct tarpc_rte_eth_xstat_name         xstats_names<>;
};

/** rte_eth_xstats_get() */
struct tarpc_rte_eth_xstats_get_in {
    struct tarpc_in_arg                     common;
    uint16_t                                port_id;
    unsigned int                            n;
};

struct tarpc_rte_eth_xstats_get_out {
    struct tarpc_out_arg                    common;
    tarpc_int                               retval;
    struct tarpc_rte_eth_xstat              xstats<>;
};

/** rte_eth_xstats_reset() */
struct tarpc_rte_eth_xstats_reset_in {
    struct tarpc_in_arg                     common;
    uint16_t                                port_id;
};

typedef struct tarpc_void_out tarpc_rte_eth_xstats_reset_out;

/** rte_eth_xstats_get_by_id() */
struct tarpc_rte_eth_xstats_get_by_id_in {
    struct tarpc_in_arg common;
    uint16_t            port_id;
    uint64_t            ids<>;
    unsigned int        n;
};

struct tarpc_rte_eth_xstats_get_by_id_out {
    struct tarpc_out_arg common;
    uint64_t             values<>;
    tarpc_int            retval;
};

/** rte_eth_xstats_get_names_by_id() */
struct tarpc_rte_eth_xstats_get_names_by_id_in {
    struct tarpc_in_arg common;
    uint16_t            port_id;
    uint64_t            ids<>;
    unsigned int        size;
};

struct tarpc_rte_eth_xstats_get_names_by_id_out {
    struct tarpc_out_arg            common;
    struct tarpc_rte_eth_xstat_name xstat_names<>;
    tarpc_int                       retval;
};

enum tarpc_rte_pktmbuf_types_masks {
    TARPC_RTE_PTYPE_L2_MASK       = 0x0000000f,
    TARPC_RTE_PTYPE_L3_MASK       = 0x000000f0,
    TARPC_RTE_PTYPE_L4_MASK       = 0x00000f00,
    TARPC_RTE_PTYPE_TUNNEL_MASK   = 0x0000f000,
    TARPC_RTE_PTYPE_INNER_L2_MASK = 0x000f0000,
    TARPC_RTE_PTYPE_INNER_L3_MASK = 0x00f00000,
    TARPC_RTE_PTYPE_INNER_L4_MASK = 0x0f000000,
    TARPC_RTE_PTYPE_ALL_MASK      = 0x0fffffff
};

enum tarpc_rte_pktmbuf_types_offsets {
    TARPC_RTE_PTYPE_L2_OFFSET       = 0,
    TARPC_RTE_PTYPE_L3_OFFSET       = 4,
    TARPC_RTE_PTYPE_L4_OFFSET       = 8,
    TARPC_RTE_PTYPE_TUNNEL_OFFSET   = 12,
    TARPC_RTE_PTYPE_INNER_L2_OFFSET = 16,
    TARPC_RTE_PTYPE_INNER_L3_OFFSET = 20,
    TARPC_RTE_PTYPE_INNER_L4_OFFSET = 24
};

/** rte_eth_dev_get_supported_ptypes() */
struct tarpc_rte_eth_dev_get_supported_ptypes_in {
    struct tarpc_in_arg                     common;
    uint16_t                                port_id;
    uint32_t                                ptype_mask;
    int                                     num;
};

struct tarpc_rte_eth_dev_get_supported_ptypes_out {
    struct tarpc_out_arg                    common;
    tarpc_int                               retval;
    uint32_t                                ptypes<>;
};

/** rte_eth_dev_set_mc_addr_list() */
struct tarpc_rte_eth_dev_set_mc_addr_list_in {
    struct tarpc_in_arg                     common;
    uint16_t                                port_id;
    struct tarpc_ether_addr                 mc_addr_set<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_set_mc_addr_list_out;

/** enum rte_eth_tunnel_type */
enum tarpc_rte_eth_tunnel_type {
    TARPC_RTE_TUNNEL_TYPE_NONE,
    TARPC_RTE_TUNNEL_TYPE_VXLAN,
    TARPC_RTE_TUNNEL_TYPE_GENEVE,
    TARPC_RTE_TUNNEL_TYPE_TEREDO,
    TARPC_RTE_TUNNEL_TYPE_NVGRE,
    TARPC_RTE_TUNNEL_TYPE_IP_IN_GRE,
    TARPC_RTE_L2_TUNNEL_TYPE_E_TAG,
    TARPC_RTE_TUNNEL_TYPE_MAX,

    TARPC_RTE_TUNNEL_TYPE__UNKNOWN
};

struct tarpc_rte_eth_udp_tunnel {
    uint16_t udp_port;
    uint8_t  prot_type;
};

/** rte_eth_dev_udp_tunnel_port_add() */
struct tarpc_rte_eth_dev_udp_tunnel_port_add_in {
    struct tarpc_in_arg             common;
    uint16_t                        port_id;
    struct tarpc_rte_eth_udp_tunnel tunnel_udp;
};

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_udp_tunnel_port_add_out;

/** rte_eth_dev_udp_tunnel_port_delete() */
typedef struct tarpc_rte_eth_dev_udp_tunnel_port_add_in tarpc_rte_eth_dev_udp_tunnel_port_delete_in;

typedef struct tarpc_int_retval_out tarpc_rte_eth_dev_udp_tunnel_port_delete_out;

/** rte_eth_dev_get_port_by_name() */
struct tarpc_rte_eth_dev_get_port_by_name_in {
    struct tarpc_in_arg common;
    string              name<>;
};

struct tarpc_rte_eth_dev_get_port_by_name_out {
    struct tarpc_out_arg common;
    uint16_t             port_id;
    tarpc_int            retval;
};

/** rte_eth_dev_get_name_by_port() */
typedef struct tarpc_rte_eth_dev_port_id_in tarpc_rte_eth_dev_get_name_by_port_in;

struct tarpc_rte_eth_dev_get_name_by_port_out {
    struct tarpc_out_arg common;
    string               name<>;
    tarpc_int            retval;
};

/** rte_eth_dev_rx_offload_name() */
struct tarpc_rte_eth_dev_rx_offload_name_in {
    struct tarpc_in_arg common;
    uint64_t            offload;
};

struct tarpc_rte_eth_dev_rx_offload_name_out {
    struct tarpc_out_arg common;
    string               retval<>;
};

/** rte_eth_dev_tx_offload_name() */
struct tarpc_rte_eth_dev_tx_offload_name_in {
    struct tarpc_in_arg common;
    uint64_t            offload;
};

struct tarpc_rte_eth_dev_tx_offload_name_out {
    struct tarpc_out_arg common;
    string               retval<>;
};

/**
 * rte_flow API
 */

/** Handle of the 'rte_flow_attr' or 0 */
typedef tarpc_ptr    tarpc_rte_flow_attr;

/** Handle of the 'rte_flow_item' or 0 */
typedef tarpc_ptr    tarpc_rte_flow_item;

/** Handle of the 'rte_flow_action' or 0 */
typedef tarpc_ptr    tarpc_rte_flow_action;

/** RTE flow handle or zero */
typedef tarpc_ptr    tarpc_rte_flow;

enum tarpc_rte_flow_error_type {
    TARPC_RTE_FLOW_ERROR_TYPE_NONE = 0,
    TARPC_RTE_FLOW_ERROR_TYPE_UNSPECIFIED,
    TARPC_RTE_FLOW_ERROR_TYPE_HANDLE,
    TARPC_RTE_FLOW_ERROR_TYPE_ATTR_GROUP,
    TARPC_RTE_FLOW_ERROR_TYPE_ATTR_PRIORITY,
    TARPC_RTE_FLOW_ERROR_TYPE_ATTR_INGRESS,
    TARPC_RTE_FLOW_ERROR_TYPE_ATTR_EGRESS,
    TARPC_RTE_FLOW_ERROR_TYPE_ATTR,
    TARPC_RTE_FLOW_ERROR_TYPE_ITEM_NUM,
    TARPC_RTE_FLOW_ERROR_TYPE_ITEM,
    TARPC_RTE_FLOW_ERROR_TYPE_ACTION_NUM,
    TARPC_RTE_FLOW_ERROR_TYPE_ACTION
};

struct tarpc_rte_flow_error {
    enum tarpc_rte_flow_error_type  type;
    string                          message<>;
};

enum tarpc_rte_flow_rule_component_flags {
    TARPC_RTE_FLOW_ATTR_FLAG    = 0x01,
    TARPC_RTE_FLOW_PATTERN_FLAG = 0x02,
    TARPC_RTE_FLOW_ACTIONS_FLAG = 0x04,
    /* Flow rule is the union of all the previous components */
    TARPC_RTE_FLOW_RULE_FLAGS   = 0x07
};

/** rte_mk_flow_rule_components() */
struct tarpc_rte_mk_flow_rule_components_in {
    struct tarpc_in_arg                 common;
    uint8_t                             component_flags;
    string                              flow_rule_components<>;
};

struct tarpc_rte_mk_flow_rule_components_out {
    struct tarpc_out_arg            common;
    tarpc_int                       retval;
    tarpc_rte_flow_attr             attr;
    tarpc_rte_flow_item             pattern;
    tarpc_rte_flow_action           actions;
};

/** rte_free_flow_rule() */
struct tarpc_rte_free_flow_rule_in {
    struct tarpc_in_arg             common;
    tarpc_rte_flow_attr             attr;
    tarpc_rte_flow_item             pattern;
    tarpc_rte_flow_action           actions;
};

typedef struct tarpc_void_out tarpc_rte_free_flow_rule_out;

/** rte_flow_validate() */
struct tarpc_rte_flow_validate_in {
    struct tarpc_in_arg             common;
    uint16_t                        port_id;
    tarpc_rte_flow_attr             attr;
    tarpc_rte_flow_item             pattern;
    tarpc_rte_flow_action           actions;
};

struct tarpc_rte_flow_validate_out {
    struct tarpc_out_arg            common;
    tarpc_int                       retval;
    struct tarpc_rte_flow_error     error;
};

/** rte_flow_create() */
typedef struct tarpc_rte_flow_validate_in tarpc_rte_flow_create_in;

struct tarpc_rte_flow_create_out {
    struct tarpc_out_arg            common;
    tarpc_rte_flow                  flow;
    struct tarpc_rte_flow_error     error;
};

/** rte_flow_destroy() */
struct tarpc_rte_flow_destroy_in {
    struct tarpc_in_arg             common;
    uint16_t                        port_id;
    tarpc_rte_flow                  flow;
};

typedef struct tarpc_rte_flow_validate_out tarpc_rte_flow_destroy_out;

/** rte_flow_flush() */
struct tarpc_rte_flow_flush_in {
    struct tarpc_in_arg             common;
    uint16_t                        port_id;
};

typedef struct tarpc_rte_flow_validate_out tarpc_rte_flow_flush_out;

/** rte_flow_isolate() */
struct tarpc_rte_flow_isolate_in {
    struct tarpc_in_arg         common;
    uint16_t                    port_id;
    tarpc_int                   set;
};

typedef struct tarpc_rte_flow_validate_out tarpc_rte_flow_isolate_out;


/**
 * Handmade DPDK utility RPCs
 */

/*
 * TODO: look for the other handmade RPCs, relist here,
 *       move to separate header and source files
 */

/** dpdk_eth_await_link_up() */
struct tarpc_dpdk_eth_await_link_up_in {
    struct tarpc_in_arg common;
    uint8_t             port_id;
    unsigned int        nb_attempts;
    unsigned int        wait_int_ms;
    unsigned int        after_up_ms;
};

typedef struct tarpc_int_retval_out tarpc_dpdk_eth_await_link_up_out;

program dpdk
{
    version ver0
    {
        RPC_DEF(rte_eal_init)
        RPC_DEF(rte_eal_process_type)

        RPC_DEF(rte_mempool_lookup)
        RPC_DEF(rte_mempool_in_use_count)
        RPC_DEF(rte_mempool_free)

        RPC_DEF(rte_pktmbuf_pool_create)
        RPC_DEF(rte_pktmbuf_pool_create_by_ops)
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
        RPC_DEF(rte_pktmbuf_get_vlan_tci)
        RPC_DEF(rte_pktmbuf_set_vlan_tci)
        RPC_DEF(rte_pktmbuf_get_vlan_tci_outer)
        RPC_DEF(rte_pktmbuf_set_vlan_tci_outer)
        RPC_DEF(rte_pktmbuf_get_flags)
        RPC_DEF(rte_pktmbuf_set_flags)
        RPC_DEF(rte_pktmbuf_get_pool)
        RPC_DEF(rte_pktmbuf_headroom)
        RPC_DEF(rte_pktmbuf_tailroom)
        RPC_DEF(rte_pktmbuf_trim)
        RPC_DEF(rte_pktmbuf_adj)
        RPC_DEF(rte_pktmbuf_get_packet_type)
        RPC_DEF(rte_pktmbuf_set_packet_type)
        RPC_DEF(rte_pktmbuf_get_rss_hash)
        RPC_DEF(rte_pktmbuf_get_fdir_id)
        RPC_DEF(rte_pktmbuf_get_tx_offload)
        RPC_DEF(rte_pktmbuf_set_tx_offload)
        RPC_DEF(rte_pktmbuf_refcnt_update)

        RPC_DEF(rte_pktmbuf_redist)

        RPC_DEF(rte_vlan_strip)

        RPC_DEF(rte_ring_create)
        RPC_DEF(rte_ring_free)
        RPC_DEF(rte_ring_enqueue_mbuf)
        RPC_DEF(rte_ring_dequeue_mbuf)

        RPC_DEF(rte_mk_mbuf_from_template)
        RPC_DEF(rte_mbuf_match_pattern)

        RPC_DEF(rte_eth_stats_get)
        RPC_DEF(rte_eth_xstats_get)
        RPC_DEF(rte_eth_xstats_get_names)
        RPC_DEF(rte_eth_xstats_reset)
        RPC_DEF(rte_eth_xstats_get_by_id)
        RPC_DEF(rte_eth_xstats_get_names_by_id)
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
        RPC_DEF(rte_eth_dev_get_vlan_offload)
        RPC_DEF(rte_eth_dev_set_vlan_pvid)
        RPC_DEF(rte_eth_rx_descriptor_done)
        RPC_DEF(rte_eth_rx_queue_count)
        RPC_DEF(rte_eth_rx_descriptor_status)
        RPC_DEF(rte_eth_tx_descriptor_status)
        RPC_DEF(rte_eth_dev_socket_id)
        RPC_DEF(rte_eth_dev_is_valid_port)
        RPC_DEF(rte_eth_dev_rx_queue_start)
        RPC_DEF(rte_eth_dev_rx_queue_stop)
        RPC_DEF(rte_eth_dev_tx_queue_start)
        RPC_DEF(rte_eth_dev_tx_queue_stop)
        RPC_DEF(rte_eth_macaddr_get)
        RPC_DEF(rte_eth_dev_default_mac_addr_set)
        RPC_DEF(rte_eth_rx_queue_info_get)
        RPC_DEF(rte_eth_tx_queue_info_get)
        RPC_DEF(rte_eth_dev_attach)
        RPC_DEF(rte_eth_dev_detach)
        RPC_DEF(rte_eth_dev_rss_reta_query)
        RPC_DEF(rte_eth_dev_rss_hash_conf_get)
        RPC_DEF(rte_eth_dev_flow_ctrl_get)
        RPC_DEF(rte_eth_dev_flow_ctrl_set)
        RPC_DEF(rte_eth_dev_filter_supported)
        RPC_DEF(rte_eth_dev_filter_ctrl)
        RPC_DEF(rte_eth_dev_rss_hash_update)
        RPC_DEF(rte_eth_dev_rss_reta_update)
        RPC_DEF(rte_eth_dev_get_supported_ptypes)
        RPC_DEF(rte_eth_dev_set_mc_addr_list)
        RPC_DEF(rte_eth_link_get_nowait)
        RPC_DEF(rte_eth_link_get)
        RPC_DEF(rte_eth_dev_fw_version_get)
        RPC_DEF(rte_eth_dev_udp_tunnel_port_add)
        RPC_DEF(rte_eth_dev_udp_tunnel_port_delete)
        RPC_DEF(rte_eth_dev_get_port_by_name)
        RPC_DEF(rte_eth_dev_get_name_by_port)
        RPC_DEF(rte_eth_dev_rx_offload_name)
        RPC_DEF(rte_eth_dev_tx_offload_name)

        RPC_DEF(rte_mk_flow_rule_components)
        RPC_DEF(rte_free_flow_rule)
        RPC_DEF(rte_flow_validate)
        RPC_DEF(rte_flow_create)
        RPC_DEF(rte_flow_destroy)
        RPC_DEF(rte_flow_flush)
        RPC_DEF(rte_flow_isolate)

        RPC_DEF(dpdk_eth_await_link_up)
    } = 1;
} = 2;
