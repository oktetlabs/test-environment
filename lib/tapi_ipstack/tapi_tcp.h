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



/** 
 * Callback type for receiving data in TCP connection.
 *
 * @param pkt           Received UDP datagram. After return from this
 *                      callback memory block under this datagram will
 *                      be freed. 
 * @param userdata      Parameter, provided by the caller.
 */
typedef void (*tcp_callback)(const tcp_message *pkt, void *userdata);


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
                                    struct sockaddr_in *local_addr, 
                                    struct sockaddr_in *remote_addr, 
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
