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


#ifndef __TE_TAPI_TCP_H__
#define __TE_TAPI_TCP_H__

#include <netinet/in.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "tapi_tad.h"
#include "tapi_ip4.h"
#include "tapi_ip6.h"

#define TCP_FIN_FLAG    0x01
#define TCP_SYN_FLAG    0x02
#define TCP_RST_FLAG    0x04
#define TCP_PSH_FLAG    0x08
#define TCP_ACK_FLAG    0x10
#define TCP_URG_FLAG    0x20

/**
 * Type for SEQ and ACK numbers (host byte order).
 */
typedef uint32_t tapi_tcp_pos_t; 



/*
 * Raw TCP methods 
 */

/**
 * Add TCP layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param local_port    Default local port in network byte order
 *                      or -1 to keep unspecified
 * @param remote_port   Default remote port in network byte order
 *                      or -1  to keep unspecified
 *
 * @retval Status code.
 */
extern te_errno tapi_tcp_add_csap_layer(asn_value **csap_spec,
                                        int         local_port,
                                        int         remote_port);


/**
 * Creates 'tcp.ip4.eth' CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Bitmask with receive mode, see 'enum
 *                      tad_eth_recv_mode' in tad_common.h.
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param loc_mac       Local MAC address  (or NULL)
 * @param rem_mac       Remote MAC address  (or NULL)
 * @param loc_addr      Local IP address in network order or
 *                      htonl(INADDR_ANY)
 * @param rem_addr      Remote IP address in network order or 
 *                      htonl(INADDR_ANY)
 * @param loc_port      Local TCP port in network byte order or -1 
 * @param rem_port      Remote TCP port in network byte order or -1
 * @param tcp_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern te_errno tapi_tcp_ip4_eth_csap_create(const char    *ta_name,
                                             int            sid, 
                                             const char    *eth_dev,
                                             unsigned int   receive_mode,
                                             const uint8_t *loc_mac,
                                             const uint8_t *rem_mac,
                                             in_addr_t      loc_addr,
                                             in_addr_t      rem_addr,
                                             int            loc_port,
                                             int            rem_port,
                                             csap_handle_t *tcp_csap);

/**
 * Creates 'tcp.ip4' CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param ifname        Interface name
 * @param loc_addr      Local IP address in network order or
 *                      htonl(INADDR_ANY)
 * @param rem_addr      Remote IP address in network order or 
 *                      htonl(INADDR_ANY)
 * @param loc_port      Local TCP port in network byte order or -1 
 * @param rem_port      Remote TCP port in network byte order or -1
 * @param tcp_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern te_errno tapi_tcp_ip4_csap_create(const char    *ta_name,
                                         int            sid,
                                         const char    *ifname,
                                         in_addr_t      loc_addr,
                                         in_addr_t      rem_addr,
                                         int            loc_port,
                                         int            rem_port,
                                         csap_handle_t *tcp_csap);


/**
 * Start receiving of IPv4 packets 'tcp.ip4.eth' CSAP, non-block
 * method.
 *
 * Started receiving process may be controlled by rcf_ta_trrecv_get, 
 * rcf_ta_trrecv_wait, and rcf_ta_trrecv_stop methods.
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param csap          Identifier of CSAP
 * @param src_addr      Source IP address in network order or
 *                      htonl(INADDR_ANY)
 * @param dst_addr      Destination IP address in network order or
 *                      htonl(INADDR_ANY)
 * @param src_port      Source TCP port in network byte order or -1
 * @param dst_port      Destination TCP port in network byte order or -1
 * @param timeout       Timeout of operation (in milliseconds, 
 *                      zero for infinitive)
 * @param num           nubmer of packets to be caugth
 * @param mode          Count received packets only or store packets
 *                      to get to the test side later
 * 
 * @return Zero on success or error code.
 */
extern te_errno tapi_tcp_ip4_eth_recv_start(const char      *ta_name,
                                            int              sid, 
                                            csap_handle_t    csap,
                                            in_addr_t        src_addr,
                                            in_addr_t        dst_addr,
                                            int              src_port,
                                            int              dst_port,
                                            unsigned int     timeout,
                                            unsigned int     num,
                                            rcf_trrecv_mode  mode); 


/**
 * Prepare ASN Pattern-Unit value for 'tcp.ip4.eth' CSAP.
 * 
 * @param src_addr      Source IP address in network order or
 *                      htonl(INADDR_ANY)
 * @param dst_addr      Destination IP address in network order or
 *                      htonl(INADDR_ANY)
 * @param src_port      Source TCP port in network byte order or -1
 * @param dst_port      Destination TCP port in network byte order or -1
 * @param result_value  Location for pointer to new ASN value
 * 
 * @return Zero on success or error code.
 */
extern te_errno tapi_tcp_ip4_pattern_unit(in_addr_t   src_addr,
                                          in_addr_t   dst_addr,
                                          int         src_port,
                                          int         dst_port,
                                          asn_value **result_value);


typedef struct tcp_message_t {
    struct      sockaddr_storage source_sa;
    struct      sockaddr_storage dest_sa;

    uint8_t    *payload;
    size_t      pld_len;
} tcp_message_t;

typedef struct tcp4_message {
    struct in_addr  src_addr;    /**< source address */
    struct in_addr  dst_addr;    /**< destination address */
    uint16_t        src_port;    /**< source port in host byte order */
    uint16_t        dst_port;    /**< destination port in host byte order */
    uint8_t         flags;       /**< TCP checksum */
    uint16_t        payload_len; /**< payload length */
    uint8_t        *payload;     /**< TCP payload */
} tcp4_message;

/**
 * Callback types for receiving data in TCP connection.
 *
 * @param pkt           Received TCP datagram. After return from this
 *                      callback memory block under this datagram will
 *                      be freed.
 * @param userdata      Parameter, provided by the caller.
 */
typedef void (*tcp_callback)(const tcp_message_t *pkt, void *userdata);
typedef void (*tcp4_callback)(const tcp4_message *pkt, void *userdata);

/**
 * Convert TCP.IPv4 datagram ASN.1 value to plain C structure.
 *
 * @param pkt           ASN.1 value of type Raw-Packet.
 * @param tcp_message   converted structure (OUT).
 *
 * @return zero on success or error code.
 *
 * @note Function allocates memory under dhcp_message data structure, which
 * should be freed with dhcpv4_message_destroy
 */
extern int ndn_tcp4_message_to_plain(asn_value *pkt,
                                     struct tcp4_message **tcp_msg);

/**
 * Prepare callback data to be passed in tapi_tad_trrecv_{wait,stop,get}
 * to process received TCP packet.
 *
 * @param callback        Callback for TCP packets handling
 * @param user_data       User-supplied data to be passed to @a callback
 *
 * @return Pointer to allocated callback data or NULL.
 */
extern tapi_tad_trrecv_cb_data *tapi_tcp_ip4_eth_trrecv_cb_data(
                                    tcp4_callback  callback,
                                    void          *user_data);


/**
 * Correct fill TCP header by specified parameter values.
 *
 * @param src_port      source port in network byte order
 * @param dst_port      destination port in network byte order
 * @param seqn          sequence number in host byte order
 * @param ackn          acknowledge number in host byte order
 * @param syn_flag      syn flag
 * @param ack_flag      ack flag
 * @param msg           location where TCP header should be placed (OUT)
 *
 * @return Status code.
 */
extern te_errno tapi_tcp_make_msg(uint16_t        src_port,
                                  uint16_t        dst_port,
                                  tapi_tcp_pos_t  seqn,
                                  tapi_tcp_pos_t  ackn, 
                                  te_bool         syn_flag,
                                  te_bool         ack_flag,
                                  uint8_t        *msg);

/**
 * Prepare TCP header PDU by specified parameter values.
 *
 * @param src_port      source TCP port in network byte order
 * @param dst_port      destination TCP port in network byte order
 * @param seqn          sequence number in host byte order
 * @param ackn          acknowledge number in host byte order
 * @param syn_flag      syn flag
 * @param ack_flag      ack flag
 * @param pdu           location for pointer to ASN value (OUT)
 *
 * @return Status code.
 */
extern te_errno tapi_tcp_pdu(int src_port, int dst_port, 
                             tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                             te_bool syn_flag, te_bool ack_flag,
                             asn_value **pdu);

/**
 * Prepare Traffic-Template ASN value for 'tcp.ip4.eth' or 'tcp.ip4' CSAP.
 * It is assumed that all connection parameters 
 * (src/dst MACs, IP, and ports) are already set in CSAP.
 * If it is not, fill there parameters in obtained traffic template
 * explicitely. 
 *
 * @param is_eth_pdu    if TRUE include eth header PDU
 * @param seqn          sequence number in host byte order
 * @param ackn          acknowledge number in host byte order
 * @param syn_flag      syn flag
 * @param ack_flag      ack flag
 * @param data          pointer to data with payload or NULL
 * @param pld_len       length of payload
 * @param tmpl          location for pointer to ASN value (OUT)
 *
 * @return Status code.
 */
extern int tapi_tcp_template_gen(te_bool is_eth_pdu,
                                 tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                                 te_bool syn_flag, te_bool ack_flag,
                                 uint8_t *data, size_t pld_len,
                                 asn_value **tmpl);

/**
 * The same function as tapi_tcp_template_gen() with is_eth_pdu 
 * parameter set to TRUE, so it prepares value for 'tcp.ip4.eth' CSAP.
 */
extern int tapi_tcp_template(tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                             te_bool syn_flag, te_bool ack_flag,
                             uint8_t *data, size_t pld_len,
                             asn_value **tmpl);

/**
 * Prepare Traffic-Pattern ASN value for 'tcp.ip4.eth' or 'tcp.ip4' CSAP.
 * It is assumed that all connection parameters 
 * (src/dst MACs, IP, and ports) are already set in CSAP.
 * If it is not, fill there parameters in obtained traffic template
 * explicitely. 
 *
 * @param is_eth_pdu    if TRUE include eth header PDU  
 * @param seqn          sequence number in host byte order
 * @param ackn          acknowledge number in host byte order
 * @param syn_flag      syn flag
 * @param ack_flag      ack flag
 * @param pattern       location for pointer to ASN value (OUT)
 *
 * @return Status code.
 */
extern int tapi_tcp_pattern_gen(te_bool is_eth_pdu, 
                                tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                                te_bool syn_flag, te_bool ack_flag,
                                asn_value **pattern);

/**
 * The same function as tapi_tcp_template_gen() with is_eth_pdu 
 * parameter set to TRUE, so it prepares value for 'tcp.ip4.eth' CSAP.
 */
extern int tapi_tcp_pattern(tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                            te_bool syn_flag, te_bool ack_flag,
                            asn_value **pattern);

/**
 * Prepare pattern for TCP segment to receive via
 * tcp.[ip4|ip6].eth and tcp.[ip4|ip6] CSAPs
 *
 * @param is_eth_layer  TRUE for tcp.[ip4|ip6].eth CSAPs
 *                      FALSE for tcp.[ip4|ip6] CSAPs
 * @param force_ip6     TRUE for tcp.ip6.eth and tcp.ip6 CSAPs
 *                      FALSE for tcp.ip4.eth and tcp.ip4 CSAPs
 * @param seqn          sequence number in host byte order
 * @param ackn          acknowledge number in host byte order
 * @param urg_flag      URG flag
 * @param ack_flag      ACK flag
 * @param psh_flag      PSH flag
 * @param rst_flag      RST flag
 * @param syn_flag      SYN flag
 * @param fin_flag      FIN flag
 * @param pattern       location for pointer to ASN value (OUT)
 *
 * @return Status code.
 */
extern int tapi_tcp_ip_segment_pattern_gen(te_bool is_eth_layer,
                                           te_bool force_ip6,
                                           tapi_tcp_pos_t seqn,
                                           tapi_tcp_pos_t ackn,
                                           te_bool urg_flag,
                                           te_bool ack_flag,
                                           te_bool psh_flag,
                                           te_bool rst_flag,
                                           te_bool syn_flag,
                                           te_bool fin_flag,
                                           asn_value **pattern);

/**
 * Prepare pattern for TCP segment to receive via
 * tcp.[ip4|ip6].eth and tcp.[ip4|ip6] CSAPs
 *
 * @param is_eth_layer  TRUE for tcp.[ip4|ip6].eth CSAPs
 *                      FALSE for tcp.[ip4|ip6] CSAPs
 * @param force_ip6     TRUE for tcp.ip6.eth and tcp.ip6 CSAPs
 *                      FALSE for tcp.ip4.eth and tcp.ip4 CSAPs
 * @param seqn          sequence number in host byte order
 * @param ackn          acknowledge number in host byte order
 * @param syn_flag      SYN flag
 * @param ack_flag      ACK flag
 * @param pattern       location for pointer to ASN value (OUT)
 *
 * @return Status code.
 */
extern int tapi_tcp_ip_pattern_gen(te_bool is_eth_layer,
                                   te_bool force_ip6,
                                   tapi_tcp_pos_t seqn,
                                   tapi_tcp_pos_t ackn,
                                   te_bool syn_flag,
                                   te_bool ack_flag,
                                   asn_value **pattern);

/**
 * The same function as tapi_tcp_segment_pattern_gen() with force_ip6
 * parameter set to FALSE and is_eth_pdu parameter set to TRUE
 * to prepare pattern for 'tcp.ip4.eth' CSAP.
 */
extern int tapi_tcp_segment_pattern(tapi_tcp_pos_t seqn,
                                    tapi_tcp_pos_t ackn,
                                    te_bool urg_flag, te_bool ack_flag,
                                    te_bool psh_flag, te_bool rst_flag,
                                    te_bool syn_flag, te_bool fin_flag,
                                    asn_value **pattern);

/**
 * The same function as tapi_tcp_segment_pattern() with force_ip6
 * boolean parameter to create pattern for tcp.ip6.eth or tcp.ip4.eth CSAP
 */
extern int tapi_tcp_ip_segment_pattern(te_bool force_ip6,
                                       tapi_tcp_pos_t seqn,
                                       tapi_tcp_pos_t ackn,
                                       te_bool urg_flag, te_bool ack_flag,
                                       te_bool psh_flag, te_bool rst_flag,
                                       te_bool syn_flag, te_bool fin_flag,
                                       asn_value **pattern);

/**
 * Prepare TCP header PDU by specified parameter values.
 * Modified version of tapi_tcp_pdu.
 *
 * @param src_port      source TCP port in network byte order
 * @param dst_port      destination TCP port in network byte order
 * @param seqn          sequence number in host byte order
 * @param ackn          acknowledge number in host byte order
 * @param urg_flag      URG flag
 * @param ack_flag      ACK flag
 * @param psh_flag      PSH flag
 * @param rst_flag      RST flag
 * @param syn_flag      SYN flag
 * @param fin_flag      FIN flag
 * @param pdu           location for pointer to ASN value (OUT)
 *
 * @return Status code.
 */
extern int tapi_tcp_segment_pdu(int src_port, int dst_port,
                                tapi_tcp_pos_t seqn,
                                tapi_tcp_pos_t ackn,
                                te_bool urg_flag, te_bool ack_flag,
                                te_bool psh_flag, te_bool rst_flag,
                                te_bool syn_flag, te_bool fin_flag,
                                asn_value **pdu);

/**
 * Prepare template for TCP segment to send via
 * tcp.[ip4|ip6].eth and tcp.[ip4|ip6] CSAPs
 *
 * @param is_eth_layer  TRUE for tcp.[ip4|ip6].eth CSAPs
 *                      FALSE for tcp.[ip4|ip6] CSAPs
 * @param force_ip6     TRUE for tcp.ip6.eth and tcp.ip6 CSAPs
 *                      FALSE for tcp.ip4.eth and tcp.ip4 CSAPs
 * @param seqn          sequence number in host byte order
 * @param ackn          acknowledge number in host byte order
 * @param urg_flag      URG flag
 * @param ack_flag      ACK flag
 * @param psh_flag      PSH flag
 * @param rst_flag      RST flag
 * @param syn_flag      SYN flag
 * @param fin_flag      FIN flag
 * @param data          pointer to buffer with payload octets
 * @param pld_len       payload length
 * @param tmpl          location for pointer to ASN value (OUT)
 *
 * @return Status code.
 */
extern int tapi_tcp_ip_segment_template_gen(te_bool is_eth_layer,
                                            te_bool force_ip6,
                                            tapi_tcp_pos_t seqn,
                                            tapi_tcp_pos_t ackn,
                                            te_bool urg_flag,
                                            te_bool ack_flag,
                                            te_bool psh_flag,
                                            te_bool rst_flag,
                                            te_bool syn_flag,
                                            te_bool fin_flag,
                                            uint8_t *data, size_t pld_len,
                                            asn_value **tmpl);

/**
 * Prepare template for TCP segment to send via
 * tcp.[ip4|ip6].eth and tcp.[ip4|ip6] CSAPs
 *
 * @param is_eth_layer  TRUE for tcp.[ip4|ip6].eth CSAPs
 *                      FALSE for tcp.[ip4|ip6] CSAPs
 * @param force_ip6     TRUE for tcp.ip6.eth and tcp.ip6 CSAPs
 *                      FALSE for tcp.ip4.eth and tcp.ip4 CSAPs
 * @param seqn          sequence number in host byte order
 * @param ackn          acknowledge number in host byte order
 * @param syn_flag      SYN flag
 * @param ack_flag      ACK flag
 * @param data          pointer to buffer with payload octets
 * @param pld_len       payload length
 * @param tmpl          location for pointer to ASN value (OUT)
 *
 * @return Status code.
 */
extern int tapi_tcp_ip_template_gen(te_bool is_eth_layer,
                                    te_bool force_ip6,
                                    tapi_tcp_pos_t seqn,
                                    tapi_tcp_pos_t ackn,
                                    te_bool syn_flag,
                                    te_bool ack_flag,
                                    uint8_t *data, size_t pld_len,
                                    asn_value **tmpl);

/**
 * The same function as tapi_tcp_ip_segment_template_gen() with
 * force_ip6 parameter set to FALSE and is_eth_pdu parameter set to TRUE
 * to prepare template for 'tcp.ip4.eth' CSAP.
 */
extern int tapi_tcp_segment_template(tapi_tcp_pos_t seqn,
                                     tapi_tcp_pos_t ackn,
                                     te_bool urg_flag, te_bool ack_flag,
                                     te_bool psh_flag, te_bool rst_flag,
                                     te_bool syn_flag, te_bool fin_flag,
                                     uint8_t *data, size_t pld_len,
                                     asn_value **tmpl);

/**
 * The same function as tapi_tcp_segment_template() with force_ip6
 * parameter to create template for tcp.ip6.eth or tcp.ip4.eth CSAP
 */
extern int tapi_tcp_ip_segment_template(te_bool force_ip6,
                                        tapi_tcp_pos_t seqn,
                                        tapi_tcp_pos_t ackn,
                                        te_bool urg_flag, te_bool ack_flag,
                                        te_bool psh_flag, te_bool rst_flag,
                                        te_bool syn_flag, te_bool fin_flag,
                                        uint8_t *data, size_t pld_len,
                                        asn_value **tmpl);

/*
 * TCP connection emulate methods
 */

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
 * Initialize process for open TCP connection.
 * This method does not blocks, only sends SYN (in client mode) 
 * or SYN-ACK (in server mode). Use 'tapi_tcp_wait_open' for wait 
 * complete open of TCP connection.
 * Local IP address should be fake, not retistered on host with TA,
 * because running OS will respond to TCP messages also, and it will
 * make side effects on IUT. 
 * Local MAC address may be fake, and it is desired to use fake, 
 * because running OS sometimes may respond with ICMP messages 
 * to IP packets which is got in its Ethernet device and have foreign
 * IP dst address. 
 *
 * If local MAC address is fake, Ethernet device will be turned to
 * promiscuous mode.
 *
 * @param agt           name of TA;
 * @param mode          TCP mode: server or client, really, this means
 *                      choice "wait for SYN" or "send SYN first";
 * @param local_addr    local socket address, unspecified values 
 *                      are zeroes, (IN/OUT);
 * @param remote_addr   remote socket address, unspecified values 
 *                      are zeroes, (IN/OUT);
 * @param local_iface   name of Ethernet interface on 'agt' host;
 * @param local_mac     local MAC address, may be fake;
 * @param remote_mac    remote MAC address, must be real address of peer
 *                      device;
 * @param window        default window size, or zero
 * @param handler       TAPI handler of created TCP connection (OUT);
 *
 * @return Status code
 */
extern int tapi_tcp_init_connection(const char *agt, tapi_tcp_mode_t mode,
                                    const struct sockaddr *local_addr, 
                                    const struct sockaddr *remote_addr, 
                                    const char *local_iface,
                                    const uint8_t *local_mac,
                                    const uint8_t *remote_mac,
                                    int window,
                                    tapi_tcp_handler_t *handler);
/**
 * Modification of tapi_tcp_init_connection() that supports
 * Layer2 encapsulation.
 *
 * Parameters are same as above plus:
 * 
 * @param enc_vlan      Use VLAN encapsulation
 * @param enc_llc       Use LLC/SNAP encapsulation
 */
extern int tapi_tcp_init_connection_enc(const char *agt,
                                        tapi_tcp_mode_t mode,
                                        const struct sockaddr *local_addr, 
                                        const struct sockaddr *remote_addr, 
                                        const char *local_iface,
                                        const uint8_t *local_mac,
                                        const uint8_t *remote_mac,
                                        int window,
                                        te_bool enc_vlan,
                                        te_bool enc_snap,
                                        tapi_tcp_handler_t *handler);

/**
 * Destroy TAPI TCP connection handler: stop receive process on CSAPs, etc. 
 *
 * This method DOES NOT close TCP connection!
 *
 * @param handler       TAPI handler of TCP connection;
 *
 * @return Status code
 */
extern int tapi_tcp_destroy_connection(tapi_tcp_handler_t handler);

/**
 * Wait for complete process of opening TCP connection: blocks until
 * SYN and ACKs from peer will be received.
 * If timed out, TAPI connection hanlder will be destroyed.
 * 
 * @param handler       TAPI handler of TCP connection;
 * @param timeout       time in milliseconds, while TA should wait for 
 *                      SYN or ACK for his SYN;
 *
 * @return Status code
 */ 
extern int tapi_tcp_wait_open(tapi_tcp_handler_t hanlder, int timeout);

/**
 * Wait for some incoming message in emulated TCP connection. 
 * If message was got, all internal state fields will be updated
 * in accordance. If it was PUSH, it will be stored in internal buffer, 
 * and may be got with tapi_tcp_recv_msg(). 
 *
 * @param handler       TAPI handler of TCP connection;
 * @param timeout       time in milliseconds, while TA should wait for 
 *                      SYN or ACK for his SYN;
 *
 * @return Status code
 */ 
extern int tapi_tcp_wait_msg(tapi_tcp_handler_t handler, int timeout);


/**
 * Send FIN in TCP connection, and wait ACK for it.
 * This is either part of close process, or shutdown for writing.
 * This method blocks until ACK will be received. 
 *
 * @param handler       TAPI handler of TCP connection;
 * @param timeout       time in milliseconds, while TA should wait 
 *                      for answer for FIN;
 *
 * @return Status code
 */
extern int tapi_tcp_send_fin(tapi_tcp_handler_t handler, int timeout);

/**
 * Send FIN in TCP connection with ACK (possibly to FIN sent by peer),
 * and wait ACK for it.
 * This is either part of close process, or shutdown for writing.
 * This method blocks until ACK will be received. 
 *
 * @param handler       TAPI handler of TCP connection;
 * @param timeout       time in milliseconds, while TA should wait 
 *                      for answer for FIN;
 *
 * @return Status code
 */
extern int tapi_tcp_send_fin_ack(tapi_tcp_handler_t handler, int timeout);

/**
 * Send RST in TCP connection.
 *
 * @param handler       TAPI handler of TCP connection;
 * @param timeout       time in milliseconds, while TA should wait 
 *                      for answer for FIN;
 *
 * @return Status code
 */
extern int tapi_tcp_send_rst(tapi_tcp_handler_t handler);

/**
 * Send TCP message via established connection.
 * With seq_mode = TAPI_TCP_AUTO it should be used ONLY for sending
 * subsequent flow of data. If user need to send some strange
 * sequence numbers, he have to use TAPI_TCP_EXPLICIT. 
 * First seqn for 'explicit' data block should be get 
 * with method 'tapi_tcp_next_seqn'.
 * To send some more data in automatic mode after explicit mode
 * user have to store total length of all data,
 * sent in explicit mode, by method 'tapi_tcp_set_next_sent'
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
 * @return Status code
 */
extern int tapi_tcp_send_msg(tapi_tcp_handler_t handler,
                             uint8_t *payload, size_t len,
                             tapi_tcp_protocol_mode_t seq_mode, 
                             tapi_tcp_pos_t seqn,
                             tapi_tcp_protocol_mode_t ack_mode, 
                             tapi_tcp_pos_t ackn, 
                             tapi_ip_frag_spec *frags,
                             size_t frag_num);

/**
 * Prepare template for TCP message via established connection.
 * Fields SEQN and ACKN are filled by default, if user want
 * to set them explicetly , one can do it via ASN lib API.
 *
 * @param handler       TAPI handler of TCP connection;     
 * @param payload       data for message payload;
 * @param len           length of payload;
 * @param tmpl          location for new ASN value with 
 *                      traffic template(OUT);
 *
 * @return Status code
 */
extern int tapi_tcp_conn_template(tapi_tcp_handler_t handler,
                                  uint8_t *payload, size_t len,
                                  asn_value **tmpl);

/**
 * Send explicitely formed traffic template. 
 * Note, that template should be prepareed accurately,
 * if you want to send valid TCP message in connection.
 * IP addresses and TCP pors should NOT be mentioned in template.
 *
 * @param handler       TAPI handler of TCP connection;     
 * @param template      ASN value of type Traffic-Template;
 * @param blk_mode      mode of the operation:
 *                      in blocking mode it suspends the caller
 *                      until sending of all the traffic finishes

 *
 * @return Status code
 */
extern int tapi_tcp_send_template(tapi_tcp_handler_t handler, 
                                  const asn_value *template,
                                  rcf_call_mode_t blk_mode);

/**
 * Wait for next incoming TCP message in connection, if buffer is empty,
 * or get oldest received message in buffer queue.
 *
 * @param handler       TAPI handler of TCP connection;     
 * @param timeout       timeout in milliseconds;
 * @param ack_mode      mode of sending ACK to the message, for this
 *                      method valid values are 'AUTO' or 'QUIET' only;
 * @param payload       pointer to message payload buffer (OUT);
 * @param len           length of buffer/ got payload (IN/OUT);
 * @param seqn_got      place for received SEQ number or NULL (OUT);
 * @param ackn_got      place for received ACK number or NULL (OUT);
 * @param flags         location for TCP flags (OUT);
 *
 * @return Status code
 */
extern int tapi_tcp_recv_msg(tapi_tcp_handler_t handler,
                             int timeout,
                             tapi_tcp_protocol_mode_t ack_mode, 
                             uint8_t *buffer, size_t *len, 
                             tapi_tcp_pos_t *seqn_got, 
                             tapi_tcp_pos_t *ackn_got,
                             uint8_t *flags);

/**
 * Send ACK via established TCP connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 * @param ackn          ACK number
 *
 * @return Status code
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
 * Return last received window in established TCP connection.
 *
 * @param handler       TAPI handler of TCP connection;     
 *
 * @return SEQ number of zero if 'hanlder' not valid.
 */
extern size_t tapi_tcp_last_win_got(tapi_tcp_handler_t handler);

/**
 * Check that a message with FIN flag set was got.
 *
 * @param handler       TAPI handler of TCP connection;     
 *
 * @return @c TRUE if a message with FIN flag set was got,
 *         @c FALSE otherwise.
 */
extern te_bool tapi_tcp_fin_got(tapi_tcp_handler_t handler);

/**
 * Check that a message with RST flag set was got.
 *
 * @param handler       TAPI handler of TCP connection;     
 *
 * @return @c TRUE if a message with RST flag set was got,
 *         @c FALSE otherwise.
 */
extern te_bool tapi_tcp_rst_got(tapi_tcp_handler_t handler);

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

/**
 * Update internal TAPI TCP connection fields according with 
 * next sent data in connection.
 * This is necessary ONLY after sending data in 'explicit' mode.
 *
 * @param handler       TAPI handler of TCP connection
 * @param new_sent_len  length of sent data
 *
 * @return status code
 */
extern int tapi_tcp_update_sent_seq(tapi_tcp_handler_t handler,
                                    size_t new_sent_len);



typedef struct {
    csap_handle_t  tcp_hack_csap;
    uint8_t        rem_mac[6];
    uint8_t        loc_mac[6];
    in_addr_t      rem_ip_addr;
    in_addr_t      loc_ip_addr;
    tapi_tcp_pos_t rem_start_seq;
    tapi_tcp_pos_t loc_start_seq;
    uint16_t       rem_port;
    uint16_t       loc_port;
    te_bool        catched;
} tapi_tcp_reset_hack_t;

/**
 * Initialize RST sending hack framework: create CSAP, start listening of 
 * SYN-ACK. Note, that it will catch FIRST SYN-ACK, matching passed
 * filtering parameters, which user fill in 'context' structure. 
 * All unset fields have to be zero. 
 *
 * @param ta_name       TA name
 * @param sid           RCF session id
 * @param iface         Ethernet interface
 * @param dir_out       boolean flag wheather SYN-ACK will be outgoing;
 *                      TRUE if it is.
 * @param context       pointer to structure with context data, IN/OUT
 *
 * @return status code
 */
extern int tapi_tcp_reset_hack_init(const char *ta_name, int session, 
                                    const char *iface, te_bool dir_out,
                                    tapi_tcp_reset_hack_t *context);

/**
 * Catch SYN-ACK in RST sending hack framework.
 * Note, that it will catch FIRST SYN-ACK, matching passed
 * filtering parameters, which user fill in 'context' structure before
 * call 'init'. 
 *
 * @param ta_name       TA name
 * @param sid           RCF session id
 * @param context       pointer to structure with context data, IN/OUT
 *
 * @return status code
 */
extern int tapi_tcp_reset_hack_catch(const char *ta_name, int session,
                                     tapi_tcp_reset_hack_t *context);

/**
 * Send TCP RST.
 *
 * @param ta_name       TA name
 * @param sid           RCF session id
 * @param context       pointer to structure with context data
 * @param received      received bytes during TCP connection life
 * @param sent          sent bytes during TCP connection life
 *
 * @return status code
 */
extern int tapi_tcp_reset_hack_send(const char *ta_name, int session, 
                                    tapi_tcp_reset_hack_t *context,
                                    size_t received, size_t sent);

/**
 * Clear TCP reset hack context
 *
 * @param ta_name       TA name
 * @param sid           RCF session id
 * @param context       pointer to structure with context data, IN/OUT
 *
 * @return status code
 */
extern int tapi_tcp_reset_hack_clear(const char *ta_name, int session, 
                                     tapi_tcp_reset_hack_t *context);

/**
 * Creates 'tcp.ip6.eth' CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Bitmask with receive mode, see 'enum
 *                      tad_eth_recv_mode' in tad_common.h.
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param loc_mac       Local MAC address  (or NULL)
 * @param rem_mac       Remote MAC address  (or NULL)
 * @param loc_addr      Local IPv6 address
 * @param rem_addr      Remote IPv6 address
 * @param loc_port      Local TCP port in network byte order or -1
 * @param rem_port      Remote TCP port in network byte order or -1
 * @param tcp_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern te_errno tapi_tcp_ip6_eth_csap_create(const char *ta_name, int sid,
                                             const char *eth_dev,
                                             unsigned int receive_mode,
                                             const uint8_t *loc_mac,
                                             const uint8_t *rem_mac,
                                             const uint8_t *loc_addr,
                                             const uint8_t *rem_addr,
                                             int loc_port,
                                             int rem_port,
                                             csap_handle_t *tcp_csap);

#endif /* !__TE_TAPI_TCP_H__ */
