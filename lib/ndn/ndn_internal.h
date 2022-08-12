/** @file
 * @brief ASN.1 library
 *
 * Definitions of internal useful macros for general NDN ASN.1 types
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 */

#ifndef __TE_NDN_INTERNAL_H__
#define __TE_NDN_INTERNAL_H__

#include "asn_usr.h"
#include "asn_impl.h"


/**
 * Macro for definition of C structures, presenting
 * DATA-UNIT based ASN types.
 *
 * @param id            suffix, used to constract global constant names
 * @param asn_t         base ASN type, constant of asn_type type
 * @param asn_bt_txt_id textual name of base ASN type
 */
#define NDN_DATA_UNIT_TYPE(id, asn_t, asn_bt_txt_id)                      \
    static asn_named_entry_t _ndn_data_unit_##id##_ne_array [] =          \
    {                                                                     \
      {"plain",     &asn_t,                 {PRIVATE, NDN_DU_PLAIN} },    \
      {"script",    &asn_base_charstring_s, {PRIVATE, NDN_DU_SCRIPT} },   \
      {"enum",      &ndn_data_unit_enum_s,  {PRIVATE, NDN_DU_ENUM} },     \
      {"mask",      &ndn_data_unit_mask_s,  {PRIVATE, NDN_DU_MASK} },     \
      {"intervals", &ndn_data_unit_ints_s,  {PRIVATE, NDN_DU_INTERVALS} },\
      {"env",       &ndn_data_unit_env_s,   {PRIVATE, NDN_DU_ENV} },      \
      {"function",  &asn_base_charstring_s, {PRIVATE, NDN_DU_FUNC} },     \
    };                                                                    \
    asn_type ndn_data_unit_##id##_s =                                     \
    {                                                                     \
      "DATA-UNIT ( " #asn_bt_txt_id " )", {PRIVATE, 1},                   \
      CHOICE, sizeof(_ndn_data_unit_##id##_ne_array)/                     \
              sizeof(_ndn_data_unit_##id##_ne_array[0]),                  \
      {_ndn_data_unit_##id##_ne_array}                                    \
    };                                                                    \
    const asn_type * const ndn_data_unit_##id = &ndn_data_unit_##id##_s

/**
 * Macro for definition of C structures, presenting
 * DATA-UNIT-range item and DATA-UNIT with it based ASN types.
 *
 * @param id            suffix, used to constract global constant names
 * @param asn_t         base ASN type, constant of asn_type type
 */
#define NDN_DATA_UNIT_WITH_RANGE_TYPE(id, asn_t, asn_bt_txt_id)                   \
    static asn_named_entry_t _ndn_data_unit_##id##_range_ne_array [] =            \
    {                                                                             \
      {"first",   &asn_t, {PRIVATE, NDN_RANGE_FIRST} },                           \
      {"last",    &asn_t, {PRIVATE, NDN_RANGE_LAST} },                            \
      {"mask",    &asn_t, {PRIVATE, NDN_RANGE_MASK} },                            \
    };                                                                            \
    asn_type ndn_data_unit_##id##_range_s =                                       \
    {                                                                             \
      "DATA-UNIT-range", {PRIVATE, NDN_DU_RANGE}, SEQUENCE,                       \
      sizeof(_ndn_data_unit_##id##_range_ne_array)/                               \
      sizeof(_ndn_data_unit_##id##_range_ne_array[0]),                            \
      {_ndn_data_unit_##id##_range_ne_array}                                      \
    };                                                                            \
    static asn_named_entry_t _ndn_data_unit_##id##_ne_array [] =                  \
    {                                                                             \
      {"plain",     &asn_t,                         {PRIVATE, NDN_DU_PLAIN} },    \
      {"script",    &asn_base_charstring_s,         {PRIVATE, NDN_DU_SCRIPT} },   \
      {"enum",      &ndn_data_unit_enum_s,          {PRIVATE, NDN_DU_ENUM} },     \
      {"mask",      &ndn_data_unit_mask_s,          {PRIVATE, NDN_DU_MASK} },     \
      {"intervals", &ndn_data_unit_ints_s,          {PRIVATE, NDN_DU_INTERVALS} },\
      {"env",       &ndn_data_unit_env_s,           {PRIVATE, NDN_DU_ENV} },      \
      {"function",  &asn_base_charstring_s,         {PRIVATE, NDN_DU_FUNC} },     \
      {"range",     &ndn_data_unit_##id##_range_s,  {PRIVATE, NDN_DU_RANGE} },    \
    };                                                                            \
    asn_type ndn_data_unit_##id##_s =                                             \
    {                                                                             \
      "DATA-UNIT ( " #asn_bt_txt_id " )", {PRIVATE, 1},                           \
      CHOICE, sizeof(_ndn_data_unit_##id##_ne_array)/                             \
              sizeof(_ndn_data_unit_##id##_ne_array[0]),                          \
      {_ndn_data_unit_##id##_ne_array}                                            \
    };                                                                            \
    const asn_type * const ndn_data_unit_##id = &ndn_data_unit_##id##_s


extern asn_type ndn_data_unit_ints_s;
extern asn_type ndn_data_unit_enum_s;
extern asn_type ndn_data_unit_mask_s;
extern asn_type ndn_data_unit_env_s;

extern asn_type ndn_data_unit_int1_s;
extern asn_type ndn_data_unit_int2_s;
extern asn_type ndn_data_unit_int3_s;
extern asn_type ndn_data_unit_int4_s;
extern asn_type ndn_data_unit_int5_s;
extern asn_type ndn_data_unit_int6_s;
extern asn_type ndn_data_unit_int7_s;
extern asn_type ndn_data_unit_int8_s;
extern asn_type ndn_data_unit_int9_s;
extern asn_type ndn_data_unit_int12_s;
extern asn_type ndn_data_unit_int16_s;
extern asn_type ndn_data_unit_int24_s;
extern asn_type ndn_data_unit_int32_s;

extern asn_type ndn_data_unit_uint32_s;

extern asn_type ndn_data_unit_octet_string_s;
extern asn_type ndn_data_unit_char_string_s;
extern asn_type ndn_data_unit_objid_s;
extern asn_type ndn_data_unit_ip_address_s;
extern asn_type ndn_data_unit_ip6_address_s;

extern asn_type ndn_ip_address_s;
extern asn_type ndn_octet_string6_s;

extern asn_type ndn_generic_pdu_s;
extern asn_type ndn_generic_csap_layer_s;
extern asn_type ndn_template_parameter_sequence_s;
extern asn_type ndn_generic_pdu_sequence_s;
extern asn_type ndn_traffic_template_s;
extern asn_type ndn_traffic_pattern_s;
extern asn_type ndn_csap_layers_s;
extern asn_type ndn_csap_spec_s;

#endif /* __TE_NDN_INTERNAL_H__ */
