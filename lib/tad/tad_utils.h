/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Utils
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions, used in common and
 * protocol-specific modules implemnting TAD.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE_TAD_UTILS_H__
#define __TE_TAD_UTILS_H__

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h"
#include "tad_common.h"
#include "rcf_ch_api.h"
#include "agentlib.h"
#include "tad_csap_inst.h"
#include "tad_csap_support.h"

#define CSAP_LOG_FMT            "CSAP %u (0x%x): "
#define CSAP_LOG_ARGS(_csap)    (_csap)->id, (_csap)->state

#ifdef __cplusplus
extern "C" {
#endif

/** Zero timeval */
#define tad_tv_zero             ((struct timeval){ 0, 0 })

/**
 * The value used to verify checksum correctness.
 *
 * When checksum is originally computed, checksum field value
 * is set to zero. If we recompute it with checksum field value
 * set to one's complement of the original sum, we get
 *
 * sum + ~sum = 0xffff
 *
 * and the final checksum value is one's complement of this sum,
 * i.e. 0x0.
 *
 * This also works for the special case of UDP where 0 value
 * (one's complement of 0xffff sum) is replaced with 0xffff
 * (see RFC 768):
 *
 * 0xffff + 0xffff = 0x1fffe -> 0xffff
 *
 * Here a carry to 17th bit occurs and 1 is added to 16bit 0xfffe in one's
 * complement sum computation.
 */
#define CKSUM_CMP_CORRECT 0x0

/** 'string' choice value to denote that correct checksums will match only */
#define TAD_CKSUM_STR_VAL_CORRECT   "correct"

/**
 * 'string' choice value to denote that either correct or zero
 * checksums match only
 */
#define TAD_CKSUM_STR_VAL_CORRECT_OR_ZERO "correct-or-zero"

/** 'string' choice value to denote that incorrect checksums will match only */
#define TAD_CKSUM_STR_VAL_INCORRECT "incorrect"

/** 'string' choice numeric designations */
typedef enum {
    TAD_CKSUM_STR_CODE_NONE = 0,
    TAD_CKSUM_STR_CODE_CORRECT,
    TAD_CKSUM_STR_CODE_CORRECT_OR_ZERO,
    TAD_CKSUM_STR_CODE_INCORRECT,
} tad_cksum_str_code;

/**
 * Convert text protocol label into integer tag.
 *
 * @param proto_txt     string with protocol text label
 *
 * @return value from enum 'te_tad_protocols_t'
 */
extern te_tad_protocols_t te_proto_from_str(const char *proto_txt);

/**
 * Convert protocol integer tag into text protocol label.
 *
 * @param proto  value from enum 'te_tad_protocols_t'
 *
 * @return constant string with protocol text label or NULL
 */
extern const char *te_proto_to_str(te_tad_protocols_t proto);

/**
 * Convert ASN specification of payload from Traffic-Template
 * to plain C sturcture.
 *
 * @param[in]  ndn_payload  ASN value of Payload type
 * @param[out] pld_spec     locaion for converted data
 *
 * @return Status code.
 */
extern te_errno tad_convert_payload(const asn_value *ndn_payload,
                                    tad_payload_spec_t *pld_spec);

/**
 * Clear payload specification, free all data, allocated for it
 * internally.
 * Does NOT free passed struct itself.
 *
 * @param pld_spec      pointer to payload spec to be cleared
 */
extern void tad_payload_spec_clear(tad_payload_spec_t *pld_spec);

/**
 * Parse textual presentation of expression. Syntax is Perl-like, references
 * to template arguments noted as $1, $2, etc.
 *
 * @param string        Text with expression.
 * @param expr          Place where pointer to new expression will
 *                      be put (OUT).
 * @param syms          Quantity of parsed symbols, if syntax error occured,
 *                      error position (OUT).
 *
 * @return status code.
 */
extern int tad_int_expr_parse(const char *string,
                              tad_int_expr_t **expr, int *syms);

/**
 * Initialize tad_int_expr_t structure with single constant value, storing
 * binary array up to 8 bytes length. Array is assumed as "network byte
 * order" and converted to "host byte order" while saveing in 64-bit
 * integer.
 *
 * @param arr   pointer to binary array.
 * @param len   length of array.
 *
 * @return  pointer to new tad_int_expr_r structure, or NULL if no memory
 *          or too beg length passed.
 */
extern tad_int_expr_t *tad_int_expr_constant_arr(uint8_t *arr, size_t len);

/**
 * Initialize tad_int_expr_t structure with single constant value.
 *
 * @param n     Value.
 *
 * @return pointer to new tad_int_expr_r structure.
 */
extern tad_int_expr_t *tad_int_expr_constant(int64_t n);

/**
 * Free data allocated for expression.
 */
extern void tad_int_expr_free(tad_int_expr_t *expr);

/**
 * Calculate value of expression as function of argument set
 *
 * @param expr          Expression structure.
 * @param args          Array with arguments.
 * @param num_args      Length of arguments array.
 * @param result        Location for result (OUT).
 *
 * @return status code.
 */
extern int tad_int_expr_calculate(const tad_int_expr_t *expr,
                                  const struct tad_tmpl_arg_t *args,
                                  size_t num_args,
                                  int64_t *result);

/**
 * Convert 64-bit integer from network order to the host and vise versa.
 *
 * @param n     Integer to be converted.
 *
 * @return converted integer
 */
extern uint64_t tad_ntohll(uint64_t n);

/**
 * Init argument iteration array template arguments specification.
 * Memory block for array assumed to be allocated.
 *
 * @param arg_specs     Array of template argument specifications.
 * @param arg_specs_num Length of array.
 * @param arg_iterated  Array of template arguments (OUT).
 *
 * @retval      - positive on success itertaion.
 * @retval      - zero if iteration finished.
 * @retval      - negative if invalid arguments passed.
 */
extern int tad_init_tmpl_args(tad_tmpl_iter_spec_t *arg_specs,
                              size_t arg_specs_num,
                              struct tad_tmpl_arg_t *arg_iterated);

/**
 * Perform next iteration for passed template arguments.
 *
 * @param arg_specs     Array of template argument specifications.
 * @param arg_specs_num Length of array.
 * @param arg_iterated  Array of template arguments (OUT).
 *
 * @retval      - positive on success itertaion.
 * @retval      - zero if iteration finished.
 * @retval      - negative if invalid arguments passed.
 */
extern int tad_iterate_tmpl_args(tad_tmpl_iter_spec_t *arg_specs,
                                 size_t arg_specs_num,
                                 struct tad_tmpl_arg_t *arg_iterated);

/**
 * Clear internal data in template iterate args specs
 *
 * @return nothing.
 */
extern void tad_tmpl_args_clear(tad_tmpl_iter_spec_t *arg_specs,
                                unsigned int arg_num);

/**
 * Get argument set from template ASN value and put it into plain-C array
 *
 * @param arg_set       ASN value of type "SEQENCE OF Template-Parameter",
 *                      which is subvalue with label 'arg-sets' in
 *                      Traffic-Template.
 * @param arg_specs     Memory block for arg_spec array. should
 *                      be allocated by user.
 * @param arg_num       Length of arg_spec array, i.e. quantity of
 *                      Template-Arguments in template.
 *
 * @return zero on success, otherwise error code.
 */
extern int tad_get_tmpl_arg_specs(const asn_value *arg_set,
                                  tad_tmpl_iter_spec_t *arg_specs,
                                  size_t arg_num);

/**
 * Convert DATA-UNIT ASN field of PDU to plain C structure.
 * Memory need to store dynamic data, is allocated in this method and
 * should be freed by user.
 *
 * @param[in]  pdu_val  ASN value with pdu, which DATA-UNIT field
 *                      should be converted.
 * @param[in]  label    Textual label of DATA_UNIT field in PDU.
 * @param[out] location Location for converted structure.
 *
 * @return zero on success or error code.
 */
extern int tad_data_unit_convert_by_label(const asn_value *pdu_val,
                                          const char      *label,
                                          tad_data_unit_t *location);

/**
 * Convert DATA-UNIT ASN field of PDU to plain C structure.
 * Uses tad_data_unit_convert_simple' method, see notes to it.
 *
 * @param[in]  pdu_val      ASN value with pdu, which DATA-UNIT field
 *                          should be converted
 * @param[in]  tag_value    ASN tag value of field, tag class is
 *                          assumed to be PRIVATE
 * @param[out] location     location for converted structure, should not
 *                          contain any 'data staff' - i.e. should be
 *                          correctly filled or zeroed
 *
 * @return zero on success or error code.
 */
extern int tad_data_unit_convert(const asn_value *pdu_val,
                                 asn_tag_value    tag_value,
                                 tad_data_unit_t *location);

/**
 * Convert DATA-UNIT ASN value to plain C structure.
 * Process only template (traffic-generating related) choices
 * in DATA-UNIT.
 * Memory need to store dynamic data, is allocated in this method and
 * should be freed with 'tad_data_unit_clear'.
 * Template specifications, which was stored in passed location
 * before the call of this method, will be cleared.
 *
 * @param[in]  du_field ASN value of DATA-UNIT type to  be converted
 * @param[out] location Location for converted structure, should not
 *                      contain any 'data staff' - i.e. should be
 *                      correctly filled or zeroed
 *
 * @return zero on success or error code.
 */
extern int tad_data_unit_convert_simple(const asn_value *du_field,
                                        tad_data_unit_t *location);

/**
 * Constract data-unit structure from specified binary data for
 * simple per-byte compare.
 *
 * @param[in]  data     Binary data which should be compared
 * @param[in]  d_len    Length of data
 * @param[out] location Location of data-unit structure to be
 *                      initialized
 *
 * @return error status.
 */
extern int tad_data_unit_from_bin(const uint8_t *data, size_t d_len,
                                  tad_data_unit_t *location);

/**
 * Clear data_unit structure, e.g. free data allocated for internal usage.
 * Memory block used by data_unit itself is not freed!
 *
 * @param du    Pointer to data_unit structure to be cleared.
 */
extern void tad_data_unit_clear(tad_data_unit_t *du);

/**
 * Generic method to match data in incoming packet with DATA_UNIT pattern
 * field. If data matches, it will be written into respective field in
 * pkt_pdu.
 * Label of field should be provided by user. If pattern has "plain"
 * type, data will be simply compared. If it is integer, data will be
 * converted from "network byte order" to "host byte order" before matching.
 *
 * @param pattern       Pattern structure.
 * @param pkt_pdu       ASN value with parsed packet PDU (OUT).
 * @param data          Binary data to be matched.
 * @param d_len         Length of data packet to be matched, in bytes.
 * @param label         Textual label of desired field.
 *
 * @return zero on success (that means "data matches to the pattern"),
 *              otherwise error code.
 *
 * This function is depricated, and leaved here only for easy backward
 * rollbacks. Use 'ndn_match_data_units' instead.
 * This function will be removed just when 'ndn_match_data_units' will
 * be completed and debugged.
 */
extern int tad_univ_match_field(const tad_data_unit_t *pattern,
                                asn_value *pkt_pdu, uint8_t *data,
                                size_t d_len, const char *label);

/**
 * Put binary data into specified place according with data unit,
 * already converted into plain C structure. All integers put in network
 * byte order.
 *
 * @param du_tmpl       data unit template structure
 * @param args          Array with arguments.
 * @param num_args      Length of arguments array.
 * @param data_place    location for generated binary data (OUT)
 * @param d_len         length of data field to be generated
 *
 * @return status code
 */
extern int tad_data_unit_to_bin(const tad_data_unit_t *du_tmpl,
                                const struct tad_tmpl_arg_t *args,
                                size_t arg_num,
                                uint8_t *data_place, size_t d_len);

/**
 * Send portion of data into TCP socket, and ensure that FIN will be send
 * in last PUSH TCP message.
 * Details: it set TCP_CORK option on the socket, write data to it,
 * and shut down socket for the write.
 * Note, that firther write to this socket are denied, and will cause
 * SIG_PIPE.
 *
 * @param socket        TCP socket fd
 * @param data          data to be sent
 * @param length        length of data
 *
 * @return status code
 */
extern te_errno tad_tcp_push_fin(int socket, const uint8_t *data,
                                 size_t length);

/**
 * Check that types in sequence of Gengeric-PDU ASN values are
 * corresponding with protocol types in CSAP protocol label stack.
 * If some PDU in ASN value sequence is absent, add empty one.
 * If it is not possible to determine, where empty PDU should be
 * inserted (for example, if CSAP is 'ip4.ip4.eth' and ASN sequence
 * has only one 'ip4' PDU), error returned.
 * If ASN pdus sequence has extra PDU, error returned.
 *
 * @param csap          Pointer to CSAP descriptor
 * @param pdus          ASN value, 'SEQUENCE OF Generic-PDU' (IN/OUT)
 *
 * @return status code
 */
extern int tad_check_pdu_seq(csap_p csap, asn_value *pdus);

/**
 * Generic implementation of write/read method using write and read
 * in sequence.
 *
 * This function complies with csap_write_read_cb_t prototype.
 */
extern te_errno tad_common_write_read_cb(csap_p csap, unsigned int timeout,
                                         const tad_pkt *w_pkt,
                                         tad_pkt *r_pkt, size_t *r_pkt_len);

/**
 * Generic implementation of read method over socket using poll and
 * recvmsg calls.
 *
 * @param csap          CSAP structure
 * @param sock          Socket file descriptor
 * @param flags         Flags to be forwarded to recvmsg
 * @param timeout       Timeout in microseconds
 * @param pkt           Packet with location from received data
 * @param from          Location from peer address
 * @param fromlen       On input length of the buffer for peer address,
 *                      on output length of the peer address
 * @param pkt_len       Location for received packet length
 * @param msg_flags     Location for flags returned by recvmsg() or @c NULL
 * @param cmsg_buf      Buffer to put ancillary data (or @c NULL)
 * @param cmsg_len      Ancillary data buffer length, in/out (or @c NULL)
 *
 * @return Status code.
 */
extern te_errno tad_common_read_cb_sock(csap_p csap, int sock,
                                        unsigned int flags,
                                        unsigned int timeout,
                                        tad_pkt *pkt,
                                        struct sockaddr *from,
                                        socklen_t *fromlen,
                                        size_t *pkt_len,
                                        int *msg_flags, void *cmsg_buf,
                                        size_t *cmsg_buf_len);

/**
 * Create detached thread.
 *
 * @param thread        Location for thread handle (may be NULL)
 * @param start_routine Thread entry point
 * @param arg           Thread entry point argument
 *
 * @return Status code.
 */
extern te_errno tad_pthread_create(pthread_t *thread,
                                   void * (*start_routine)(void *),
                                   void *arg);

extern te_errno tad_du_realloc(tad_data_unit_t *du, size_t size);

/**
 * An auxiliary tool to obtain a code of a DU value of @c TAD_DU_STRING type
 *
 * @param  du  The DU to be examined (may be @c NULL)
 *
 * @note If @p du is @c NULL or if it's type is different from
 *       @c TAD_DU_STRING, then @c TAD_CKSUM_STR_CODE_NONE will be returned
 *
 * @return  The numeric code of the DU value
 */
extern tad_cksum_str_code tad_du_get_cksum_str_code(tad_data_unit_t *du);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_UTILS_H__ */
