/** @file
 * @brief NDN support for VxLAN
 *
 * ASN.1 type definition for VxLAN
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */
#ifndef __TE_NDN_VXLAN_H__
#define __TE_NDN_VXLAN_H__

#include "asn_usr.h"
#include "ndn.h"

typedef enum {
    NDN_TAG_VXLAN_FLAGS_RESERVED_1,
    NDN_TAG_VXLAN_VNI_VALID,
    NDN_TAG_VXLAN_FLAGS_RESERVED_2,
    NDN_TAG_VXLAN_RESERVED_1,
    NDN_TAG_VXLAN_VNI,
    NDN_TAG_VXLAN_RESERVED_2,

    NDN_TAG_VXLAN_HEADER,


    NDN_TAG_VXLAN_CSAP,
} ndn_vxlan_tags_t;

#ifdef __cplusplus
extern "C" {
#endif

extern const asn_type * const ndn_vxlan_header;
extern const asn_type * const ndn_vxlan_csap;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_VXLAN_H__ */
