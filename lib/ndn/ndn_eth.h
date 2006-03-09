/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for Ethernet protocol. 
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifndef __TE_NDN_ETH_H__
#define __TE_NDN_ETH_H__

#if HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#if HAVE_NET_IF_ETHER_H
#include <net/if_ether.h>
#endif

#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Structure for Ethernet frame header. written according to IEEE 802.3 */
typedef struct ndn_eth_header_plain {

    uint8_t  dst_addr[ETHER_ADDR_LEN];  /**< Destination MAC address */
    uint8_t  src_addr[ETHER_ADDR_LEN];  /**< Source MAC address */ 

    uint16_t len_type;      /**< Ethernet Length/Type */
    int      is_tagged;     /**< BOOL, flag - is frame tagged. */
    int      cfi;           /**< BOOL, Canonical Format Indicator*/
    uint8_t  priority;      /**< Priority */
    uint16_t vlan_id;       /**< VLAN tag */

} ndn_eth_header_plain;


enum eth_csap_receive_mode {
    ETH_RECV_HOST      = 0x01,
    ETH_RECV_BROADCAST = 0x02,
    ETH_RECV_MULTICAST = 0x04,
    ETH_RECV_OTHERHOST = 0x08,
    ETH_RECV_OUTGOING  = 0x10,
    ETH_RECV_ALL       = ETH_RECV_HOST | ETH_RECV_BROADCAST |
                         ETH_RECV_MULTICAST | ETH_RECV_OTHERHOST |
                         ETH_RECV_OUTGOING,
    ETH_RECV_DEF       = ETH_RECV_ALL & (~ETH_RECV_OUTGOING),
};

typedef enum {

    NDN_TAG_ETH_DEVICE = 0,
    NDN_TAG_ETH_RECV_MODE,
    NDN_TAG_ETH_LOCAL,
    NDN_TAG_ETH_REMOTE,

    NDN_TAG_ETH_FRAME_TYPE,
    NDN_TAG_ETH_UNTAGGED,
    NDN_TAG_ETH_TAGGED,
    NDN_TAG_ETH_SNAP,

    NDN_TAG_ETH_DST,
    NDN_TAG_ETH_SRC,
    NDN_TAG_ETH_LENGTH_TYPE,

    NDN_TAG_ETH_SNAP_OUI,
    NDN_TAG_ETH_SNAP_PID,

    NDN_TAG_ETH_PRIO,
    NDN_TAG_ETH_CFI,
    NDN_TAG_ETH_VLAN_ID,

    NDN_TAG_ETH_ERIF_RC_RT,
    NDN_TAG_ETH_ERIF_RC_LTH,
    NDN_TAG_ETH_ERIF_RC_D,
    NDN_TAG_ETH_ERIF_RC_LF,
    NDN_TAG_ETH_ERIF_RC_NCFI,
    NDN_TAG_ETH_ERIF_RD,

} ndn_eth_tags_t;


/** 
 * Convert Ethernet-Header ASN value to plain C structure. 
 * 
 * @param pkt           ASN value of type Ethernet Header or 
 *                      Generic-PDU with choice "eth". 
 * @param eth_header    location for converted structure (OUT).
 *
 * @return zero on success or error code.
 */ 
extern int ndn_eth_packet_to_plain(const asn_value *pkt, 
                                   ndn_eth_header_plain *eth_header);


/** 
 * Convert plain C structure to Ethernet-Header ASN value. 
 * 
 * @param eth_header    structure to be converted. 
 *
 * @return pointer to new ASN value object or NULL.
 */ 
extern asn_value *ndn_eth_plain_to_packet(const ndn_eth_header_plain 
                                          *eth_header);


extern const asn_type * const ndn_eth_tagged;
extern const asn_type * const ndn_eth_snap;
extern const asn_type * const ndn_eth_frame_type;

extern const asn_type * const ndn_eth_header;
extern const asn_type * const ndn_eth_csap;
extern const asn_type * const ndn_data_unit_eth_address;  
extern const asn_type * const ndn_eth_address;  

extern asn_type ndn_eth_header_s;
extern asn_type ndn_eth_csap_s;
extern asn_type ndn_data_unit_eth_address_s;  
extern asn_type ndn_eth_address_s;  


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_ETH_H__ */
