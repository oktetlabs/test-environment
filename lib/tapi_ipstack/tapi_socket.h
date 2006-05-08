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


#ifndef __TE_TAPI_SOCKET_H__
#define __TE_TAPI_SOCKET_H__

#include <netinet/in.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "tapi_ip.h"

/*
 * ===================== DATA-TCP methods ======================
 */

/**
 * Creates 'data.tcp.ip4' CSAP, 'server' mode: listening for incoming
 * connections.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param loc_addr      Local IP address in network order
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
 * Creates 'socket' CSAP, 'TCP client' mode. 
 * Connects to remote TCP server.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param loc_addr      Local IP address in network order
 * @param rem_addr      Remote IP address in network order
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
 * Wait some data on connected (i.e. non-server) 'socket' CSAP.
 *
 * CSAP can wait for any non-zero amount of bytes or for 
 * exactly specified number; use 'exact' argument to manage it. 
 *
 * For UDP socket pårameter 'exact' is ignored, since
 * UDP socket receiveѕ data by datagrams. 
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param tcp_csap      CSAP handle
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
extern int tapi_socket_recv(const char *ta_name, int sid, 
                            csap_handle_t csap, 
                            unsigned int timeout, 
                            csap_handle_t forward, te_bool exact,
                            uint8_t *buf, size_t *length);

/**
 * Send data via connected (non-server) 'socket' CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param tcp_csap      CSAP handle
 * @param buf           pointer to the data to be send 
 * @param length        length of data
 *
 * @return  Status of the operation
 */
extern int tapi_socket_send(const char *ta_name, int sid, 
                            csap_handle_t csap, 
                            uint8_t *buf, size_t length);



/**
 * Creates usual 'socket' CSAP of some network type.
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param type          String, type of socket: 
 *                      should be either @c NDN_TAG_SOCKET_TYPE_UDP
 *                      or @c NDN_TAG_SOCKET_TYPE_TCP_CLIENT.
 * @param loc_addr      Local IP address in network order
 * @param rem_addr      Remote IP address in network order
 * @param loc_port      Local TCP port in network byte order 
 * @param rem_port      Remote TCP port in network byte order 
 * @param csap      Identifier of an SNMP CSAP (OUT)
 * 
 * @return Zero on success or error code.
 */
extern int tapi_socket_csap_create(const char *ta_name, int sid, int type,
                                   in_addr_t loc_addr, in_addr_t rem_addr,
                                   uint16_t loc_port, uint16_t rem_port,
                                   csap_handle_t *csap);

/**
 * Creates usual 'socket' CSAP of UDP type on specified Test Agent
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param loc_addr      Local IP address in network order
 * @param rem_addr      Remote IP address in network order
 * @param loc_port      Local TCP port in network byte order 
 * @param rem_port      Remote TCP port in network byte order 
 * @param udp_csap      Identifier of an SNMP CSAP (OUT)
 * 
 * @return Zero on success or error code.
 */
extern int tapi_udp_csap_create(const char *ta_name, int sid,
                                in_addr_t loc_addr, in_addr_t rem_addr,
                                uint16_t loc_port, uint16_t rem_port,
                                csap_handle_t *udp_csap);





#endif /* !__TE_TAPI_SOCKET_H__ */
