/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for IEEE Std 802.2 LLC protocol. 
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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

#ifndef __TE_NDN_LLC_H__
#define __TE_NDN_LLC_H__

#include "te_stdint.h"
#include "te_ethernet.h"
#include "asn_usr.h"
#include "ndn.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {

    NDN_TAG_LLC_DSAP_IG,
    NDN_TAG_LLC_DSAP,
    NDN_TAG_LLC_SSAP_CR,
    NDN_TAG_LLC_SSAP,
    NDN_TAG_LLC_CTL,

    NDN_TAG_LLC_SNAP_HEADER,
    NDN_TAG_SNAP_OUI,
    NDN_TAG_SNAP_PID,

} ndn_llc_tags_t;

extern const asn_type * const ndn_snap_header;
extern const asn_type * const ndn_llc_header;

extern asn_type ndn_snap_header_s;
extern asn_type ndn_llc_header_s;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_LLC_H__ */
