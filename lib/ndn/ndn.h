/** @file
 * @brief ASN.1 library
 *
 * Declarations of general NDN ASN.1 types
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */ 
#ifndef __TE_NDN_GENEREIC_H__
#define __TE_NDN_GENEREIC_H__

/* for 'struct timeval' */
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Constants for ASN tags of protocol choices in PDUs and CSAPs 
 */
typedef enum { 
    NDN_TAD_BRIDGE,
    NDN_TAD_CLI,
    NDN_TAD_DHCP,
    NDN_TAD_ETH,
    NDN_TAD_FILE,
    NDN_TAD_ICMP4,
    NDN_TAD_IP4,
    NDN_TAD_TCP,
    NDN_TAD_SNMP,
    NDN_TAD_UDP,
} ndn_tad_protocols_t;


/** 
 * ASN tag values for DATA-UNIT choice, see definition of DATA-UNIT
 * macro in ASN module TE-Network-Data-Notation-General 
 * ($TE_BASE/doc/ndn/ndn-gen.asn).
 */
typedef enum { 
    NDN_DU_PLAIN = 1, 
    NDN_DU_SCRIPT, 
    NDN_DU_ENUM, 
    NDN_DU_MASK, 
    NDN_DU_INTERVALS, 
    NDN_DU_ENV, 
    NDN_DU_FUNC, 
} ndn_data_unit_tags_t;



/* Generic useful ASN types */
extern const asn_type *const asn_base_int4;
extern const asn_type *const asn_base_int8;
extern const asn_type *const asn_base_int16;


/* NDN ASN types. */
    /* DATA-UNIT wrappers over respecive bit integer fields */
extern const asn_type *const ndn_data_unit_int4; 
extern const asn_type *const ndn_data_unit_int5;
extern const asn_type *const ndn_data_unit_int8;
extern const asn_type *const ndn_data_unit_int16;
extern const asn_type *const ndn_data_unit_int16;
extern const asn_type *const ndn_data_unit_int32;

    /* DATA-UNIT(OCTET STRING) */
extern const asn_type *const ndn_data_unit_octet_string;
    /* DATA-UNIT(OCTET STRING SIZE 6) , for MAC addresses */
extern const asn_type *const ndn_data_unit_octet_string6;

extern const asn_type *const ndn_data_unit_char_string;
extern const asn_type *const ndn_data_unit_objid;
extern const asn_type *const ndn_ip_address;
extern const asn_type *const ndn_octet_string6; 
extern const asn_type *const ndn_generic_pdu_sequence;
extern const asn_type *const ndn_payload;
extern const asn_type *const ndn_interval;
extern const asn_type *const ndn_interval_sequence;
extern const asn_type *const ndn_csap_spec;
extern const asn_type *const ndn_traffic_template;
extern const asn_type *const ndn_template_parameter;
extern const asn_type *const ndn_traffic_pattern;
extern const asn_type *const ndn_traffic_pattern_unit;
extern const asn_type *const ndn_raw_packet;

extern const asn_type *const ndn_generic_csap_level;
extern const asn_type *const ndn_generic_pdu; 


/**
 * Match data with DATA-UNIT pattern.  
 *
 * @param pat           ASN value with pattern PDU. 
 * @param pkt_pdu       ASN value with parsed packet PDU, may be NULL 
 *                      if parsed packet is not need (OUT). 
 * @param data          binary data to be matched.
 * @param d_len         length of data packet to be matched, in bytes. 
 * @param label         textual label of desired field, which should be 
 *                      DATA-UNIT{}type.
 *
 * @return zero if matches, errno otherwise.
 */ 
extern int ndn_match_data_units(const asn_value *pat, asn_value *pkt_pdu,
                                uint8_t *data, size_t d_len, 
                                const char *label);

/**
 * Get timestamp from recieved Raw-Packet
 *
 * @param packet ASN-value of Raw-Packet type.
 * @param ts     location for timestamp (OUT).
 *
 * return zero on success or appropriate error code otherwise.
 */
extern int ndn_get_timestamp(const asn_value *packet, struct timeval *ts);


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /*  __TE_NDN_GENEREIC_H__*/
