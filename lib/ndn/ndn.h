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
 * Constants for ASN tags of NDN messages
 */
typedef enum {
    NDN_TRAFFIC_TEMPLATE,
    NDN_TRAFFIC_PACKET,
    NDN_TRAFFIC_PATTERN,
    NDN_TRAFFIC_PATTERN_UNIT,
    NDN_CSAP_SPEC,
} ndn_message_tags_t;
/**
 * Constants for ASN tags of protocol choices in PDUs and CSAPs 
 */
typedef enum { 
    NDN_TAD_BRIDGE,
    NDN_TAD_CLI,
    NDN_TAD_DHCP,
    NDN_TAD_ETH,
    NDN_TAD_PCAP,
    NDN_TAD_FILE,
    NDN_TAD_ICMP4,
    NDN_TAD_IP4,
    NDN_TAD_ISCSI,
    NDN_TAD_TCP,
    NDN_TAD_SNMP,
    NDN_TAD_UDP,
} ndn_tad_protocols_t;


/** 
 * ASN.1 tag values for DATA-UNIT choice, see definition of DATA-UNIT
 * macro in ASN module TE-Network-Data-Notation-General 
 * ($TE_BASE/doc/ndn/ndn-gen.asn).
 */
typedef enum { 
    NDN_DU_UNDEF, 
    NDN_DU_PLAIN, 
    NDN_DU_SCRIPT, 
    NDN_DU_ENUM, 
    NDN_DU_MASK, 
    NDN_DU_INTERVALS, 
    NDN_DU_ENV, 
    NDN_DU_FUNC, 
} ndn_data_unit_tags_t;

/**
 * ASN.1 tag values for entries in 'Interval' type.
 */
typedef enum {
    NDN_INTERVALS_BEGIN,
    NDN_INTERVALS_END,
} ndn_intervals_tags_t;

/**
 * ASN.1 tag values for entries in 'DATA-UNIT-mask' type.
 */
typedef enum {
    NDN_MASK_VALUE,
    NDN_MASK_PATTERN,
    NDN_MASK_EXACT_LEN,
} ndn_mask_tags_t;


/**
 * ASN.1 tag values for entries in 'DATA-UNIT-env' type.
 */
typedef enum {
    NDN_ENV_NAME,
    NDN_ENV_TYPE,
} ndn_env_tags_t;


/**
 * ASN.1 tag values for entries in 'Payload' type.
 */ 
typedef enum {
    NDN_PLD_BYTES,
    NDN_PLD_MASK,
    NDN_PLD_FUNC,
    NDN_PLD_FILE,
    NDN_PLD_LEN,
    NDN_PLD_STREAM,
    NDN_PLD_STR_FUNC,
    NDN_PLD_STR_OFF,
    NDN_PLD_STR_LEN,
} ndn_pld_tags_t;

/**
 * ASN.1 tag values for template iterated parameters
 */
typedef enum {
    NDN_FOR_BEGIN,
    NDN_FOR_END,
    NDN_FOR_STEP,
    NDN_ITER_INTS,
    NDN_ITER_INTS_ASSOC,
    NDN_ITER_STRINGS,
    NDN_ITER_FOR,
} ndn_tmpl_iter_tags_t;

/**
 * ASN.1 tag values for entries in 'Traffic-Template' type.
 */ 
typedef enum {
    NDN_TMPL_ARGS,
    NDN_TMPL_DELAYS,
    NDN_TMPL_PDUS,
    NDN_TMPL_PAYLOAD,
} ndn_traffic_template_tags_t;

/**
 * ASN.1 tag values for entries in 'Packet-Action' type.
 */ 
typedef enum {
    NDN_ACT_ECHO,
    NDN_ACT_FORWARD,
    NDN_ACT_FUNCTION,
    NDN_ACT_FILE,
} ndn_packet_action_tags_t;


/**
 * ASN.1 tag values for entries in 'Pattern-Unit'' type.
 */ 
typedef enum {
    NDN_PU_PDUS,
    NDN_PU_PAYLOAD,
    NDN_PU_ACTION,
} ndn_pattern_unit_tags_t;

/**
 * ASN.1 tag values for entries in 'NDN-TimeStamp' type.
 */ 
typedef enum {
    NDN_TIME_SEC,
    NDN_TIME_MCS,
} ndn_timestamp_tags_t;

/**
 * ASN.1 tag values for entries in 'Raw-Packet' type.
 */ 
typedef enum {
    NDN_PKT_TIMESTAMP,
    NDN_PKT_PDUS,
    NDN_PKT_PAYLOAD,
} ndn_raw_packet_tags_t;



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
extern const asn_type *const ndn_template_params_seq;
extern const asn_type *const ndn_traffic_pattern;
extern const asn_type *const ndn_traffic_pattern_unit;
extern const asn_type *const ndn_raw_packet;

extern const asn_type *const ndn_generic_csap_level;
extern const asn_type *const ndn_generic_pdu; 


/**
 * Write integer "plain" value into specified Data-Unit field of NDS PDU.
 *
 * @param pdu       ASN value with NDN PDU
 * @param tag       tag of Data-Unit field
 * @param value     new value to be written
 *
 * @return status code
 */
extern int ndn_du_write_plain_int(asn_value *pdu, uint16_t tag,
                                  int32_t value);

/**
 * Read integer "plain" value from specified Data-Unit field of NDS PDU.
 *
 * @param pdu       ASN value with NDN PDU
 * @param tag       tag of Data-Unit field
 * @param value     location for read value (OUT)
 *
 * @return status code
 */
extern int ndn_du_read_plain_int(const asn_value *pdu, uint16_t tag,
                                 int32_t *value);

/**
 * Write character string "plain" value into specified Data-Unit field
 * of NDS PDU.
 *
 * @param pdu       ASN value with NDN PDU
 * @param tag       tag of Data-Unit field
 * @param value     new value to be written
 *
 * @return status code
 */
extern int ndn_du_write_plain_string(asn_value *pdu, uint16_t tag,
                                     const char *value);

/**
 * Read character string "plain" value from specified Data-Unit field
 * of NDS PDU.
 * User have to free() got pointer.
 *
 * @param pdu       ASN value with NDN PDU
 * @param tag       tag of Data-Unit field
 * @param value     place for pointer to read string (OUT)
 *
 * @return status code
 */
extern int ndn_du_read_plain_string(const asn_value *pdu, uint16_t tag,
                                    char **value);

/**
 * Write octet string "plain" value into specified Data-Unit field
 * of NDS PDU.
 *
 * @param pdu       ASN value with NDN PDU
 * @param tag       tag of Data-Unit field
 * @param value     new value to be written
 * @param len       length of data to be written
 *
 * @return status code
 */
extern int ndn_du_write_plain_oct(asn_value *pdu, uint16_t tag,
                                  const uint8_t *value, size_t len);

/**
 * Read octet string "plain" value from specified Data-Unit field
 * of NDS PDU.
 *
 * @param pdu       ASN value with NDN PDU
 * @param tag       tag of Data-Unit field
 * @param value     location for read octet string (OUT)
 * @param len       length of buffer/read data (IN/OUT)
 *
 * @return status code
 */
extern int ndn_du_read_plain_oct(const asn_value *pdu, uint16_t tag,
                                 uint8_t *value, size_t *len);

/**
 * Match data with DATA-UNIT pattern.  
 *
 * @param pat           ASN value with pattern PDU
 * @param pkt_pdu       ASN value with parsed packet PDU, may be NULL 
 *                      if parsed packet is not need (OUT)
 * @param data          binary data to be matched
 * @param d_len         length of data packet to be matched, in bytes
 * @param label         textual label of desired field, which should be 
 *                      DATA-UNIT{} type
 *
 * @return zero if matches, errno otherwise.
 */ 
extern int ndn_match_data_units(const asn_value *pat, asn_value *pkt_pdu,
                                uint8_t *data, size_t d_len, 
                                const char *label);

/**
 * Match data with mask pattern.
 *
 * @param mask_pat      ASN value with mask pattern
 * @param data          binary data to be matched
 * @param d_len         length of data packet to be matched, in bytes
 *
 * @return zero if matches, errno otherwise.
 */ 
extern int ndn_match_mask(const asn_value *mask_pat,
                          uint8_t *data, size_t d_len);

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
