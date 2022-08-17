/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD RTE mbuf, NDN
 *
 * Declarations of ASN.1 types for NDN of RTE mbuf pseudo-protocol
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_NDN_RTE_MBUF_H__
#define __TE_NDN_RTE_MBUF_H__

#include "te_stdint.h"
#include "te_ethernet.h"
#include "asn_usr.h"
#include "ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

/* These values must match the NDN for RTE mbuf */
typedef enum {
    NDN_TAG_RTE_MBUF_RING = 0,
    NDN_TAG_RTE_MBUF_POOL,
} ndn_rte_mbuf_tags_t;

extern const asn_type * const ndn_rte_mbuf_pdu;
extern const asn_type * const ndn_rte_mbuf_csap;

extern asn_type ndn_rte_mbuf_pdu_s;
extern asn_type ndn_rte_mbuf_csap_s;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_RTE_MBUF_H__ */
