/** @file
 * @brief TAPI NDN
 *
 * Declarations of API for TAPI NDN.
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
 * $Id: tapi_arp.h 20428 2005-11-01 18:54:10Z arybchik $
 */ 

#ifndef __TE_TAPI_NDN_H__
#define __TE_TAPI_NDN_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "asn_usr.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check ASN.1 value pointer. If it is NULL, initialize a new value of
 * specified type. All errors are logged inside the function. There is
 * no necessity to log them after call.
 *
 * @param value     Location of ASN.1 value pointer
 * @param type      ASN.1 type to which value should belong
 *
 * @return Status code.
 *
 * @todo Check that ASN.1 value belongs to @a type
 */
extern te_errno tapi_tad_init_asn_value(asn_value      **value, 
                                        const asn_type  *type);

/**
 * Add a new CSAP specification layer.
 *
 * @param csap_spec     Location of ASN.1 value with CSAP specification
 *                      (if NULL pointer is stored in location, a new
 *                      CSAP specification is initialized)
 * @param layer_type    ASN.1 type of a new layer
 * @param layer_choice  String name of a new layer as ASN.1 choice
 *                      (including '#', e.g. "#eth")
 * @param layer_spec    Location for a new ASN.1 value with layer
 *                      specification (may be NULL)
 *
 * @return Status code.
 */
extern te_errno tapi_tad_csap_add_layer(asn_value       **csap_spec,
                                        const asn_type   *layer_type,
                                        const char       *layer_choice,
                                        asn_value       **layer_spec);

/**
 * Add a new layer specification in traffic template/pattern.
 *
 * @param csap_spec     Location of ASN.1 value with CSAP specification
 *                      (if NULL pointer is stored in location, a new
 *                      CSAP specification is initialized)
 * @param pdu_type      ASN.1 type of a new PDU
 * @param pdu_choice    String name of a new PDU as ASN.1 choice
 *                      (including '#', e.g. "#eth")
 * @param pdu_spec      Location for a new ASN.1 value with PDU
 *                      specification (may be NULL)
 *
 * @return Status code.
 */
extern te_errno tapi_tad_tmpl_ptrn_add_layer(asn_value       **obj_spec,
                                             te_bool           is_pattern,
                                             const asn_type   *pdu_type,
                                             const char       *pdu_choice,
                                             asn_value       **pdu_spec);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAPI_NDN_H__ */
