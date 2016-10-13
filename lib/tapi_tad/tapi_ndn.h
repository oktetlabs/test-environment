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
 * $Id$
 */ 

#ifndef __TE_TAPI_NDN_H__
#define __TE_TAPI_NDN_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "asn_usr.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Flags used to designate transformations which take place in hardware */
#define SEND_COND_HW_OFFL_IP_CKCUM  (1U << 0)
#define SEND_COND_HW_OFFL_L4_CKSUM  (1U << 1)
#define SEND_COND_HW_OFFL_TSO       (1U << 2)

typedef struct send_transform {
    unsigned int                    hw_flags;

    uint16_t                        tso_segsz;
} send_transform;

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
 * @param obj_spec      Location of ASN.1 value with Template of pattern spec
 *                      (if NULL pointer is stored in location, a new
 *                      CSAP specification is initialized)
 * @param is_pattern    Flag wheather required NDN is traffic pattern
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

/**
 * Add a new unit in the traffic pattern specification.
 *
 * @param obj_spec      Location of ASN.1 value with Template of pattern spec
 *                      (if NULL pointer is stored in location, a new
 *                      CSAP specification is initialized)
 * @param unit_spec     Location for a pointer to a new pattern unit or
 *                      NULL
 *
 * @return Status code.
 */
extern te_errno tapi_tad_new_ptrn_unit(asn_value **obj_spec,
                                       asn_value **unit_spec);

/**
 * Set payload of the last unit in the traffic template or pattern
 * specification.
 *
 * @param obj_spec      Location of ASN.1 value with Template of Pattern
 *                      specification (if NULL pointer is stored in
 *                      location, a new one is initialized)
 * @param is_pattern    Flag whether required NDN is traffic pattern or
 *                      template
 * @param payload       Pointer to payload data
 * @param length        Payload length
 *
 * @note If @a payload is @c NULL and @a length is not @c 0, random
 *       payload contents is generated on sending and any payload of
 *       specified length is matched.
 *
 * @return Status code.
 */
extern te_errno tapi_tad_tmpl_ptrn_set_payload_plain(
                    asn_value  **obj_spec,
                    te_bool      is_pattern,
                    const void  *payload,
                    size_t       length);

/**
 * Convert an ASN.1 template to a pattern containing
 * the same set of PDUs as ones in the template
 *
 * @param template      ASN.1 template to be converted into a pattern
 *
 * @return ASN.1 value containing a pattern or @c NULL
 */
extern asn_value *tapi_tad_mk_pattern_from_template(asn_value *template);

/**
 * Convert an array of ASN.1 'ndn_raw_packet'-s to a unified
 * ASN.1 'ndn_traffic_pattern' carrying an exact sequence of
 * 'ndn_traffic_pattern_unit'-s applicable to match definite
 * packets which are to be received by the peer side in case
 * if the initial 'ndn_raw_packet'-s are sent and (possibly)
 * undergo some transformations (eg, HW offloads are active)
 *
 * @note The given set of possible transformations is only
 *       considered with respect to all 'ndn_raw_packet'-s
 *       in the array, i.e., some individual peculiarities
 *       of the items cannot be taken into account, hence,
 *       if one needs to process any of individual packets
 *       independently, separate calls should be performed
 *       (eg, if SEND_COND_HW_OFFL_TSO flag is present, it
 *       means that TSO shall be done for all the packets)
 *
 * @param packets      An ASN.1 'ndn_raw_packet'-s to be processed
 * @param n_packets    The number of items available within @p packets
 * @param transform    A set of parameters describing some trasformations
 *                     which are expected to affect the outgoing packets
 * @param pattern_out  Location for the pattern which is to be produced
 *
 * @return Status code
 */
extern te_errno tapi_tad_packets_to_pattern(asn_value         **packets,
                                            unsigned int        n_packets,
                                            send_transform     *transform,
                                            asn_value         **pattern_out);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAPI_NDN_H__ */
