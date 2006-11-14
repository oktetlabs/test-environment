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
 * $Id$
 */

/** @page ipstack-ip4_raw_send Send IP datagram via ip4.eth CSAP and receive it via RAW socket
 *
 * @objective Check that ip4.eth CSAP can send correctly formed
 *            IP datagrams.
 *
 * @param host_csap     TA with CSAP
 * @param csap_if       Interface on @p host_csap connected to the host
 *                      with @p pco
 * @param pco           TA with RAW socket
 * @param csap_addr     CSAP local IPv4 address
 * @param sock_addr     CSAP remote IPv4 address
 * @param csap_hwaddr   CSAP local MAC address
 * @param sock_hwaddr   CSAP remote MAC address
 * @param pld_len       Datagram's payload length
 * @param proto         IP header's protocol field
 * @param hcsum         IP header checksum (correct, =<value>, +<diff>)
 *
 * @par Scenario:
 *
 * -# Create ip4.eth CSAP on @p host_csap TA and @p csap_if interface.
 *    Specify @p proto as IPv4 protocol to be used, @p csap_addr as
 *    local IPv4 address, @p sock_addr as remote IPv4 address,
 *    @p csap_hwaddr as local Ethernet address,
 *    @p sock_hwaddr as remote Ethernet address.
 * -# Create IPv4 raw socket with @p proto protocol on @p pco_sock.
 * -# Prepare ip4.eth traffic template with payload of @p pld_len length:
 *      - if @p hcsum is @c correct, skip 'h-checksum' specification;
 *      - if @p hcsum is @c =<value>, specify 'h-checksum' as plain value;
 *      - if @p hcsum is @c +<diff>, specify 'h-checksum' as script
 *        "expr:<diff>".
 * -# Send prepared IPv4 datagrem via created CSAP.
 * -# Receive datagram via socket.
 * -# Check that correct IPv4 addresses and protocol are set in IPv4
 *    header.
 * -# Check that IPv4 header has correct checksum
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

#if HAVE_NETINET_IP_H
#include <netinet/ip.h>
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
#include "tapi_rpc_params.h"

int
main(int argc, char *argv[])
{
    int                         pld_len;
    const char                 *hcsum;

    uint8_t                     ip_header_len;

    tapi_env_host               *host_csap = NULL;
    rcf_rpc_server              *pco = NULL;
    const struct sockaddr       *csap_addr;
    const struct sockaddr       *sock_addr;
    const struct sockaddr       *csap_hwaddr;
    const struct sockaddr       *sock_hwaddr;
    const struct if_nameindex   *csap_if;

    rpc_socket_proto            proto;

    csap_handle_t               ip4_send_csap = CSAP_INVALID_HANDLE;
    int                         recv_socket = -1;

    asn_value                   *template = NULL;
    
    void                       *send_buf = NULL;
    void                       *recv_buf = NULL;
    size_t                      send_buf_len;
    size_t                      recv_buf_len;

    te_bool3                    receive;
    ssize_t                     r;


    TEST_START; 

    TEST_GET_HOST(host_csap);
    TEST_GET_PCO(pco);
    TEST_GET_ADDR(csap_addr);
    TEST_GET_ADDR(sock_addr);
    TEST_GET_ADDR(csap_hwaddr);
    TEST_GET_ADDR(sock_hwaddr);
    TEST_GET_IF(csap_if);
    TEST_GET_INT_PARAM(pld_len);
    TEST_GET_PROTOCOL(proto);
    TEST_GET_STRING_PARAM(hcsum);

    /* Create send-recv buffers */
    send_buf_len = pld_len;
    recv_buf_len = pld_len + sizeof(struct iphdr) + MAX_IPOPTLEN;
    send_buf = te_make_buf_by_len(send_buf_len);
    recv_buf = te_make_buf_by_len(recv_buf_len);

    /* Create RAW socket */
    recv_socket = rpc_socket(pco, RPC_PF_INET, 
                             RPC_SOCK_RAW, proto);
    
    CHECK_RC(tapi_ip4_eth_csap_create(host_csap->ta, 0, csap_if->if_name,
                                  TAD_ETH_RECV_NO,
                                  csap_hwaddr->sa_data,
                                  sock_hwaddr->sa_data,
                                  SIN(csap_addr)->sin_addr.s_addr,
                                  SIN(sock_addr)->sin_addr.s_addr,
                                  proto_rpc2h(proto),
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

    /* 'hcsum' parameter processing */
    if (strcmp(hcsum, "correct") == 0)
    {
        /* Nothing to do */
        receive = TE_BOOL3_TRUE;
    }
    else if (hcsum[0] == '=' || hcsum[0] == '+')
    {
        char           *end;
        unsigned long   v = strtoul(hcsum + 1, &end, 0);

        if (end == hcsum + 1 || *end != '\0')
            TEST_FAIL("Invalide 'hcsum' parameter value '%s'", hcsum);

        if (hcsum[0] == '=')
        {
            CHECK_RC(asn_write_int32(template, v,
                                     "pdus.0.#ip4.h-checksum.#plain"));
            receive = TE_BOOL3_ANY;
        }
        else
        {
            char buf[32];

            snprintf(buf, sizeof(buf), "expr:%lu", v);
            CHECK_RC(asn_write_string(template, buf,
                                      "pdus.0.#ip4.h-checksum.#script"));
            receive = TE_BOOL3_FALSE;
        }
    }
    else
    {
        TEST_FAIL("Invalide 'hcsum' parameter value '%s'", hcsum);
    }

    /* Start sending data */
    CHECK_RC(tapi_tad_trsend_start(host_csap->ta, 0, ip4_send_csap,
                               template, RCF_MODE_BLOCKING));

    MSLEEP(100);

    /* Start receiving data */
    RPC_AWAIT_IUT_ERROR(pco);
    r = rpc_recv(pco, recv_socket, recv_buf, recv_buf_len,
                 RPC_MSG_DONTWAIT);
    if (r == -1)
    {
        CHECK_RPC_ERRNO(pco, RPC_EAGAIN, "recv() with MSG_DONTWAIT");
        if (receive == TE_BOOL3_TRUE)
            TEST_FAIL("IPv4 packet is expected to be received, "
                      "but it is not");
        TEST_SUCCESS;
    }
    else
    {
        if (receive == TE_BOOL3_FALSE)
            TEST_FAIL("IPv4 packet is expected to be not received, "
                      "but it is");
    }
    
    /* Check IP header */
    /* Check header length */
    ip_header_len = ((struct iphdr *)recv_buf)->ihl;
    if (ip_header_len > 5)
        WARN("IP header has %d fields of "
             "additional options", ip_header_len - 5);

    /* Check total length */
    if (ntohs(((struct iphdr *)recv_buf)->tot_len) !=
            send_buf_len + ip_header_len * 4)
        TEST_FAIL("Total length field differs from expected");
    
    /* Check protocol */
    if (((struct iphdr *)recv_buf)->protocol != proto_rpc2h(proto))
        TEST_FAIL("Protocol field was corrupted");

    /* IP checksum test */
    if (~(short)calculate_checksum(recv_buf,
                            ((size_t)ip_header_len) * 4))
        TEST_FAIL("IP header's checksum was corrupted");

    /* Check IP addresses */
    if (SIN(csap_addr)->sin_addr.s_addr != 
            ((struct iphdr *)recv_buf)->saddr)
        TEST_FAIL("Source IP field was corrupted");
    if (SIN(sock_addr)->sin_addr.s_addr !=
            ((struct iphdr *)recv_buf)->daddr)
        TEST_FAIL("Destination IP field was corrupted");
        
    TEST_SUCCESS;

cleanup:

    CLEANUP_RPC_CLOSE(pco, recv_socket);
    
    asn_free_value(template);
    
    if (host_csap != NULL)
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(host_csap->ta, 
                                             0, ip4_send_csap));

    free(send_buf);
    free(recv_buf);

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
