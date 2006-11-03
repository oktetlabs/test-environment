/** @file
 * @brief Test API for TAD. ipstack CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_ICMP4_H__
#define __TE_TAPI_ICMP4_H__

#include "te_defs.h"
#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add ICMPv4 layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 *
 * @retval Status code.
 */
extern te_errno tapi_icmp4_add_csap_layer(asn_value **csap_spec);

/**
 * Add ICMPv4 PDU as the last PDU to the last unit of the traffic 
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param type          Type of ICMP message or negative to keep
 *                      unspecified.
 * @param code          ICMP message code or negative to keep unspecified.
 *
 * @return Status code.
 */
extern te_errno tapi_icmp4_add_pdu(asn_value **tmpl_or_ptrn,
                                   asn_value **pdu,
                                   te_bool     is_pattern,
                                   int         type,
                                   int         code);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_ICMP4_H__ */
