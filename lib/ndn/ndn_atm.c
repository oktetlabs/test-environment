/** @file
 * @brief ATM NDN
 *
 * Definitions of ASN.1 types for NDN for ATM (ITU-T I.361, I.363.5;
 * RFC 2684).
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_atm.h"


static asn_named_entry_t _ndn_atm_header_ne_array[] = 
{
    { "gfc",            &ndn_data_unit_int4_s,
      { PRIVATE, NDN_TAG_ATM_GFC } },
    { "vpi",            &ndn_data_unit_int12_s,
      { PRIVATE, NDN_TAG_ATM_VPI } },
    { "vci",            &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ATM_VCI } },
    { "payload-type",   &ndn_data_unit_int3_s,
      { PRIVATE, NDN_TAG_ATM_PAYLOAD_TYPE } },
    { "congestion",     &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_ATM_CONGESTION } },
    { "clp",            &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_ATM_CLP } },
    { "hec",            &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_ATM_HEC } },
};

asn_type ndn_atm_header_s =
{
    "ATM-Header", { PRIVATE, 100 /* FIXME */ }, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_atm_header_ne_array),
    { _ndn_atm_header_ne_array }
};

const asn_type * const ndn_atm_header = &ndn_atm_header_s;


asn_enum_entry_t _ndn_atm_type_enum_entries[] = 
{
    { "nni", NDN_ATM_NNI },
    { "uni", NDN_ATM_UNI },
};

asn_type ndn_atm_type_s =
{
    "ATM-CSAP-Type",
    { PRIVATE, NDN_TAG_ATM_TYPE_ENUM },
    ENUMERATED,
    TE_ARRAY_LEN(_ndn_atm_type_enum_entries),
    { enum_entries: _ndn_atm_type_enum_entries }
};

static asn_named_entry_t _ndn_atm_csap_ne_array[] = 
{
    { "type",           &ndn_atm_type_s,
      { PRIVATE, NDN_TAG_ATM_TYPE } },
    { "gfc",            &ndn_data_unit_int4_s,
      { PRIVATE, NDN_TAG_ATM_GFC } },
    { "vpi",            &ndn_data_unit_int12_s,
      { PRIVATE, NDN_TAG_ATM_VPI } },
    { "vci",            &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ATM_VCI } },
    { "payload-type",   &ndn_data_unit_int3_s,
      { PRIVATE, NDN_TAG_ATM_PAYLOAD_TYPE } },
    { "congestion",     &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_ATM_CONGESTION } },
    { "clp",            &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_ATM_CLP } },
};

asn_type ndn_atm_csap_s =
{
    "ATM-CSAP", { PRIVATE, 101 /* FIXME */ }, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_atm_csap_ne_array),
    { _ndn_atm_csap_ne_array }
};

const asn_type * const ndn_atm_csap = &ndn_atm_csap_s;
