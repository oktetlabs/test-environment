/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for PPP&PPPoE protocols.
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE_NDN_PPP_H__
#define __TE_NDN_PPP_H__

#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASN.1 tags for PPP CSAP NDN
 */
typedef enum {
    NDN_TAG_PPP_PROTOCOL,
    NDN_TAG_PPP_PKT,
    NDN_TAG_PPPOE_VERSION,
    NDN_TAG_PPPOE_TYPE,
    NDN_TAG_PPPOE_CODE,
    NDN_TAG_PPPOE_SESSION_ID,
    NDN_TAG_PPPOE_LENGTH,
    NDN_TAG_PPPOE_PAYLOAD,
    NDN_TAG_PPPOE_PKT,
} ndn_ppp_tags_t;

extern asn_type *ndn_pppoe_pkt;
extern asn_type *ndn_pppoe_csap;

extern asn_type *ndn_ppp_pkt;
extern asn_type *ndn_ppp_csap;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_PPP_H__ */
