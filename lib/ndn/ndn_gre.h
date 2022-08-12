/** @file
 * @brief NDN support for GRE
 *
 * ASN.1 type definition for GRE
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */
#ifndef __TE_NDN_GRE_H__
#define __TE_NDN_GRE_H__

#include "asn_usr.h"
#include "ndn.h"

typedef enum {
    NDN_TAG_GRE_CKSUM_PRESENT,
    NDN_TAG_GRE_FLAGS_RESERVED_1,
    NDN_TAG_GRE_KEY_PRESENT,
    NDN_TAG_GRE_SEQN_PRESENT,
    NDN_TAG_GRE_FLAGS_RESERVED_2,
    NDN_TAG_GRE_VERSION,
    NDN_TAG_GRE_PROTOCOL,

    NDN_TAG_GRE_HEADER,


    NDN_TAG_GRE_OPT_CKSUM_VALUE,
    NDN_TAG_GRE_OPT_CKSUM_RESERVED,

    NDN_TAG_GRE_OPT_CKSUM,


    NDN_TAG_GRE_OPT_KEY_NVGRE_VSID,
    NDN_TAG_GRE_OPT_KEY_NVGRE_FLOWID,

    NDN_TAG_GRE_OPT_KEY_NVGRE,
    NDN_TAG_GRE_OPT_KEY,


    NDN_TAG_GRE_OPT_SEQN_VALUE,
    NDN_TAG_GRE_OPT_SEQN,


    NDN_TAG_GRE_CSAP,
} ndn_gre_tags_t;

#ifdef __cplusplus
extern "C" {
#endif

extern const asn_type * const ndn_gre_header;
extern const asn_type * const ndn_gre_header_opt_cksum;
extern const asn_type * const ndn_gre_header_opt_key_nvgre;
extern const asn_type * const ndn_gre_header_opt_key;
extern const asn_type * const ndn_gre_header_opt_seqn;
extern const asn_type * const ndn_gre_csap;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_GRE_H__ */
