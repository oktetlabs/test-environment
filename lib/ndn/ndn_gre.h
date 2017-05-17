/** @file
 * @brief NDN support for GRE
 *
 * ASN.1 type definition for GRE
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
