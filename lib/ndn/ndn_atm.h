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


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    NDN_TAG_ATM_HW_TYPE,
    NDN_TAG_ATM_PROTO,
    NDN_TAG_ATM_HW_SIZE,
    NDN_TAG_ATM_PROTO_SIZE,
    NDN_TAG_ATM_OPCODE,
    NDN_TAG_ATM_SND_HW_ADDR,
    NDN_TAG_ATM_SND_PROTO_ADDR,
    NDN_TAG_ATM_TGT_HW_ADDR,
    NDN_TAG_ATM_TGT_PROTO_ADDR,
} ndn_atm_tags_t;


extern const asn_type * const ndn_atm_header;
extern const asn_type * const ndn_atm_csap;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_ATM_H__ */
