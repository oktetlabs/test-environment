/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief ASN.1 library
 *
 * Declarations of general NDN ASN.1 types
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
    NDN_CSAP_LAYERS,
    NDN_CSAP_PARAMS,
    NDN_CSAP_RECV_TIMEOUT,
    NDN_CSAP_STOP_LATENCY_TIMEOUT,
} ndn_message_tags_t;


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
    NDN_DU_RANGE,
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
 * ASN.1 tag values for entries in 'DATA-UNIT-range' type.
 */
typedef enum {
    NDN_RANGE_FIRST,
    NDN_RANGE_LAST,
    NDN_RANGE_MASK,
} ndn_range_tags_t;


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
    NDN_TMPL_FUNCTION,
} ndn_traffic_template_tags_t;

/**
 * ASN.1 tag values for entries in 'Packet-Action' type.
 */
typedef enum {
    NDN_ACT_FORWARD_PLD,
    NDN_ACT_FORWARD_RAW,
    NDN_ACT_FUNCTION,
    NDN_ACT_FILE,
    NDN_ACT_BREAK,
    NDN_ACT_NO_REPORT,
} ndn_packet_action_tags_t;


/**
 * ASN.1 tag values for entries in 'Pattern-Unit'' type.
 */
typedef enum {
    NDN_PU_PDUS,
    NDN_PU_PAYLOAD,
    NDN_PU_ACTION,
    NDN_PU_ACTIONS,
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
    NDN_PKT_MATCH_UNIT,
} ndn_raw_packet_tags_t;

typedef enum {
    TE_IP4_UPPER_LAYER_CSUM_CORRECT = 0,
    TE_IP4_UPPER_LAYER_CSUM_ZERO,
    TE_IP4_UPPER_LAYER_CSUM_BAD
} te_ip4_upper_layer_csum_t;

typedef enum {
    TE_IP6_UPPER_LAYER_CSUM_CORRECT = 0,
    TE_IP6_UPPER_LAYER_CSUM_ZERO,
    TE_IP6_UPPER_LAYER_CSUM_BAD
} te_ip6_upper_layer_csum_t;

/* NDN ASN types. */
    /* DATA-UNIT wrappers over respecive bit integer fields */
extern const asn_type *const ndn_data_unit_int4;
extern const asn_type *const ndn_data_unit_int5;
extern const asn_type *const ndn_data_unit_int8;
extern const asn_type *const ndn_data_unit_int16;
extern const asn_type *const ndn_data_unit_int16;
extern const asn_type *const ndn_data_unit_int24;
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
extern const asn_type *const ndn_csap_params;
extern const asn_type *const ndn_csap_layers;
extern const asn_type *const ndn_csap_spec;
extern const asn_type *const ndn_traffic_template;
extern const asn_type *const ndn_template_parameter;
extern const asn_type *const ndn_template_params_seq;
extern const asn_type *const ndn_traffic_pattern;
extern const asn_type *const ndn_traffic_pattern_unit;
extern const asn_type *const ndn_raw_packet;

extern const asn_type *const ndn_generic_csap_layer;
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
                                const uint8_t *data, size_t d_len,
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
                          const uint8_t *data, size_t d_len);

/**
 * Get timestamp from recieved Raw-Packet
 *
 * @param packet ASN-value of Raw-Packet type.
 * @param ts     location for timestamp (OUT).
 *
 * @return zero on success or appropriate error code otherwise.
 */
extern int ndn_get_timestamp(const asn_value *packet, struct timeval *ts);

/**
 * Convert received packet to the template to be sent.
 *
 * @param pkt   ASN value with received packet.
 * @param tmpl  Location for ASN value with template.
 *
 * @return status code.
 */
extern te_errno ndn_packet_to_template(const asn_value *pkt,
                                       asn_value **tmpl);

/**
 * Produce CSAP spec. ID by means of parsing ASN.1 representation
 *
 * @param csap_spec    ASN.1 CSAP spec.
 *
 * @note CSAP spec. ID is allocated from heap and should be freed by the caller
 *
 * @return CSAP spec. ID string or @c NULL on failure
 */
extern char *ndn_csap_stack_by_spec(const asn_value *csap_spec);

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
extern te_errno ndn_init_asn_value(asn_value **value,
                                   const asn_type *type);

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
extern te_errno ndn_csap_add_layer(asn_value       **csap_spec,
                                   const asn_type   *layer_type,
                                   const char       *layer_choice,
                                   asn_value       **layer_spec);

/**
 * Create empty CSAP spec by template.
 * User have to asn_free_value() got pointer.
 *
 * @param tmpl  Location for ASN value with template.
 *
 * @return CSAP spec on success, otherwise @b NULL is returned on error.
 */
extern asn_value * ndn_csap_spec_by_traffic_template(const asn_value *tmpl);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /*  __TE_NDN_GENEREIC_H__*/
