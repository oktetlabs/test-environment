/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for Ethernet protocol. 
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */ 
#ifndef __TE_NDN_FORW_H__
#define __TE_NDN_FORW_H__

#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn.h"

#ifndef ETH_ALEN
#define ETH_ALEN (6)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FORW_DELAY_DISABLED,
    FORW_DELAY_CONSTANT,
    FORW_DELAY_RAND_CONT,
    FORW_DELAY_RAND_DISCR,
} ndn_forw_delay_type_t;

typedef struct {
    int prob;
    int delay;
} ndn_delay_discr_pair_t;


#define DELAY_DISCR_MAX 0x10
typedef struct {
    ndn_forw_delay_type_t type;

    int min;
    int max; 

    size_t n_pairs; 
    ndn_delay_discr_pair_t discr[DELAY_DISCR_MAX];
} ndn_forw_delay_t;






typedef enum {
    FORW_REORDER_DISABLED,
    FORW_REORDER_RANDOM,
    FORW_REORDER_REVERSED,
} ndn_forw_reorder_type_t;

typedef struct {
    ndn_forw_reorder_type_t type;
    
    int timeout;
    int r_size;
} ndn_forw_reorder_t;




typedef enum {
    FORW_DROP_DISABLED,
    FORW_DROP_RANDOM,
    FORW_DROP_PATTERN,
} ndn_forw_drop_type_t;

typedef struct {
    ndn_forw_drop_type_t type;

    int rate;

    size_t   mask_len; /* length of mask in bits */
    uint8_t *pattern_mask;

} ndn_forw_drop_t;

typedef struct { 
    char               *id;
    ndn_forw_delay_t    delay;
    ndn_forw_reorder_t  reorder;
    ndn_forw_drop_t     drop;
} ndn_forw_action_plain;

extern const asn_type * const ndn_forw_action;

/** 
 * Convert Forwarder-Action ASN value to plain C structrue. 
 * 
 * @param val           ASN value of type  
 * @param forw_action   converted structure (OUT).
 *
 * @return zero on success or error code.
 */ 
extern int ndn_forw_action_asn_to_plain(const asn_value *val, 
                                ndn_forw_action_plain *forw_action);

/** 
 * Convert plain C structrue to Forwarder-Action ASN value. 
 * 
 * @param forw_action   converted structure.
 * @param val           location for pointer to ASN value of type (OUT)
 *
 * @return zero on success or error code.
 */ 
extern int ndn_forw_action_plain_to_asn(
                                const ndn_forw_action_plain *forw_action,
                                asn_value **val);

extern asn_type ndn_forw_action_s;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_FORW_H__ */
