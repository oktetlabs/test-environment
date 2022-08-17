/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for file protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE_NDN_IPSTACK_H__
#define __TE_NDN_IPSTACK_H__


#include "asn_usr.h"
#include "ndn.h"

typedef enum {
    NDN_TAG_IP4_VERSION,
    NDN_TAG_IP4_HLEN,
    NDN_TAG_IP4_TOS,
    NDN_TAG_IP4_LEN,
    NDN_TAG_IP4_IDENT,
    NDN_TAG_IP4_FLAG_RESERVED,
    NDN_TAG_IP4_DONT_FRAG,
    NDN_TAG_IP4_MORE_FRAGS,
    NDN_TAG_IP4_FRAG_OFFSET,
    NDN_TAG_IP4_TTL,
    NDN_TAG_IP4_PROTOCOL,
    NDN_TAG_IP4_H_CHECKSUM,
    NDN_TAG_IP4_SRC_ADDR,
    NDN_TAG_IP4_DST_ADDR,
    NDN_TAG_IP4_OPTIONS,
    NDN_TAG_IP4_LOCAL_ADDR,
    NDN_TAG_IP4_REMOTE_ADDR,
    NDN_TAG_IP4_MTU,
    NDN_TAG_IP4_FRAGMENTS,
    NDN_TAG_IP4_FR_HO, /**< fragment offset: value for IP header */
    NDN_TAG_IP4_FR_RO, /**< "real" fragment offset */
    NDN_TAG_IP4_FR_HL, /**< fragment length: value for IP header */
    NDN_TAG_IP4_FR_RL, /**< "real" fragment length */
    NDN_TAG_IP4_FR_MF, /**< value for "more fragments" flag in fragment */
    NDN_TAG_IP4_FR_DF, /**< value for "don't fragment" flag in fragment */
    NDN_TAG_IP4_FR_ID, /**< value for ID field in fragment */
    NDN_TAG_IP4_PLD_CHECKSUM,
    NDN_TAG_IP4_PLD_CH_OFFSET,
    NDN_TAG_IP4_PLD_CH_DISABLE,
    NDN_TAG_IP4_PLD_CH_DIFF,
    NDN_TAG_IP4_IFNAME,
} ndn_ip4_tags_t;


typedef enum {
    NDN_TAG_IP6_PLD_CH_OFFSET,
    NDN_TAG_IP6_PLD_CH_DISABLE,
    NDN_TAG_IP6_PLD_CH_DIFF,
    NDN_TAG_IP6_PLD_CHECKSUM,

    NDN_TAG_IP6_EXT_HEADER_OPT_TYPE,
    NDN_TAG_IP6_EXT_HEADER_OPT_LEN,
    NDN_TAG_IP6_EXT_HEADER_OPT_DATA,
    NDN_TAG_IP6_EXT_HEADER_OPT_VALUE,

    NDN_TAG_IP6_EXT_HEADER_OPT_PAD1,
    NDN_TAG_IP6_EXT_HEADER_OPT_TLV,
    NDN_TAG_IP6_EXT_HEADER_OPT_ROUTER_ALERT,

    NDN_TAG_IP6_EXT_HEADER_LEN,
    NDN_TAG_IP6_EXT_HEADER_OPTIONS,
    NDN_TAG_IP6_EXT_HEADER_HOP_BY_HOP,
    NDN_TAG_IP6_EXT_HEADER_DESTINATION,

    /* IPv6 Fragment extension header and its fields */
    NDN_TAG_IP6_EXT_HEADER_FRAGMENT,        /**< IPv6 Fragment extension
                                                 header */
    NDN_TAG_IP6_EXT_HEADER_FRAGMENT_RES1,   /**< The first reserved field
                                                 in IPv6 Fragment header */
    NDN_TAG_IP6_EXT_HEADER_FRAGMENT_OFFSET, /**< IPv6 Fragment offset */
    NDN_TAG_IP6_EXT_HEADER_FRAGMENT_RES2,   /**< The second reserved field
                                                 in IPv6 Fragment header */
    NDN_TAG_IP6_EXT_HEADER_FRAGMENT_M_FLAG, /**< IPv6 Fragment M flag */
    NDN_TAG_IP6_EXT_HEADER_FRAGMENT_ID,     /**< IPv6 Fragment
                                                 Identification */

    NDN_TAG_IP6_VERSION,
    NDN_TAG_IP6_TCL,
    NDN_TAG_IP6_FLAB,
    NDN_TAG_IP6_LEN,
    NDN_TAG_IP6_NEXT_HEADER,
    NDN_TAG_IP6_HLIM,
    NDN_TAG_IP6_SRC_ADDR,
    NDN_TAG_IP6_DST_ADDR,
    NDN_TAG_IP6_EXT_HEADERS,

    NDN_TAG_IP6_FRAGMENTS, /**< IPv6 Fragments specification in
                                packet template */
    NDN_TAG_IP6_FR_HO,     /**< Fragment offset: value for IPv6 header */
    NDN_TAG_IP6_FR_RO,     /**< "Real" fragment offset */
    NDN_TAG_IP6_FR_HL,     /**< Fragment length: value for IPv6 header */
    NDN_TAG_IP6_FR_RL,     /**< "Real" fragment length */
    NDN_TAG_IP6_FR_MF,     /**< Value for "more fragments" flag */
    NDN_TAG_IP6_FR_ID,     /**< Value for ID field */

    NDN_TAG_IP6_LOCAL_ADDR,
    NDN_TAG_IP6_REMOTE_ADDR,
} ndn_ip6_tags_t;

typedef enum {
    NDN_TAG_ICMP4_TYPE,
    NDN_TAG_ICMP4_CODE,
    NDN_TAG_ICMP4_CHECKSUM,
    NDN_TAG_ICMP4_UNUSED,
    NDN_TAG_ICMP4_PP_PTR,
    NDN_TAG_ICMP4_REDIRECT_GW,
    NDN_TAG_ICMP4_ID,
    NDN_TAG_ICMP4_SEQ,
    NDN_TAG_ICMP4_ORIG_TS,
    NDN_TAG_ICMP4_RX_TS,
    NDN_TAG_ICMP4_TX_TS,
} ndn_icmp4_tags_t;

typedef enum {
    /*
     * ICMPv6 message
     */
    NDN_TAG_ICMP6_TYPE,
    NDN_TAG_ICMP6_CODE,
    NDN_TAG_ICMP6_CHECKSUM,
    NDN_TAG_ICMP6_BODY, /* choice */
    NDN_TAG_ICMP6_OPTS, /* list */
    /*
     * ICMPv6 option
     */
    NDN_TAG_ICMP6_OPT,
    /*
     * Message variants
     */
    NDN_TAG_ICMP6_DEST_UNREACH,
    NDN_TAG_ICMP6_PACKET_TOO_BIG,
    NDN_TAG_ICMP6_TIME_EXCEEDED,
    NDN_TAG_ICMP6_PARAM_PROB,
    NDN_TAG_ICMP6_ROUTER_SOL,
    NDN_TAG_ICMP6_ROUTER_ADV,
    NDN_TAG_ICMP6_NEIGHBOR_SOL,
    NDN_TAG_ICMP6_NEIGHBOR_ADV,
    NDN_TAG_ICMP6_ECHO,
    NDN_TAG_ICMP6_MLD,
    /*
     * Message body layout
     *
     * Destination unreachable
     */
    NDN_TAG_ICMP6_DEST_UNREACH_UNUSED,
    /* Packet too big */
    NDN_TAG_ICMP6_PACKET_TOO_BIG_MTU,
    /* Time exceeded */
    NDN_TAG_ICMP6_TIME_EXCEEDED_UNUSED,
    /* Parameter problem message */
    NDN_TAG_ICMP6_PARAM_PROB_PTR,
    /* Router solicitation */
    NDN_TAG_ICMP6_ROUTER_SOL_RESERVED,
    /* Router advertisement */
    NDN_TAG_ICMP6_ROUTER_ADV_CUR_HOP_LIMIT,
    NDN_TAG_ICMP6_ROUTER_ADV_FLAGS,
    NDN_TAG_ICMP6_ROUTER_ADV_LIFETIME,
    NDN_TAG_ICMP6_ROUTER_ADV_REACHABLE_TIME,
    NDN_TAG_ICMP6_ROUTER_ADV_RETRANS_TIMER,
    /* Neighbor solicitation */
    NDN_TAG_ICMP6_NEIGHBOR_SOL_RESERVED,
    NDN_TAG_ICMP6_NEIGHBOR_SOL_TARGET_ADDR,
    /* Neighbor advertisement */
    NDN_TAG_ICMP6_NEIGHBOR_ADV_FLAGS,
    NDN_TAG_ICMP6_NEIGHBOR_ADV_TARGET_ADDR,
    /* Echo */
    NDN_TAG_ICMP6_ECHO_ID,
    NDN_TAG_ICMP6_ECHO_SEQ,
    /* MLD */
    NDN_TAG_ICMP6_MLD_MAX_RESPONSE_DELAY,
    NDN_TAG_ICMP6_MLD_RESERVED,
    NDN_TAG_ICMP6_MLD_GROUP_ADDR,
    /*
     * ICMPv6 option layout
     */
    NDN_TAG_ICMP6_OPT_TYPE,
    NDN_TAG_ICMP6_OPT_LEN,
    NDN_TAG_ICMP6_OPT_BODY,
    /*
     * ICMPv6 option variants
     */
    NDN_TAG_ICMP6_OPT_LL_ADDR,
    NDN_TAG_ICMP6_OPT_PREFIX,
    /*
     * ICMPv6 option 'Source link-layer address layout
     */
    NDN_TAG_ICMP6_OPT_LL_ADDR_MAC,
    /*
     * ICMPv6 option 'Prefix information' layout
     */
    NDN_TAG_ICMP6_OPT_PREFIX_PREFIX_LENGTH,
    NDN_TAG_ICMP6_OPT_PREFIX_FLAGS,
    NDN_TAG_ICMP6_OPT_PREFIX_VALID_LIFETIME,
    NDN_TAG_ICMP6_OPT_PREFIX_PREFERRED_LIFETIME,
    NDN_TAG_ICMP6_OPT_PREFIX_PREFIX,
} ndn_icmp6_tags_t;

typedef enum {
    NDN_TAG_TCP_SRC_PORT,
    NDN_TAG_TCP_DST_PORT,
    NDN_TAG_TCP_SEQN,
    NDN_TAG_TCP_ACKN,
    NDN_TAG_TCP_HLEN,
    NDN_TAG_TCP_FLAGS,
    NDN_TAG_TCP_WINDOW,
    NDN_TAG_TCP_CHECKSUM,
    NDN_TAG_TCP_URG,
    NDN_TAG_TCP_OPTIONS,
    NDN_TAG_TCP_LOCAL_PORT,
    NDN_TAG_TCP_REMOTE_PORT,
    NDN_TAG_TCP_DATA,
    NDN_TAG_TCP_DATA_SERVER,
    NDN_TAG_TCP_DATA_CLIENT,
    NDN_TAG_TCP_DATA_SOCKET,
    NDN_TAG_TCP_DATA_LENGTH,
} ndn_tcp_tags_t;

typedef enum {
    TE_TCP_OPT_EOL,
    TE_TCP_OPT_NOP,
    TE_TCP_OPT_MSS,
    TE_TCP_OPT_WIN_SCALE,
    TE_TCP_OPT_SACK_PERM,
    TE_TCP_OPT_SACK_DATA,
    TE_TCP_OPT_TIMESTAMP = 8,
} te_tcp_options_kind_t;

typedef enum {
    NDN_TAG_TCP_OPT_EOL,
    NDN_TAG_TCP_OPT_NOP,
    NDN_TAG_TCP_OPT_MSS,
    NDN_TAG_TCP_OPT_WIN_SCALE,
    NDN_TAG_TCP_OPT_SACK_PERM,
    NDN_TAG_TCP_OPT_SACK_DATA,
    NDN_TAG_TCP_OPT_TIMESTAMP = 8,
    NDN_TAG_TCP_OPT_LEN,
    NDN_TAG_TCP_OPT_SCALE,
    NDN_TAG_TCP_OPT_LEFT,
    NDN_TAG_TCP_OPT_RIGHT,
    NDN_TAG_TCP_OPT_VALUE,
    NDN_TAG_TCP_OPT_ECHO_REPLY,
    NDN_TAG_TCP_OPT_SACK_BLOCKS,
    NDN_TAG_TCP_OPT_SACK_BLOCK,
    NDN_TAG_TCP_OPT_SACK_LEFT,
    NDN_TAG_TCP_OPT_SACK_RIGHT,
} ndn_tcp_options_tags_t;

typedef enum {
    NDN_TAG_UDP_LOCAL_PORT,
    NDN_TAG_UDP_REMOTE_PORT,
    NDN_TAG_UDP_SRC_PORT,
    NDN_TAG_UDP_DST_PORT,
    NDN_TAG_UDP_LENGTH,
    NDN_TAG_UDP_CHECKSUM,
} ndn_udp_tags_t;


#ifdef __cplusplus
extern "C" {
#endif

extern const asn_type * const ndn_ip6_header;
extern const asn_type * const ndn_ip6_csap;

extern const asn_type * const ndn_ip6_ext_header_option;
extern const asn_type * const ndn_ip6_ext_header_options_seq;
extern const asn_type * const ndn_ip6_ext_header_hop_by_hop;
extern const asn_type * const ndn_ip6_ext_header_destination;
extern const asn_type * const ndn_ip6_ext_header;
extern const asn_type * const ndn_ip6_ext_headers_seq;
extern const asn_type * const ndn_ip6_ext_header_fragment;
extern const asn_type * const ndn_ip6_frag_spec;
extern const asn_type * const ndn_ip6_frag_seq;

extern const asn_type * const ndn_icmp6_dest_unreach;
extern const asn_type * const ndn_icmp6_packet_too_big;
extern const asn_type * const ndn_icmp6_time_exceeded;
extern const asn_type * const ndn_icmp6_param_prob;
extern const asn_type * const ndn_icmp6_router_sol;
extern const asn_type * const ndn_icmp6_router_adv;
extern const asn_type * const ndn_icmp6_neighbor_sol;
extern const asn_type * const ndn_icmp6_neighbor_adv;
extern const asn_type * const ndn_icmp6_echo;
extern const asn_type * const ndn_icmp6_mld;
extern const asn_type * const ndn_icmp6_body;
extern const asn_type * const ndn_icmp6_opt_ll_addr;
extern const asn_type * const ndn_icmp6_opt_prefix;
extern const asn_type * const ndn_icmp6_opt_body;
extern const asn_type * const ndn_icmp6_opt;
extern const asn_type * const ndn_icmp6_opts;
extern const asn_type * const ndn_icmp6_message;
extern const asn_type * const ndn_icmp6_csap;

extern const asn_type * const ndn_ip4_header;
extern const asn_type * const ndn_ip4_csap;

extern const asn_type * const ndn_ip4_frag_spec;
extern const asn_type * const ndn_ip4_frag_seq;

extern const asn_type * const ndn_icmp4_message;
extern const asn_type * const ndn_icmp4_csap;

extern const asn_type * const ndn_udp_header;
extern const asn_type * const ndn_udp_csap;

extern const asn_type * const ndn_tcp_header;
extern const asn_type * const ndn_tcp_csap;
extern const asn_type * const ndn_tcp_option;
extern const asn_type * const ndn_tcp_options_seq;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_IPSTACK_H__ */
