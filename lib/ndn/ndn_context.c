/** @file
 * @brief ASN.1 library
 *
 * Declarations of context-specific NDN ASN.1 types
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 */

#include "te_config.h"

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
#include "ndn_rte_mbuf.h"


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
extern asn_type ndn_rte_mbuf_csap_s;
extern asn_type ndn_vxlan_csap_s;
extern asn_type ndn_geneve_csap_s;
extern asn_type ndn_gre_csap_s;

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
    { "rtembuf",&ndn_rte_mbuf_csap_s,   {PRIVATE, TE_PROTO_RTE_MBUF} },
    { "vxlan",  &ndn_vxlan_csap_s,      {PRIVATE, TE_PROTO_VXLAN} },
    { "geneve", &ndn_geneve_csap_s,     {PRIVATE, TE_PROTO_GENEVE} },
    { "gre",    &ndn_gre_csap_s,        {PRIVATE, TE_PROTO_GRE} },
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
extern asn_type ndn_rte_mbuf_pdu_s;
extern asn_type ndn_vxlan_header_s;
extern asn_type ndn_geneve_header_s;
extern asn_type ndn_gre_header_s;

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
    { "rtembuf",&ndn_rte_mbuf_pdu_s,      {PRIVATE, TE_PROTO_RTE_MBUF} },
    { "vxlan",  &ndn_vxlan_header_s,      {PRIVATE, TE_PROTO_VXLAN} },
    { "geneve", &ndn_geneve_header_s,     {PRIVATE, TE_PROTO_GENEVE} },
    { "gre",    &ndn_gre_header_s,        {PRIVATE, TE_PROTO_GRE} },
    { "void",   &asn_base_null_s,         {PRIVATE, 0} },
};

asn_type ndn_generic_pdu_s = {
    "Generic-PDU", {APPLICATION, 2}, CHOICE,
    TE_ARRAY_LEN(_ndn_generic_pdu_ne_array),
    {_ndn_generic_pdu_ne_array}
};

const asn_type * const ndn_generic_pdu = &ndn_generic_pdu_s;
