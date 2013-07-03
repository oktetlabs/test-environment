/** @file
 * @brief ASN.1 library
 *
 * Declarations of context-specific NDN ASN.1 types
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

#include "config.h"

#include "asn_impl.h"
#include "tad_common.h"
#include "ndn.h"

#ifdef HAS_SNMP
#include "ndn_snmp.h"
#endif

#include "ndn_eth.h"
#include "ndn_pcap.h"
#include "ndn_ipstack.h"
#include "ndn_cli.h"
#include "ndn_iscsi.h"
#include "ndn_bridge.h"
#include "ndn_ppp.h"


/* Add here declaraions of protocol-specific CSAP init params */
extern asn_type ndn_pcap_csap_s;
extern asn_type ndn_atm_csap_s;
extern asn_type ndn_aal5_csap_s;
extern asn_type ndn_eth_csap_s;
extern asn_type ndn_bridge_csap_s;
extern asn_type ndn_arp_csap_s;
extern asn_type ndn_ip4_csap_s;
extern asn_type ndn_ip6_csap_s;
extern asn_type ndn_udp_csap_s;
extern asn_type ndn_dhcpv4_csap_s;
extern asn_type ndn_dhcpv6_csap_s;
extern asn_type ndn_tcp_csap_s;
extern asn_type ndn_iscsi_csap_s;
#ifdef HAS_SNMP
extern asn_type ndn_snmp_csap_s;
#endif
extern asn_type ndn_cli_csap_s;
extern asn_type ndn_socket_csap_s;
extern asn_type ndn_igmp_csap_s;
extern asn_type ndn_ppp_csap_s;
extern asn_type ndn_pppoe_csap_s;

static asn_named_entry_t _ndn_generic_csap_layer_ne_array[] = {
/* Add here reference to protocol-specific CSAP init params */
    { "atm",    &ndn_atm_csap_s,        {PRIVATE, TE_PROTO_ATM} },
    { "aal5",   &ndn_aal5_csap_s,       {PRIVATE, TE_PROTO_AAL5} },
    { "pcap",   &ndn_pcap_csap_s,       {PRIVATE, TE_PROTO_PCAP} },
    { "eth",    &ndn_eth_csap_s,        {PRIVATE, TE_PROTO_ETH} },
    { "bridge", &ndn_bridge_csap_s,     {PRIVATE, TE_PROTO_BRIDGE} },
    { "arp",    &ndn_arp_csap_s,        {PRIVATE, TE_PROTO_ARP} },
    { "ip4",    &ndn_ip4_csap_s,        {PRIVATE, TE_PROTO_IP4} },
    { "ip6",    &ndn_ip6_csap_s,        {PRIVATE, TE_PROTO_IP6} },
    { "icmp4",  &asn_base_null_s,       {PRIVATE, TE_PROTO_ICMP4} },
    { "icmp6",  &asn_base_null_s,       {PRIVATE, TE_PROTO_ICMP6} },
    { "udp",    &ndn_udp_csap_s,        {PRIVATE, TE_PROTO_UDP} },
    { "dhcp",   &ndn_dhcpv4_csap_s,     {PRIVATE, TE_PROTO_DHCP} },
    { "dhcp6",  &ndn_dhcpv6_csap_s,     {PRIVATE, TE_PROTO_DHCP6} },
    { "tcp",    &ndn_tcp_csap_s,        {PRIVATE, TE_PROTO_TCP} },
    { "iscsi",  &ndn_iscsi_csap_s,      {PRIVATE, TE_PROTO_ISCSI} },
#ifdef HAS_SNMP
    { "snmp",   &ndn_snmp_csap_s,       {PRIVATE, TE_PROTO_SNMP} },
#endif
    { "cli",    &ndn_cli_csap_s,        {PRIVATE, TE_PROTO_CLI} },
    { "socket", &ndn_socket_csap_s,     {PRIVATE, TE_PROTO_SOCKET} },
    { "igmp",   &ndn_igmp_csap_s,       {PRIVATE, TE_PROTO_IGMP} },
    { "ppp",    &ndn_ppp_csap_s,        {PRIVATE, TE_PROTO_PPP} },
    { "pppoe",  &ndn_pppoe_csap_s,      {PRIVATE, TE_PROTO_PPPOE} },
};

asn_type ndn_generic_csap_layer_s = {
    "Generic-CSAP-Level", {APPLICATION, 1}, CHOICE,
    TE_ARRAY_LEN(_ndn_generic_csap_layer_ne_array),
    {_ndn_generic_csap_layer_ne_array}
};

const asn_type * const ndn_generic_csap_layer = &ndn_generic_csap_layer_s;


/* Add here declaraions of protocol-specific PDU ASN type */
extern asn_type ndn_atm_header_s;
extern asn_type ndn_aal5_cpcs_trailer_s;
extern asn_type ndn_pcap_filter_s;
extern asn_type ndn_eth_header_s;
extern asn_type ndn_bridge_pdu_s;
extern asn_type ndn_arp_header_s;
extern asn_type ndn_ip4_header_s;
extern asn_type ndn_ip6_header_s;
extern asn_type ndn_icmp4_message_s;
extern asn_type ndn_icmp6_message_s;
extern asn_type ndn_udp_header_s;
extern asn_type ndn_dhcpv4_message_s;
extern asn_type ndn_dhcpv6_message_s;
extern asn_type ndn_tcp_header_s;
extern asn_type ndn_iscsi_message_s;
#ifdef HAS_SNMP
extern asn_type ndn_snmp_message_s;
#endif
extern asn_type ndn_cli_message_s;
extern asn_type ndn_socket_message_s;
extern asn_type ndn_igmp_message_s;
extern asn_type ndn_ppp_message_s;
extern asn_type ndn_pppoe_message_s;

static asn_named_entry_t _ndn_generic_pdu_ne_array[] = {
    { "atm",    &ndn_atm_header_s,        {PRIVATE, TE_PROTO_ATM} },
    { "aal5",   &ndn_aal5_cpcs_trailer_s, {PRIVATE, TE_PROTO_AAL5} },
    { "pcap",   &ndn_pcap_filter_s,       {PRIVATE, TE_PROTO_PCAP} },
    { "eth",    &ndn_eth_header_s,        {PRIVATE, TE_PROTO_ETH} },
    { "bridge", &ndn_bridge_pdu_s,        {PRIVATE, TE_PROTO_BRIDGE} },
    { "arp",    &ndn_arp_header_s,        {PRIVATE, TE_PROTO_ARP} },
    { "ip4",    &ndn_ip4_header_s,        {PRIVATE, TE_PROTO_IP4} },
    { "ip6",    &ndn_ip6_header_s,        {PRIVATE, TE_PROTO_IP6} },
    { "icmp4",  &ndn_icmp4_message_s,     {PRIVATE, TE_PROTO_ICMP4} },
    { "icmp6",  &ndn_icmp6_message_s,     {PRIVATE, TE_PROTO_ICMP6} },
    { "udp",    &ndn_udp_header_s,        {PRIVATE, TE_PROTO_UDP} },
    { "dhcp",   &ndn_dhcpv4_message_s,    {PRIVATE, TE_PROTO_DHCP} },
    { "dhcp6",  &ndn_dhcpv6_message_s,    {PRIVATE, TE_PROTO_DHCP6} },
    { "tcp",    &ndn_tcp_header_s,        {PRIVATE, TE_PROTO_TCP} },
    { "iscsi",  &ndn_iscsi_message_s,     {PRIVATE, TE_PROTO_ISCSI} },
#ifdef HAS_SNMP
    { "snmp",   &ndn_snmp_message_s,      {PRIVATE, TE_PROTO_SNMP} },
#endif
    { "cli",    &ndn_cli_message_s,       {PRIVATE, TE_PROTO_CLI} },
    { "socket", &ndn_socket_message_s,    {PRIVATE, TE_PROTO_SOCKET} },
    { "igmp",   &ndn_igmp_message_s,      {PRIVATE, TE_PROTO_IGMP} },
    { "ppp",    &ndn_ppp_message_s,       {PRIVATE, TE_PROTO_PPP} },
    { "pppoe",  &ndn_pppoe_message_s,     {PRIVATE, TE_PROTO_PPPOE} },
};

asn_type ndn_generic_pdu_s = {
    "Generic-PDU", {APPLICATION, 2}, CHOICE,
    TE_ARRAY_LEN(_ndn_generic_pdu_ne_array),
    {_ndn_generic_pdu_ne_array}
};

const asn_type * const ndn_generic_pdu = &ndn_generic_pdu_s;
