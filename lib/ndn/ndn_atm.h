/** @file
 * @brief ATM NDN
 *
 * Declarations of ASN.1 types for NDN for ATM (ITU-T I.361, I.363.5;
 * RFC 2684).
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifndef __TE_NDN_ATM_H__
#define __TE_NDN_ATM_H__

#if HAVE_NET_IF_ATM_H
#include <net/if_atm.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h"


/** Length of ATM cell */
#define ATM_CELL_LEN        53
/** Length of ATM cell header */
#define ATM_HEADER_LEN      5
/** Length of ATM cell payload */
#define ATM_PAYLOAD_LEN     (ATM_CELL_LEN - ATM_HEADER_LEN)

/** Length of AAL5 CPCS PDU trailer */
#define AAL5_TRAILER_LEN    8


#ifdef __cplusplus
extern "C" {
#endif


/** ATM cell header format */
typedef enum ndn_atm_type {
    NDN_ATM_NNI,    /**< Network-Node Interface */
    NDN_ATM_UNI,    /**< User-Network Interface */
} ndn_atm_type;

/** ASN.1 tags for ATM */
typedef enum {
    NDN_TAG_ATM_TYPE,
    NDN_TAG_ATM_GFC,
    NDN_TAG_ATM_VPI,
    NDN_TAG_ATM_VCI,
    NDN_TAG_ATM_PAYLOAD_TYPE,
    NDN_TAG_ATM_CONGESTION,
    NDN_TAG_ATM_CLP,
    NDN_TAG_ATM_HEC,
    NDN_TAG_ATM_TYPE_ENUM,

    NDN_TAG_AAL5_CPCS_UU,
    NDN_TAG_AAL5_CPI,
    NDN_TAG_AAL5_LENGTH,
    NDN_TAG_AAL5_CRC,
} ndn_atm_tags_t;


extern const asn_type * const ndn_atm_header;
extern const asn_type * const ndn_atm_csap;

extern const asn_type * const ndn_aal5_cpcs_trailer;
extern const asn_type * const ndn_aal5_csap;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_ATM_H__ */
