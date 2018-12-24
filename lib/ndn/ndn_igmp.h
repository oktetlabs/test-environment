/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for IGMPv2 protocol.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Alexander.Kukuta <kam@oktetlabs.ru>
 *
 * $Id: $
 */
#ifndef __TE_NDN_IGMP_H__
#define __TE_NDN_IGMP_H__


#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASN.1 tags for IGMPv2 PDUs
 */
typedef enum {
    NDN_TAG_IGMP_VERSION,
    NDN_TAG_IGMP_TYPE,
    NDN_TAG_IGMP_MAX_RESPONSE_TIME,
    NDN_TAG_IGMP_CHECKSUM,
    NDN_TAG_IGMP_GROUP_ADDRESS,
    NDN_TAG_IGMP3_RESERVED,
    NDN_TAG_IGMP3_S_FLAG,
    NDN_TAG_IGMP3_QRV,
    NDN_TAG_IGMP3_QQIC,
    NDN_TAG_IGMP3_NUMBER_OF_SOURCES,
    NDN_TAG_IGMP3_SOURCE_ADDRESS_LIST,
    NDN_TAG_IGMP3_NUMBER_OF_GROUPS,
    NDN_TAG_IGMP3_GROUP_RECORD_LIST,
#ifdef NDN_IGMP_STRUCTURED
    NDN_TAG_IGMP3_GROUP_RECORD,
    NDN_TAG_IGMP3_GROUP_RECORD_TYPE,
    NDN_TAG_IGMP3_AUX_DATA_LEN,
    NDN_TAG_IGMP3_AUX_DATA,
#endif
    NDN_TAG_IGMP_,
} ndn_igmp_tags_t;


extern const asn_type * const ndn_igmp_message;
extern const asn_type * const ndn_igmp_csap;

extern asn_type ndn_igmp_message_s;
extern asn_type ndn_igmp_csap_s;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_IGMP_H__ */
