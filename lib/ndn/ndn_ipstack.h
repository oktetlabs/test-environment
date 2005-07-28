/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for file protocol. 
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
#ifndef __TE_NDN_IPSTACK_H__
#define __TE_NDN_IPSTACK_H__


#include "asn_usr.h"
#include "ndn.h"

typedef enum {
    NDN_TAG_IP4_VERSION,
    NDN_TAG_IP4_HLEN,
    NDN_TAG_IP4_TOS,
    NDN_TAG_IP4_LEN,
    NDN_TAG_IP4_IDENT,
    NDN_TAG_IP4_FLAGS,
    NDN_TAG_IP4_OFFSET,
    NDN_TAG_IP4_TTL,
    NDN_TAG_IP4_PROTOCOL,
    NDN_TAG_IP4_H_CHECKSUM,
    NDN_TAG_IP4_SRC_ADDR,
    NDN_TAG_IP4_DST_ADDR,
    NDN_TAG_IP4_LOCAL_ADDR,
    NDN_TAG_IP4_REMOTE_ADDR,
    NDN_TAG_IP4_MTU,
    NDN_TAG_IP4_FRAGMENTS,
    NDN_TAG_IP4_FR_HO, /**< fragment offset: value for IP header */
    NDN_TAG_IP4_FR_RO, /**< "real" fragment offset */
    NDN_TAG_IP4_FR_HL, /**< fragment length: value for IP header */
    NDN_TAG_IP4_FR_RL, /**< "real" fragment length */
    NDN_TAG_IP4_FR_MF, /**< value for "more fragments" flag in fragment */
    NDN_TAG_IP4_FR_DF, /**< value for "don't fragment" flag in fragment */
    NDN_TAG_IP4_PLD_CHECKSUM,
    NDN_TAG_IP4_PLD_CH_OFFSET,
    NDN_TAG_IP4_PLD_CH_DISABLE,
} ndn_ip4_tags_t;


typedef enum {
    NDN_TAG_TCP_SRC_PORT,
    NDN_TAG_TCP_DST_PORT,
    NDN_TAG_TCP_SEQN,
    NDN_TAG_TCP_ACKN,
    NDN_TAG_TCP_HLEN,
    NDN_TAG_TCP_FLAGS,
    NDN_TAG_TCP_WINDOW,
    NDN_TAG_TCP_CHECKSUM,
    NDN_TAG_TCP_URG,
    NDN_TAG_TCP_LOCAL_PORT,
    NDN_TAG_TCP_REMOTE_PORT,
    NDN_TAG_TCP_DATA,
    NDN_TAG_TCP_DATA_SERVER,
    NDN_TAG_TCP_DATA_CLIENT,
    NDN_TAG_TCP_DATA_SOCKET,
} ndn_tcp_tags_t;

typedef enum {
    NDN_TAG_UDP_LOCAL_PORT,
    NDN_TAG_UDP_REMOTE_PORT,
    NDN_TAG_UDP_SRC_PORT,
    NDN_TAG_UDP_DST_PORT,
    NDN_TAG_UDP_LENGTH,
    NDN_TAG_UDP_CHECKSUM,
} ndn_udp_tags_t;


#ifdef __cplusplus
extern "C" {
#endif

extern asn_type_p ndn_ip4_header;
extern asn_type_p ndn_ip4_csap; 

extern asn_type_p ndn_ip4_frag_spec;
extern asn_type_p ndn_ip4_frag_seq;


extern asn_type_p ndn_icmp4_message;
extern asn_type_p ndn_icmp4_csap;


extern asn_type_p ndn_udp_header;
extern asn_type_p ndn_udp_csap;


extern asn_type_p ndn_tcp_header;
extern asn_type_p ndn_tcp_csap;
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_IPSTACK_H__ */
