/** @file
 * @brief Test API for TAD. ipstack CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS in the
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
 * @author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */


#ifndef __TE_TAPI_IP_H__
#define __TE_TAPI_IP_H__

#include <assert.h>
#include <netinet/in.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"




/**
 * Creates 'ip4.eth' CSAP
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param loc_mac_addr  Local MAC address (or NULL)
 * @param rem_mac_addr  Remote MAC address (or NULL)
 * @param loc_ip4_addr  Local IP address in network order (or NULL)
 * @param rem_ip4_addr  Remote IP address in network order (or NULL)
 * @param ip4_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern int tapi_ip4_eth_csap_create(const char *ta_name, int sid, 
                                    const char *eth_dev,
                                    const uint8_t *loc_mac_addr, 
                                    const uint8_t *rem_mac_addr, 
                                    const uint8_t *loc_ip4_addr,
                                    const uint8_t *rem_ip4_addr,
                                    csap_handle_t *ip4_csap);


/**
 * Start receiving of IPv4 packets 'ip4.eth' CSAP, non-block
 * method. It cannot report received packets, only count them.
 *
 * Started receiving process may be controlled by rcf_ta_trrecv_get, 
 * rcf_ta_trrecv_wait, and rcf_ta_trrecv_stop methods.
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param csap          Identifier of CSAP
 * @param src_mac_addr  Source MAC address (or NULL)
 * @param dst_mac_addr  Destination MAC address (or NULL)
 * @param src_ip4_addr  Source IP address in network order (or NULL)
 * @param dst_ip4_addr  Destination IP address in network order (or NULL)
 * @param timeout       Timeout of operation (in milliseconds, 
 *                      zero for infinitive)
 * @param num           nubmer of packets to be caugth
 * 
 * @return Zero on success or error code.
 */
extern int tapi_ip4_eth_recv_start(const char *ta_name, int sid, 
                                   csap_handle_t csap,
                                   const uint8_t *src_mac_addr,
                                   const uint8_t *dst_mac_addr,
                                   const uint8_t *src_ip4_addr,
                                   const uint8_t *dst_ip4_addr,
                                   unsigned int timeout, int num);

typedef struct tapi_ip_frag_spec_t {
    uint32_t    pkt_offset;     /**< value for "offset" in IP header */
    uint32_t    real_offset;    /**< begin of frag data in real payload */
    size_t      pkt_length;     /**< vlaue for "length" in IP header */
    size_t      real_length;    /**< length of frag data in real payload */
    int         more_frags_flag;/**< value for "more frags flag" */
} tapi_ip_frag_spec_t;

#endif /* !__TE_TAPI_IP_H__ */
