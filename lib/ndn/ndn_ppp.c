/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for file protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#include "te_config.h"


#include <stdlib.h>
#include "asn_impl.h"
#include "tad_common.h"
#include "ndn.h"
#include "ndn_internal.h"
#include "ndn_ppp.h"
#include "tad_common.h"

/* PPPoE Layer */

static asn_named_entry_t _ndn_pppoe_pkt_ne_array [] = {
    { "version", &ndn_data_unit_int8_s, {PRIVATE, NDN_TAG_PPPOE_VERSION} },
    { "type",    &ndn_data_unit_int8_s, {PRIVATE, NDN_TAG_PPPOE_TYPE} },
    { "code",    &ndn_data_unit_int8_s, {PRIVATE, NDN_TAG_PPPOE_CODE} },
    { "session-id",  &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_PPPOE_SESSION_ID} },
    { "length",  &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_PPPOE_LENGTH} },
    { "payload",   &ndn_data_unit_octet_string_s, {PRIVATE, NDN_TAG_PPPOE_PAYLOAD} },
};

asn_type ndn_pppoe_message_s = {
    "PPPoE-Message", {PRIVATE, NDN_TAG_PPPOE_PKT}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_pppoe_pkt_ne_array),
    {_ndn_pppoe_pkt_ne_array}
};

static asn_named_entry_t _ndn_pppoe_csap_ne_array[] = {
    { "version", &ndn_data_unit_int8_s, {PRIVATE, NDN_TAG_PPPOE_VERSION} },
    { "type",    &ndn_data_unit_int8_s, {PRIVATE, NDN_TAG_PPPOE_TYPE} },
    { "code",    &ndn_data_unit_int8_s, {PRIVATE, NDN_TAG_PPPOE_CODE} },
    { "session-id",  &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_PPPOE_SESSION_ID} },
    { "length",  &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_PPPOE_LENGTH} },
    { "payload",   &ndn_data_unit_octet_string_s, {PRIVATE, NDN_TAG_PPPOE_PAYLOAD} },
};

asn_type ndn_pppoe_csap_s = {
    "PPPoE-CSAP", {PRIVATE, TE_PROTO_PPPOE}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_pppoe_csap_ne_array),
    {_ndn_pppoe_csap_ne_array}
};

const asn_type * const ndn_pppoe_csap = &ndn_pppoe_csap_s;

const asn_type * const ndn_pppoe_message = &ndn_pppoe_message_s;

/* PPP Layer */

static asn_named_entry_t _ndn_ppp_pkt_ne_array [] = {
    { "protocol", &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_PPP_PROTOCOL} },
};

asn_type ndn_ppp_message_s = {
    "PPP-Message", {PRIVATE, NDN_TAG_PPP_PKT}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ppp_pkt_ne_array),
    {_ndn_ppp_pkt_ne_array}
};

const asn_type * const ndn_ppp_message = &ndn_ppp_message_s;


static asn_named_entry_t _ndn_ppp_csap_ne_array[] = {
    { "protocol",   &ndn_data_unit_int16_s, {PRIVATE, NDN_TAG_PPP_PROTOCOL } },
};

asn_type ndn_ppp_csap_s = {
    "PPP-CSAP", {PRIVATE, TE_PROTO_PPP}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_ppp_csap_ne_array),
    {_ndn_ppp_csap_ne_array}
};

const asn_type * const ndn_ppp_csap = &ndn_ppp_csap_s;

