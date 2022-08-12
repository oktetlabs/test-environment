/** @file
 * @brief ATM NDN
 *
 * Declarations of ASN.1 types for NDN for ATM (ITU-T I.361, I.363.5;
 * RFC 2684).
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#ifndef __TE_NDN_ATM_H__
#define __TE_NDN_ATM_H__

#if HAVE_NET_IF_ATM_H
#include <net/if_atm.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h"


/** Length of ATM cell */
#define ATM_CELL_LEN        53
/** Length of ATM cell header */
#define ATM_HEADER_LEN      5
/** Length of ATM cell payload */
#define ATM_PAYLOAD_LEN     (ATM_CELL_LEN - ATM_HEADER_LEN)

/** Length of AAL5 CPCS PDU trailer */
#define AAL5_TRAILER_LEN    8


#ifdef __cplusplus
extern "C" {
#endif


/** ATM cell header format */
typedef enum ndn_atm_type {
    NDN_ATM_NNI,    /**< Network-Node Interface */
    NDN_ATM_UNI,    /**< User-Network Interface */
} ndn_atm_type;

/** ASN.1 tags for ATM */
typedef enum {
    NDN_TAG_ATM_DEVICE,
    NDN_TAG_ATM_TYPE,
    NDN_TAG_ATM_GFC,
    NDN_TAG_ATM_VPI,
    NDN_TAG_ATM_VCI,
    NDN_TAG_ATM_PAYLOAD_TYPE,
    NDN_TAG_ATM_CONGESTION,
    NDN_TAG_ATM_CLP,
    NDN_TAG_ATM_HEC,
    NDN_TAG_ATM_TYPE_ENUM,

    NDN_TAG_AAL5_ENCAP,
    NDN_TAG_AAL5_VC_MUX_ROUTED,
    NDN_TAG_AAL5_LLC,
    NDN_TAG_AAL5_CPCS_UU,
    NDN_TAG_AAL5_CPI,
    NDN_TAG_AAL5_LENGTH,
    NDN_TAG_AAL5_CRC,
} ndn_atm_tags_t;


extern const asn_type * const ndn_atm_header;
extern const asn_type * const ndn_atm_csap;

extern const asn_type * const ndn_aal5_cpcs_trailer;
extern const asn_type * const ndn_aal5_csap;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_ATM_H__ */
