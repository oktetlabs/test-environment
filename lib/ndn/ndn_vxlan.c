/** @file
 * @brief NDN support for VxLAN
 *
 * ASN.1 type declaration for VxLAN
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS in the
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
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */
#include "te_config.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_vxlan.h"


/* VxLAN (RFC 7348) */
static asn_named_entry_t _ndn_vxlan_header_ne_array [] = {
    { "flags-reserved-1", &ndn_data_unit_int4_s,
      { PRIVATE, NDN_TAG_VXLAN_FLAGS_RESERVED_1 } },
    { "vni-valid",        &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_VXLAN_VNI_VALID } },
    { "flags-reserved-2", &ndn_data_unit_int3_s,
      { PRIVATE, NDN_TAG_VXLAN_FLAGS_RESERVED_2 } },
    { "reserved-1",       &ndn_data_unit_int24_s,
      { PRIVATE, NDN_TAG_VXLAN_RESERVED_1 } },
    { "vni",              &ndn_data_unit_int24_s,
      { PRIVATE, NDN_TAG_VXLAN_VNI } },
    { "reserved-2",       &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_VXLAN_RESERVED_2 } },
};

asn_type ndn_vxlan_header_s = {
    "VxLAN-Header", { PRIVATE, NDN_TAG_VXLAN_HEADER }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_vxlan_header_ne_array),
    {_ndn_vxlan_header_ne_array}
};

const asn_type * const ndn_vxlan_header = &ndn_vxlan_header_s;


/* VxLAN CSAP */
static asn_named_entry_t _ndn_vxlan_csap_ne_array [] = {
    { "vni", &ndn_data_unit_int24_s,
      { PRIVATE, NDN_TAG_VXLAN_VNI } },
};

asn_type ndn_vxlan_csap_s = {
    "VxLAN-CSAP", { PRIVATE, NDN_TAG_VXLAN_CSAP }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_vxlan_csap_ne_array),
    {_ndn_vxlan_csap_ne_array}
};

const asn_type * const ndn_vxlan_csap = &ndn_vxlan_csap_s;
