/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD CLI protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for ISCSI protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE_NDN_ISCSI_H__
#define __TE_NDN_ISCSI_H__

#include "asn_usr.h"
#include "ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ISCSI_BHS_LENGTH (48)

/**
 * ASN.1 tags for ISCSI CSAP NDN
 */
typedef enum {
    NDN_TAG_ISCSI_TYPE,
    NDN_TAG_ISCSI_SOCKET,
    NDN_TAG_ISCSI_HEADER_DIGEST,
    NDN_TAG_ISCSI_DATA_DIGEST,
    NDN_TAG_ISCSI_MESSAGE,
    NDN_TAG_ISCSI_I_BIT,
    NDN_TAG_ISCSI_OPCODE,
    NDN_TAG_ISCSI_F_BIT,
    NDN_TAG_ISCSI_OP_SPECIFIC,
    NDN_TAG_ISCSI_AHS_LEN,
    NDN_TAG_ISCSI_DS_LEN,
    NDN_TAG_ISCSI_LEN,
    NDN_TAG_ISCSI_LAST,
} ndn_iscsi_tags_t;

/**
 * ASN.1 tags for ISCSI Segment Data fields access
 */
typedef enum {
    NDN_TAG_ISCSI_SD_KEY_VALUE,
    NDN_TAG_ISCSI_SD_KEY_VALUES,
    NDN_TAG_ISCSI_SD_KEY,
    NDN_TAG_ISCSI_SD_VALUES,
    NDN_TAG_ISCSI_SD_KEY_PAIR,
    NDN_TAG_ISCSI_SD_SEGMENT_DATA,
    NDN_TAG_ISCSI_SD,
} ndn_iscsi_sd_tags_t;


/**
 * Types of iSCSI digests.
 *
 * @attention Values are used as index in iscsi_digest_length array.
 */
typedef enum {
    ISCSI_DIGEST_NONE = 0,
    ISCSI_DIGEST_CRC32C,
} iscsi_digest_type;


extern const asn_type *ndn_iscsi_message;
extern const asn_type *ndn_iscsi_csap;

typedef struct iscsi_target_params_t {
    int param;
} iscsi_target_params_t;

typedef int iscsi_key;
typedef int iscsi_key_value;
typedef asn_value *iscsi_segment_data;
typedef asn_value *iscsi_key_values;

extern const asn_type * const ndn_iscsi_segment_data;
extern const asn_type * const ndn_iscsi_key_pair;
extern const asn_type * const ndn_iscsi_key_values;
extern const asn_type * const ndn_iscsi_key_value;

/**
 * Convert asn representation of iSCSI Segment Data
 * to binary data
 *
 * @param segment_data   asn representation of iSCSI Segment Data
 * @param data           buffer to contain result of convertion
 * @param data_len       IN - length of buffer
 *                       OUT - length of binary data - result of converion
 *
 * @return Status code
 */
extern int
asn2bin_data(asn_value *segment_data,
             uint8_t *data, uint32_t *data_len);

/**
 * Convert iSCSI Segment Data represented as binary data
 * to asn representation
 *
 * @param data           binary data
 * @param data_len       binary data length
 * @param segment_data   location for result of convertion
 *
 * @return Status code
 */
extern int bin_data2asn(uint8_t *data, uint32_t data_len,
                        asn_value **value);

/**
 * Calculate extra (non-BHS) length of iSCSI PDU.
 *
 * @param bhs           Pointer to iSCSI PDU begin
 * @param header_digest HeaderDigest type
 * @param data_digest   DataDigest type
 *
 * @return Number of bytes rest in PDU
 */
extern size_t iscsi_rest_data_len(uint8_t           *bhs,
                                  iscsi_digest_type  header_digest,
                                  iscsi_digest_type  data_digest);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_ISCSI_H__ */
