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
};

asn_type ndn_ip4_csap_s = {
    "IPv4-CSAP", {PRIVATE, 101}, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_ip4_csap_ne_array),
    {_ndn_ip4_csap_ne_array}
};

const asn_type * const ndn_ip4_csap = &ndn_ip4_csap_s;



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
    win₋scale   [3] TCP-Option-WindScale,
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
