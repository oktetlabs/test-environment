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
#ifndef __PROTEOS__ASN_LIB__ETH_NDN__H__
#define __PROTEOS__ASN_LIB__ETH_NDN__H__

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


/* Structure for Ethernet frame header. written according to IEEE 802.3 */
typedef struct ndn_eth_header_plain 
{
    uint8_t  dst_addr [ETH_ALEN]; /**< Destination MAC address */
    uint8_t  src_addr [ETH_ALEN]; /**< Source MAC address */ 
    uint16_t eth_type_len;  	/**< Ethernet Length/Type */
    int      is_tagged;  	/**< BOOL, flag - is frame tagged. */
    int      cfi;		/**< BOOL, Canonical Format Indicator*/
    uint8_t  priority;  	/**< Prioriy */
    uint16_t vlan_id;  		/**< Ethernet Length/Type */
} ndn_eth_header_plain;


enum eth_csap_receive_mode {
    ETH_RECV_HOST = 1,
    ETH_RECV_BROADCAST = 2,
    ETH_RECV_MULTICAST = 4,
    ETH_RECV_OTHERHOST = 8,
    ETH_RECV_OUTGOING = 16,
    ETH_RECV_ALL = 31,
};

/** 
 * Convert Ehternet-Header ASN value to plain C structure. 
 * 
 * @param pkt           ASN value of type Ethernet Header or Generic-PDU with 
 *                      choice "eth". 
 * @param eth_header    location for converted structure (OUT).
 *
 * @return zero on success or error code.
 */ 
extern int ndn_eth_packet_to_plain(const asn_value *pkt, 
                                ndn_eth_header_plain *eth_header);


/** 
 * Convert plain C structure to Ehternet-Header ASN value. 
 * 
 * @param eth_header    structure to be converted. 
 *
 * @return pointer to new ASN value object or NULL.
 */ 
extern asn_value *ndn_eth_plain_to_packet(const ndn_eth_header_plain *eth_header);

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

#endif /* __PROTEOS__ASN_LIB__ETH_NDN__H__ */
