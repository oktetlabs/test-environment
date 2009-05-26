/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for IGMPv2 protocol.
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
 * @author Alexander Kukuta <kam@oktetlabs.ru>
 *
 * $Id: $
 */
#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_errno.h"
#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_igmpv2.h"
#include "tad_common.h"




static asn_named_entry_t _ndn_igmpv2_pdu_ne_array [] = {
    { "type",          &ndn_data_unit_int8_s,
        { PRIVATE, NDN_IGMPV2_TYPE }},
    { "max-resp-time", &ndn_data_unit_int8_s ,
        { PRIVATE, NDN_IGMPV2_MAX_RESPONSE_TIME }},
    { "checksum", &ndn_data_unit_int16_s,
        { PRIVATE, NDN_IGMPV2_CHECKSUM }},
    { "group-address", &ndn_data_unit_ip_address_s,
        { PRIVATE, NDN_IGMPV2_GROUP_ADDRESS }},
    { "mac-src-address", &ndn_data_unit_eth_address_s,
        { PRIVATE, NDN_IGMPV2_MAC_SOURCE_ADDRESS }},
    { "mac-dst-address", &ndn_data_unit_eth_address_s,
        { PRIVATE, NDN_IGMPV2_MAC_DESTINATION_ADDRESS }},
    { "ip4-src-address", &ndn_data_unit_ip_address_s,
        { PRIVATE, NDN_IGMPV2_IP4_SOURCE_ADDRESS }},
    { "ip4-dst-address", &ndn_data_unit_ip_address_s,
        { PRIVATE, NDN_IGMPV2_IP4_DESTINATION_ADDRESS }},
};

asn_type ndn_igmpv2_pdu_s = {
    "IGMPv2-PDU-Content", {PRIVATE, NDN_IGMPV2_}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_igmpv2_pdu_ne_array),
    {_ndn_igmpv2_pdu_ne_array}
};


const asn_type * const ndn_igmpv2_pdu = &ndn_igmpv2_pdu_s;


static asn_named_entry_t _ndn_igmpv2_csap_ne_array [] = {
    { "version",&ndn_data_unit_int8_s ,
        { PRIVATE, NDN_IGMPV2_VERSION }},
    { "max-resp-time", &ndn_data_unit_int8_s ,
        { PRIVATE, NDN_IGMPV2_MAX_RESPONSE_TIME }},
    { "group-address", &ndn_data_unit_ip_address_s,
        { PRIVATE, NDN_IGMPV2_GROUP_ADDRESS }},
    { "mac-src-address", &ndn_data_unit_eth_address_s,
        { PRIVATE, NDN_IGMPV2_MAC_SOURCE_ADDRESS }},
    { "mac-dst-address", &ndn_data_unit_eth_address_s,
        { PRIVATE, NDN_IGMPV2_MAC_DESTINATION_ADDRESS }},
    { "ip4-src-address", &ndn_data_unit_ip_address_s,
        { PRIVATE, NDN_IGMPV2_IP4_SOURCE_ADDRESS }},
    { "ip4-dst-address", &ndn_data_unit_ip_address_s,
        { PRIVATE, NDN_IGMPV2_IP4_DESTINATION_ADDRESS }},
};

asn_type ndn_igmpv2_csap_s = {
    "IGMPv2-CSAP", {PRIVATE, TE_PROTO_IGMPV2}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_igmpv2_csap_ne_array),
    {_ndn_igmpv2_csap_ne_array}
};

const asn_type * const ndn_igmpv2_csap = &ndn_igmpv2_csap_s;

