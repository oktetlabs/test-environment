/** @file
 * @brief NDN support for Geneve
 *
 * ASN.1 type declaration for Geneve
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */
#include "te_config.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_geneve.h"


/* Geneve options (draft-gross-geneve-00) */
static asn_named_entry_t _ndn_geneve_option_ne_array [] = {
    { "option-class",   &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_GENEVE_OPTION_CLASS } },
    { "type",           &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_GENEVE_OPTION_TYPE } },
    { "flags-reserved", &ndn_data_unit_int3_s,
      { PRIVATE, NDN_TAG_GENEVE_OPTION_FLAGS_RESERVED } },
    { "length",         &ndn_data_unit_int5_s,
      { PRIVATE, NDN_TAG_GENEVE_OPTION_LENGTH } },
    { "data",           &ndn_data_unit_octet_string_s,
      { PRIVATE, NDN_TAG_GENEVE_OPTION_DATA } } ,
};

asn_type ndn_geneve_option_s = {
    "Geneve-Option",
    { PRIVATE, NDN_TAG_GENEVE_OPTION }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_geneve_option_ne_array),
    {_ndn_geneve_option_ne_array}
};

const asn_type * const ndn_geneve_option = &ndn_geneve_option_s;

asn_type ndn_geneve_options_s = {
    "Geneve-Options",
    { PRIVATE, NDN_TAG_GENEVE_OPTIONS }, SEQUENCE_OF, 0,
    {subtype: &ndn_geneve_option_s}
};

const asn_type * const ndn_geneve_options = &ndn_geneve_options_s;


/* Geneve (draft-gross-geneve-00) */
static asn_named_entry_t _ndn_geneve_header_ne_array [] = {
    { "version",        &ndn_data_unit_int2_s,
      { PRIVATE, NDN_TAG_GENEVE_VERSION } },
    { "options-length", &ndn_data_unit_int6_s,
      { PRIVATE, NDN_TAG_GENEVE_OPTIONS_LENGTH } },
    { "oam",            &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_GENEVE_OAM } },
    { "critical",       &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_GENEVE_CRITICAL } },
    { "reserved-1",     &ndn_data_unit_int6_s,
      { PRIVATE, NDN_TAG_GENEVE_RESERVED_1 } },
    { "protocol",       &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_GENEVE_PROTOCOL } },
    { "vni",            &ndn_data_unit_int24_s,
      { PRIVATE, NDN_TAG_GENEVE_VNI } },
    { "reserved-2",     &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_GENEVE_RESERVED_2 } },

    { "options",        &ndn_geneve_options_s,
      { PRIVATE, NDN_TAG_GENEVE_OPTIONS } },
};

asn_type ndn_geneve_header_s = {
    "Geneve-Header", { PRIVATE, NDN_TAG_GENEVE_HEADER }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_geneve_header_ne_array),
    {_ndn_geneve_header_ne_array}
};

const asn_type * const ndn_geneve_header = &ndn_geneve_header_s;


/* Geneve CSAP */
static asn_named_entry_t _ndn_geneve_csap_ne_array [] = {
    { "protocol", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_GENEVE_PROTOCOL } },
    { "vni",      &ndn_data_unit_int24_s,
      { PRIVATE, NDN_TAG_GENEVE_VNI } },
};

asn_type ndn_geneve_csap_s = {
    "Geneve-CSAP", { PRIVATE, NDN_TAG_GENEVE_CSAP }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_geneve_csap_ne_array),
    {_ndn_geneve_csap_ne_array}
};

const asn_type * const ndn_geneve_csap = &ndn_geneve_csap_s;
