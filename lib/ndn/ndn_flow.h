/** @file
 * @brief Traffic flow processing, NDN.
 *
 * Definitions of ASN.1 types for NDN for traffic flow processing.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#ifndef __TE_NDN_FLOW_H__
#define __TE_NDN_FLOW_H__

#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn.h"


#ifdef __cplusplus
extern "C" {
#endif

extern const asn_type * const ndn_flow_ep;
extern const asn_type * const ndn_flow_pdu;
extern const asn_type * const ndn_flow_traffic;
extern const asn_type * const ndn_flow;

extern asn_type ndn_flow_ep_s;
extern asn_type ndn_flow_pdu_s;
extern asn_type ndn_flow_traffic_s;
extern asn_type ndn_flow_s;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_ETH_H__ */
