/** @file
 * @brief NDN for base ASN types
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
 * @author Igor Labutin <Igor.Labutin@oktetlabs.ru>
 *
 * $Id: $
 */
#ifndef __TE_NDN_BASE_H__
#define __TE_NDN_BASE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "asn_usr.h"
#include "ndn.h"

/* Commitable types macro to be used in NDN specification */
#define NDN_BASE_BOOLEAN &ndn_base_boolean_s
#define NDN_BASE_S8      &ndn_base_sint8_s
#define NDN_BASE_U8      &ndn_base_uint8_s
#define NDN_BASE_S16     &ndn_base_sint16_s
#define NDN_BASE_U16     &ndn_base_uint16_s
#define NDN_BASE_S32     &ndn_base_sint32_s
#define NDN_BASE_U32     &ndn_base_uint32_s
#define NDN_BASE_S64     /* Not supported now */
#define NDN_BASE_U64     /* Not supported now */
#define NDN_BASE_STRING  &ndn_base_charstring_s
#define NDN_BASE_OCTETS  &ndn_base_octstring_s
#define NDN_BASE_LINK    &ndn_base_charstring_s

/* Non-commitable types macro to be used in NDN specification */
#define NDN_BASE_RO_BOOLEAN &ndn_base_boolean_ro_s
#define NDN_BASE_RO_S8      &ndn_base_sint8_ro_s
#define NDN_BASE_RO_U8      &ndn_base_uint8_ro_s
#define NDN_BASE_RO_S16     &ndn_base_sint16_ro_s
#define NDN_BASE_RO_U16     &ndn_base_uint16_ro_s
#define NDN_BASE_RO_S32     &ndn_base_sint32_ro_s
#define NDN_BASE_RO_U32     &ndn_base_uint32_ro_s
#define NDN_BASE_RO_S64     /* Not supported now */
#define NDN_BASE_RO_U64     /* Not supported now */
#define NDN_BASE_RO_STRING  &ndn_base_charstring_ro_s
#define NDN_BASE_RO_OCTETS  &ndn_base_octstring_ro_s
#define NDN_BASE_RO_LINK    &ndn_base_charstring_ro_s

/* Auxiliary types for commitable items */
extern const asn_type ndn_base_boolean_s;
extern const asn_type ndn_base_sint8_s;
extern const asn_type ndn_base_uint8_s;
extern const asn_type ndn_base_sint16_s;
extern const asn_type ndn_base_uint16_s;
extern const asn_type ndn_base_sint32_s;
extern const asn_type ndn_base_uint32_s;
extern const asn_type ndn_base_octstring_s;
extern const asn_type ndn_base_charstring_s;

/* Auxiliary types for non-commitable items */
extern const asn_type ndn_base_boolean_ro_s;
extern const asn_type ndn_base_sint8_ro_s;
extern const asn_type ndn_base_uint8_ro_s;
extern const asn_type ndn_base_sint16_ro_s;
extern const asn_type ndn_base_uint16_ro_s;
extern const asn_type ndn_base_sint32_ro_s;
extern const asn_type ndn_base_uint32_ro_s;
extern const asn_type ndn_base_octstring_ro_s;
extern const asn_type ndn_base_charstring_ro_s;

/* 
 * Types that correspond to CRM types
 * These types should be used in asn_platform.c
 */
extern const asn_type * const ndn_base_boolean;
extern const asn_type * const ndn_base_s8;
extern const asn_type * const ndn_base_u8;
extern const asn_type * const ndn_base_s16;
extern const asn_type * const ndn_base_u16;
extern const asn_type * const ndn_base_s32;
extern const asn_type * const ndn_base_u32;
#if 0
extern const asn_type * const ndn_base_s64;
extern const asn_type * const ndn_base_u64;
#endif
extern const asn_type * const ndn_base_string;
extern const asn_type * const ndn_base_octets;
extern const asn_type * const ndn_base_link;

extern const asn_type * const ndn_base_ro_boolean;
extern const asn_type * const ndn_base_ro_s8;
extern const asn_type * const ndn_base_ro_u8;
extern const asn_type * const ndn_base_ro_s16;
extern const asn_type * const ndn_base_ro_u16;
extern const asn_type * const ndn_base_ro_s32;
extern const asn_type * const ndn_base_ro_u32;
#if 0
extern const asn_type * const ndn_base_ro_s64;
extern const asn_type * const ndn_base_ro_u64;
#endif
extern const asn_type * const ndn_base_ro_string;
extern const asn_type * const ndn_base_ro_octets;
extern const asn_type * const ndn_base_ro_link;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_BASE_H__ */

