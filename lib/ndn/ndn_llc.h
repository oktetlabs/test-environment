/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for IEEE Std 802.2 LLC protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_NDN_LLC_H__
#define __TE_NDN_LLC_H__

#include "te_stdint.h"
#include "te_ethernet.h"
#include "asn_usr.h"
#include "ndn.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {

    NDN_TAG_LLC_DSAP_IG,
    NDN_TAG_LLC_DSAP,
    NDN_TAG_LLC_SSAP_CR,
    NDN_TAG_LLC_SSAP,
    NDN_TAG_LLC_CTL,

    NDN_TAG_LLC_SNAP_HEADER,
    NDN_TAG_SNAP_OUI,
    NDN_TAG_SNAP_PID,

} ndn_llc_tags_t;

extern const asn_type * const ndn_snap_header;
extern const asn_type * const ndn_llc_header;

extern asn_type ndn_snap_header_s;
extern asn_type ndn_llc_header_s;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_LLC_H__ */
