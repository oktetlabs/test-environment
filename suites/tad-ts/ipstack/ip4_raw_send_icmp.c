/** @file
 * @brief Test Environment
 *
 * Check IP4/ETH CSAP data-sending behaviour
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * $Id:$
 */

/** @page ipstack-ip4_raw_send Send icmp datagram via ip4.eth CSAP and receive it via RAW socket
 *
 * @objective Check that ip4.eth CSAP can send correctly formed
 *            icmp datagrams to receive them via IPv4 raw socket.
 *
 * @param pco_csap      TA with CSAP
 * @param pco_sock      TA with RAW socket
 * @param csap_addr     CSAP local IPv4 address
 * @param sock_addr     CSAP remote IPv4 address
 * @param csap_hwaddr   CSAP local MAC address
 * @param sock_hwaddr   CSAP remote MAC address
 * @param pld_len       Datagram's payload length
 *
 * @par Scenario:
 *
 * -# Create ip4.eth CSAP on @p pco_csap. Specify local/remote addresses
 *    and icmp protocol to use.
 * -# Create IPv4 raw socket with protocol icmp on @p pco_sock.
 * -# Send IP4 datagrem with specified payload length and protocol.
 * -# Receive datagram via socket.
 * -# Check that correct IPv4 addresses and protocol are set in IPv4
 *    header.
 * -# Check that received IPv4 packet payload is equal to send one.
 * -# Destroy CSAP and close socket
 *
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "ipstack/ip4_raw_send_icmp"

#define TEST_START_VARS         TEST_START_ENV_VARS
#define TEST_START_SPECIFIC     TEST_START_ENV
#define TEST_END_SPECIFIC       TEST_END_ENV

#include "te_config.h"

#if HAVE_STRING_H
#include <string.h>
#endif

#include "tad_common.h"
#include "rcf_rpc.h"
#include "asn_usr.h"
#include "ndn_eth.h"
#include "ndn_ipstack.h"
#include "tapi_ndn.h"
#include "tapi_tad.h"
#include "tapi_ip4.h"
#include "tapi_env.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_test.h"

#include "tad_ts_ipstack.h"

int
main(int argc, char *argv[])
{
    int                         pld_len;

    uint8_t                     ip_header_len;
    uint8_t                     ip_opts_len;

    rcf_rpc_server              *pco_csap = NULL;
    rcf_rpc_server              *pco_sock = NULL;
    const struct sockaddr       *csap_addr;
    const struct sockaddr       *sock_addr;
    const struct sockaddr       *csap_hwaddr;
    const struct sockaddr       *sock_hwaddr;
    const struct if_nameindex   *csap_if;

    csap_handle_t               ip4_send_csap = CSAP_INVALID_HANDLE;
    int                         recv_socket = -1;

    asn_value                   *template = NULL;
    
    void                       *send_buf;
    void                       *recv_buf;
    size_t                      send_buf_len;
    size_t                      recv_buf_len;


    TEST_START; 

    TEST_GET_PCO(pco_csap);
    TEST_GET_PCO(pco_sock);
    TEST_GET_ADDR(csap_addr);
    TEST_GET_ADDR(sock_addr);
    TEST_GET_ADDR(csap_hwaddr);
    TEST_GET_ADDR(sock_hwaddr);
    TEST_GET_IF(csap_if);
    TEST_GET_INT_PARAM(pld_len);

    /* Create send-recv buffers */
    send_buf_len = pld_len + ICMP_HEAD_LEN;
    recv_buf_len = pld_len + ICMP_HEAD_LEN + IP_HEAD_LEN + MAX_OPTIONS_LEN;
    send_buf = te_make_buf_by_len(send_buf_len);
    recv_buf = te_make_buf_by_len(recv_buf_len);

    /* Create RAW socket */
    recv_socket = rpc_socket(pco_sock, RPC_PF_INET, 
                             RPC_SOCK_RAW, RPC_IPPROTO_ICMP);
    if (recv_socket == -1)
        TEST_FAIL("Unable to create RAW socket");
    
    CHECK_RC(tapi_ip4_eth_csap_create(pco_csap->ta, 0, csap_if->if_name,
                                  TAD_ETH_RECV_NO,
                                  csap_hwaddr->sa_data,
                                  sock_hwaddr->sa_data,
                                  SIN(csap_addr)->sin_addr.s_addr,
                                  SIN(sock_addr)->sin_addr.s_addr,
                                  IPPROTO_ICMP,
                                  &ip4_send_csap));
    
    /* Prepare data-sending template */
    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(&template, FALSE,
                                          ndn_ip4_header,
                                          "#ip4", NULL));
    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(&template, FALSE,
                                          ndn_eth_header,
                                          "#eth", NULL));
    CHECK_RC(tapi_tad_tmpl_ptrn_add_payload_plain(&template, FALSE,
                                                  send_buf, send_buf_len));

    /* Start sending data */
    CHECK_RC(tapi_tad_trsend_start(pco_csap->ta, 0, ip4_send_csap,
                               template, RCF_MODE_BLOCKING));

    /* Start receiving data */
    if (rpc_recv(pco_sock, recv_socket, recv_buf, recv_buf_len, 0) <= 0)
        TEST_FAIL("Unable to receive data via socket");
    
    /* Check IP header */
    /* Check header length */
    ip_header_len = ((ip_header *)recv_buf)->ver_len;
    ip_header_len = ip_header_len & 0x0f;
    ip_opts_len = 0;
    if (ip_header_len > 5)
    {
        ip_opts_len = ip_header_len - 5;
        WARN("IP header has %d fields of "
             "additional options", ip_opts_len);
    }

    /* Check total length */
    if (ntohs(((ip_header *)recv_buf)->totlen) !=
            send_buf_len + IP_HEAD_LEN)
        TEST_FAIL("Total length field differs from expected");
    
    /* Check protocol */
    if ( ((ip_header *)recv_buf)->protocol != IPPROTO_ICMP )
        TEST_FAIL("Protocol field was corrupted");

    /* TODO IP checksum */
    
    /* Check IP addresses */
    if (SIN(csap_addr)->sin_addr.s_addr != 
            ((ip_header *)recv_buf)->srcaddr)
        TEST_FAIL("Source IP field was corrupted");
    if (SIN(sock_addr)->sin_addr.s_addr !=
            ((ip_header *)recv_buf)->dstaddr)
        TEST_FAIL("Destination IP field was corrupted");
        
    /* Check protocol header */
    /* Check messages */
    if (((icmp_header *)send_buf)->message !=
            ((icmp_header *)(recv_buf + IP_HEAD_LEN + 
                                       ip_opts_len * 4))->message)
        TEST_FAIL("ICMP message was corrupted");

    /* TODO icmp checksum */
    
    /* Check payload */
    if (memcmp(send_buf + ICMP_HEAD_LEN, recv_buf + ICMP_HEAD_LEN + IP_HEAD_LEN + ip_opts_len * 4, pld_len) != 0)
    {
        RING("Received payload does not match the send one:%Tm%Tm",
                  send_buf + ICMP_HEAD_LEN, pld_len, recv_buf + ICMP_HEAD_LEN + IP_HEAD_LEN + ip_opts_len * 4, pld_len);
        RING_VERDICT("Received payload does not match the send one");
    }

    TEST_SUCCESS;

cleanup:

    CLEANUP_RPC_CLOSE(pco_sock, recv_socket);
    
    asn_free_value(template);
    
    if (pco_csap != NULL)
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(pco_csap->ta, 
                                             0, ip4_send_csap));

    free(send_buf);
    free(recv_buf);

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
