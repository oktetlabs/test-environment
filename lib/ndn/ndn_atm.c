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
#include "ndn_llc.h"
#include "ndn_atm.h"


/*
 * ATM
 */

static asn_named_entry_t _ndn_atm_header_ne_array[] = {
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

asn_type ndn_atm_header_s = {
    "ATM-Header", { PRIVATE, 100 /* FIXME */ }, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_atm_header_ne_array),
    { _ndn_atm_header_ne_array }
};

const asn_type * const ndn_atm_header = &ndn_atm_header_s;


asn_enum_entry_t _ndn_atm_type_enum_entries[] = {
    { "nni", NDN_ATM_NNI },
    { "uni", NDN_ATM_UNI },
};

asn_type ndn_atm_type_s = {
    "ATM-CSAP-Type",
    { PRIVATE, NDN_TAG_ATM_TYPE_ENUM },
    ENUMERATED,
    TE_ARRAY_LEN(_ndn_atm_type_enum_entries),
    { enum_entries: _ndn_atm_type_enum_entries }
};

static asn_named_entry_t _ndn_atm_csap_ne_array[] = {
    { "device-id",      &ndn_data_unit_char_string_s,
      { PRIVATE, NDN_TAG_ATM_DEVICE } },
    { "type",           &ndn_atm_type_s,
      { PRIVATE, NDN_TAG_ATM_TYPE } },
    { "vpi",            &ndn_data_unit_int12_s,
      { PRIVATE, NDN_TAG_ATM_VPI } },
    { "vci",            &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ATM_VCI } },
    { "congestion",     &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_ATM_CONGESTION } },
    { "clp",            &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_ATM_CLP } },
};

asn_type ndn_atm_csap_s = {
    "ATM-CSAP", { PRIVATE, 101 /* FIXME */ }, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_atm_csap_ne_array),
    { _ndn_atm_csap_ne_array }
};

const asn_type * const ndn_atm_csap = &ndn_atm_csap_s;


/*
 * AAL5
 */

static asn_named_entry_t _ndn_aal5_cpcs_trailer_ne_array[] = {
    { "cpcs-uu",    &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_AAL5_CPCS_UU } },
    { "cpi",        &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_AAL5_CPI } },
    { "length",     &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_AAL5_LENGTH } },
    { "crc",        &ndn_data_unit_int32_s,
      { PRIVATE, NDN_TAG_AAL5_CRC } },
};

asn_type ndn_aal5_cpcs_trailer_s = {
    "AAL5-CPCS-Trailer", { PRIVATE, 100 /* FIXME */ }, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_aal5_cpcs_trailer_ne_array),
    { _ndn_aal5_cpcs_trailer_ne_array }
};

const asn_type * const ndn_aal5_cpcs_trailer = &ndn_aal5_cpcs_trailer_s;


static asn_named_entry_t _ndn_aal5_encap_ne_array[] = {
    { "vcMultiplexRoutedProtocol", &asn_base_null_s,
      { PRIVATE, NDN_TAG_AAL5_VC_MUX_ROUTED } },
    { "llc", &ndn_llc_header_s,
      { PRIVATE, NDN_TAG_AAL5_LLC } },
};

static asn_type ndn_aal5_encap_s = {
    "AAL5-Encapsulation", { PRIVATE, 100 }, CHOICE, 
    TE_ARRAY_LEN(_ndn_aal5_encap_ne_array),
    { _ndn_aal5_encap_ne_array }
};


static asn_named_entry_t _ndn_aal5_csap_ne_array[] = {
    { "encap",      &ndn_aal5_encap_s,
      { PRIVATE, NDN_TAG_AAL5_ENCAP } },

    { "cpcs-uu",    &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_AAL5_CPCS_UU } },
    { "cpi",        &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_AAL5_CPI } },
};

asn_type ndn_aal5_csap_s = {
    "AAL5-CSAP", { PRIVATE, 101 /* FIXME */ }, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_aal5_csap_ne_array),
    { _ndn_aal5_csap_ne_array }
};

const asn_type * const ndn_aal5_csap = &ndn_aal5_csap_s;
