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
 * $Id: $
 */

/** @page ipstack-ip4_raw_send Send data via ip4.eth CSAP and receive it via RAW socket
 *
 * @objective Check that ip4.eth CSAP can send correctly formed
 *            IPv4 datagrams to receive them via IPv4 raw socket.
 *
 * @param pco_csap      TA with CSAP
 * @param pco_sock      TA with RAW socket
 * @param csap_addr     CSAP local IPv4 address
 * @param sock_addr     CSAP remote IPv4 address
 * @param csap_hwaddr   CSAP local MAC address
 * @param sock_hwaddr   CSAP remote MAC address
 * @param pld_len       Datagram's payload length
 * @param proto         Datagram's protocol
 *
 * @par Scenario:
 *
 * -# Create ip4.eth CSAP on @p pco_csap. Specify local/remote addresses
 *    and protocol to use.
 * -# Create IPv4 raw socket with protocol @p proto on @p pco_sock.
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

#define TE_TEST_NAME    "ipstack/ip4_raw_send"

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
#include "tapi_ndn.h"
#include "tapi_tad.h"
#include "tapi_ip4.h"
#include "tapi_env.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_test.h"

int
main(int argc, char *argv[])
{
    int                         proto;
    int                         pld_len;

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
    size_t                      recv_buf_len;


    TEST_START; 

    TEST_GET_PCO(pco_csap);
    TEST_GET_PCO(pco_sock);
    TEST_GET_ADDR(csap_addr);
    TEST_GET_ADDR(sock_addr);
    TEST_GET_ADDR(csap_hwaddr);
    TEST_GET_ADDR(sock_hwaddr);
    TEST_GET_IF(csap_if);
    TEST_GET_INT_PARAM(proto);
    TEST_GET_INT_PARAM(pld_len);

    send_buf = te_make_buf_by_len(pld_len);
    recv_buf = te_make_buf_min(pld_len, &recv_buf_len);

    /* Create RAW socket */
    recv_socket = rpc_socket(pco_sock, RPC_PF_INET, RPC_SOCK_RAW, proto);
    
    rc = tapi_ip4_eth_csap_create(pco_csap->ta, 0, csap_if->if_name,
                                  TAD_ETH_RECV_NO,
                                  csap_hwaddr->sa_data,
                                  sock_hwaddr->sa_data,
                                  SIN(csap_addr)->sin_addr.s_addr,
                                  SIN(sock_addr)->sin_addr.s_addr,
                                  proto,
                                  &ip4_send_csap);
    if (rc != 0)
        TEST_FAIL("CSAP create failed"); 
    
    /* Prepare data-sending template */
    CHECK_RC(tapi_tad_tmpl_ptrn_add_payload_plain(&template, FALSE,
                                                  send_buf, pld_len));

    /* Start sending data */
    rc = tapi_tad_trsend_start(pco_csap->ta, 0, ip4_send_csap,
                               template, RCF_MODE_BLOCKING);
    if (rc != 0) 
        TEST_FAIL("send start failed");

    /* Start receiving data */
    rc = rpc_recv(pco_sock, recv_socket, recv_buf, recv_buf_len, 0);
    if (rc != pld_len)
        TEST_FAIL("Unexpected length of received payload: %d vs %d",
                  rc, pld_len);

    if (memcmp(send_buf, recv_buf, pld_len) != 0)
        TEST_FAIL("Received payload does not match send ones:%Tm%Tm",
                  send_buf, pld_len, recv_buf, pld_len);

    TEST_SUCCESS;

cleanup:

    CLEANUP_RPC_CLOSE(pco_sock, recv_socket);
    
    asn_free_value(template);
    
    if (pco_csap != NULL)
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(pco_csap->ta, 
                                             0, ip4_send_csap));

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
