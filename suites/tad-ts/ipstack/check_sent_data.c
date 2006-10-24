/** @file
 * @brief Test Environment
 *
 * Check IP4/ETH CSAP data-sending behaviour
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 * 
 * $Id: $
 */

/** @page ipstack-check_sent_data Send data via CSAP IP4/ETH and receive it via RAW socket.
 *
 * @objective Check that CSAP IP4/ETH can send correctly formed
 *            ip datagrams to receive them via RAW socket.
 *
 * @reference @ref ITU-T-Std-I361
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
 * -# Create CSAP IP4/ETH on @p pco_csap.
 * -# Create RAW socket on @p pco_sock.
 * -# Send IP4 datagrem with specified payload length and protocol.
 * -# Receive datagram via socket.
 * -# Destroy CSAP and close socket
 *
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 */

#define TE_TEST_NAME    "ipstack/check_sent_data"
#define RECV_BUF_LEN    2048

#include "te_config.h"
#include "ipstack-ts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

#include "conf_api.h"
#include "tapi_tad.h"
#include "tapi_eth.h"

#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_ip.h"
#include "tapi_udp.h"
#include "tapi_tcp.h"
#include "tapi_rpc_params.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"


int
main(int argc, char *argv[])
{

    rcf_rpc_server              *pco_csap = NULL;
    rcf_rpc_server              *pco_sock = NULL;
    const struct sockaddr       *csap_addr;
    const struct sockaddr       *sock_addr;
    const struct sockaddr       *csap_hwaddr;
    const struct sockaddr       *sock_hwaddr;
    const struct if_nameindex   *csap_if;

    asn_value                   *template;
    
    csap_handle_t               ip4_send_csap = CSAP_INVALID_HANDLE;
    rpc_socket_domain           domain;
    rpc_socket_proto            proto;
    int                         tst_proto;
    int                         recv_socket = -1;

    int                         syms;
    int                         num_pkts = 1;
    int                         pld_len;

    char                        recv_buf[RECV_BUF_LEN];

    TEST_START; 

    TEST_GET_PCO(pco_csap);
    TEST_GET_PCO(pco_sock);
    TEST_GET_ADDR(csap_addr);
    TEST_GET_ADDR(sock_addr);
    TEST_GET_ADDR(csap_hwaddr);
    TEST_GET_ADDR(sock_hwaddr);
    TEST_GET_IF(csap_if);
    TEST_GET_PROTOCOL(proto);
    TEST_GET_INT_PARAM(pld_len);

    /* Create data-sending CSAP */
    switch (proto){
        
        case RPC_IPPROTO_TCP:
            tst_proto = IPPROTO_TCP;
            break;
            
        case RPC_IPPROTO_UDP:
            tst_proto = IPPROTO_UDP;
            break;
            
        case RPC_IPPROTO_ICMP:
            tst_proto = IPPROTO_ICMP;
            break;
            
        default:
            TEST_FAIL("Unsupported protocol");
            
    }
    
    rc = tapi_ip4_eth_csap_create(pco_csap->ta, 0, csap_if->if_name,
                                  TAD_ETH_RECV_DEF &
                                  ~TAD_ETH_RECV_OTHER,
                                  csap_hwaddr->sa_data,
                                  sock_hwaddr->sa_data,
                                  SIN(csap_addr)->sin_addr.s_addr,
                                  SIN(sock_addr)->sin_addr.s_addr,
                                  tst_proto,
                                  &ip4_send_csap);
    if (rc != 0)
        TEST_FAIL("CSAP create failed"); 

    /* Create RAW socket */
    domain =  rpc_socket_domain_by_addr(sock_addr);
    recv_socket = rpc_socket(pco_sock, domain, 
                             RPC_SOCK_RAW,
                             proto);
    if (recv_socket < 0)
        TEST_FAIL("Socket create failed");
    
    /* Prepare data-sending template */
    rc = asn_parse_value_text("{ arg-sets { simple-for:{begin 1} }, "
                                  "  pdus     { ip4:{}, eth:{}} }",
                                  ndn_traffic_template,
                                  &template, &syms);
    if (rc != 0)
        TEST_FAIL("parse of template failed %X, syms %d", rc, syms);
    rc = asn_write_value_field(template, &num_pkts, sizeof(num_pkts),
                                   "arg-sets.0.#simple-for.end");
    if (rc != 0)
        TEST_FAIL("write num_pkts failed %X", rc);
    rc = asn_write_value_field(template, &pld_len, sizeof(pld_len),
                               "payload.#length");
    if (rc != 0)
        TEST_FAIL("write payload len failed %X", rc); 
    
    /* Start sending data */
    rc = tapi_tad_trsend_start(pco_csap->ta, 0, ip4_send_csap,
                               template, RCF_MODE_NONBLOCKING);
    if (rc != 0) 
        TEST_FAIL("send start failed");

    MSLEEP(200);
    
    /* Start receiving data */
    if ( (rc = rpc_recv(pco_sock, recv_socket, 
                        recv_buf, RECV_BUF_LEN, 0)) != -1)
        RING("%d bytes were received via socket", rc);

    /* Stop sending data */
    rc = rcf_ta_trsend_stop(pco_csap->ta, 0, ip4_send_csap, NULL);
    if (rc != 0) 
        TEST_FAIL("send stop failed"); 

    TEST_SUCCESS;

cleanup:

    CLEANUP_RPC_CLOSE(pco_sock, recv_socket);
    
    if (template != NULL)
        asn_free_value(template);
    
    if (ip4_send_csap != CSAP_INVALID_HANDLE)
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(pco_csap->ta, 
                                             0, ip4_send_csap));

    TEST_END;
}
