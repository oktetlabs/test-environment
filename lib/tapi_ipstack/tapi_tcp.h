/** @file
 * @brief Test API for TAD. ipstack CSAP
 *
 * Declarations of TAPI methods for raw-TCP CSAP.
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


#ifndef __TE_TAPI_TAD_H__
#define __TE_TAPI_TAD_H__

#include <netinet/in.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "tapi_ip.h"


/**
 * Prepare ASN Pattern-Unit value for 'tcp.ip4.eth' CSAP.
 * 
 * @param src_mac_addr  Source MAC address (or NULL)
 * @param dst_mac_addr  Destination MAC address (or NULL)
 * @param src_ip4_addr  Source IP address in network order (or NULL)
 * @param dst_ip4_addr  Destination IP address in network order (or NULL)
 * @param result_value  Location for pointer to new ASN value (OUT)
 * 
 * @return Zero on success or error code.
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
 * Creates 'tcp.ip4.eth' CSAP
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param loc_addr      Local IP address in network order (or NULL)
 * @param rem_addr      Remote IP address in network order (or NULL)
 * @param loc_port      Local TCP port in HOST byte order 
 * @param rem_port      Remote TCP port in HOST byte order 
 * @param tcp_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern int tapi_tcp_ip4_eth_csap_create(const char *ta_name, int sid, 
                                        const char *eth_dev,
                                        const uint8_t *loc_addr,
                                        const uint8_t *rem_addr,
                                        uint16_t loc_port,
                                        uint16_t rem_port,
                                        csap_handle_t *tcp_csap);


/**
 * Start receiving of IPv4 packets 'tcp.ip4.eth' CSAP, non-block
 * method. It cannot report received packets, only count them.
 *
 * Started receiving process may be controlled by rcf_ta_trrecv_get, 
 * rcf_ta_trrecv_wait, and rcf_ta_trrecv_stop methods.
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param csap          Identifier of CSAP
 * @param src_addr      Source IP address in network order (or NULL)
 * @param dst_addr      Destination IP address in network order (or NULL)
 * @param loc_port      Local TCP port in HOST byte order 
 * @param rem_port      Remote TCP port in HOST byte order 
 * @param timeout       Timeout of operation (in milliseconds, 
 *                      zero for infinitive)
 * @param num           nubmer of packets to be caugth
 * 
 * @return Zero on success or error code.
 */
extern int tapi_tcp_ip4_eth_recv_start(const char *ta_name, int sid, 
                        csap_handle_t csap,
                        const uint8_t *src_addr, const uint8_t *dst_addr,
                        uint16_t loc_port, uint16_t rem_port,
                        unsigned int timeout, int num);


/**
 * Prepare ASN Pattern-Unit value for 'tcp.ip4.eth' CSAP.
 * 
 * @param src_addr      Source IP address in network order (or NULL)
 * @param dst_addr      Destination IP address in network order (or NULL)
 * @param src_port      Source TCP port in HOST byte order 
 * @param dst_port      Destination TCP port in HOST byte order 
 * @param result_value  Location for pointer to new ASN value
 * 
 * @return Zero on success or error code.
 */
extern int tapi_tcp_ip4_pattern_unit(
                        const uint8_t *src_addr, const uint8_t *dst_addr,
                        uint16_t src_port, uint16_t dst_port,
                        asn_value **result_value);


typedef struct tcp_message_t {
    struct sockaddr_storage source_sa;
    struct sockaddr_storage dest_sa;

    uint8_t *payload;
    size_t pld_len;
} tcp_message_t;

/** 
 * Callback type for receiving data in TCP connection.
 *
 * @param pkt           Received TCP datagram. After return from this
 *                      callback memory block under this datagram will
 *                      be freed. 
 * @param userdata      Parameter, provided by the caller.
 */
typedef void (*tcp_callback)(const tcp_message_t *pkt, void *userdata);


/**
 * Modes for connection establishment.
 */
typedef enum {
    TAPI_TCP_SERVER,
    TAPI_TCP_CLIENT,
} tapi_tcp_mode_t;

/**
 * Modes for TCP messages/acks exchange methods 
 */
typedef enum {
    TAPI_TCP_AUTO,      /**< Fill seq or ack number automatically */
    TAPI_TCP_EXPLICIT,  /**< Fill seq or ack number with specified value */
    TAPI_TCP_QUIET,     /**< Do NOT fill seq or ack number */ 
} tapi_tcp_protocol_mode_t;

typedef int tapi_tcp_handler_t;

/**
 * Type for SEQ and ACK numbers
 */
typedef uint32_t tapi_tcp_pos_t;

/**
 * Initialize TCP connection. This method blocks until connection is 
 * established or timeout expired.
 *
 * @param agt           name of TA;
 * @param mode          TCP mode: server or client, really, this means
 *                      choice "wait for SYN" or "send SYN first";
 * @param local_addr    local socket address, unspecified values 
 *                      are zeroes, (IN/OUT);
 * @param remote_addr   remote socket address, unspecified values 
 *                      are zeroes, (IN/OUT);
 * @param timeout       time in milliseconds, while TA should wait for 
 *                      SYN or ACK for his SYN;
 * @param handle        TAPI handler of created TCP connection (OUT);
 *
 * @return status code
 */
extern int tapi_tcp_init_connection(const char *agt, tapi_tcp_mode_t mode, 
                                    struct sockaddr *local_addr, 
                                    struct sockaddr *remote_addr, 
                                    int timeout,
                                    tapi_tcp_handler_t *handler);

/**
 * Correct close TCP connection, established by 'tapi_tcp_init_connection'.
 *
 * @param handler       TAPI handler of TCP connection;
 * @param timeout       time in milliseconds, while TA should wait 
 *                      for answer for FIN;
 *
 * @return status code
 */
extern int tapi_tcp_close_connection(tapi_tcp_handler_t handler, 
                                     int timeout);

/**
 * Send TCP message via established connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 * @param payload       data for message payload;
 * @param len           length of payload;
 * @param seq_mode      mode of insertion SEQ number into message;
 * @param seqn          explicit SEQ number for message, used only
 *                      if 'seq_mode' passed is TAPI_TCP_EXPLICIT;
 * @param ack_mode      mode of insertion ACK number into message;
 * @param ackn          explicit ACK number for message, used only
 *                      if 'ack_mode' passed is TAPI_TCP_EXPLICIT;
 *
 * @return status code
 */
extern int tapi_tcp_send_msg(tapi_tcp_handler_t handler,
                             uint8_t *payload, size_t len,
                             tapi_tcp_protocol_mode_t seq_mode, 
                             tapi_tcp_pos_t seqn,
                             tapi_tcp_protocol_mode_t ack_mode, 
                             tapi_tcp_pos_t ackn);

/**
 * Wait for next incoming TCP message in connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 * @param ack_mode      mode of sending ACK to the message, for this
 *                      method valid values are 'AUTO' or 'QUIET' only;
 * @param payload       pointer to message payload buffer (OUT);
 * @param len           length of buffer/ got payload (IN/OUT);
 * @param seqn_got      received SEQ number or zero (OUT);
 * @param ackn_got      received ACK number or zero (OUT);
 *
 * @return status code
 */
extern int tapi_tcp_recv_msg(tapi_tcp_handler_t handler,
                             tapi_tcp_protocol_mode_t ack_mode, 
                             uint8_t *buffer, size_t *len, 
                             tapi_tcp_pos_t *seqn_got, 
                             tapi_tcp_pos_t *ackn_got);

/**
 * Send ACK via established TCP connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 * @param ackn          ACK number
 *
 * @return status code
 */
extern int tapi_tcp_send_ack(tapi_tcp_handler_t handler, 
                             tapi_tcp_pos_t ackn);


/**
 * Return last received SEQ number in established TCP connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 *
 * @return SEQ number of zero if 'hanlder' not valid.
 */
extern tapi_tcp_pos_t tapi_tcp_last_seqn_got(tapi_tcp_handler_t handler);

/**
 * Return last received ACK number in established TCP connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 *
 * @return ACK number of zero if 'hanlder' not valid.
 */
extern tapi_tcp_pos_t tapi_tcp_last_ackn_got(tapi_tcp_handler_t handler);

/**
 * Return last sent SEQ number in established TCP connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 *
 * @return SEQ number of zero if 'hanlder' not valid.
 */
extern tapi_tcp_pos_t tapi_tcp_last_seqn_sent(tapi_tcp_handler_t handler);

/**
 * Return last sent ACK number in established TCP connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 *
 * @return ACK number of zero if 'hanlder' not valid.
 */
extern tapi_tcp_pos_t tapi_tcp_last_ackn_sent(tapi_tcp_handler_t handler);

/**
 * Return next SEQ number to be sent in established TCP connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 *
 * @return SEQ number of zero if 'hanlder' not valid.
 */
extern tapi_tcp_pos_t tapi_tcp_next_seqn(tapi_tcp_handler_t handler);

/**
 * Return next ACK number to be sent in established TCP connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 *
 * @return ACK number of zero if 'hanlder' not valid.
 */
extern tapi_tcp_pos_t tapi_tcp_next_ackn(tapi_tcp_handler_t handler);


#endif /* !__TE_TAPI_TAD_H__ */
