/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for PPP&PPPoE protocols.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 */
#ifndef __TE_NDN_PPP_H__
#define __TE_NDN_PPP_H__

#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASN.1 tags for PPP CSAP NDN
 */
typedef enum {
    NDN_TAG_PPP_PROTOCOL,
    NDN_TAG_PPP_PKT,
    NDN_TAG_PPPOE_VERSION,
    NDN_TAG_PPPOE_TYPE,
    NDN_TAG_PPPOE_CODE,
    NDN_TAG_PPPOE_SESSION_ID,
    NDN_TAG_PPPOE_LENGTH,
    NDN_TAG_PPPOE_PAYLOAD,
    NDN_TAG_PPPOE_PKT,
} ndn_ppp_tags_t;

extern const asn_type * const ndn_pppoe_message;
extern const asn_type * const ndn_pppoe_csap;

extern asn_type ndn_pppoe_message_s;
extern asn_type ndn_pppoe_csap_s;

extern const asn_type * const ndn_ppp_message;
extern const asn_type * const ndn_ppp_csap;


extern asn_type ndn_ppp_message_s;
extern asn_type ndn_ppp_csap_s;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_PPP_H__ */
