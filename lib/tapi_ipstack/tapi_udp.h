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
#include "tapi_ip.h"


/** Structure of UDP/IPv4 datagram */
typedef struct udp4_datagram {
    struct timeval      ts;          /**< packet timestamp */
    struct in_addr      src_addr;    /**< source address */
    struct in_addr      dst_addr;    /**< destination address */
    uint16_t            src_port;    /**< source port */
    uint16_t            dst_port;    /**< destination port */
    uint16_t            payload_len; /**< payload length */
    uint8_t            *payload;     /**< UDP payload */
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
extern int ndn_udp4_dgram_to_plain(asn_value_p pkt, 
                                   struct udp4_datagram **udp_dgram);



/**
 * Creates usual 'data.udp.ip4' CSAP on specified Test Agent and got its
 * handle.
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param loc_addr_str  Character string with local IP address (or NULL)
 * @param rem_addr_str  Character string with remote IP address (or NULL)
 * @param loc_port      Local UDP port (may be zero)
 * @param rem_port      Remote UDP port (may be zero)
 * @param udp_csap      Identifier of an SNMP CSAP (OUT)
 * 
 * @return Zero on success or error code.
 */
extern int tapi_udp4_csap_create(const char *ta_name, int sid,
                                 const char *loc_addr_str,
                                 const char *rem_addr_str,
                                 uint16_t loc_port, uint16_t rem_port,
                                 csap_handle_t *udp_csap);


/**
 * Send UDP datagram via 'data.udp.ip4' CSAP.
 * 
 * @param ta_name       Test Agent name.
 * @param sid           RCF SID
 * @param csap          identifier of an SNMP CSAP (OUT).
 * @param udp_dgram     UDP datagram to be sent.
 * 
 * @return zero on success or error code.
 */
extern int tapi_udp4_dgram_send(const char *ta_name, int sid,
            csap_handle_t csap, const udp4_datagram *udp_dgram);



/**
 * Start receiving of UDP datagrams via 'data.udp.ip4' CSAP, non-block
 * method.
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param csap          Identifier of an SNMP CSAP (OUT)
 * @param udp_dgram     UDP datagram with pattern for filter
 * @param mode          Count received packets only or store packets
 *                      to get to the test side later
 * 
 * @return Zero on success or error code.
 */
extern int tapi_udp4_dgram_recv_start(const char *ta_name,  int sid,
            csap_handle_t csap, const  udp4_datagram *udp_dgram,
            rcf_trrecv_mode mode);

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
 * @param udp_dgram     UDP datagram with pattern for filter
 * @param mode          Count received packets only or store packets
 *                      to get to the test side later
 *
 * @return Zero on success or error code.
 */
extern int tapi_udp_ip4_eth_recv_start(const char *ta_name, int sid,
                                       csap_handle_t csap,
                                       const udp4_datagram *udp_dgram,
                                       rcf_trrecv_mode mode);

#if 0
/**
 * Receive some number of UDP datagrams via 'data.udp.ip4' CSAP, block
 * method.
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param csap          Identifier of an SNMP CSAP (OUT)
 * @param number        Number of dgrams to be received
 * @param timeout       Total timeout for receive, in seconds
 * @param udp_dgram     UDP datagram with pattern for filter 
 * @param callback      Callback function, which will be call for each 
 *                      received packet
 * @param user_data     Opaque data to be passed into the callback function
 * 
 * @return Zero on success or error code.
 */
extern int tapi_udp4_dgram_recv(const char *ta_name, int sid,
                                csap_handle_t csap,
                                int number, int timeout,
                                const udp4_datagram *udp_dgram, 
                                udp4_callback callback, void *user_data);
#endif

/**
 * Send UDP datagram via 'data.udp.ip4' CSAP and receive response to it
 * (that is first received UDP datagram with reverse to the sent 
 * source/destination addresses and ports).
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param csap          Identifier of an SNMP CSAP (OUT)
 * @param timeout       Timeout for receive, in seconds
 * @param dgram_sent    UDP datagram to be sent
 * @param dgram_recv    Location for received UDP datagram; memory for
 *                      payload will be allocated by TAPI and should be
 *                      freed by user
 * 
 * @return Zero on success or error code.
 */
extern int tapi_udp4_dgram_send_recv(const char *ta_name, int sid, 
                                     csap_handle_t csap,
                                     unsigned int timeout,
                                     const udp4_datagram *dgram_sent,
                                     udp4_datagram *dgram_recv);

/**
 * Create 'udp.ip4.eth' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param loc_mac       Local MAC address (or NULL)
 * @param rem_mac       Remote MAC address (or NULL)
 * @param loc_addr      Local IP address in network byte order (or NULL)
 * @param rem_addr      Remote IP address in network byte order (or NULL)
 * @param loc_port      Local UDP port in network byte order
 * @param rem_port      Remote UDP port in network byte order
 * @param udp_csap      Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
extern int tapi_udp_ip4_eth_csap_create(const char *ta_name, int sid,
                                        const char *eth_dev,
                                        const uint8_t *loc_mac,
                                        const uint8_t *rem_mac,
                                        in_addr_t loc_addr,
                                        in_addr_t rem_addr,
                                        uint16_t loc_port,
                                        uint16_t rem_port,
                                        csap_handle_t *udp_csap);

#endif /* !__TE_TAPI_UDP_H__ */
