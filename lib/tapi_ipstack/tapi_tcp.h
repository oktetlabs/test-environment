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
#include "tapi_ip.h"

#define TCP_FIN_FLAG    0x01
#define TCP_SYN_FLAG    0x02
#define TCP_RST_FLAG    0x04
#define TCP_PSH_FLAG    0x08
#define TCP_ACK_FLAG    0x10
#define TCP_URG_FLAG    0x20

/**
 * Type for SEQ and ACK numbers
 */
typedef uint32_t tapi_tcp_pos_t;

/*
 * ===================== DATA-TCP methods ======================
 */

/**
 * Creates 'data.tcp.ip4' CSAP, 'server' mode: listening for incoming
 * connections.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param loc_addr      Local IP address in network order (or NULL)
 * @param loc_port      Local TCP port in network byte order 
 * @param tcp_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern int tapi_tcp_server_csap_create(const char *ta_name, int sid, 
                                       in_addr_t loc_addr,
                                       uint16_t loc_port,
                                       csap_handle_t *tcp_csap);
/**
 * Creates 'data.tcp.ip4' CSAP, 'client' mode, connects to remote server.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param loc_addr      Local IP address in network order (or NULL)
 * @param rem_addr      Remote IP address in network order (or NULL)
 * @param loc_port      Local TCP port in network byte order 
 * @param rem_port      Remote TCP port in network byte order 
 * @param tcp_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern int tapi_tcp_client_csap_create(const char *ta_name, int sid, 
                                       in_addr_t loc_addr,
                                       in_addr_t rem_addr,
                                       uint16_t loc_port,
                                       uint16_t rem_port,
                                       csap_handle_t *tcp_csap);
/**
 * Creates 'data.tcp.ip4' CSAP, 'socket' mode, over socket, 
 * accepted on TA from some 'server' CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param socket        socket fd on TA
 * @param tcp_csap      Location for the TCP CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern int tapi_tcp_socket_csap_create(const char *ta_name, int sid, 
                                       int socket,
                                       csap_handle_t *tcp_csap);

/**
 * Wait new one connection from 'server' TCP CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param tcp_csap      TCP CSAP handle
 * @param timeout       timeout in milliseconds
 * @param socket        location for accepted socket fd on TA (OUT)
 *
 * @return  Status of the operation
 */
extern int tapi_tcp_server_recv(const char *ta_name, int sid, 
                                csap_handle_t tcp_csap, 
                                unsigned int timeout, int *socket);

/**
 * Wait some data on connected (i.e. non-server) TCP data CSAP.
 * CSAP may wait for any non-zero amount of bytes or for 
 * exactly specified number. 
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param tcp_csap      TCP CSAP handle
 * @param timeout       timeout in milliseconds
 * @param forward       id of CSAP to which forward recєived messages, 
 *                      may be CSAP_INVALID_HANDLE for disabled forward
 * @param exact         boolean flag: whether CSAP have to wait 
 *                      all specified number of bytes or not
 * @param buf           location for received data, may be NULL 
 *                      if received data is not wanted on test (OUT)
 * @param length        location with/for buffer/data length (IN/OUT)
 *
 * @return  Status of the operation
 */
extern int tapi_tcp_buffer_recv(const char *ta_name, int sid, 
                                csap_handle_t tcp_csap, 
                                unsigned int timeout, 
                                csap_handle_t forward, te_bool exact,
                                uint8_t *buf, size_t *length);

/**
 * Send data via connected (non-server) TCP data CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param tcp_csap      TCP CSAP handle
 * @param buf           pointer to the data to be send 
 * @param length        length of data
 *
 * @return  Status of the operation
 */
extern int tapi_tcp_buffer_send(const char *ta_name, int sid, 
                                csap_handle_t tcp_csap, 
                                uint8_t *buf, size_t length);









/************************************************************************
 * ======================== ROW-TCP methods =============================
 */

/** 
 * Callback function for the receiving TCP datagrams.
 *
 * @param pkt           Received IP packet.
 * @param userdata      Parameter, provided by the caller.
 */
typedef void (*tcp_row_callback)(const asn_value *pkt, void *userdata);


/**
 * Creates 'tcp.ip4.eth' CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param loc_mac       Local MAC address  (or NULL)
 * @param rem_mac       Remote MAC address  (or NULL)
 * @param loc_addr      Local IP address in network order (or NULL)
 * @param rem_addr      Remote IP address in network order (or NULL)
 * @param loc_port      Local TCP port in network byte order 
 * @param rem_port      Remote TCP port in network byte order 
 * @param tcp_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern int tapi_tcp_ip4_eth_csap_create(const char *ta_name, int sid, 
                                        const char *eth_dev,
                                        const uint8_t *loc_mac,
                                        const uint8_t *rem_mac,
                                        in_addr_t loc_addr,
                                        in_addr_t rem_addr,
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
 * @param callback      pointer of method to be called for every packet
 * @param userdata      magic pointer which will be passed to user callback
 * 
 * @return Zero on success or error code.
 */
extern int tapi_tcp_ip4_eth_recv_start(const char *ta_name, int sid, 
                                       csap_handle_t csap,
                                       in_addr_t  src_addr,
                                       in_addr_t  dst_addr,
                                       uint16_t src_port, uint16_t dst_port,
                                       unsigned int timeout, int num,
                                       tcp_row_callback callback,
                                       void *userdata); 


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
                        in_addr_t  src_addr, in_addr_t  dst_addr,
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
extern int tapi_tcp_make_msg(uint16_t src_port, uint16_t dst_port,
                             tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                             te_bool syn_flag, te_bool ack_flag,
                             uint8_t *msg);

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
extern int tapi_tcp_pdu(uint16_t src_port, uint16_t dst_port, 
                        tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                        te_bool syn_flag, te_bool ack_flag,
                        asn_value **pdu);

/**
 * Prepare Traffic-Template ASN value for 'tcp.ip4.eth' CSAP. 
 * It is assumed that all connection parameters 
 * (src/dst MACs, IP, and ports) are already set in CSAP.
 * If it is not, fill there parameters in obtained traffic template
 * explicitely. 
 *
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
extern int tapi_tcp_template(tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                             te_bool syn_flag, te_bool ack_flag,
                             uint8_t *data, size_t pld_len,
                             asn_value **tmpl);











/************************************************************************
 * =================== TCP connection emulate methods ===================
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
 * This method does not blocks. Use 'tapi_tcp_wait_open' for wait 
 * complete open of TCP connection.
 *
 * @param agt           name of TA;
 * @param mode          TCP mode: server or client, really, this means
 *                      choice "wait for SYN" or "send SYN first";
 * @param local_addr    local socket address, unspecified values 
 *                      are zeroes, (IN/OUT);
 * @param remote_addr   remote socket address, unspecified values 
 *                      are zeroes, (IN/OUT);
 * @param window        default window size, or zero
 * @param handler       TAPI handler of created TCP connection (OUT);
 *
 * @return Status code
 */
extern int tapi_tcp_init_connection(const char *agt, tapi_tcp_mode_t mode, 
                                    const struct sockaddr *local_addr, 
                                    const struct sockaddr *remote_addr, 
                                    const char *local_iface,
                                    uint8_t *local_mac,
                                    uint8_t *remote_mac,
                                    int window,
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
                             tapi_ip_frag_spec_t *frags,
                             size_t frag_num);

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





#endif /* !__TE_TAPI_TCP_H__ */
