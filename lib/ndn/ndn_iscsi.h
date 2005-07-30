/** @file
 * @brief Proteos, TAD CLI protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for ISCSI protocol. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */ 
#ifndef __TE_NDN_ISCSI_H__
#define __TE_NDN_ISCSI_H__

#include "asn_usr.h"
#include "ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASN.1 tags for ISCSI CSAP NDN
 */
typedef enum {
    NDN_TAG_ISCSI_TYPE,
    NDN_TAG_ISCSI_ADDR,
    NDN_TAG_ISCSI_PORT,
    NDN_TAG_ISCSI_SOCKET,
    NDN_TAG_ISCSI_MESSAGE,
    NDN_TAG_ISCSI_LEN,
    NDN_TAG_ISCSI_PARAM,
} ndn_iscsi_tags_t;


extern const asn_type *ndn_iscsi_message;
extern const asn_type *ndn_iscsi_csap;

typedef struct iscsi_target_params_t {
    int param;
} iscsi_target_params_t;



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_ISCSI_H__ */
