/** @file
 * @brief Test Environment
 *
 * Check UDP/IP4/ICMP4/IP4/ETH CSAP behaviour
 * when sendind ICMP messages with udp error replies
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
 * $Id$
 */

/** @page ipstack-ip4_send_icmp_error_udp Send ICMP datagram with udp error replu message via udp.ip4.icmp4.ip4.eth CSAP and check UDP socket error
 *
 * @objective Check that udp.ip4.icmp4.ip4.eth CSAP can be used to
 *            send ICMP datagrams with user-specified UDP error reply
 *            messages.
 *
 * @param host_csap     TA with CSAP
 * @param pco           TA with UDP socket
 * @param csap_addr     CSAP local IPv4 address
 * @param sock_addr     CSAP remote IPv4 address
 * @param csap_hwaddr   CSAP local MAC address
 * @param sock_hwaddr   CSAP remote MAC address
 * @param type          ICMP message's type
 * @param code          ICMP message's code
 *
 * @par Scenario:
 *
 * -# Create udp.ip4.icmp4.ip4.eth CSAP on @p pco_csap. 
 * -# Create datagram socket on @p pco_sock.
 * -# Send ICMP message having user-specified 
 *    udp error reply message.
 * -# Check that UDP socket has socket error appropriate to
 *    the sent error message.
 * -# Destroy CSAP and close socket
 *
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "ipstack/ip4_send_icmp_error_udp"

#define TEST_START_VARS         TEST_START_ENV_VARS
#define TEST_START_SPECIFIC     TEST_START_ENV
#define TEST_END_SPECIFIC       TEST_END_ENV

#include "te_config.h"

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#if HAVE_NETINET_IP_ICMP_H
#include <netinet/ip_icmp.h>
#endif

#include "tad_common.h"
#include "rcf_rpc.h"
#include "asn_usr.h"
#include "ndn_eth.h"
#include "ndn_ipstack.h"
#include "tapi_ndn.h"
#include "tapi_tad.h"
#include "tapi_eth.h"
#include "tapi_ip4.h"
#include "tapi_icmp4.h"
#include "tapi_env.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_test.h"
#include "tapi_rpc_params.h"

int
main(int argc, char *argv[])
{
    const uint16_t              ip_eth = ETHERTYPE_IP;
    tapi_env_host              *host_csap = NULL;
    rcf_rpc_server             *pco = NULL;
    const struct sockaddr      *csap_addr;
    const struct sockaddr      *sock_addr;
    const struct sockaddr      *csap_hwaddr;
    const struct sockaddr      *sock_hwaddr;
    const struct if_nameindex  *csap_if;
    int                         type;
    int                         code;
    rpc_errno                   exp_errno;

    csap_handle_t               send_csap = CSAP_INVALID_HANDLE;
    asn_value                  *csap_spec = NULL;
    asn_value                  *template = NULL;
    
    int                         recv_socket = -1;

    int                         set_option;
    int                         sock_error;

    TEST_START; 

    TEST_GET_HOST(host_csap);
    TEST_GET_PCO(pco);
    TEST_GET_ADDR(csap_addr);
    TEST_GET_ADDR(sock_addr);
    TEST_GET_ADDR(csap_hwaddr);
    TEST_GET_ADDR(sock_hwaddr);
    TEST_GET_IF(csap_if);
    TEST_GET_INT_PARAM(type);
    TEST_GET_INT_PARAM(code);
    TEST_GET_ERRNO_PARAM(exp_errno);

    /* Create UDP socket */
    recv_socket = rpc_socket(pco, RPC_PF_INET, 
                             RPC_SOCK_DGRAM, RPC_IPPROTO_UDP);
    /* Bind socket to the local IP/port */
    CHECK_RC(rpc_bind(pco, recv_socket, sock_addr));

    /* 
     * Prepare socket to receive ICMP error
     * messages
     */
    set_option = 1;
    CHECK_RC(rpc_setsockopt(pco, recv_socket,
                            RPC_IP_RECVERR,
                            &set_option));
    
    /* Add udp layer to the CSAP (to send UDP error message) */
    CHECK_RC(tapi_tad_csap_add_layer(&csap_spec, 
                                     ndn_udp_csap,
                                     "#udp",
                                     NULL));
    /* Add ip4 layer to the CSAP (to send UDP error message) */
    CHECK_RC(tapi_tad_csap_add_layer(&csap_spec, 
                                     ndn_ip4_csap,
                                     "#ip4",
                                     NULL));
    /* Add icmp4 layer to the CSAP */
    CHECK_RC(tapi_tad_csap_add_layer(&csap_spec, 
                                     ndn_icmp4_csap,
                                     "#icmp4",
                                     NULL));
    /* Add ip4 layer to the CSAP */
    CHECK_RC(tapi_ip4_add_csap_layer(&csap_spec, 
                                     SIN(csap_addr)->sin_addr.s_addr, 
                                     SIN(sock_addr)->sin_addr.s_addr,
                                     IPPROTO_ICMP,
                                     -1,
                                     -1));
    /* Add ethernet layer to the CSAP*/
    CHECK_RC(tapi_eth_add_csap_layer(&csap_spec,
                                     csap_if->if_name,
                                     TAD_ETH_RECV_NO,
                                     sock_hwaddr->sa_data,
                                     csap_hwaddr->sa_data,
                                     &ip_eth,
                                     TE_BOOL3_ANY,
                                     TE_BOOL3_ANY));
    /* Create CSAP */
    CHECK_RC(tapi_tad_csap_create(host_csap->ta, 0,
                                  "udp.ip4.icmp4.ip4.eth",
                                  csap_spec,
                                  &send_csap));

    /* Prepare data-sending template */
    /* 
     * Prepare UDP error message's UDP header, src_port being
     * port to which UDP socket is bound
     */
    CHECK_RC(tapi_udp_add_pdu(&template, NULL,
                              FALSE, 
                              htons(SIN(sock_addr)->sin_port),
                              htons(SIN(csap_addr)->sin_port)));
    /*
     * Prepare UDP error message's IP header, src_addr
     * being IP to which UDP socket is bound
     */
    CHECK_RC(tapi_ip4_add_pdu(&template, NULL,
                              FALSE,
                              SIN(sock_addr)->sin_addr.s_addr,
                              SIN(csap_addr)->sin_addr.s_addr,
                              IPPROTO_UDP,
                              -1,
                              -1));
    /* Fill ICMP error message's type and code fields */
    CHECK_RC(tapi_icmp4_add_pdu(&template, NULL,
                                FALSE, type, code));
    /* Prepare ICMP error message's IP header */
    CHECK_RC(tapi_ip4_add_pdu(&template, NULL,
                              FALSE,
                              SIN(csap_addr)->sin_addr.s_addr,
                              SIN(sock_addr)->sin_addr.s_addr,
                              IPPROTO_ICMP,
                              -1, 
                              -1));
    /* Prepare ICMP error message's ETH header */
    CHECK_RC(tapi_eth_add_pdu(&template, NULL,
                              FALSE,
                              sock_hwaddr->sa_data,
                              csap_hwaddr->sa_data,
                              &ip_eth,
                              TE_BOOL3_ANY,
                              TE_BOOL3_ANY));
    
    /* Start sending data via CSAP */
    CHECK_RC(tapi_tad_trsend_start(host_csap->ta, 0, send_csap,
                                   template, RCF_MODE_NONBLOCKING));

    MSLEEP(100);

    /* Get socket error */
    CHECK_RC(rpc_getsockopt(pco, recv_socket,
                            RPC_SO_ERROR,
                            &sock_error));
    /* Check socket error */
    if (sock_error != (int)exp_errno)
        TEST_FAIL("SO_ERROR is set to %s "
                  "instead of expected %s",
                  errno_rpc2str(sock_error),
                  errno_rpc2str(exp_errno));
    
    TEST_SUCCESS;

cleanup:

    CLEANUP_RPC_CLOSE(pco, recv_socket);

    asn_free_value(template);
    asn_free_value(csap_spec);
    
    if (host_csap != NULL)
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(host_csap->ta, 
                                             0, 
                                             send_csap));

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
