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
#include "ndn_dhcp.h"

/* DHCPv4 */
static asn_type ndn_dhcpv4_options_s;

static asn_named_entry_t _ndn_dhcpv4_option_ne_array [] = {
    { "type",    &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP_TYPE} },
    { "length",  &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP_LENGTH} },
    { "value",   &ndn_data_unit_octet_string_s, {PRIVATE, NDN_DHCP_VALUE} },
    { "options", &ndn_dhcpv4_options_s, {PRIVATE, NDN_DHCP_OPTIONS} },
};

static asn_named_entry_t _ndn_dhcpv4_end_pad_option_ne_array [] = {
    { "type",    &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP_TYPE} },
};

static asn_type ndn_dhcpv4_option_s = {
    "DHCPv4-Option", {PRIVATE, NDN_DHCP_OPTIONS}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv4_option_ne_array),
    {_ndn_dhcpv4_option_ne_array}
};

static asn_type ndn_dhcpv4_end_pad_option_s = {
    "DHCPv4-Option", {PRIVATE, NDN_DHCP_OPTIONS}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv4_end_pad_option_ne_array),
    {_ndn_dhcpv4_end_pad_option_ne_array}
};

static asn_type ndn_dhcpv4_options_s = {
    "DHCPv4-Options", {PRIVATE, NDN_DHCP_OPTIONS}, SEQUENCE_OF,
    0, {subtype: &ndn_dhcpv4_option_s}
};

asn_type *ndn_dhcpv4_option  = &ndn_dhcpv4_option_s;
asn_type *ndn_dhcpv4_end_pad_option  = &ndn_dhcpv4_end_pad_option_s;
asn_type * ndn_dhcpv4_options = &ndn_dhcpv4_options_s;

static asn_named_entry_t _ndn_dhcpv4_message_ne_array [] = {
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

asn_type ndn_dhcpv4_message_s = {
    "DHCPv4-Message", {PRIVATE, TE_PROTO_DHCP}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv4_message_ne_array),
    {_ndn_dhcpv4_message_ne_array}
};

asn_type * ndn_dhcpv4_message = &ndn_dhcpv4_message_s;

asn_enum_entry_t _ndn_dhcp_mode_enum_entries[] = {
    {"server", DHCP4_CSAP_MODE_SERVER},
    {"client", DHCP4_CSAP_MODE_CLIENT},
};

asn_type ndn_dhcp_mode_s = {
    "DHCPv4-CSAP-Mode",
    {PRIVATE, NDN_DHCP_MODE},
    ENUMERATED,
    TE_ARRAY_LEN(_ndn_dhcp_mode_enum_entries),
    {enum_entries: _ndn_dhcp_mode_enum_entries}
};

static asn_named_entry_t _ndn_dhcpv4_csap_ne_array[] = {
    { "mode",   &ndn_dhcp_mode_s, {PRIVATE, NDN_DHCP_MODE } },
    { "iface",  &asn_base_charstring_s, {PRIVATE, NDN_DHCP_IFACE} },
};

asn_type ndn_dhcpv4_csap_s = {
    "DHCPv4-CSAP", {PRIVATE, TE_PROTO_DHCP}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv4_csap_ne_array),
    {_ndn_dhcpv4_csap_ne_array}
};

asn_type * ndn_dhcpv4_csap = &ndn_dhcpv4_csap_s;

/* DHCPv6 */
static asn_named_entry_t _ndn_dhcpv6_duid_ne_array [] = {
    { "type",    &ndn_data_unit_int16_s,
        {PRIVATE, NDN_DHCP6_DUID_TYPE} },
    { "hardware-type",  &ndn_data_unit_int16_s,
        {PRIVATE, NDN_DHCP6_DUID_HWTYPE} },
    { "enterprise-number", &ndn_data_unit_int32_s,
        {PRIVATE, NDN_DHCP6_ENTERPRISE_NUMBER} },
    { "time", &ndn_data_unit_int32_s, {PRIVATE, NDN_DHCP6_TIME} },
    { "link-layer-address", &ndn_data_unit_octet_string_s,
        {PRIVATE, NDN_DHCP6_DUID_LL_ADDR} },
    { "identifier", &ndn_data_unit_octet_string_s,
        {PRIVATE, NDN_DHCP6_DUID_IDENTIFIER} },
};

static asn_type ndn_dhcpv6_duid_s = {
    "DHCPv6-DUID", {PRIVATE, NDN_DHCP6_DUID}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_duid_ne_array),
    {_ndn_dhcpv6_duid_ne_array}
};

asn_type *ndn_dhcpv6_duid = &ndn_dhcpv6_duid_s;

static asn_type ndn_dhcpv6_options_s;
asn_type ndn_dhcpv6_message_s;

static asn_named_entry_t _ndn_dhcpv6_ia_na_ne_array [] = {
    { "iaid", &ndn_data_unit_int32_s, {PRIVATE, NDN_DHCP6_IAID} },
    { "t1", &ndn_data_unit_int32_s, {PRIVATE, NDN_DHCP6_TIME} },
    { "t2", &ndn_data_unit_int32_s, {PRIVATE, NDN_DHCP6_TIME} },
    { "options", &ndn_dhcpv6_options_s, {PRIVATE, NDN_DHCP6_OPTIONS} },
};

static asn_type ndn_dhcpv6_ia_na_s = {
    "DHCPv6-IA-NA", {PRIVATE, NDN_DHCP6_IA_NA}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_ia_na_ne_array),
    {_ndn_dhcpv6_ia_na_ne_array}
};

static asn_named_entry_t _ndn_dhcpv6_ia_ta_ne_array [] = {
    { "iaid", &ndn_data_unit_int32_s, {PRIVATE, NDN_DHCP6_IAID} },
    { "options", &ndn_dhcpv6_options_s, {PRIVATE, NDN_DHCP6_OPTIONS} },
};

static asn_type ndn_dhcpv6_ia_ta_s = {
    "DHCPv6-IA-TA", {PRIVATE, NDN_DHCP6_IA_TA}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_ia_ta_ne_array),
    {_ndn_dhcpv6_ia_ta_ne_array}
};

static asn_named_entry_t _ndn_dhcpv6_ia_addr_ne_array [] = {
    { "ipv6-address", &ndn_data_unit_ip6_address_s,
        {PRIVATE, NDN_DHCP6_IP6_ADDR} },
    { "preferred-lifetime", &ndn_data_unit_int32_s,
        {PRIVATE, NDN_DHCP6_TIME} },
    { "valid-lifetime", &ndn_data_unit_int32_s,
        {PRIVATE, NDN_DHCP6_TIME} },
    { "options", &ndn_dhcpv6_options_s, {PRIVATE, NDN_DHCP6_OPTIONS} },
};

static asn_type ndn_dhcpv6_ia_addr_s = {
    "DHCPv6-IA-ADDR", {PRIVATE, NDN_DHCP6_IA_ADDR}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_ia_addr_ne_array),
    {_ndn_dhcpv6_ia_addr_ne_array}
};

static asn_named_entry_t _ndn_dhcpv6_ia_pd_ne_array [] = {
    { "iaid", &ndn_data_unit_int32_s, {PRIVATE, NDN_DHCP6_IAID} },
    { "t1", &ndn_data_unit_int32_s, {PRIVATE, NDN_DHCP6_TIME} },
    { "t2", &ndn_data_unit_int32_s, {PRIVATE, NDN_DHCP6_TIME} },
    { "options", &ndn_dhcpv6_options_s, {PRIVATE, NDN_DHCP6_OPTIONS} },
};

static asn_type ndn_dhcpv6_ia_pd_s = {
    "DHCPv6-IA-PD", {PRIVATE, NDN_DHCP6_IA_PD}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_ia_pd_ne_array),
    {_ndn_dhcpv6_ia_pd_ne_array}
};

static asn_named_entry_t _ndn_dhcpv6_ia_prefix_ne_array [] = {
    { "preferred-lifetime", &ndn_data_unit_int32_s,
        {PRIVATE, NDN_DHCP6_TIME} },
    { "valid-lifetime", &ndn_data_unit_int32_s,
        {PRIVATE, NDN_DHCP6_TIME} },
    { "prefix-length", &ndn_data_unit_int8_s,
        {PRIVATE, NDN_DHCP6_IP6_PREFIX}},
    { "prefix-address", &ndn_data_unit_ip6_address_s,
        {PRIVATE, NDN_DHCP6_IP6_ADDR} },
    { "options", &ndn_dhcpv6_options_s, {PRIVATE, NDN_DHCP6_OPTIONS} },
};

static asn_type ndn_dhcpv6_ia_prefix_s = {
    "DHCPv6-IA-PREFIX", {PRIVATE, NDN_DHCP6_IA_PREFIX}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_ia_prefix_ne_array),
    {_ndn_dhcpv6_ia_prefix_ne_array}
};

asn_type *ndn_dhcpv6_ia_na = &ndn_dhcpv6_ia_na_s;
asn_type *ndn_dhcpv6_ia_ta = &ndn_dhcpv6_ia_ta_s;
asn_type *ndn_dhcpv6_ia_addr = &ndn_dhcpv6_ia_addr_s;
asn_type *ndn_dhcpv6_ia_pd = &ndn_dhcpv6_ia_pd_s;
asn_type *ndn_dhcpv6_ia_prefix = &ndn_dhcpv6_ia_prefix_s;

static asn_named_entry_t _ndn_dhcpv6_opcode_ne_array [] = {
    { "opcode", &ndn_data_unit_int16_s, {PRIVATE, NDN_DHCP6_OPCODE}},
};

static asn_type ndn_dhcpv6_opcode_s = {
    "DHCPv6-OPCODE", {PRIVATE, NDN_DHCP6_OPCODE}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_opcode_ne_array),
    {_ndn_dhcpv6_opcode_ne_array}
};

static asn_type ndn_dhcpv6_oro_s = {
    "DHCPv6-ORO", {PRIVATE, NDN_DHCP6_ORO}, SEQUENCE_OF,
    0, {subtype: &ndn_dhcpv6_opcode_s}
};

asn_type *ndn_dhcpv6_opcode = &ndn_dhcpv6_opcode_s;
asn_type *ndn_dhcpv6_oro = &ndn_dhcpv6_oro_s;

static asn_named_entry_t _ndn_dhcpv6_auth_ne_array [] = {
    { "protocol", &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP6_AUTH_PROTO}},
    { "algorithm", &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP6_AUTH_ALG}},
    { "rdm", &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP6_AUTH_RDM}},
    { "relay-detection", &ndn_data_unit_octet_string_s,
        {PRIVATE, NDN_DHCP6_AUTH_RELAY_DETECT}},
    { "auth-info", &ndn_data_unit_octet_string_s,
        {PRIVATE, NDN_DHCP6_AUTH_INFO}},
};

static asn_type ndn_dhcpv6_auth_s = {
    "DHCPv6-Auth", {PRIVATE, NDN_DHCP6_AUTH}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_auth_ne_array),
    {_ndn_dhcpv6_auth_ne_array}
};

asn_type *ndn_dhcpv6_auth = &ndn_dhcpv6_auth_s;

static asn_named_entry_t _ndn_dhcpv6_status_ne_array [] = {
    { "status-code", &ndn_data_unit_int16_s,
        {PRIVATE, NDN_DHCP6_STATUS_CODE}},
    { "sytatus-message", &ndn_data_unit_octet_string_s,
        {PRIVATE, NDN_DHCP6_STATUS_MESSAGE}},
};

static asn_type ndn_dhcpv6_status_s = {
    "DHCPv6-Status", {PRIVATE, NDN_DHCP6_STATUS}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_status_ne_array),
    {_ndn_dhcpv6_status_ne_array}
};

asn_type *ndn_dhcpv6_status = &ndn_dhcpv6_status_s;

static asn_named_entry_t _ndn_dhcpv6_class_data_ne_array [] = {
    { "class-data-len", &ndn_data_unit_int16_s,
        {PRIVATE, NDN_DHCP6_CLASS_DATA_LEN}},
    { "class-data-opaque", &ndn_data_unit_octet_string_s,
        {PRIVATE, NDN_DHCP6_CLASS_DATA_OPAQUE}},
};

static asn_type ndn_dhcpv6_class_data_s = {
    "DHCPv6-ClassData", {PRIVATE, NDN_DHCP6_CLASS_DATA}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_class_data_ne_array),
    {_ndn_dhcpv6_class_data_ne_array}
};

static asn_type ndn_dhcpv6_class_data_list_s = {
    "DHCPv6-ClassDataList", {PRIVATE, NDN_DHCP6_CLASS_DATA},
    SEQUENCE_OF, 0, {subtype: &ndn_dhcpv6_class_data_s}
};

asn_type *ndn_dhcpv6_class_data = &ndn_dhcpv6_class_data_s;
asn_type *ndn_dhcpv6_class_data_list = &ndn_dhcpv6_class_data_list_s;

static asn_named_entry_t _ndn_dhcpv6_vendor_class_ne_array [] = {
    { "enterprise-number", &ndn_data_unit_int32_s,
        {PRIVATE, NDN_DHCP6_ENTERPRISE_NUMBER}},
    { "vendor-class-data", &ndn_dhcpv6_class_data_list_s,
        {PRIVATE, NDN_DHCP6_VENDOR_CLASS_DATA}},
};

static asn_type ndn_dhcpv6_vendor_class_s = {
    "DHCPv6-VendorClass", {PRIVATE, NDN_DHCP6_VENDOR_CLASS}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_vendor_class_ne_array),
    {_ndn_dhcpv6_vendor_class_ne_array}
};

asn_type *ndn_dhcpv6_vendor_class = &ndn_dhcpv6_vendor_class_s;

static asn_named_entry_t _ndn_dhcpv6_vendor_specific_ne_array [] = {
    { "enterprise-number", &ndn_data_unit_int32_s,
        {PRIVATE, NDN_DHCP6_ENTERPRISE_NUMBER}},
    { "option-data", &ndn_data_unit_octet_string_s,
        {PRIVATE, NDN_DHCP6_VENDOR_SPECIFIC_DATA}},
};

static asn_type ndn_dhcpv6_vendor_specific_s = {
    "DHCPv6-VendorSpecific", {PRIVATE, NDN_DHCP6_VENDOR_SPECIFIC}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_vendor_specific_ne_array),
    {_ndn_dhcpv6_vendor_specific_ne_array}
};

asn_type *ndn_dhcpv6_vendor_specific = &ndn_dhcpv6_vendor_specific_s;

static asn_named_entry_t _ndn_dhcpv6_option_ne_array [] = {
    { "type",    &ndn_data_unit_int16_s, {PRIVATE, NDN_DHCP6_TYPE} },
    { "length",  &ndn_data_unit_int16_s, {PRIVATE, NDN_DHCP6_LENGTH} },
    { "value", &ndn_data_unit_octet_string_s, {PRIVATE, NDN_DHCP6_VALUE} },
    { "options", &ndn_dhcpv6_options_s, {PRIVATE, NDN_DHCP6_OPTIONS} },
    { "relay-messade", &ndn_dhcpv6_message_s,
        {PRIVATE, NDN_DHCP6_RELAY_MESSAGE}},
    { "duid", &ndn_dhcpv6_duid_s, {PRIVATE, NDN_DHCP6_DUID}},
    { "ia-na", &ndn_dhcpv6_ia_na_s, {PRIVATE, NDN_DHCP6_IA_NA}},
    { "ia-ta", &ndn_dhcpv6_ia_ta_s, {PRIVATE, NDN_DHCP6_IA_TA}},
    { "ia-addr", &ndn_dhcpv6_ia_addr_s, {PRIVATE, NDN_DHCP6_IA_ADDR}},
    { "oro", &ndn_dhcpv6_oro_s, {PRIVATE, NDN_DHCP6_ORO}},
    { "auth", &ndn_dhcpv6_auth_s, {PRIVATE, NDN_DHCP6_AUTH}},
    { "servaddr", &ndn_data_unit_ip6_address_s,
        {PRIVATE, NDN_DHCP6_SERVADDR}},
    { "status", &ndn_dhcpv6_status_s, {PRIVATE, NDN_DHCP6_STATUS}},
    { "user-class-data", &ndn_dhcpv6_class_data_list_s,
        {PRIVATE, NDN_DHCP6_USER_CLASS}},
    { "vendor-class", &ndn_dhcpv6_vendor_class_s,
        {PRIVATE, NDN_DHCP6_VENDOR_CLASS}},
    { "vendor-specific", &ndn_dhcpv6_vendor_specific_s,
        {PRIVATE, NDN_DHCP6_VENDOR_SPECIFIC}},
    { "elapsed-time",  &ndn_data_unit_int16_s,
        {PRIVATE, NDN_DHCP6_ELAPSED_TIME} },
    { "ia-pd", &ndn_dhcpv6_ia_pd_s, {PRIVATE, NDN_DHCP6_IA_PD}},
    { "ia-prefix", &ndn_dhcpv6_ia_prefix_s,
        {PRIVATE, NDN_DHCP6_IA_PREFIX}},
};

static asn_type ndn_dhcpv6_option_s = {
    "DHCPv6-Option", {PRIVATE, NDN_DHCP6_OPTIONS}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_option_ne_array),
    {_ndn_dhcpv6_option_ne_array}
};

static asn_type ndn_dhcpv6_options_s = {
    "DHCPv6-Options", {PRIVATE, NDN_DHCP6_OPTIONS}, SEQUENCE_OF,
    0, {subtype: &ndn_dhcpv6_option_s}
};

asn_type *ndn_dhcpv6_option  = &ndn_dhcpv6_option_s;
asn_type *ndn_dhcpv6_options = &ndn_dhcpv6_options_s;

static asn_named_entry_t _ndn_dhcpv6_message_ne_array [] = {
    { "msg-type",   &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP6_TYPE} },
    { "transaction-id", &ndn_data_unit_int24_s, {PRIVATE, NDN_DHCP6_TID} },
    { "hop-count", &ndn_data_unit_int8_s, {PRIVATE, NDN_DHCP6_HOPCNT} },
    { "link-addr", &ndn_data_unit_ip6_address_s,
        {PRIVATE, NDN_DHCP6_LADDR} },
    { "peer-addr", &ndn_data_unit_ip6_address_s,
        {PRIVATE, NDN_DHCP6_PADDR} },
    { "options",  &ndn_dhcpv6_options_s, {PRIVATE, NDN_DHCP6_OPTIONS} },
};

asn_type ndn_dhcpv6_message_s = {
    "DHCPv6-Message", {PRIVATE, TE_PROTO_DHCP6}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_message_ne_array),
    {_ndn_dhcpv6_message_ne_array}
};

asn_type * ndn_dhcpv6_message = &ndn_dhcpv6_message_s;

asn_enum_entry_t _ndn_dhcp6_mode_enum_entries[] = {
    {"server", DHCP6_CSAP_MODE_SERVER},
    {"client", DHCP6_CSAP_MODE_CLIENT},
};

asn_type ndn_dhcp6_mode_s = {
    "DHCPv6-CSAP-Mode", {PRIVATE, NDN_DHCP6_MODE}, ENUMERATED,
    TE_ARRAY_LEN(_ndn_dhcp6_mode_enum_entries),
    {enum_entries: _ndn_dhcp6_mode_enum_entries}
};

static asn_named_entry_t _ndn_dhcpv6_csap_ne_array[] = {
    { "mode",   &ndn_dhcp6_mode_s, {PRIVATE, NDN_DHCP6_MODE} },
    { "iface",  &asn_base_charstring_s, {PRIVATE, NDN_DHCP6_IFACE} },
};

asn_type ndn_dhcpv6_csap_s = {
    "DHCPv6-CSAP", {PRIVATE, TE_PROTO_DHCP6}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_dhcpv6_csap_ne_array), {_ndn_dhcpv6_csap_ne_array}
};

asn_type * ndn_dhcpv6_csap = &ndn_dhcpv6_csap_s;
