/** @file
 * @brief NDN support for VxLAN
 *
 * ASN.1 type definition for VxLAN
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
