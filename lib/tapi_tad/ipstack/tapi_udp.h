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


#ifndef __TE_TAPI_UDP_H__
#define __TE_TAPI_UDP_H__

#include <assert.h>
#include <netinet/in.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "tapi_tad.h"
#include "tapi_ip4.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add UDP layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param local_port    Default local port in network byte order
 *                      or -1 to keep unspecified
 * @param remote_port   Default remote port in network byte order
 *                      or -1  to keep unspecified
 *
 * @retval Status code.
 */
extern te_errno tapi_udp_add_csap_layer(asn_value **csap_spec,
                                        int         local_port,
                                        int         remote_port);

/**
 * Add UDP PDU as the last PDU to the last unit of the traffic 
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param src_port      Source port in network byte order or -1. 
 *                      If -1, default value is specified during CSAP
 *                      creation (as local port for sending, as remote
 *                      port for receiving).
 * @param dst_addr      Destination port in network byte order or -1.
 *                      If -1, default value is specified during CSAP
 *                      creation (as remote port for sending, as local
 *                      port for receiving).
 *
 * @return Status code.
 */
extern te_errno tapi_udp_add_pdu(asn_value **tmpl_or_ptrn,
                                 asn_value **pdu,
                                 te_bool     is_pattern,
                                 int         src_port,
                                 int         dst_port);

/**
 * Create 'udp.ip4.eth' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Bitmask with receive mode, see 'enum
 *                      tad_eth_recv_mode' in tad_common.h.
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param loc_mac       Local MAC address (or NULL)
 * @param rem_mac       Remote MAC address (or NULL)
 * @param loc_addr      Local IP address in network byte order (or NULL)
 * @param rem_addr      Remote IP address in network byte order (or NULL)
 * @param loc_port      Local UDP port in network byte order or -1
 * @param rem_port      Remote UDP port in network byte order or -1
 * @param udp_csap      Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
extern te_errno tapi_udp_ip4_eth_csap_create(const char    *ta_name,
                                             int            sid,
                                             const char    *eth_dev,
                                             unsigned int   receive_mode,
                                             const uint8_t *loc_mac,
                                             const uint8_t *rem_mac,
                                             in_addr_t      loc_addr,
                                             in_addr_t      rem_addr,
                                             int            loc_port,
                                             int            rem_port,
                                             csap_handle_t *udp_csap);
/**
 * Create 'udp.ip4' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param loc_addr      Local IP address in network byte order (or NULL)
 * @param rem_addr      Remote IP address in network byte order (or NULL)
 * @param loc_port      Local UDP port in network byte order or -1
 * @param rem_port      Remote UDP port in network byte order or -1
 * @param udp_csap      Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
extern te_errno tapi_udp_ip4_csap_create(const char    *ta_name,
                                         int            sid,
                                         const char    *ifname,
                                         in_addr_t      loc_addr,
                                         in_addr_t      rem_addr,
                                         int            loc_port,
                                         int            rem_port,
                                         csap_handle_t *udp_csap);



/** Structure of UDP/IPv4 datagram */
typedef struct udp4_datagram {
    struct timeval  ts;          /**< packet timestamp */
    struct in_addr  src_addr;    /**< source address */
    struct in_addr  dst_addr;    /**< destination address */
    uint16_t        src_port;    /**< source port in host byte order */
    uint16_t        dst_port;    /**< destination port in host byte order */
    uint16_t        payload_len; /**< payload length */
    uint8_t        *payload;     /**< UDP payload */
} udp4_datagram;

/** 
 * Callback function for the receiving UDP datagrams.
 *
 * @param pkt           Received UDP datagram. After return from this
 *                      callback memory block under this datagram will
 *                      be freed. 
 * @param userdata      Parameter, provided by the caller of
 *                      tapi_snmp_walk().
 */
typedef void (*udp4_callback)(const udp4_datagram *pkt, void *userdata);



/**
 * Convert UDP.IPv4 datagram ASN.1 value to plain C structure.
 *
 * @param pkt           ASN.1 value of type Raw-Packet.
 * @param udp_dgram     converted structure (OUT).
 *
 * @return zero on success or error code.
 *
 * @note Function allocates memory under dhcp_message data structure, which
 * should be freed with dhcpv4_message_destroy
 */
extern int ndn_udp4_dgram_to_plain(asn_value *pkt, 
                                   struct udp4_datagram **udp_dgram);



/**
 * Prepare callback data to be passed in tapi_tad_trrecv_{wait,stop,get}
 * to process received UDP packet.
 *
 * @param callback        Callback for UDP packets handling
 * @param user_data       User-supplied data to be passed to @a callback
 *
 * @return Pointer to allocated callback data or NULL.
 */
extern tapi_tad_trrecv_cb_data *tapi_udp_ip4_eth_trrecv_cb_data(
                                    udp4_callback  callback,
                                    void          *user_data);

/**
 * Start receiving of UDP datagrams via 'udp.ip4.eth' CSAP, non-block
 * method.
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param csap          Identifier of CSAP
 * @param mode          Count received packets only or store packets
 *                      to get to the test side later
 *
 * @return Zero on success or error code.
 */
extern int tapi_udp_ip4_eth_recv_start(const char *ta_name, int sid,
                                       csap_handle_t csap,
                                       rcf_trrecv_mode mode);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_UDP_H__ */
