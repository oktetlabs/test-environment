/** @file
 * @brief Test Environment
 *
 * Check ICMP4/IP4/ETH CSAP data-sending behaviour
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

/** @page ipstack-ip4_send_icmp Send ICMP datagram via icmp4.ip4.eth CSAP and receive it via RAW socket
 *
 * @objective Check that ip4.eth CSAP can send ICMP 
 *            datagrams with user-specified type, code and 
 *            checksum fields.
 *
 * @param host_csap     TA with CSAP
 * @param pco           TA with RAW socket
 * @param csap_addr     CSAP local IPv4 address
 * @param sock_addr     CSAP remote IPv4 address
 * @param csap_hwaddr   CSAP local MAC address
 * @param sock_hwaddr   CSAP remote MAC address
 * @param type          ICMP message's type
 * @param code          ICMP message's code
 * @param chksum        ICMP message's checksum 
 *                      (correct or corrupted by user)
 *
 * @par Scenario:
 *
 * -# Create icmp4.ip4.eth CSAP on @p pco_csap. 
 * -# Create IPv4 raw socket on @p pco_sock.
 * -# Send IPv4 datagram with ICMP message having
 *    user-specified type, code and checksum.
 * -# Receive datagram via socket.
 * -# In case @p chksum is specified as 'correct', check that 
 *    ICMP message has correctly formed type, code and checksum fields
 * -# In other cases check that ICMP message has incorrect
 *    checksum field.
 * -# Destroy CSAP and close socket
 *
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "ipstack/ip4_send_icmp"

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
    const char                 *chksum;

    csap_handle_t               send_csap = CSAP_INVALID_HANDLE;
    asn_value                  *csap_spec = NULL;
    asn_value                  *template = NULL;
    
    int                         recv_socket = -1;

    void                       *recv_buf = NULL;
    size_t                      recv_buf_len = 0;
    ssize_t                     r;

    uint8_t                     ip_header_len;

    te_bool                     sum_ok;
    
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
    TEST_GET_STRING_PARAM(chksum);

    /* Create buffer to receive ICMP message */
    recv_buf_len = sizeof(struct icmphdr) + 
                   sizeof(struct iphdr) + 
                   MAX_IPOPTLEN;
    recv_buf = te_make_buf_by_len(recv_buf_len);

    /* Create RAW socket */
    recv_socket = rpc_socket(pco, RPC_PF_INET, 
                             RPC_SOCK_RAW, RPC_IPPROTO_ICMP);
    
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
                                  "icmp4.ip4.eth",
                                  csap_spec,
                                  &send_csap));
            
    /* Prepare data-sending template */
    CHECK_RC(tapi_icmp4_add_pdu(&template, NULL,
                                FALSE, type, code));
    CHECK_RC(tapi_ip4_add_pdu(&template, NULL,
                              FALSE,
                              SIN(csap_addr)->sin_addr.s_addr,
                              SIN(sock_addr)->sin_addr.s_addr,
                              IPPROTO_ICMP,
                              -1, 
                              -1));
    CHECK_RC(tapi_eth_add_pdu(&template, NULL,
                              FALSE,
                              sock_hwaddr->sa_data,
                              csap_hwaddr->sa_data,
                              &ip_eth,
                              TE_BOOL3_ANY,
                              TE_BOOL3_ANY));

    if (strcmp(chksum, "correct") == 0)
        sum_ok = TRUE;
    else if (chksum[0] == '=')
    {
        char           *end;
        unsigned long   v = strtoul(chksum + 1, &end, 0);

        if (end == chksum + 1 || *end != '\0')
        TEST_FAIL("Invalide 'chksum' parameter value '%s'", chksum);

        CHECK_RC(asn_write_int32(template, v,
                                 "pdus.1.#ip4.pld-checksum.#diff"));
        sum_ok = FALSE;
    }
    else
        TEST_FAIL("Invalide 'chksum' parameter value '%s'", chksum);
    
    /* Start sending data via CSAP */
    CHECK_RC(tapi_tad_trsend_start(host_csap->ta, 0, send_csap,
                                   template, RCF_MODE_NONBLOCKING));

    MSLEEP(100);

    /* Start receiving data via socket */
    RPC_AWAIT_IUT_ERROR(pco);
    r = rpc_recv(pco, recv_socket, recv_buf, recv_buf_len,
                 RPC_MSG_DONTWAIT);

    if (r < sizeof(struct icmphdr) + sizeof(struct iphdr))
        TEST_FAIL("Number of received bytes is less than "
                  "minimal expected %d", 
                  sizeof(struct icmphdr) + sizeof(struct iphdr));

    /* Check received data */
    /* Check header length */
    ip_header_len = ((struct iphdr *)recv_buf)->ihl;
    if (ip_header_len > 5)
        WARN("IP header has %d fields of "
             "additional options", ip_header_len - 5);

    /* Check ICMP header's fields */
    /* Check type field */
    if (sum_ok)
    {
        if (((struct icmphdr *)
                    (recv_buf + ip_header_len * 4))->type != type)
            TEST_FAIL("ICMP message was received with "
                      "corrupted type field");
        /* Check code field */
        if (((struct icmphdr *)
                    (recv_buf + ip_header_len * 4))->code != code)
            TEST_FAIL("ICMP message was received with "
                      "corrupted code field");
        /* Check checksum field */
        if (~(short)calculate_checksum(recv_buf + ip_header_len * 4, 
                                    sizeof(struct icmphdr)))
            TEST_FAIL("ICMP message was unexpectedly "
                      "received with "
                      "corrupted checksum field");
    }
    else
    {
        /* Check checksum field */
        if (~(short)calculate_checksum(recv_buf + ip_header_len * 4, 
                                    sizeof(struct icmphdr)))
            ;
        else
            TEST_FAIL("ICMP message was unexpectedly "
                      "received with "
                      "correct checksum field");
    }

    TEST_SUCCESS;

cleanup:

    CLEANUP_RPC_CLOSE(pco, recv_socket);
    
    asn_free_value(template);
    asn_free_value(csap_spec);
    
    if (host_csap != NULL)
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(host_csap->ta, 
                                             0, 
                                             send_csap));

    free(recv_buf);

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
