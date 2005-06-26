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

static asn_named_entry_t _ndn_ip4_frag_spec_ne_array [] = 
{
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

asn_type ndn_ip4_frag_spec_s =
{
    "IPv4-Fragment", {PRIVATE, NDN_TAG_IP4_FRAGMENTS}, SEQUENCE, 
    sizeof(_ndn_ip4_frag_spec_ne_array)/
        sizeof(_ndn_ip4_frag_spec_ne_array[0]),
    {_ndn_ip4_frag_spec_ne_array}
};


asn_type_p ndn_ip4_frag_spec = &ndn_ip4_frag_spec_s;

asn_type ndn_ip4_frag_seq_s =
{
    "IPv4-Fragments", {PRIVATE, NDN_TAG_IP4_FRAGMENTS}, SEQUENCE_OF, 0,
    {subtype: &ndn_ip4_frag_spec_s}
};

asn_type_p ndn_ip4_frag_seq = &ndn_ip4_frag_seq_s;

/*
IP-Payload-Checksum ::= CHOICE {
    offset  INTEGER, 
    disable NULL
}
*/
static asn_named_entry_t _ndn_ip4_pld_chksm_ne_array [] = 
{
    { "offset", &asn_base_integer_s, 
        { PRIVATE, NDN_TAG_IP4_PLD_CH_OFFSET } },
    { "disable", &asn_base_null_s, 
        { PRIVATE, NDN_TAG_IP4_PLD_CH_DISABLE } },
};

asn_type ndn_ip4_pld_chksm_s =
{
    "IP-Payload-Checksum", {PRIVATE, NDN_TAG_IP4_PLD_CHECKSUM}, CHOICE, 
    sizeof(_ndn_ip4_pld_chksm_ne_array)/
        sizeof(_ndn_ip4_pld_chksm_ne_array[0]),
    {_ndn_ip4_pld_chksm_ne_array}
};

asn_type_p ndn_ip4_pld_chksm = &ndn_ip4_pld_chksm_s;

/* IPv4 PDU */

static asn_named_entry_t _ndn_ip4_header_ne_array [] = 
{
    { "version",         &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_IP4_VERSION} },
    { "header-len",      &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_IP4_HLEN} },
    { "type-of-service", &ndn_data_unit_int8_s, 
        {PRIVATE, NDN_TAG_IP4_TTL} },
    { "ip-len",          &ndn_data_unit_int16_s, 
        {PRIVATE, NDN_TAG_IP4_LEN} },
    { "ip-ident",        &ndn_data_unit_int16_s, 
        {PRIVATE, NDN_TAG_IP4_IDENT} },
    { "flags",           &ndn_data_unit_int8_s, 
        {PRIVATE, NDN_TAG_IP4_FLAGS} },
    { "ip-offset",       &ndn_data_unit_int16_s, 
        {PRIVATE, NDN_TAG_IP4_OFFSET} },
    { "time-to-live",    &ndn_data_unit_int8_s, 
        {PRIVATE, NDN_TAG_IP4_TTL} },
    { "protocol",        &ndn_data_unit_int8_s, 
        {PRIVATE, NDN_TAG_IP4_PROTOCOL} },
    { "h-checksum",      &ndn_data_unit_int16_s, 
        {PRIVATE, NDN_TAG_IP4_H_CHECKSUM} },
    { "src-addr",        &ndn_data_unit_ip_address_s, 
        {PRIVATE, NDN_TAG_IP4_SRC_ADDR} },
    { "dst-addr",        &ndn_data_unit_ip_address_s, 
        {PRIVATE, NDN_TAG_IP4_DST_ADDR} }, 
    { "fragment-spec",   &ndn_ip4_frag_seq_s, 
        {PRIVATE, NDN_TAG_IP4_FRAGMENTS} }, 
    { "pld-checksum",    &ndn_ip4_pld_chksm_s, 
        {PRIVATE, NDN_TAG_IP4_PLD_CHECKSUM} }, 
};

asn_type ndn_ip4_header_s =
{
    "IPv4-Header", {PRIVATE, 100}, SEQUENCE, 
    sizeof(_ndn_ip4_header_ne_array)/
        sizeof(_ndn_ip4_header_ne_array[0]),
    {_ndn_ip4_header_ne_array}
};

asn_type_p ndn_ip4_header = &ndn_ip4_header_s;








static asn_named_entry_t _ndn_ip4_csap_ne_array [] = 
{
    { "version",         &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_IP4_VERSION} },
    { "type-of-service", &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_IP4_TOS} },
    { "ip-ident",        &ndn_data_unit_int16_s,
        {PRIVATE, NDN_TAG_IP4_IDENT} },
    { "flags",           &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_IP4_FLAGS} },
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
};

asn_type ndn_ip4_csap_s =
{
    "IPv4-CSAP", {PRIVATE, 101}, SEQUENCE, 
    sizeof(_ndn_ip4_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_ip4_csap_ne_array}
};

asn_type_p ndn_ip4_csap = &ndn_ip4_csap_s;



/*
 * ICMPv4
 */ 

static asn_named_entry_t _ndn_icmp4_message_ne_array [] = 
{
    { "version",  &ndn_data_unit_int8_s, {PRIVATE, 1} },
    { "ip-len", &ndn_data_unit_int16_s, {PRIVATE, 1} },
    { "src-addr",  &ndn_data_unit_int32_s, {PRIVATE, 1} },
};

asn_type ndn_icmp4_message_s =
{
    "ICMPv4-Header", {PRIVATE, 100}, SEQUENCE, 
    sizeof(_ndn_icmp4_message_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_icmp4_message_ne_array}
};

asn_type_p ndn_icmp4_message = &ndn_icmp4_message_s;





static asn_named_entry_t _ndn_icmp4_csap_ne_array [] = 
{
    { "remote-port",    &ndn_data_unit_int16_s, {PRIVATE, 1} },
    { "local-port",     &ndn_data_unit_int16_s, {PRIVATE, 1} },
};

asn_type ndn_icmp4_csap_s =
{
    "ICMPv4-CSAP", {PRIVATE, 101}, SEQUENCE, 
    sizeof(_ndn_icmp4_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_icmp4_csap_ne_array}
};



/*
 * UDP
 */ 

static asn_named_entry_t _ndn_udp_header_ne_array [] = 
{
    { "src-port", &ndn_data_unit_int16_s, {PRIVATE, 1} },
    { "dst-port", &ndn_data_unit_int16_s, {PRIVATE, 1} },
    { "length",   &ndn_data_unit_int16_s, {PRIVATE, 1} },
    { "checksum", &ndn_data_unit_int16_s, {PRIVATE, 1} },
};

asn_type ndn_udp_header_s =
{
    "UDP-Header", {PRIVATE, 100}, SEQUENCE, 
    sizeof(_ndn_udp_header_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_udp_header_ne_array}
};

asn_type_p ndn_udp_header = &ndn_udp_header_s;





static asn_named_entry_t _ndn_udp_csap_ne_array [] = 
{
    { "local-port",     &ndn_data_unit_int16_s, {PRIVATE, 1} },
    { "remote-port",    &ndn_data_unit_int16_s, {PRIVATE, 1} },
};

asn_type ndn_udp_csap_s =
{
    "UDP-CSAP", {PRIVATE, 101}, SEQUENCE, 
    sizeof(_ndn_udp_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_udp_csap_ne_array}
};

asn_type_p ndn_udp_csap = &ndn_udp_csap_s;






/*
 * TCP
 */ 

static asn_named_entry_t _ndn_tcp_header_ne_array [] = 
{
    { "src-port", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_TCP_SRC_PORT} },
    { "dst-port", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_TCP_DST_PORT} },
    { "seqn",     &ndn_data_unit_int32_s, {PRIVATE, NDN_TAG_TCP_SEQN} },
    { "ackn",     &ndn_data_unit_int32_s, {PRIVATE, NDN_TAG_TCP_ACKN} },
    { "hlen",     &ndn_data_unit_int8_s,  {PRIVATE, NDN_TAG_TCP_HLEN} },
    { "flags",    &ndn_data_unit_int8_s,  {PRIVATE, NDN_TAG_TCP_FLAGS} },
    { "win-size", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_TCP_WINDOW} },
    { "checksum", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_TCP_CHECKSUM} },
    { "urg-p",    &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_TCP_URG} },
};

asn_type ndn_tcp_header_s =
{
    "TCP-Header", {PRIVATE, 101}, SEQUENCE, 
    sizeof(_ndn_tcp_header_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_tcp_header_ne_array}
};

asn_type_p ndn_tcp_header = &ndn_tcp_header_s;





static asn_named_entry_t _ndn_tcp_csap_ne_array [] = 
{
    { "local-port", &ndn_data_unit_int16_s,
        {PRIVATE, NDN_TAG_TCP_LOCAL_PORT} },
    { "remote-port",&ndn_data_unit_int16_s,
        {PRIVATE, NDN_TAG_TCP_REMOTE_PORT} },
};

asn_type ndn_tcp_csap_s =
{
    "TCP-CSAP", {PRIVATE, 102}, SEQUENCE, 
    sizeof(_ndn_tcp_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_tcp_csap_ne_array}
};

asn_type_p ndn_tcp_csap = &ndn_tcp_csap_s;

