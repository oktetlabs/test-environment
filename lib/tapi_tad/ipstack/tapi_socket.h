/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI TAD Socket
 *
 * @defgroup tapi_tad_socket Socket
 * @ingroup tapi_tad_ipstack
 * @{
 *
 * Declarations of TAPI methods for raw-TCP CSAP.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */


#ifndef __TE_TAPI_SOCKET_H__
#define __TE_TAPI_SOCKET_H__

#include <netinet/in.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "tapi_ip4.h"

/*
 * ===================== DATA-TCP methods ======================
 */

/**
 * Creates 'data.tcp.ip4' CSAP, 'server' mode: listening for incoming
 * connections.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param sa            Local address and port
 * @param tcp_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern te_errno tapi_tcp_server_csap_create(const char *ta_name, int sid,
                                            const struct sockaddr *sa,
                                            csap_handle_t *tcp_csap);
/**
 * Creates 'socket' CSAP, 'TCP client' mode.
 * Connects to remote TCP server.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param loc           Local address and port
 * @param rem           Remote address and port
 * @param tcp_csap      Location for the IPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
extern te_errno tapi_tcp_client_csap_create(const char *ta_name, int sid,
                                            const struct sockaddr *loc,
                                            const struct sockaddr *rem,
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
extern te_errno tapi_tcp_socket_csap_create(const char *ta_name, int sid,
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
extern te_errno tapi_tcp_server_recv(const char *ta_name, int sid,
                                     csap_handle_t tcp_csap,
                                     unsigned int timeout, int *socket);

/**
 * Wait some data on connected (i.e. non-server) 'socket' CSAP.
 *
 * CSAP can wait for any non-zero amount of bytes or for
 * exactly specified number; use 'exact' argument to manage it.
 *
 * For UDP socket parameter 'exact' is ignored, since
 * UDP socket receives data by datagrams.
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
extern te_errno tapi_socket_recv(const char *ta_name, int sid,
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
extern te_errno tapi_socket_send(const char *ta_name, int sid,
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
 * @param loc           Local address and port
 * @param rem           Remote address and port
 * @param csap      Identifier of an SNMP CSAP (OUT)
 *
 * @return Zero on success or error code.
 */
extern te_errno tapi_socket_csap_create(const char *ta_name,
                                        int sid, int type,
                                        const struct sockaddr *loc,
                                        const struct sockaddr *rem,
                                        csap_handle_t *csap);

/**
 * Creates usual 'socket' CSAP of UDP type on specified Test Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param loc           Local address and port
 * @param rem           Remote address and port
 * @param udp_csap      Identifier of an SNMP CSAP (OUT)
 *
 * @return Zero on success or error code.
 */
extern te_errno tapi_udp_csap_create(const char *ta_name, int sid,
                                     const struct sockaddr *loc,
                                     const struct sockaddr *rem,
                                     csap_handle_t *udp_csap);


#endif /* !__TE_TAPI_SOCKET_H__ */

/**@} <!-- END tapi_tad_socket --> */
