/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for IGMPv2 protocol. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Alexander.Kukuta <kam@oktetlabs.ru>
 *
 * $Id: $
 */ 
#ifndef __TE_NDN_IGMP2_H__
#define __TE_NDN_IGMP2_H__


#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASN.1 tags for IGMPv2 PDUs
 */
typedef enum {
    NDN_IGMP2_VERSION,
    NDN_IGMP2_TYPE,
    NDN_IGMP2_MAX_RESPONSE_TIME,
    NDN_IGMP2_CHECKSUM,
    NDN_IGMP2_GROUP_ADDRESS,
    NDN_IGMP2_,
} ndn_igmp2_tags_t;

typedef enum {
    NDN_IGMP2_TYPE_QUERY = 0x11,     /**< Group Query message */
    NDN_IGMP2_TYPE_REPORT_V1 = 0x12, /**< IGMP Version 1 Membership report */
    NDN_IGMP2_TYPE_JOIN = 0x16,      /**< Membership Report/Join message  */
    NDN_IGMP2_TYPE_LEAVE = 0x16,     /**< Group Leave message */
} ndn_igmp2_type_t;


extern const asn_type * const ndn_igmp2_message;
extern const asn_type * const ndn_igmp2_csap;

extern asn_type ndn_igmp2_message_s;
extern asn_type ndn_igmp2_csap_s;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_IGMP2_H__ */
