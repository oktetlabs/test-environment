/** @file
 * @brief Proteos, TAD file protocol, NDN.
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


#include <stdlib.h>
#include "asn_impl.h"
#include "ndn.h"
#include "ndn_internal.h"
#include "ndn_dhcp.h"


extern asn_type asn_base_charstring_s;

static asn_type ndn_dhcpv4_options_s;


static asn_named_entry_t _ndn_dhcpv4_option_ne_array [] = 
{
    { "type",    &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP_TYPE} },
    { "length",  &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP_LENGTH} },
    { "value",   &ndn_data_unit_octet_string_s, {PRIVATE, NDN_DHCP_VALUE} },
    { "options", &ndn_dhcpv4_options_s, {PRIVATE, NDN_DHCP_OPTIONS} },
};

static asn_type ndn_dhcpv4_option_s = 
{
    "DHCPv4-Option", {PRIVATE, NDN_DHCP_OPTIONS}, SEQUENCE, 
    sizeof(_ndn_dhcpv4_option_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_dhcpv4_option_ne_array}
};

static asn_type ndn_dhcpv4_options_s =
{ 
    "DHCPv4-Options", {PRIVATE, NDN_DHCP_OPTIONS}, SEQUENCE_OF, 
    0, {subtype: &ndn_dhcpv4_option_s} 
};


asn_type_p ndn_dhcpv4_option  = &ndn_dhcpv4_option_s;
asn_type_p ndn_dhcpv4_options = &ndn_dhcpv4_options_s;


static asn_named_entry_t _ndn_dhcpv4_message_ne_array [] = 
{
    { "op",     &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP_OP} },
    { "htype",  &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP_HTYPE} },
    { "hlen",   &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP_HLEN} },
    { "hops",   &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP_HOPS} },
    { "xid",    &ndn_data_unit_int32_s, {PRIVATE, NDN_DHCP_XID} },
    { "secs",   &ndn_data_unit_int16_s, {PRIVATE, NDN_DHCP_SECS} },
    { "flags",  &ndn_data_unit_int16_s, {PRIVATE, NDN_DHCP_FLAGS} },
    { "ciaddr", &ndn_data_unit_ip_address_s, {PRIVATE, NDN_DHCP_CIADDR} },
    { "yiaddr", &ndn_data_unit_ip_address_s, {PRIVATE, NDN_DHCP_YIADDR} },
    { "siaddr", &ndn_data_unit_ip_address_s, {PRIVATE,NDN_DHCP_SIADDR} },
    { "giaddr", &ndn_data_unit_ip_address_s, {PRIVATE,NDN_DHCP_GIADDR} },
    { "chaddr", &ndn_data_unit_octet_string_s, {PRIVATE,NDN_DHCP_CHADDR} },
    { "sname",  &ndn_data_unit_octet_string_s, {PRIVATE,NDN_DHCP_SNAME} },
    { "file",   &ndn_data_unit_octet_string_s, {PRIVATE,NDN_DHCP_FILE} },
    { "options",  &ndn_dhcpv4_options_s, {PRIVATE,NDN_DHCP_OPTIONS} },
};

asn_type ndn_dhcpv4_message_s = 
{
    "DHCPv4-Message", {PRIVATE, NDN_TAD_DHCP}, SEQUENCE, 
    sizeof(_ndn_dhcpv4_message_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_dhcpv4_message_ne_array}
};


asn_type_p ndn_dhcpv4_message = &ndn_dhcpv4_message_s;





asn_enum_entry_t _ndn_dhcp_mode_enum_entries[] = 
{
    {"server", DHCP4_CSAP_MODE_SERVER},
    {"client", DHCP4_CSAP_MODE_CLIENT},
};

asn_type ndn_dhcp_mode_s = {
    "DHCPv4-CSAP-Mode",
    {PRIVATE, NDN_DHCP_MODE},
    ENUMERATED,
    sizeof(_ndn_dhcp_mode_enum_entries)/sizeof(asn_enum_entry_t), 
    {enum_entries: _ndn_dhcp_mode_enum_entries}
};




static asn_named_entry_t _ndn_dhcpv4_csap_ne_array [] = 
{
    { "mode",   &ndn_dhcp_mode_s, {PRIVATE, NDN_DHCP_MODE } },
    { "iface",  &asn_base_charstring_s, {PRIVATE, NDN_DHCP_IFACE} },
};

asn_type ndn_dhcpv4_csap_s =
{
    "DHCPv4-CSAP", {PRIVATE, NDN_TAD_DHCP}, SEQUENCE, 
    sizeof(_ndn_dhcpv4_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_dhcpv4_csap_ne_array}
};

asn_type_p ndn_dhcpv4_csap = &ndn_dhcpv4_csap_s;



