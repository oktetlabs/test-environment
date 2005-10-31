/** @file
 * @brief Proteos, TAPI ARP.
 *
 * Declarations of API for TAPI ARP.
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifndef __TE_TAPI_ARP_H__
#define __TE_TAPI_ARP_H__

#include <stdio.h>
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <net/ethernet.h>
#include <net/if_arp.h>

#include "te_stdint.h"
#include "te_defs.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "ndn_eth.h"
#include "ndn_arp.h"


/** Structure that represents ARP frame: Ethernet header and ARP header */
typedef struct tapi_arp_frame {
    ndn_eth_header_plain  eth_hdr;  /**< Ethernet header */
    ndn_arp_header_plain  arp_hdr;  /**< ARP header */

    uint32_t              data_len; /**< Data length */
    uint8_t              *data;     /**< The data that goes after 
                                         ARP header */
} tapi_arp_frame_t;

/**
 * Fills in Ethernet header of ARP frame with 802.3 source and destination 
 * MAC addresses.
 *
 * @param arp_frame_  ARP frame to be updated
 * @param src_mac_    Source MAC address
 * @param dst_mac_    Destination MAC address
 */
#define TAPI_ARP_FILL_ETH_HDR(arp_frame_, src_mac_, dst_mac_) \
    do {                                                            \
        memset(&((arp_frame_)->eth_hdr), 0,                         \
                sizeof((arp_frame_)->eth_hdr));                     \
        assert(sizeof((arp_frame_)->eth_hdr.src_addr) == ETH_ALEN); \
        memcpy((arp_frame_)->eth_hdr.src_addr, src_mac_, ETH_ALEN); \
        assert(sizeof((arp_frame_)->eth_hdr.dst_addr) == ETH_ALEN); \
        memcpy((arp_frame_)->eth_hdr.dst_addr, dst_mac_, ETH_ALEN); \
        (arp_frame_)->eth_hdr.eth_type_len = ETHERTYPE_ARP;         \
        (arp_frame_)->eth_hdr.is_tagged = FALSE;                    \
    } while (0)
    
/**
 * Fills in ARP frame header
 *
 * @param  arp_frame_    structure storing arp frame
 * @param  op_           operation type
 * @param  snd_hw_       sender hardware address
 * @param  snd_proto_    sender protocol address
 * @param  tgt_hw_       target hardware address
 * @param  tgt_proto_    target protocol address
 *
 */
#define TAPI_ARP_FILL_HDR(arp_frame_, op_, \
                          snd_hw_, snd_proto_, tgt_hw_, tgt_proto_) \
    do {                                                                \
        memset(&((arp_frame_)->arp_hdr), 0,                             \
               sizeof((arp_frame_)->arp_hdr));                          \
        (arp_frame_)->arp_hdr.hw_type = ARPHRD_ETHER;                   \
        (arp_frame_)->arp_hdr.proto_type = ETHERTYPE_IP;                \
        (arp_frame_)->arp_hdr.hw_size = ETH_ALEN;                       \
        (arp_frame_)->arp_hdr.proto_size = sizeof(struct in_addr);      \
        (arp_frame_)->arp_hdr.opcode = (op_);                           \
        if ((snd_hw_) != NULL)                                          \
            memcpy((arp_frame_)->arp_hdr.snd_hw_addr, snd_hw_,          \
                   ETH_ALEN);                                           \
        if ((snd_proto_) != NULL)                                       \
            memcpy((arp_frame_)->arp_hdr.snd_proto_addr, snd_proto_,    \
                   sizeof(struct in_addr));                             \
        if ((tgt_hw_) != NULL)                                          \
            memcpy((arp_frame_)->arp_hdr.tgt_hw_addr, tgt_hw_,          \
                   ETH_ALEN);                                           \
        if ((tgt_proto_) != NULL)                                       \
            memcpy((arp_frame_)->arp_hdr.tgt_proto_addr, tgt_proto_,    \
                   sizeof(struct in_addr));                             \
                                                                        \
        (arp_frame_)->data = NULL;                                      \
        (arp_frame_)->data_len = 0;                                     \
    } while (0)

/**
 * Create ARP CSAP that runs over Ethernet RFC 894.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param device        interface name on TA host
 * @param remote_addr   default remote MAC address, may be NULL - in this
 *                      case frames will be sent only if dst is specified in
 *                      template, and frames from all src's will be catched.
 *                      If NULL, CSAP will have remote address unconfigured
 *                      and will require it in traffic template.
 * @param local_addr    default local MAC address. may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template).
 * @param arp_csap      Identifier of created CSAP (OUT)
 *
 * @return zero on success, otherwise standard or common TE error code. 
 */
extern int tapi_arp_csap_create(const char *ta_name, int sid,
                                const char *device,
                                const uint8_t *remote_addr, 
                                const uint8_t *local_addr, 
                                csap_handle_t *arp_csap);

/**
 * Callback function for the tapi_arp_recv_start() routine, it is called
 * for each packet received on CSAP.
 *
 * @param header        Structure with ARP and Ethernet header of the frame
 * @param payload       Payload of the frame
 * @param plen          Length of the frame payload
 * @param userdata      Pointer to user data, provided by  the caller of 
 *                      tapi_arp_recv_start
 */
typedef void (*tapi_arp_frame_callback)(const tapi_arp_frame_t *header, 
                                        void *userdata);

/**
 * Start receive process on specified ARP CSAP. If process
 * was started correctly (rc is OK) it can be managed by common RCF
 * methods 'rcf_ta_trrecv_wait', 'rcf_ta_trrecv_get' and
 * 'rcf_ta_trrecv_stop'.
 *
 * @param ta_name    Test Agent name
 * @param sid        RCF session
 * @param arp_csap   CSAP handle 
 * @param pattern    ASN value with receive pattern
 * @param cb         Callback function which will be called for each 
 *                   received frame, may me NULL if frames are not need
 * @param cb_data    pointer to be passed to user callback
 * @param timeout    Timeout for receiving of packets, measured in 
 *                   milliseconds
 * @param num        Number of packets caller wants to receive
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
extern int tapi_arp_recv_start(const char *ta_name, int sid,
                               csap_handle_t arp_csap,
                               const asn_value *pattern,
                               tapi_arp_frame_callback cb, void *cb_data,
                               unsigned int timeout, int num);

/**
 * Receives specified number of ARP frames matched with the pattern.
 * The function blocks the caller until all the frames are received or 
 * timeout occurred.
 * 
 * @param ta_name    Test Agent name
 * @param sid        RCF session
 * @param arp_csap   CSAP handle 
 * @param pattern    ASN value with receive pattern
 * @param timeout    Timeout for receiving of packets, measured in 
 *                   milliseconds
 * @param frames     Pointer to the array of packets (OUT)
 *                   the function allocates memory under the packets that 
 *                   should be freed with free() function
 * @param num        Number of packets caller wants to receive (IN)
 *                   number of received packets (OUT)
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
extern int tapi_arp_recv(const char *ta_name, int sid,
                         csap_handle_t arp_csap,
                         const asn_value *pattern, unsigned int timeout,
                         tapi_arp_frame_t **frames, int *num);

/**
 * Creates traffic template for a single ARP frame.
 *
 * @param frame  ARP frame data structure as the source of ARP frame values
 * @param templ  Placeholder for ARP template (OUT)
 *
 * @returns Status of the operation
 */
extern int tapi_arp_prepare_template(const tapi_arp_frame_t *frame,
                                     asn_value **templ);

/**
 * Creates traffic pattern for a single Ethernet frame with 'type' field 
 * set to ARP and specified source and destination MAC addresses.
 *
 * @param src_mac  Desired source MAC address value. if NULL, source address
 *                 of the Ethernet frame is not matched
 * @param dst_mac  Desired destination MAC address value. if NULL,
 *                 destination address of the Ethernet frame is not matched
 * @param pattern  Placeholder for the pattern (OUT)
 *
 * @returns Status of the operation
 */
extern int tapi_arp_prepare_pattern_eth_only(const uint8_t *src_mac,
                                             const uint8_t *dst_mac,
                                             asn_value **pattern);

/**
 * Creates traffic pattern for a single ARP frame with specified 
 * Ethernet source, destination MAC addresses and ARP header fields.
 *
 * @param eth_src_mac     Desired source MAC address value
 * @param eth_dst_mac     Desired destination MAC address value
 * @param opcode          Desired operation code
 * @param snd_hw_addr     Desired sender hardware address
 * @param snd_proto_addr  Desired sender protocol address
 * @param tgt_hw_addr     Desired target hardware address
 * @param tgt_proto_addr  Desired target protocol address
 * @param pattern         Placeholder for the pattern (OUT)
 *
 * @note If the pointer is NULL, the field is not matched
 *
 * @returns Status of the operation
 */
extern int tapi_arp_prepare_pattern_with_arp(const uint8_t *eth_src_mac,
                                             const uint8_t *eth_dst_mac,
                                             const uint16_t *opcode,
                                             const uint8_t *snd_hw_addr,
                                             const uint8_t *snd_proto_addr,
                                             const uint8_t *tgt_hw_addr,
                                             const uint8_t *tgt_proto_addr,
                                             asn_value **pattern);

#endif /* __TE_TAPI_ARP_H__ */
