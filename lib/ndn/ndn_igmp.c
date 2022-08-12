/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for IGMPv2 protocol.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Alexander Kukuta <kam@oktetlabs.ru>
 *
 */
#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_errno.h"
#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_igmp.h"
#include "tad_common.h"

#ifdef NDN_IGMP_STRUCTURED
asn_type ndn_igmp_source_address_list_s = {
    "IGMP-Source-Address-List", {PRIVATE, NDN_TAG_IGMP3_SOURCE_ADDRESS_LIST},
    SEQUENCE_OF, 0, {subtype: &ndn_data_unit_ip_address_s}}
};

static asn_named_entry_t _ndn_igmp_group_record_ne_array [] = {
    /* Common for IGMP fields */
    { "record-type", &ndn_data_unit_int8_s,
        { PRIVATE, NDN_TAG_IGMP3_RECORD_TYPE }},
    { "aux-data-length", &ndn_data_unit_int8_s,
        { PRIVATE, NDN_TAG_IGMP3_AUX_DATA_LENGTH }},
    { "number-of-sources", &ndn_data_unit_int16_s,
        { PRIVATE, NDN_TAG_IGMP3_NUMBER_OF_SOURCES }},
    { "group-address", &ndn_data_unit_ip_address_s,
        { PRIVATE, NDN_TAG_IGMP_GROUP_ADDRESS }},
    { "source-address-list", &ndn_igmp_source_address_list_s,
        { PRIVATE, NDN_TAG_IGMP3_SOURCE_ADDRESS_LIST }},
    { "aux-data", &ndn_data_unit_octet_string_s,
        { PRIVATE, NDN_TAG_IGMP3_AUX_DATA }},
};

asn_type ndn_igmp_group_record_s = {
    "IGMP-Group-Record", {PRIVATE, NDN_TAG_IGMP3_GROUP_RECORD}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_igmp_group_record_ne_array),
    {_ndn_igmp_group_record_ne_array}
};

asn_type ndn_igmp_group_record_list_s = {
    "IGMP-Group-Record-List", {PRIVATE, NDN_TAG_IGMP3_GROUP_RECORD_LIST},
    SEQUENCE_OF, 0, {subtype: &ndn_igmp_group_record_s}}
};
#endif /* NDN_IGMP_STRUCTURED */

static asn_named_entry_t _ndn_igmp_message_ne_array [] = {
    /* Common for IGMP fields */
    { "type", &ndn_data_unit_int8_s,
        { PRIVATE, NDN_TAG_IGMP_TYPE }},
    { "max-resp-time", &ndn_data_unit_int8_s ,
        { PRIVATE, NDN_TAG_IGMP_MAX_RESPONSE_TIME }},
    { "checksum", &ndn_data_unit_int8_s,
        { PRIVATE, NDN_TAG_IGMP_CHECKSUM }},
    { "group-address", &ndn_data_unit_ip_address_s,
        { PRIVATE, NDN_TAG_IGMP_GROUP_ADDRESS }},

    /* IGMPv3 Query specific fields */
    { "reserved", &ndn_data_unit_int4_s,
        { PRIVATE, NDN_TAG_IGMP3_RESERVED }},
    { "s-flag", &ndn_data_unit_int8_s,
        { PRIVATE, NDN_TAG_IGMP3_S_FLAG }},
    { "qrv", &ndn_data_unit_int8_s,
        { PRIVATE, NDN_TAG_IGMP3_QRV }},
    { "qqic", &ndn_data_unit_int8_s,
        { PRIVATE, NDN_TAG_IGMP3_QQIC }},
    { "number-of-sources", &ndn_data_unit_int16_s,
        { PRIVATE, NDN_TAG_IGMP3_NUMBER_OF_SOURCES }},

#ifdef NDN_IGMP_STRUCTURED
    { "source-address-list", &ndn_igmp_source_address_list_s,
#else
    { "source-address-list", &ndn_data_unit_octet_string_s,
#endif
        { PRIVATE, NDN_TAG_IGMP3_SOURCE_ADDRESS_LIST }},

    /* IGMPv3 Report specific fields */
    { "number-of-groups", &ndn_data_unit_int16_s,
        { PRIVATE, NDN_TAG_IGMP3_NUMBER_OF_GROUPS }},

#ifdef NDN_IGMP_STRUCTURED
    { "group-record-list", &ndn_igmp_group_record_list_s,
#else
    { "group-record-list", &ndn_data_unit_octet_string_s,
#endif
        { PRIVATE, NDN_TAG_IGMP3_GROUP_RECORD_LIST }},
};

asn_type ndn_igmp_message_s = {
    "IGMP-PDU-Content", {PRIVATE, 111}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_igmp_message_ne_array),
    {_ndn_igmp_message_ne_array}
};


const asn_type * const ndn_igmp_message = &ndn_igmp_message_s;


static asn_named_entry_t _ndn_igmp_csap_ne_array [] = {
    { "version",&ndn_data_unit_int8_s ,
        { PRIVATE, NDN_TAG_IGMP_VERSION }},
    { "max-resp-time", &ndn_data_unit_int8_s ,
        { PRIVATE, NDN_TAG_IGMP_MAX_RESPONSE_TIME }},
};

asn_type ndn_igmp_csap_s = {
    "IGMP-CSAP", {PRIVATE, TE_PROTO_IGMP}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_igmp_csap_ne_array),
    {_ndn_igmp_csap_ne_array}
};

const asn_type * const ndn_igmp_csap = &ndn_igmp_csap_s;

