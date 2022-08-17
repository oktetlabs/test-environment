/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief NDN support for GRE
 *
 * ASN.1 type declaration for GRE
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#include "te_config.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_gre.h"


/* GRE optional checksum field (RFC 2784) */
static asn_named_entry_t _ndn_gre_header_opt_cksum_ne_array [] = {
    { "value",    &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_GRE_OPT_CKSUM_VALUE } },
    { "reserved", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_GRE_OPT_CKSUM_RESERVED } },
};

asn_type ndn_gre_header_opt_cksum_s = {
    "GRE-Header-Optional-Checksum",
    { PRIVATE, NDN_TAG_GRE_OPT_CKSUM }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_gre_header_opt_cksum_ne_array),
    {_ndn_gre_header_opt_cksum_ne_array}
};

const asn_type * const ndn_gre_header_opt_cksum =
    &ndn_gre_header_opt_cksum_s;


/* NVGRE-specific implementation of GRE key optional field (RFC 7637) */
static asn_named_entry_t _ndn_gre_header_opt_key_nvgre_ne_array [] = {
    { "vsid",   &ndn_data_unit_int24_s,
      { PRIVATE, NDN_TAG_GRE_OPT_KEY_NVGRE_VSID } },
    { "flowid", &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_GRE_OPT_KEY_NVGRE_FLOWID } },
};

asn_type ndn_gre_header_opt_key_nvgre_s = {
    "GRE-Header-Optional-Key-NVGRE",
    { PRIVATE, NDN_TAG_GRE_OPT_KEY_NVGRE }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_gre_header_opt_key_nvgre_ne_array),
    {_ndn_gre_header_opt_key_nvgre_ne_array}
};

const asn_type * const ndn_gre_header_opt_key_nvgre =
    &ndn_gre_header_opt_key_nvgre_s;


/* Registry of specific layouts for the GRE key optional field */
static asn_named_entry_t _ndn_gre_header_opt_key_ne_array [] = {
    { "nvgre", &ndn_gre_header_opt_key_nvgre_s,
      { PRIVATE, NDN_TAG_GRE_OPT_KEY_NVGRE } },
};


/*
 * GRE Key extension (RFC 2890) to introduce
 * optional key field to GRE header
 */
asn_type ndn_gre_header_opt_key_s = {
    "GRE-Header-Optional-Key",
    { PRIVATE, NDN_TAG_GRE_OPT_KEY }, CHOICE,
    TE_ARRAY_LEN(_ndn_gre_header_opt_key_ne_array),
    {_ndn_gre_header_opt_key_ne_array}
};

const asn_type * const ndn_gre_header_opt_key =
    &ndn_gre_header_opt_key_s;


/*
 * GRE Sequence Number extension (RFC 2890) to introduce
 * optional sequence number field to GRE header
 */
static asn_named_entry_t _ndn_gre_header_opt_seqn_ne_array [] = {
    { "value", &ndn_data_unit_int32_s,
      { PRIVATE, NDN_TAG_GRE_OPT_SEQN_VALUE } },
};

asn_type ndn_gre_header_opt_seqn_s = {
    "GRE-Header-Optional-Sequence-Number",
    { PRIVATE, NDN_TAG_GRE_OPT_SEQN }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_gre_header_opt_seqn_ne_array),
    {_ndn_gre_header_opt_seqn_ne_array}
};

const asn_type * const ndn_gre_header_opt_seqn =
    &ndn_gre_header_opt_seqn_s;


/* GRE (RFC 2784 updated by RFC 2890) */
static asn_named_entry_t _ndn_gre_header_ne_array [] = {
    { "cksum-present",    &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_GRE_CKSUM_PRESENT } },
    { "flags-reserved-1", &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_GRE_FLAGS_RESERVED_1 } },
    { "key-present",      &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_GRE_KEY_PRESENT } },
    { "seqn-present",     &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_GRE_SEQN_PRESENT } },
    { "flags-reserved-2", &ndn_data_unit_int9_s,
      { PRIVATE, NDN_TAG_GRE_FLAGS_RESERVED_2 } },
    { "version",          &ndn_data_unit_int3_s,
      { PRIVATE, NDN_TAG_GRE_VERSION } },
    { "protocol",         &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_GRE_PROTOCOL } },

    { "opt-cksum",        &ndn_gre_header_opt_cksum_s,
      { PRIVATE, NDN_TAG_GRE_OPT_CKSUM } },
    { "opt-key",          &ndn_gre_header_opt_key_s,
      { PRIVATE, NDN_TAG_GRE_OPT_KEY } },
    { "opt-seqn",         &ndn_gre_header_opt_seqn_s,
      { PRIVATE, NDN_TAG_GRE_OPT_SEQN } },
};

asn_type ndn_gre_header_s = {
    "GRE-Header", { PRIVATE, NDN_TAG_GRE_HEADER }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_gre_header_ne_array),
    {_ndn_gre_header_ne_array}
};

const asn_type * const ndn_gre_header = &ndn_gre_header_s;


/* GRE CSAP */
static asn_named_entry_t _ndn_gre_csap_ne_array [] = {
    { "protocol",         &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_GRE_PROTOCOL } },
    { "vsid",             &ndn_data_unit_int24_s,
      { PRIVATE, NDN_TAG_GRE_OPT_KEY_NVGRE_VSID } },
};

asn_type ndn_gre_csap_s = {
    "GRE-CSAP", { PRIVATE, NDN_TAG_GRE_CSAP }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_gre_csap_ne_array),
    {_ndn_gre_csap_ne_array}
};

const asn_type * const ndn_gre_csap = &ndn_gre_csap_s;
