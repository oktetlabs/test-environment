/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD IP stack protocols, NDN.
 *
 * Definitions of ASN.1 types for NDN for file protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
    { "id",                &asn_base_uint32_s,
        {PRIVATE, NDN_TAG_IP4_FR_ID} },
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

/** Fields of IPv6 Fragment extension header */
static asn_named_entry_t _ndn_ip6_ext_header_fragment_hdr_ne_array [] = {
    { "next-header",  &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_NEXT_HEADER } },
    { "res1",  &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_FRAGMENT_RES1 } },
    { "offset",  &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_FRAGMENT_OFFSET } },
    { "res2",  &ndn_data_unit_int2_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_FRAGMENT_RES2 } },
    { "m-flag",  &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_FRAGMENT_M_FLAG } },
    { "id",  &ndn_data_unit_uint32_s,
      { PRIVATE, NDN_TAG_IP6_EXT_HEADER_FRAGMENT_ID } },
};

/** IPv6 Fragment extension header */
asn_type ndn_ip6_ext_header_fragment_s = {
    "IP6-Extension-Header-Fragment",
    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_FRAGMENT}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip6_ext_header_fragment_hdr_ne_array),
    {_ndn_ip6_ext_header_fragment_hdr_ne_array}
};

const asn_type * const ndn_ip6_ext_header_fragment =
                                &ndn_ip6_ext_header_fragment_s;

static asn_named_entry_t _ndn_ip6_ext_header_ne_array [] = {
    { "hop-by-hop", &ndn_ip6_ext_header_hop_by_hop_s,
                    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_HOP_BY_HOP} },
    { "destination", &ndn_ip6_ext_header_destination_s,
                    {PRIVATE, NDN_TAG_IP6_EXT_HEADER_DESTINATION} },
    { "fragment", &ndn_ip6_ext_header_fragment_s,
                  {PRIVATE, NDN_TAG_IP6_EXT_HEADER_FRAGMENT} },
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

/** Fields of IPv6 Fragment specification in packet template */
static asn_named_entry_t _ndn_ip6_frag_spec_ne_array [] = {
    { "hdr-offset", &asn_base_integer_s,
      {PRIVATE, NDN_TAG_IP6_FR_HO} },
    { "real-offset", &asn_base_integer_s,
      {PRIVATE, NDN_TAG_IP6_FR_RO} },
    { "hdr-length", &asn_base_integer_s,
      {PRIVATE, NDN_TAG_IP6_FR_HL} },
    { "real-length", &asn_base_integer_s,
      {PRIVATE, NDN_TAG_IP6_FR_RL} },
    { "more-frags", &asn_base_boolean_s,
      {PRIVATE, NDN_TAG_IP6_FR_MF} },
    { "id", &asn_base_uint32_s,
      {PRIVATE, NDN_TAG_IP6_FR_ID} },
};

/** IPv6 Fragment specification in packet template */
asn_type ndn_ip6_frag_spec_s = {
    "IPv6-Fragment", {PRIVATE, NDN_TAG_IP6_FRAGMENTS}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ip6_frag_spec_ne_array),
    {_ndn_ip6_frag_spec_ne_array}
};

const asn_type * const ndn_ip6_frag_spec = &ndn_ip6_frag_spec_s;

/** Specification of IPv6 fragmentation in packet template */
asn_type ndn_ip6_frag_seq_s = {
    "IPv6-Fragments", {PRIVATE, NDN_TAG_IP6_FRAGMENTS}, SEQUENCE_OF, 0,
    {subtype: &ndn_ip6_frag_spec_s}
};

const asn_type * const ndn_ip6_frag_seq = &ndn_ip6_frag_seq_s;

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

    { "fragment-spec",   &ndn_ip6_frag_seq_s,
      { PRIVATE, NDN_TAG_IP6_FRAGMENTS } },

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
 * ICMP6
 *
 * ICMPv6 messages.
 * Message layout: SEQUENCE:
 *
 * type                     - ndn_data_unit_int8_s,
 * code                     - ndn_data_unit_int8_s,
 * checksum                 - ndn_data_unit_int16_s,
 * body                     - CHOICE, see below,
 * options                  - SEQUENCE OF, see below
 *
 *
 * Field 'body'.
 *
 * Layout depends on the type of ICMPv6 message.
 *
 * Supported variants are:
 *
 * 1) 'Router solicitation' message
 */
static asn_named_entry_t _ndn_icmp6_router_sol_ne_array [] = {
    {"reserved",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_ROUTER_SOL_RESERVED}},
};

asn_type ndn_icmp6_router_sol_s = {
    "ICMPv6-Router-Solicitation-Message",
    {PRIVATE, NDN_TAG_ICMP6_ROUTER_SOL}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_router_sol_ne_array),
    {_ndn_icmp6_router_sol_ne_array}
};

const asn_type * const ndn_icmp6_router_sol = &ndn_icmp6_router_sol_s;

/*
 * 2) 'Router advertisement' message
 */
static asn_named_entry_t _ndn_icmp6_router_adv_ne_array [] = {
    {"cur-hop-limit",
        &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_ICMP6_ROUTER_ADV_CUR_HOP_LIMIT}},
    {"flags",
        &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_ICMP6_ROUTER_ADV_FLAGS}},
    {"lifetime",
        &ndn_data_unit_int16_s,
            {PRIVATE, NDN_TAG_ICMP6_ROUTER_ADV_LIFETIME} },
    {"reachable-time",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_ROUTER_ADV_REACHABLE_TIME} },
    {"retrans-timer",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_ROUTER_ADV_RETRANS_TIMER} },
};

asn_type ndn_icmp6_router_adv_s = {
    "ICMPv6-Router-Advertisement-Message",
    {PRIVATE, NDN_TAG_ICMP6_ROUTER_ADV}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_router_adv_ne_array),
    {_ndn_icmp6_router_adv_ne_array}
};

const asn_type * const ndn_icmp6_router_adv = &ndn_icmp6_router_adv_s;

/*
 * 3) 'Neighbor solicitation' message
 */
static asn_named_entry_t _ndn_icmp6_neighbor_sol_ne_array [] = {
    {"reserved",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_NEIGHBOR_SOL_RESERVED}},
    {"target-addr",
        &ndn_data_unit_ip6_address_s,
            {PRIVATE, NDN_TAG_ICMP6_NEIGHBOR_SOL_TARGET_ADDR}},
};

asn_type ndn_icmp6_neighbor_sol_s = {
    "ICMPv6-Neighbor-Solicitation-Message",
    {PRIVATE, NDN_TAG_ICMP6_NEIGHBOR_SOL}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_neighbor_sol_ne_array),
    {_ndn_icmp6_neighbor_sol_ne_array}
};

const asn_type * const ndn_icmp6_neighbor_sol = &ndn_icmp6_neighbor_sol_s;

/*
 * 4) 'Neighbor advertisement' message
 */
static asn_named_entry_t _ndn_icmp6_neighbor_adv_ne_array [] = {
    {"flags",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_NEIGHBOR_ADV_FLAGS}},
    {"target-addr",
        &ndn_data_unit_ip6_address_s,
            {PRIVATE, NDN_TAG_ICMP6_NEIGHBOR_ADV_TARGET_ADDR}},
};

asn_type ndn_icmp6_neighbor_adv_s = {
    "ICMPv6-Neighbor-Advertisement-Message",
    {PRIVATE, NDN_TAG_ICMP6_NEIGHBOR_ADV}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_neighbor_adv_ne_array),
    {_ndn_icmp6_neighbor_adv_ne_array}
};

const asn_type * const ndn_icmp6_neighbor_adv = &ndn_icmp6_neighbor_adv_s;

/*
 * 5) 'Echo request' and 'Echo reply' messages
 */
static asn_named_entry_t _ndn_icmp6_echo_ne_array [] = {
    {"id",
        &ndn_data_unit_int16_s,
            {PRIVATE, NDN_TAG_ICMP6_ECHO_ID}},
    {"seq",
        &ndn_data_unit_int16_s,
            {PRIVATE, NDN_TAG_ICMP6_ECHO_SEQ}},
};

asn_type ndn_icmp6_echo_s = {
    "ICMPv6-Echo-Message",
    {PRIVATE, NDN_TAG_ICMP6_ECHO}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_echo_ne_array),
    {_ndn_icmp6_echo_ne_array}
};

const asn_type * const ndn_icmp6_echo = &ndn_icmp6_echo_s;

/*
 * 6) 'MLD query' 'MLD report' and 'MLD done' messages
 */
static asn_named_entry_t _ndn_icmp6_mld_ne_array[] = {
    {"max-response-delay",
        &ndn_data_unit_int16_s,
            {PRIVATE, NDN_TAG_ICMP6_MLD_MAX_RESPONSE_DELAY}},
    {"reserved",
        &ndn_data_unit_int16_s,
            {PRIVATE, NDN_TAG_ICMP6_MLD_RESERVED}},
    {"group-addr",
        &ndn_data_unit_ip6_address_s,
            {PRIVATE, NDN_TAG_ICMP6_MLD_GROUP_ADDR}},
};

asn_type ndn_icmp6_mld_s = {
    "ICMPv6-MLD-Message",
    {PRIVATE, NDN_TAG_ICMP6_MLD}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_mld_ne_array),
    {_ndn_icmp6_mld_ne_array}
};

const asn_type * const ndn_icmp6_mld = &ndn_icmp6_mld_s;

/*
 * 7) 'Destination unreachable' message
 */
static asn_named_entry_t _ndn_icmp6_dest_unreach_ne_array [] = {
    {"unused",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_DEST_UNREACH_UNUSED}},
};

asn_type ndn_icmp6_dest_unreach_s = {
    "ICMPv6-Destination-Unreachable-Message",
    {PRIVATE, NDN_TAG_ICMP6_DEST_UNREACH}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_dest_unreach_ne_array),
    {_ndn_icmp6_dest_unreach_ne_array}
};

const asn_type * const ndn_icmp6_dest_unreach = &ndn_icmp6_dest_unreach_s;

/*
 * 8) 'Packet too big' message
 */
static asn_named_entry_t _ndn_icmp6_packet_too_big_ne_array [] = {
    {"mtu",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_PACKET_TOO_BIG_MTU}},
};

asn_type ndn_icmp6_packet_too_big_s = {
    "ICMPv6-Packet-Too-Big-Message",
    {PRIVATE, NDN_TAG_ICMP6_PACKET_TOO_BIG}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_packet_too_big_ne_array),
    {_ndn_icmp6_packet_too_big_ne_array}
};

const asn_type * const ndn_icmp6_packet_too_big = &ndn_icmp6_packet_too_big_s;

/*
 * 9) 'Time exceeded' message
 */
static asn_named_entry_t _ndn_icmp6_time_exceeded_ne_array [] = {
    {"unused",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_TIME_EXCEEDED_UNUSED}},
};

asn_type ndn_icmp6_time_exceeded_s = {
    "ICMPv6-Time-Exceeded-Message",
    {PRIVATE, NDN_TAG_ICMP6_TIME_EXCEEDED}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_time_exceeded_ne_array),
    {_ndn_icmp6_time_exceeded_ne_array}
};

const asn_type * const ndn_icmp6_time_exceeded = &ndn_icmp6_time_exceeded_s;

/*
 * 10) 'Parameter problem' message
 */
static asn_named_entry_t _ndn_icmp6_param_prob_ne_array [] = {
    {"pointer",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_PARAM_PROB_PTR}},
};

asn_type ndn_icmp6_param_prob_s = {
    "ICMPv6-Parameter-Problem-Message",
    {PRIVATE, NDN_TAG_ICMP6_PARAM_PROB}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_param_prob_ne_array),
    {_ndn_icmp6_param_prob_ne_array}
};

const asn_type * const ndn_icmp6_param_prob = &ndn_icmp6_param_prob_s;

/*
 * Message body
 * CHOICE: supported variants
 */
static asn_named_entry_t _ndn_icmp6_body_ne_array [] = {
    {"dest-unreach",
        &ndn_icmp6_dest_unreach_s,
            {PRIVATE, NDN_TAG_ICMP6_DEST_UNREACH}},
    {"packet-too-big",
        &ndn_icmp6_packet_too_big_s,
            {PRIVATE, NDN_TAG_ICMP6_PACKET_TOO_BIG}},
    {"time-exceeded",
        &ndn_icmp6_time_exceeded_s,
            {PRIVATE, NDN_TAG_ICMP6_TIME_EXCEEDED}},
    {"param-prob",
        &ndn_icmp6_param_prob_s,
            {PRIVATE, NDN_TAG_ICMP6_PARAM_PROB}},
    {"router-sol",
        &ndn_icmp6_router_sol_s,
            {PRIVATE, NDN_TAG_ICMP6_ROUTER_SOL}},
    {"router-adv",
        &ndn_icmp6_router_adv_s,
            {PRIVATE, NDN_TAG_ICMP6_ROUTER_ADV}},
    {"neighbor-sol",
        &ndn_icmp6_neighbor_sol_s,
            {PRIVATE, NDN_TAG_ICMP6_NEIGHBOR_SOL}},
    {"neighbor-adv",
        &ndn_icmp6_neighbor_adv_s,
            {PRIVATE, NDN_TAG_ICMP6_NEIGHBOR_ADV}},
    {"echo",
        &ndn_icmp6_echo_s,
            {PRIVATE, NDN_TAG_ICMP6_ECHO}},
    {"mld",
        &ndn_icmp6_mld_s,
            {PRIVATE, NDN_TAG_ICMP6_MLD}},
};

asn_type ndn_icmp6_body_s = {
    "ICMPv6-Message-Body",
    {PRIVATE, NDN_TAG_ICMP6_BODY}, CHOICE,
    TE_ARRAY_LEN(_ndn_icmp6_body_ne_array),
    {_ndn_icmp6_body_ne_array}
};

const asn_type * const ndn_icmp6_body = &ndn_icmp6_body_s;

/*
 * Field 'options'.
 *
 * List of options in ICMPv6 message.
 *
 * Option layout depends on the type of option.
 *
 * Supported variants are:
 *
 * 1) Option 'Source link-layer address'
 */
static asn_named_entry_t _ndn_icmp6_opt_ll_addr_ne_array [] = {
    {"mac",
        &ndn_data_unit_eth_address_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_LL_ADDR_MAC}},
};

asn_type ndn_icmp6_opt_ll_addr_s = {
    "ICMPv6-Option-Source-ll-address",
    {PRIVATE, NDN_TAG_ICMP6_OPT_LL_ADDR}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_opt_ll_addr_ne_array),
    {_ndn_icmp6_opt_ll_addr_ne_array}
};

const asn_type * const ndn_icmp6_opt_ll_addr = &ndn_icmp6_opt_ll_addr_s;

/*
 * 2) Option 'Prefix information'
 */
static asn_named_entry_t _ndn_icmp6_opt_prefix_ne_array [] = {
    {"prefix-length",
        &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_PREFIX_PREFIX_LENGTH}},
    {"flags",
        &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_PREFIX_FLAGS}},
    {"valid-lifetime",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_PREFIX_VALID_LIFETIME}},
    {"preferred-lifetime",
        &ndn_data_unit_int32_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_PREFIX_PREFERRED_LIFETIME}},
    {"prefix",
        &ndn_data_unit_ip6_address_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_PREFIX_PREFIX}}
};

asn_type ndn_icmp6_opt_prefix_s = {
    "ICMPv6-Option-Prefix-Information",
    {PRIVATE, NDN_TAG_ICMP6_OPT_PREFIX}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_opt_prefix_ne_array),
    {_ndn_icmp6_opt_prefix_ne_array}
};

const asn_type * const ndn_icmp6_opt_prefix = &ndn_icmp6_opt_prefix_s;

/*
 * Option body
 * CHOICE: supported variants:
 */
static asn_named_entry_t _ndn_icmp6_opt_body_ne_array [] = {
    {"ll-addr",
        &ndn_icmp6_opt_ll_addr_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_LL_ADDR}},
    {"prefix",
        &ndn_icmp6_opt_prefix_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_PREFIX}}
};

asn_type ndn_icmp6_opt_body_s = {
    "ICMPv6-Option-Body",
    {PRIVATE, NDN_TAG_ICMP6_OPT_BODY}, CHOICE,
    TE_ARRAY_LEN(_ndn_icmp6_opt_body_ne_array),
    {_ndn_icmp6_opt_body_ne_array}
};

const asn_type * const ndn_icmp6_opt_body = &ndn_icmp6_opt_body_s;

/*
 * Option layout:
 * type, length(in 8-byte units) and body
 */
static asn_named_entry_t _ndn_icmp6_opt_ne_array [] = {
    {"type",
        &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_TYPE}},
    {"length",
        &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_LEN}},
    {"body",
        &ndn_icmp6_opt_body_s,
            {PRIVATE, NDN_TAG_ICMP6_OPT_BODY}}
};

asn_type ndn_icmp6_opt_s = {
    "ICMPv6-Option",
    {PRIVATE, NDN_TAG_ICMP6_OPT}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_icmp6_opt_ne_array),
    {_ndn_icmp6_opt_ne_array}
};

const asn_type * const ndn_icmp6_opt = &ndn_icmp6_opt_s;

/*
 * List of options in ICMPv6 message
 */
asn_type ndn_icmp6_opts_s = {
    "SEQUENCE OF ICMPv6-Options",
    {PRIVATE, NDN_TAG_ICMP6_OPTS}, SEQUENCE_OF, 0,
    {subtype: &ndn_icmp6_opt_s}
};

const asn_type * const ndn_icmp6_opts = &ndn_icmp6_opts_s;

/*
 * ICMPv6 message fields: type, code, checksum, body
 * and list of options
 */
static asn_named_entry_t _ndn_icmp6_message_ne_array [] = {
    {"type",
        &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_ICMP6_TYPE}},
    {"code",
        &ndn_data_unit_int8_s,
            {PRIVATE, NDN_TAG_ICMP6_CODE}},
    {"checksum",
        &ndn_data_unit_int16_s,
            {PRIVATE, NDN_TAG_ICMP6_CHECKSUM}},
    {"body",
        &ndn_icmp6_body_s,
            {PRIVATE, NDN_TAG_ICMP6_BODY}},
    {"options",
        &ndn_icmp6_opts_s,
            {PRIVATE, NDN_TAG_ICMP6_OPTS}},
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
    flags     [5] DATA-UNIT{INTEGER (0..255)} OPTIONAL,
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
    { "src-port", &ndn_data_unit_int16_s,  {PRIVATE, NDN_TAG_TCP_SRC_PORT   } },
    { "dst-port", &ndn_data_unit_int16_s,  {PRIVATE, NDN_TAG_TCP_DST_PORT   } },
    { "seqn",     &ndn_data_unit_uint32_s, {PRIVATE, NDN_TAG_TCP_SEQN       } },
    { "ackn",     &ndn_data_unit_uint32_s, {PRIVATE, NDN_TAG_TCP_ACKN       } },
    { "hlen",     &ndn_data_unit_int8_s,   {PRIVATE, NDN_TAG_TCP_HLEN       } },
    { "flags",    &ndn_data_unit_int8_s,   {PRIVATE, NDN_TAG_TCP_FLAGS      } },
    { "win-size", &ndn_data_unit_int16_s,  {PRIVATE, NDN_TAG_TCP_WINDOW     } },
    { "checksum", &ndn_data_unit_int16_s,  {PRIVATE, NDN_TAG_TCP_CHECKSUM   } },
    { "urg-p",    &ndn_data_unit_int16_s,  {PRIVATE, NDN_TAG_TCP_URG        } },
    { "options",  &ndn_tcp_options_seq_s,  {PRIVATE, NDN_TAG_TCP_OPTIONS    } },
    { "socket",   &asn_base_integer_s,     {PRIVATE, NDN_TAG_TCP_DATA_SOCKET} },
    { "length",   &asn_base_integer_s,     {PRIVATE, NDN_TAG_TCP_DATA_LENGTH} },
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
