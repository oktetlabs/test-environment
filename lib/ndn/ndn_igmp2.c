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
#include "ndn_igmp2.h"
#include "tad_common.h"


static asn_named_entry_t _ndn_igmp2_message_ne_array [] = {
    { "type",          &ndn_data_unit_int8_s,
        { PRIVATE, NDN_IGMP2_TYPE }},
    { "max-resp-time", &ndn_data_unit_int8_s ,
        { PRIVATE, NDN_IGMP2_MAX_RESPONSE_TIME }},
    { "checksum", &ndn_data_unit_int8_s,
        { PRIVATE, NDN_IGMP2_CHECKSUM }},
    { "group-address", &ndn_data_unit_ip_address_s,
        { PRIVATE, NDN_IGMP2_GROUP_ADDRESS }},
};

asn_type ndn_igmp2_message_s = {
    "IGMP2-PDU-Content", {PRIVATE, 111}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_igmp2_message_ne_array),
    {_ndn_igmp2_message_ne_array}
};


const asn_type * const ndn_igmp2_message = &ndn_igmp2_message_s;


static asn_named_entry_t _ndn_igmp2_csap_ne_array [] = {
    { "version",&ndn_data_unit_int8_s ,
        { PRIVATE, NDN_IGMP2_VERSION }},
    { "max-resp-time", &ndn_data_unit_int8_s ,
        { PRIVATE, NDN_IGMP2_MAX_RESPONSE_TIME }},
    { "group-address", &ndn_data_unit_ip_address_s,
        { PRIVATE, NDN_IGMP2_GROUP_ADDRESS }},
};

asn_type ndn_igmp2_csap_s = {
    "IGMP2-CSAP", {PRIVATE, TE_PROTO_IGMP2}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_igmp2_csap_ne_array),
    {_ndn_igmp2_csap_ne_array}
};

const asn_type * const ndn_igmp2_csap = &ndn_igmp2_csap_s;

