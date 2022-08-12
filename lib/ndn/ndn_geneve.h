/** @file
 * @brief NDN support for Geneve
 *
 * ASN.1 type definition for Geneve
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */
#ifndef __TE_NDN_GENEVE_H__
#define __TE_NDN_GENEVE_H__

#include "asn_usr.h"
#include "ndn.h"

typedef enum {
    NDN_TAG_GENEVE_VERSION,
    NDN_TAG_GENEVE_OPTIONS_LENGTH,
    NDN_TAG_GENEVE_OAM,
    NDN_TAG_GENEVE_CRITICAL,
    NDN_TAG_GENEVE_RESERVED_1,
    NDN_TAG_GENEVE_PROTOCOL,
    NDN_TAG_GENEVE_VNI,
    NDN_TAG_GENEVE_RESERVED_2,

    NDN_TAG_GENEVE_HEADER,


    NDN_TAG_GENEVE_OPTION_CLASS,
    NDN_TAG_GENEVE_OPTION_TYPE,
    NDN_TAG_GENEVE_OPTION_FLAGS_RESERVED,
    NDN_TAG_GENEVE_OPTION_LENGTH,
    NDN_TAG_GENEVE_OPTION_DATA,

    NDN_TAG_GENEVE_OPTION,
    NDN_TAG_GENEVE_OPTIONS,


    NDN_TAG_GENEVE_CSAP,
} ndn_geneve_tags_t;

#ifdef __cplusplus
extern "C" {
#endif

extern const asn_type * const ndn_geneve_header;
extern const asn_type * const ndn_geneve_option;
extern const asn_type * const ndn_geneve_options;
extern const asn_type * const ndn_geneve_csap;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_GENEVE_H__ */
