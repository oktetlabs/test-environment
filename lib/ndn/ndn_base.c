/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief NDN for base ASN types
 *
 * Definitions of ASN.1 types for NDN for ATM (ITU-T I.361, I.363.5;
 * RFC 2684).
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#include "te_config.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_base.h"


/* Auxiliary types for commitable items */
const asn_type ndn_base_boolean_s =
{ "BOOLEAN",           {UNIVERSAL, 1}, BOOL, 0, {NULL} };
const asn_type  ndn_base_sint8_s =
{ "INTEGER (0..255)",   {UNIVERSAL, 2}, INTEGER, 8, {0}};
const asn_type  ndn_base_uint8_s =
{ "INTEGER (0..255)",   {UNIVERSAL, 2}, INTEGER, 8, {0}};
const asn_type  ndn_base_sint16_s =
{ "INTEGER (0..65535)",   {UNIVERSAL, 2}, INTEGER, 16, {0}};
const asn_type  ndn_base_uint16_s =
{ "INTEGER (0..65535)",   {UNIVERSAL, 2}, INTEGER, 16, {0}};
const asn_type  ndn_base_sint32_s =
{ "INTEGER (0..4294967295)",   {UNIVERSAL, 2}, INTEGER, 32, {0}};
const asn_type  ndn_base_uint32_s =
{ "INTEGER (0..4294967295)",   {UNIVERSAL, 2}, INTEGER, 32, {0}};
const asn_type ndn_base_octstring_s =
{ "OCTET STRING",      {UNIVERSAL, 4}, OCT_STRING,  0, {NULL} };
const asn_type ndn_base_charstring_s =
{ "UniversalString",   {UNIVERSAL,28}, CHAR_STRING, 0, {NULL} };

/* Auxiliary types for non-commitable items */
const asn_type ndn_base_boolean_ro_s =
{ "BOOLEAN",           {UNIVERSAL, 1}, BOOL, 0, {NULL} };
const asn_type  ndn_base_sint8_ro_s =
{ "INTEGER (0..255)",   {UNIVERSAL, 2}, INTEGER, 8, {0}};
const asn_type  ndn_base_uint8_ro_s =
{ "INTEGER (0..255)",   {UNIVERSAL, 2}, INTEGER, 8, {0}};
const asn_type  ndn_base_sint16_ro_s =
{ "INTEGER (0..65535)",   {UNIVERSAL, 2}, INTEGER, 16, {0}};
const asn_type  ndn_base_uint16_ro_s =
{ "INTEGER (0..65535)",   {UNIVERSAL, 2}, INTEGER, 16, {0}};
const asn_type  ndn_base_sint32_ro_s =
{ "INTEGER (0..4294967295)",   {UNIVERSAL, 2}, INTEGER, 32, {0}};
const asn_type  ndn_base_uint32_ro_s =
{ "INTEGER (0..4294967295)",   {UNIVERSAL, 2}, INTEGER, 32, {0}};
const asn_type ndn_base_octstring_ro_s =
{ "OCTET STRING",      {UNIVERSAL, 4}, OCT_STRING,  0, {NULL} };
const asn_type ndn_base_charstring_ro_s =
{ "UniversalString",   {UNIVERSAL,28}, CHAR_STRING, 0, {NULL} };

/* Types to be referred outside asn_platform.c */
const asn_type * const ndn_base_boolean = NDN_BASE_BOOLEAN;
const asn_type * const ndn_base_s8      = NDN_BASE_S8;
const asn_type * const ndn_base_u8      = NDN_BASE_U8;
const asn_type * const ndn_base_s16     = NDN_BASE_S16;
const asn_type * const ndn_base_u16     = NDN_BASE_U16;
const asn_type * const ndn_base_s32     = NDN_BASE_S32;
const asn_type * const ndn_base_u32     = NDN_BASE_U32;
#if 0
const asn_type * const ndn_base_s64;
const asn_type * const ndn_base_u64;
#endif
const asn_type * const ndn_base_string  = NDN_BASE_STRING;
const asn_type * const ndn_base_octets  = NDN_BASE_OCTETS;
const asn_type * const ndn_base_link    = NDN_BASE_LINK;

const asn_type * const ndn_base_ro_boolean = NDN_BASE_RO_BOOLEAN;
const asn_type * const ndn_base_ro_s8      = NDN_BASE_RO_S8;
const asn_type * const ndn_base_ro_u8      = NDN_BASE_RO_U8;
const asn_type * const ndn_base_ro_s16     = NDN_BASE_RO_S16;
const asn_type * const ndn_base_ro_u16     = NDN_BASE_RO_U16;
const asn_type * const ndn_base_ro_s32     = NDN_BASE_RO_S32;
const asn_type * const ndn_base_ro_u32     = NDN_BASE_RO_U32;
#if 0
const asn_type * const ndn_base_ro_s64;
const asn_type * const ndn_base_ro_u64;
#endif
const asn_type * const ndn_base_ro_string  = NDN_BASE_RO_STRING;
const asn_type * const ndn_base_ro_octets  = NDN_BASE_RO_OCTETS;
const asn_type * const ndn_base_ro_link    = NDN_BASE_RO_LINK;

