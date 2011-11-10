/** @file
 * @brief Proteos, TAD IP stack protocols, NDN.
 *
 * Definitions of ASN.1 types for NDN for file protocol.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_ipstack.h"
#include "ndn_eth.h"

/*
 * IPv4
 */

static asn_named_entry_t _ndn_ip4_frag_spec_ne_array [] = {
    { "hdr-offset",         &asn_base_integer_s,
        {PRIVATE, NDN_TAG_IP4_FR_HO} },
    { "real-offset",         &asn_base_integer_s,
        {PRIVATE, NDN_TAG_IP4_FR_RO} },
    { "hdr-length",         &asn_base_integer_s,
        {PRIVATE, NDN_TAG_IP4_FR_HL} },
    { "real-length",         &asn_base_integer_s,
        {PRIVATE, NDN_TAG_IP4_FR_RL} },
    { "more-frags",         &asn_base_boolean_s,
        {PRIVATE, NDN_TAG_IP4_FR_MF} },
    { "dont-frag",         &asn_base_boolean_s,
        {PRIVATE, NDN_TAG_IP4_FR_DF} },
};

asn_type ndn_ip4_frag_spec_s = {
    "IPv4-Fragment", {PRIVATE, NDN_TAG_IP4_FRAGMENTS}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip4_frag_spec_ne_array),
    {_ndn_ip4_frag_spec_ne_array}
};

const asn_type * const ndn_ip4_frag_spec = &ndn_ip4_frag_spec_s;

asn_type ndn_ip4_frag_seq_s = {
    "IPv4-Fragments", {PRIVATE, NDN_TAG_IP4_FRAGMENTS}, SEQUENCE_OF, 0,
    {subtype: &ndn_ip4_frag_spec_s}
};

const asn_type * const ndn_ip4_frag_seq = &ndn_ip4_frag_seq_s;

/*
IP-Payload-Checksum ::= CHOICE {
    offset  INTEGER,
    disable NULL
}
*/
static asn_named_entry_t _ndn_ip4_pld_chksm_ne_array [] = {
    { "offset", &asn_base_integer_s,
      { PRIVATE, NDN_TAG_IP4_PLD_CH_OFFSET } },
    { "disable", &asn_base_null_s,
      { PRIVATE, NDN_TAG_IP4_PLD_CH_DISABLE } },
    { "diff", &asn_base_integer_s,
      { PRIVATE, NDN_TAG_IP4_PLD_CH_DIFF } },
};

asn_type ndn_ip4_pld_chksm_s = {
    "IP-Payload-Checksum", {PRIVATE, NDN_TAG_IP4_PLD_CHECKSUM}, CHOICE,
    TE_ARRAY_LEN(_ndn_ip4_pld_chksm_ne_array),
    {_ndn_ip4_pld_chksm_ne_array}
};

asn_type *ndn_ip4_pld_chksm = &ndn_ip4_pld_chksm_s;

/* IPv4 PDU */

static asn_named_entry_t _ndn_ip4_header_ne_array [] = {
    { "version",         &ndn_data_unit_int4_s,
      { PRIVATE, NDN_TAG_IP4_VERSION } },
    { "h-length",        &ndn_data_unit_int4_s,
      { PRIVATE, NDN_TAG_IP4_HLEN } },
    { "type-of-service", &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP4_TOS } },
    { "total-length",    &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_IP4_LEN } },
    { "ip-ident",        &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_IP4_IDENT } },
    { "flag-reserved",   &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_IP4_FLAG_RESERVED } },
    { "dont-frag",       &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_IP4_DONT_FRAG } },
    { "more-frags",      &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_IP4_MORE_FRAGS } },
    { "frag-offset",     &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_IP4_FRAG_OFFSET } },
    { "time-to-live",    &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP4_TTL } },
    { "protocol",        &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP4_PROTOCOL } },
    { "h-checksum",      &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_IP4_H_CHECKSUM } },
    { "src-addr",        &ndn_data_unit_ip_address_s,
      { PRIVATE, NDN_TAG_IP4_SRC_ADDR } },
    { "dst-addr",        &ndn_data_unit_ip_address_s,
      { PRIVATE, NDN_TAG_IP4_DST_ADDR } },

    { "options",         &ndn_data_unit_octet_string_s,
      { PRIVATE, NDN_TAG_IP4_OPTIONS } } ,

    { "fragment-spec",   &ndn_ip4_frag_seq_s,
      { PRIVATE, NDN_TAG_IP4_FRAGMENTS } },

    { "pld-checksum",    &ndn_ip4_pld_chksm_s,
      { PRIVATE, NDN_TAG_IP4_PLD_CHECKSUM } },
};

asn_type ndn_ip4_header_s = {
    "IPv4-Header", {PRIVATE, 100}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip4_header_ne_array),
    {_ndn_ip4_header_ne_array}
};

const asn_type * const ndn_ip4_header = &ndn_ip4_header_s;



static asn_named_entry_t _ndn_ip4_csap_ne_array [] = {
    { "type-of-service", &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_IP4_TOS} },
    { "time-to-live",    &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_IP4_TTL} },
    { "protocol",        &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_IP4_PROTOCOL} },
    { "local-addr",      &ndn_data_unit_ip_address_s,
        {PRIVATE, NDN_TAG_IP4_LOCAL_ADDR} },
    { "remote-addr",     &ndn_data_unit_ip_address_s,
        {PRIVATE, NDN_TAG_IP4_REMOTE_ADDR} },
    { "max-packet-size", &ndn_data_unit_int32_s,
        {PRIVATE, NDN_TAG_IP4_MTU} },
    { "ifname", &ndn_data_unit_char_string_s,
        {PRIVATE, NDN_TAG_IP4_IFNAME} },
    { "remote-hwaddr", &ndn_data_unit_eth_address_s,
      { PRIVATE, NDN_TAG_ETH_REMOTE } },
};

asn_type ndn_ip4_csap_s = {
    "IPv4-CSAP", {PRIVATE, 101}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip4_csap_ne_array),
    {_ndn_ip4_csap_ne_array}
};

const asn_type * const ndn_ip4_csap = &ndn_ip4_csap_s;




/*
 * IPv6
 */
static asn_named_entry_t _ndn_ip6_pld_chksm_ne_array [] = {
    { "offset", &asn_base_integer_s,
      { PRIVATE, NDN_TAG_IP6_PLD_CH_OFFSET } },
    { "disable", &asn_base_null_s,
      { PRIVATE, NDN_TAG_IP6_PLD_CH_DISABLE } },
    { "diff", &asn_base_integer_s,
      { PRIVATE, NDN_TAG_IP6_PLD_CH_DIFF } },
};

asn_type ndn_ip6_pld_chksm_s = {
    "IP6-Payload-Checksum", {PRIVATE, NDN_TAG_IP6_PLD_CHECKSUM}, CHOICE,
    TE_ARRAY_LEN(_ndn_ip6_pld_chksm_ne_array),
    {_ndn_ip6_pld_chksm_ne_array}
};


/**
 * Type-Length-Value (TLV) encoded "options" (see RFC 2460, section 4.2).
 */
static asn_named_entry_t _ndn_ip6_ext_header_option_tlv_ne_array [] = {
    { "type",   &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_TYPE } },
    { "length", &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_LEN } },
    { "data",   &ndn_data_unit_octet_string_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_DATA } } ,
};

asn_type ndn_ip6_ext_header_option_tlv_s = {
    "IP6-Extention-Header-Option-TLV",
    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_TLV}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip6_ext_header_option_tlv_ne_array),
    {_ndn_ip6_ext_header_option_tlv_ne_array}
};

/**
 * Router Alert Option definition (see RFC 2711).
 */
static asn_named_entry_t _ndn_ip6_ext_header_option_ra_ne_array [] = {
    { "type",   &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_TYPE } },
    { "length", &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_LEN } },
    { "value",   &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_VALUE } } ,
};

asn_type ndn_ip6_ext_header_option_router_alert_s = {
    "IP6-Extention-Header-Option-Router-Alert",
    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_ROUTER_ALERT}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip6_ext_header_option_ra_ne_array),
    {_ndn_ip6_ext_header_option_ra_ne_array}
};

/**
 * An array of possible option types carried in Hop-by-Hop and
 * Destination extention headers (see RFC 2460, section 4.2).
 */
static asn_named_entry_t _ndn_ip6_ext_header_option_ne_array [] = {
    { "pad1", &asn_base_null_s,
      {PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_PAD1} },
    { "tlv",  &ndn_ip6_ext_header_option_tlv_s,
      {PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_TLV} },
    { "router-alert",  &ndn_ip6_ext_header_option_router_alert_s,
      {PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPT_ROUTER_ALERT} },
};

asn_type ndn_ip6_ext_header_option_s = {
    "IPv6-Extention-Header-Option",
    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPTIONS}, CHOICE,
    TE_ARRAY_LEN(_ndn_ip6_ext_header_option_ne_array),
    {_ndn_ip6_ext_header_option_ne_array}
};

const asn_type * const ndn_ip6_ext_header_option =
                                        &ndn_ip6_ext_header_option_s;

asn_type ndn_ip6_ext_header_options_seq_s = {
    "SEQUENCE OF IPv6-Extention-Header-Option",
    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPTIONS}, SEQUENCE_OF, 0,
    {subtype: &ndn_ip6_ext_header_option_s}
};

const asn_type * const ndn_ip6_ext_header_options_seq =
                                        &ndn_ip6_ext_header_options_seq_s;

/**
 * Sequence of fields for Options Header:
 * Hop-by-Hop and Destination headers.
 *
 * IP6-Extention-Header-[Hop-by-Hop|Destination] ::= SEQUENCE {
 *     next-header [0] DATA-UNIT{INTEGER (0..255) }
 *     length      [1] DATA-UNIT{INTEGER (0..255) } OPTIONAL,
 *     options     [3] SEQUENCE OF IPv6-Extention-Header-Option OPTIONAL,
 * }
 */
static asn_named_entry_t _ndn_ip6_ext_header_options_hdr_ne_array [] = {
    { "next-header",  &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_NEXT_HEADER } },
    { "length",       &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_LEN } },
    { "options", &ndn_ip6_ext_header_options_seq_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_OPTIONS } },
};

asn_type ndn_ip6_ext_header_hop_by_hop_s = {
    "IP6-Extention-Header-Hop-by-Hop",
    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_HOP_BY_HOP}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip6_ext_header_options_hdr_ne_array),
    {_ndn_ip6_ext_header_options_hdr_ne_array}
};
const asn_type * const ndn_ip6_ext_header_hop_by_hop =
                                &ndn_ip6_ext_header_hop_by_hop_s;

asn_type ndn_ip6_ext_header_destination_s = {
    "IP6-Extention-Header-Destination",
    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_DESTINATION}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip6_ext_header_options_hdr_ne_array),
    {_ndn_ip6_ext_header_options_hdr_ne_array}
};
const asn_type * const ndn_ip6_ext_header_destination =
                                &ndn_ip6_ext_header_destination_s;

static asn_named_entry_t _ndn_ip6_ext_header_ne_array [] = {
    { "hop-by-hop", &ndn_ip6_ext_header_hop_by_hop_s,
                    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_HOP_BY_HOP} },
    { "destination", &ndn_ip6_ext_header_destination_s,
                    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_DESTINATION} },
};

asn_type ndn_ip6_ext_header_s = {
    "IPv6-Extention-Header", {PRIVATE, NDN_TAG_IP6_EXT_HEADERS}, CHOICE,
    TE_ARRAY_LEN(_ndn_ip6_ext_header_ne_array),
    {_ndn_ip6_ext_header_ne_array}
};

const asn_type * const ndn_ip6_ext_header = &ndn_ip6_ext_header_s;

asn_type ndn_ip6_ext_headers_seq_s = {
    "SEQUENCE OF IPv6-Extention-Header",
    {PRIVATE, NDN_TAG_IP6_EXT_HEADERS}, SEQUENCE_OF, 0,
    {subtype: &ndn_ip6_ext_header_s}
};

const asn_type * const ndn_ip6_ext_headers_seq = &ndn_ip6_ext_headers_seq_s;

static asn_named_entry_t _ndn_ip6_header_ne_array [] = {
    { "version",         &ndn_data_unit_int4_s,
      { PRIVATE, NDN_TAG_IP6_VERSION } },
    { "traffic-class",   &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_TCL } },
    { "flow-label",      &ndn_data_unit_int32_s,
      { PRIVATE, NDN_TAG_IP6_FLAB } },
    { "payload-length",  &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_IP6_LEN } },
    { "next-header",     &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_NEXT_HEADER } },
    { "hop-limit",       &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_HLIM } },
    { "src-addr",        &ndn_data_unit_ip6_address_s,
      { PRIVATE, NDN_TAG_IP6_SRC_ADDR } },
    { "dst-addr",        &ndn_data_unit_ip6_address_s,
      { PRIVATE, NDN_TAG_IP6_DST_ADDR } },

    { "ext-headers",     &ndn_ip6_ext_headers_seq_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADERS } } ,

    { "pld-checksum",    &ndn_ip6_pld_chksm_s,
      { PRIVATE, NDN_TAG_IP6_PLD_CHECKSUM } },
};

asn_type ndn_ip6_header_s = {
    "IPv6-Header", {PRIVATE, 100}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip6_header_ne_array),
    {_ndn_ip6_header_ne_array}
};

const asn_type * const ndn_ip6_header = &ndn_ip6_header_s;

static asn_named_entry_t _ndn_ip6_csap_ne_array [] = {
    { "next-header", &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_IP6_NEXT_HEADER} },
    { "local-addr",      &ndn_data_unit_ip6_address_s,
        {PRIVATE, NDN_TAG_IP6_LOCAL_ADDR} },
    { "remote-addr",     &ndn_data_unit_ip6_address_s,
        {PRIVATE, NDN_TAG_IP6_REMOTE_ADDR} },
};

asn_type ndn_ip6_csap_s = {
    "IPv6-CSAP", {PRIVATE, 101}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip6_csap_ne_array),
    {_ndn_ip6_csap_ne_array}
};

const asn_type * const ndn_ip6_csap = &ndn_ip6_csap_s;



/*
 * ICMPv4
 */

static asn_named_entry_t _ndn_icmp4_message_ne_array [] = {
    { "type",           &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_ICMP4_TYPE } },
    { "code",           &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_ICMP4_CODE } },
    { "checksum",       &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ICMP4_CHECKSUM } },

    /* In general, the following fields should be structured */
    { "unused",         &ndn_data_unit_int32_s,
      { PRIVATE, NDN_TAG_ICMP4_UNUSED } },
    { "ptr",            &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_ICMP4_PP_PTR } },
    { "id",             &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ICMP4_ID } },
    { "seq",            &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ICMP4_SEQ } },
    { "redirect-gw",    &ndn_data_unit_int32_s,
      { PRIVATE, NDN_TAG_ICMP4_REDIRECT_GW } },
    { "orig-ts",        &ndn_data_unit_int32_s,
      { PRIVATE, NDN_TAG_ICMP4_ORIG_TS } },
    { "rx-ts",          &ndn_data_unit_int32_s,
      { PRIVATE, NDN_TAG_ICMP4_RX_TS } },
    { "tx-ts",          &ndn_data_unit_int32_s,
      { PRIVATE, NDN_TAG_ICMP4_TX_TS } },
};

asn_type ndn_icmp4_message_s = {
    "ICMPv4-Message", {PRIVATE, 100}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp4_message_ne_array),
    {_ndn_icmp4_message_ne_array}
};

const asn_type * const ndn_icmp4_message = &ndn_icmp4_message_s;

const asn_type * const ndn_icmp4_csap = &asn_base_null_s;



/*
 * ICMPv6
 */

static asn_named_entry_t _ndn_icmp6_message_ne_array [] = {
    { "type",           &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_ICMP6_TYPE } },
    { "code",           &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_ICMP6_CODE } },
    { "checksum",       &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ICMP6_CHECKSUM } },

    { "id",             &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ICMP6_ID } },
    { "seq",            &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ICMP6_SEQ } },

    { "max-response-delay", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ICMP6_MLD_MAX_RESPONSE_DELAY } },
    { "reserved",           &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ICMP6_MLD_RESERVED } },
    { "group-addr",        &ndn_data_unit_ip6_address_s,
      { PRIVATE, NDN_TAG_ICMP6_MLD_GROUP_ADDR } },
};

asn_type ndn_icmp6_message_s = {
    "ICMPv6-Message", {PRIVATE, 100}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_message_ne_array),
    {_ndn_icmp6_message_ne_array}
};

const asn_type * const ndn_icmp6_message = &ndn_icmp6_message_s;

const asn_type * const ndn_icmp6_csap = &asn_base_null_s;



/*
 * UDP
 */

static asn_named_entry_t _ndn_udp_header_ne_array [] = {
    { "src-port", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_UDP_SRC_PORT} },
    { "dst-port", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_UDP_DST_PORT} },
    { "length",   &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_UDP_LENGTH} },
    { "checksum", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_UDP_CHECKSUM} },
};

asn_type ndn_udp_header_s = {
    "UDP-Header", {PRIVATE, 100}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_udp_header_ne_array),
    {_ndn_udp_header_ne_array}
};

const asn_type * const ndn_udp_header = &ndn_udp_header_s;


static asn_named_entry_t _ndn_udp_csap_ne_array [] = {
    { "local-port",  &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_UDP_LOCAL_PORT } },
    { "remote-port", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_UDP_REMOTE_PORT } },
};

asn_type ndn_udp_csap_s = {
    "UDP-CSAP", {PRIVATE, 101}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_udp_csap_ne_array),
    {_ndn_udp_csap_ne_array}
};

const asn_type * const ndn_udp_csap = &ndn_udp_csap_s;




/*
 * TCP
 */

/*
TCP-Option-MSS ::= SEQUENCE {
    length      [0] DATA-UNIT{INTEGER (0..255) } OPTIONAL,
    mss         [1] DATA-UNIT{INTEGER (0..65535) }
}
*/
static asn_named_entry_t _ndn_tcp_opt_mss_ne_array [] = {
    { "length", &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_TCP_OPT_LEN} },
    { "mss", &ndn_data_unit_int16_s,
            {PRIVATE, NDN_TAG_TCP_OPT_MSS} },
};
asn_type ndn_tcp_opt_mss_s = {
    "TCP-Option-MSS", {PRIVATE, NDN_TAG_TCP_OPT_MSS}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_tcp_opt_mss_ne_array),
    {_ndn_tcp_opt_mss_ne_array}
};


/*
TCP-Option-WindScale ::= SEQUENCE {
    length      [0] DATA-UNIT{INTEGER (0..255) } OPTIONAL,
    scale       [1] DATA-UNIT{INTEGER (0..255) }
}
*/
static asn_named_entry_t _ndn_tcp_opt_win_scale_ne_array [] = {
    { "length", &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_TCP_OPT_LEN} },
    { "scale", &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_TCP_OPT_MSS} },
};

asn_type ndn_tcp_opt_win_scale_s = {
    "TCP-Option-WindScale", {PRIVATE, NDN_TAG_TCP_OPT_WIN_SCALE}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_tcp_opt_win_scale_ne_array),
    {_ndn_tcp_opt_win_scale_ne_array}
};


/*
TCP-Option-SackPerm ::= SEQUENCE {
    length      [0] DATA-UNIT{INTEGER (0..255) } OPTIONAL,
}
*/
static asn_named_entry_t _ndn_tcp_opt_sack_perm_ne_array [] = {
    { "length", &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_TCP_OPT_LEN} },
};

asn_type ndn_tcp_opt_sack_perm_s = {
    "TCP-Option-SackPerm", {PRIVATE, NDN_TAG_TCP_OPT_SACK_PERM},
    SEQUENCE,
    TE_ARRAY_LEN(_ndn_tcp_opt_sack_perm_ne_array),
    {_ndn_tcp_opt_sack_perm_ne_array}
};

/*
TCP-Option-SackBlock ::= SEQUENCE {
    left      [0] DATA-UNIT{INTEGER },
    right      [0] DATA-UNIT{INTEGER },
}
*/
static asn_named_entry_t _ndn_tcp_opt_sackblock_ne_array [] = {
    { "left", &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_TCP_OPT_SACK_LEFT} },
    { "right", &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_TCP_OPT_SACK_RIGHT} },
};
asn_type ndn_tcp_opt_sackblock_s = {
    "TCP-Option-SackBlock", {PRIVATE, NDN_TAG_TCP_OPT_SACK_BLOCK},
    SEQUENCE,
    TE_ARRAY_LEN(_ndn_tcp_opt_sackblock_ne_array),
    {_ndn_tcp_opt_sackblock_ne_array}
};

asn_type ndn_tcp_opt_sackblocks_seq_s = {
    "SEQUENCE OF TCP-Option-SackBlock",
    {PRIVATE, NDN_TAG_TCP_OPT_SACK_BLOCKS},
    SEQUENCE_OF, 0,
    {subtype: &ndn_tcp_opt_sackblock_s}
};

/*
TCP-Option-SackData ::= SEQUENCE {
    length      [0] DATA-UNIT{INTEGER (0..255) } OPTIONAL,
    blocks      [1] SEQUENCE OF TCP-Option-SackBlock,
}
*/
static asn_named_entry_t _ndn_tcp_opt_sack_data_ne_array [] = {
    { "length", &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_TCP_OPT_LEN} },
    { "blocks", &ndn_tcp_opt_sackblocks_seq_s,
            {PRIVATE, NDN_TAG_TCP_OPT_SACK_BLOCKS} },
};

asn_type ndn_tcp_opt_sack_data_s = {
    "TCP-Option-SackData", {PRIVATE, NDN_TAG_TCP_OPT_SACK_DATA}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_tcp_opt_sack_data_ne_array),
    {_ndn_tcp_opt_sack_data_ne_array}
};

/*
TCP-Option-Timestamp ::= SEQUENCE {
    length      [0] DATA-UNIT{INTEGER (0..255) } OPTIONAL,
    value       [1] DATA-UNIT{INTEGER }
    echo-reply  [2] DATA-UNIT{INTEGER }
}
*/
static asn_named_entry_t _ndn_tcp_opt_timestamp_ne_array [] = {
    { "length", &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_TCP_OPT_LEN} },
    { "value", &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_TCP_OPT_VALUE} },
    { "echo-reply", &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_TCP_OPT_ECHO_REPLY} },
};

asn_type ndn_tcp_opt_timestamp_s = {
    "TCP-Option-Timestamp", {PRIVATE, NDN_TAG_TCP_OPT_TIMESTAMP}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_tcp_opt_timestamp_ne_array),
    {_ndn_tcp_opt_timestamp_ne_array}
};

/*
TCP-Option ::= CHOICE {
    eol         [0] NULL,
    nop         [1] NULL,
    mss         [2] TCP-Option-MSS,
    win-scale   [3] TCP-Option-WindScale,
    sack-perm   [4] TCP-Option-SackPerm,
    sack-data   [5] TCP-Option-SackData,
    timestamp   [8] TCP-Option-Timestamp,
}
*/
static asn_named_entry_t _ndn_tcp_option_ne_array [] = {
    { "eol",       &asn_base_null_s, {PRIVATE, NDN_TAG_TCP_OPT_EOL} },
    { "nop",       &asn_base_null_s, {PRIVATE, NDN_TAG_TCP_OPT_NOP} },
    { "mss",       &ndn_tcp_opt_mss_s, {PRIVATE, NDN_TAG_TCP_OPT_MSS} },
    { "win-scale", &ndn_tcp_opt_win_scale_s,
                    {PRIVATE, NDN_TAG_TCP_OPT_WIN_SCALE} },
    { "sack-perm", &ndn_tcp_opt_sack_perm_s,
                    {PRIVATE, NDN_TAG_TCP_OPT_SACK_PERM} },
    { "sack-data", &ndn_tcp_opt_sack_data_s,
                    {PRIVATE, NDN_TAG_TCP_OPT_SACK_DATA} },
    { "timestamp", &ndn_tcp_opt_timestamp_s,
                    {PRIVATE, NDN_TAG_TCP_OPT_TIMESTAMP} },
};

asn_type ndn_tcp_option_s = {
    "TCP-Option", {PRIVATE, NDN_TAG_TCP_OPTIONS}, CHOICE,
    TE_ARRAY_LEN(_ndn_tcp_option_ne_array),
    {_ndn_tcp_option_ne_array}
};

const asn_type * const ndn_tcp_option = &ndn_tcp_option_s;

asn_type ndn_tcp_options_seq_s = {
    "SEQUENCE OF TCP-Option",
    {PRIVATE, NDN_TAG_TCP_OPTIONS},
    SEQUENCE_OF, 0,
    {subtype: &ndn_tcp_option_s}
};

const asn_type * const ndn_tcp_options_seq = &ndn_tcp_options_seq_s;

/*
TCP-Header ::= SEQUENCE {
    src-port  [0] DATA-UNIT{INTEGER (0..65535)} OPTIONAL,
    dst-port  [1] DATA-UNIT{INTEGER (0..65535)} OPTIONAL,
    seqn      [2] DATA-UNIT{INTEGER } OPTIONAL,
    acqn      [3] DATA-UNIT{INTEGER } OPTIONAL,
    hlen      [4] DATA-UNIT{INTEGER (0..15)} OPTIONAL,
    flags     [5] DATA-UNIT{INTEGER (0..63)} OPTIONAL,
    win-size  [6] DATA-UNIT{INTEGER (0..65535)} OPTIONAL,
    checksum  [7] DATA-UNIT{INTEGER (0..65535)} OPTIONAL,
    urg-p     [8] DATA-UNIT{INTEGER (0..65535)} OPTIONAL,
    options   [9] SEQUENCE OF TCP-Option OPTIONAL,
    socket   [10]   INTEGER OPTIONAL,
    length   [11]   INTEGER OPTIONAL,
    ...
}
*/

static asn_named_entry_t _ndn_tcp_header_ne_array [] = {
    { "src-port", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_TCP_SRC_PORT} },
    { "dst-port", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_TCP_DST_PORT} },
    { "seqn",     &ndn_data_unit_int32_s, {PRIVATE, NDN_TAG_TCP_SEQN} },
    { "ackn",     &ndn_data_unit_int32_s, {PRIVATE, NDN_TAG_TCP_ACKN} },
    { "hlen",     &ndn_data_unit_int8_s,  {PRIVATE, NDN_TAG_TCP_HLEN} },
    { "flags",    &ndn_data_unit_int8_s,  {PRIVATE, NDN_TAG_TCP_FLAGS} },
    { "win-size", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_TCP_WINDOW} },
    { "checksum", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_TCP_CHECKSUM} },
    { "urg-p",    &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_TCP_URG} },
    { "options",  &ndn_tcp_options_seq_s, {PRIVATE, NDN_TAG_TCP_OPTIONS} },
    { "socket",   &asn_base_integer_s, {PRIVATE, NDN_TAG_TCP_DATA_SOCKET} },
    { "length",   &asn_base_integer_s, {PRIVATE, NDN_TAG_TCP_DATA_LENGTH} },
};

asn_type ndn_tcp_header_s = {
    "TCP-Header", {PRIVATE, 101}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_tcp_header_ne_array),
    {_ndn_tcp_header_ne_array}
};

const asn_type * const ndn_tcp_header = &ndn_tcp_header_s;




static asn_named_entry_t _ndn_tcp_data_ne_array [] = {
    { "server", &asn_base_null_s,
        {PRIVATE, NDN_TAG_TCP_DATA_SERVER} },
    { "client", &asn_base_null_s,
        {PRIVATE, NDN_TAG_TCP_DATA_CLIENT} },
    { "socket", &asn_base_integer_s,
        {PRIVATE, NDN_TAG_TCP_DATA_SOCKET} },
};

asn_type ndn_tcp_data_s = {
    "TCP-CSAP", {PRIVATE, NDN_TAG_TCP_DATA}, CHOICE,
    TE_ARRAY_LEN(_ndn_tcp_data_ne_array),
    {_ndn_tcp_data_ne_array}
};


static asn_named_entry_t _ndn_tcp_csap_ne_array [] = {
    { "local-port", &ndn_data_unit_int16_s,
        {PRIVATE, NDN_TAG_TCP_LOCAL_PORT} },
    { "remote-port",&ndn_data_unit_int16_s,
        {PRIVATE, NDN_TAG_TCP_REMOTE_PORT} },
    { "data",&ndn_tcp_data_s,
        {PRIVATE, NDN_TAG_TCP_DATA} },
};

asn_type ndn_tcp_csap_s = {
    "TCP-CSAP", {PRIVATE, 102}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_tcp_csap_ne_array),
    {_ndn_tcp_csap_ne_array}
};

const asn_type * const ndn_tcp_csap = &ndn_tcp_csap_s;
