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



typedef struct tapi_ip_frag_spec_t {
    uint32_t    hdr_offset;     /**< value for "offset" in IP header */
    uint32_t    real_offset;    /**< begin of frag data in real payload */
    size_t      hdr_length;     /**< vlaue for "length" in IP header */
    size_t      real_length;    /**< length of frag data in real payload */
    te_bool     more_frags;     /**< value for "more frags" flag */
    te_bool     dont_frag;      /**< value for "dont frag" flag */
} tapi_ip_frag_spec_t;


typedef struct tapi_ip4_packet_t {
    in_addr_t   src_addr;
    in_addr_t   dst_addr;

    uint8_t    *payload;
    size_t      pld_len;
} tapi_ip4_packet_t;


/** 
 * Callback function for the receiving IP datagrams.
 *
 * Neither pkt, nor pkt->payload MAY NOT be stored for future use
 * by this callback: they are freed just after callback return.
 *
 * @param pkt           Received IP packet.
 * @param userdata      Parameter, provided by the caller.
 */
typedef void (*ip4_callback)(const tapi_ip4_packet_t *pkt, void *userdata);

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

/**
 * Start receiving of IPv4 packets 'ip4.eth' CSAP, non-block
 * method. 
 *
 * Started receiving process may be controlled by rcf_ta_trrecv_get, 
 * rcf_ta_trrecv_wait, and rcf_ta_trrecv_stop methods.
 * 
 * @param ta_name       test Agent name
 * @param sid           RCF SID
 * @param csap          identifier of CSAP
 * @param src_mac_addr  source MAC address (or NULL)
 * @param dst_mac_addr  destination MAC address (or NULL)
 * @param src_ip4_addr  source IP address in network order (or NULL)
 * @param dst_ip4_addr  destination IP address in network order (or NULL)
 * @param timeout       timeout of operation (in milliseconds, 
 *                      zero for infinitive)
 * @param num           nubmer of packets to be caugth
 * @param callback      pointer of method to be called for every packet
 * @param userdata      magic pointer which will be passed to user callback
 * 
 * @return Zero on success or error code.
 */
extern int tapi_ip4_eth_recv_start_pkt(const char *ta_name, int sid, 
                                       csap_handle_t csap,
                                       const uint8_t *src_mac_addr,
                                       const uint8_t *dst_mac_addr,
                                       const uint8_t *src_ip4_addr,
                                       const uint8_t *dst_ip4_addr,
                                       unsigned int timeout, int num, 
                                       ip4_callback callback,
                                       void *userdata);

/**
 * Prepare ASN PDU value of IPv4 CSAP layer type for Traffic-Template. 
 *
 * @param src_ip4_addr  source IP address in network order, or NULL;
 * @param dst_ip4_addr  destination IP address in network order, or NULL;
 * @param fragments     array with IP fragments specifictaions, or NULL;
 * @param num_frags     number of IP fragments;
 * @param ttl           time-to-live field, or negative for CSAP default;
 * @param protocol      protocol field, or negative for CSAP default;
 * @param result_value  location for pointer to new ASN value (OUT);
 * 
 * @return status code.
 */
extern int tapi_ip4_pdu(const uint8_t *src_ip4_addr,
                        const uint8_t *dst_ip4_addr,
                        tapi_ip_frag_spec_t *fragments,
                        size_t num_frags,
                        int ttl, int protocol, 
                        asn_value **result_value);


/**
 * Prepare ASN Pattern-Unit value for 'ip4.eth' CSAP.
 * 
 * @param src_mac_addr  source MAC address, or NULL;
 * @param dst_mac_addr  destination MAC address, or NULL;
 * @param src_ip4_addr  source IP address in network order, or NULL;
 * @param dst_ip4_addr  destination IP address in network order, or NULL;
 * @param result_value  location for pointer to new ASN value (OUT);
 * 
 * @return status code.
 */
extern int tapi_ip4_eth_pattern_unit(const uint8_t *src_mac_addr,
                                     const uint8_t *dst_mac_addr,
                                     const uint8_t *src_ip4_addr,
                                     const uint8_t *dst_ip4_addr,
                                     asn_value **result_value);




/**
 * Find in passed ASN value of Pattern-Unit type IPv4 PDU in 'pdus' array
 * and set in it specified masks for src and/or dst addresses.
 *
 * @param pattern_unit  ASN value of type Traffic-Pattern-Unit (IN/OUT)
 * @param src_mask_len  Length of mask for IPv4 source address or zero
 * @param dst_mask_len  Length of mask for IPv4 dest. address or zero
 *
 * @return Zero on success or error code.
 */
extern int tapi_pattern_unit_ip4_mask(asn_value *pattern_unit, 
                                      size_t src_mask_len,
                                      size_t dst_mask_len);

/**
 * Preapre ASN Traffic-Template value for 'ip4.eth' CSAP.
 *
 * @param src_mac_addr  source MAC address, or NULL;
 * @param dst_mac_addr  destination MAC address, or NULL;
 * @param src_ip4_addr  source IP address in network order, or NULL;
 * @param dst_ip4_addr  destination IP address in network order, or NULL;
 * @param fragments     array with IP fragments specifictaions, or NULL;
 * @param num_frags     number of IP fragments;
 * @param ttl           time-to-live field, or negative for CSAP default;
 * @param protocol      protocol field, or negative for CSAP default;
 * @param payload       payload of IPv4 packet, or NULL;
 * @param pld_len       length of payload;
 * @param result_value  location for pointer to new ASN value (OUT);
 *
 * @return status code
 */ 
extern int tapi_ip4_eth_template(const uint8_t *src_mac_addr,
                                 const uint8_t *dst_mac_addr,
                                 const uint8_t *src_ip4_addr,
                                 const uint8_t *dst_ip4_addr,
                                 tapi_ip_frag_spec_t *fragments,
                                 size_t num_frags,
                                 int ttl, int protocol, 
                                 const uint8_t *payload,
                                 size_t pld_len,
                                 asn_value **result_value);


#endif /* !__TE_TAPI_IP_H__ */
