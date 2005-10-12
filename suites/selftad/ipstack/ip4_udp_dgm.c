/** @file
 * @brief Test Environment
 *
 * IPv4 CSAP test: send packet through ip4.eth CSAP with UDP payload, 
 * caught it with UDP socket. 
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * 
 * $Id$
 */

#define TE_TEST_NAME    "ipstack/ip4_udp_dgm"

#define TE_LOG_LEVEL 0xff

#include "config.h"

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
#include "tapi_rpc.h"
#include "tapi_tad.h"
#include "tapi_eth.h"

#include "tapi_test.h"
#include "tapi_ip.h"
#include "tapi_udp.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"


int
main(int argc, char *argv[])
{
    int  udp_socket = -1;
    int  syms;
    int  sid_a;
    int  sid_b;
    char ta[32];
    char *agt_a = ta;
    char *agt_b;
    size_t  len = sizeof(ta);

    in_addr_t ip_addr_a = inet_addr("192.168.72.18");
    in_addr_t ip_addr_b = inet_addr("192.168.72.38");

    csap_handle_t ip4_send_csap = CSAP_INVALID_HANDLE;

    rcf_rpc_server *srv_listen = NULL;

    asn_value *template; /* template for traffic generation */ 

    /* src port = 20000, dst port = 20001, checksum = 0 */
    uint8_t udp_dgm_image[] = {0x4e, 0x20, 0x4e, 0x21,
                               0x00, 0x00, 0x00, 0x00,
                               0x03, 0x04, 0x05, 0x06,
                               0x00, 0x00, 0x00, 0x00,
                               0x01, 0x01, 0x02, 0x02, 0x02};

    struct sockaddr_in listen_sa;
    struct sockaddr_in from_sa;
    size_t from_len = sizeof(from_sa);

    uint8_t rcv_buffer[2000];

    TEST_START; 

    /******** Find TA names *************/
    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed: %r", rc);

    INFO("Found first TA: %s; len %d", ta, len);

    agt_a = ta;
    if (strlen(ta) + 1 >= len) 
        TEST_FAIL("There is no second Test Agent");

    agt_b = ta + strlen(ta) + 1;

    INFO("Found second TA: %s", agt_b, len);

    /******** Create RCF sessions *************/
    if (rcf_ta_create_session(agt_a, &sid_a) != 0)
    {
        TEST_FAIL("rcf_ta_create_session failed");
    }
    INFO("Test: Created session for A agt: %d", sid_a); 

    if (rcf_ta_create_session(agt_b, &sid_b) != 0)
    {
        TEST_FAIL("rcf_ta_create_session failed");
    }
    INFO("Test: Created session for B: %d", sid_b); 


    /******** Init RPC server and UDP socket *************/

    memset(&listen_sa, 0, sizeof(listen_sa)); 

    listen_sa.sin_family = AF_INET;
    listen_sa.sin_port = htons(20001); /* TODO generic port */

    if ((rc = rcf_rpc_server_create(agt_b, "FIRST", &srv_listen)) != 0) 
        TEST_FAIL("Cannot create server %x", rc); 

    srv_listen->def_timeout = 5000; 
    rcf_rpc_setlibname(srv_listen, NULL); 

    udp_socket = rpc_socket(srv_listen, RPC_PF_INET,
                            RPC_SOCK_DGRAM, RPC_PROTO_DEF);
    if (udp_socket < 0)
    {
        TEST_FAIL("create socket failed");
    }

    rc = rpc_bind(srv_listen, udp_socket,
                  SA(&listen_sa), sizeof(listen_sa));
    if (rc != 0)
        TEST_FAIL("bind failed");

    /******** Create Traffic Template *************/
    rc = asn_parse_value_text("{ pdus { ip4:{ protocol plain:17 }, eth:{ "
                              "    dst-addr plain:'00 0D 88 65 D4 9F'H}} }",
                              ndn_traffic_template,
                              &template, &syms);
    if (rc != 0)
        TEST_FAIL("parse of template failed %X, syms %d", rc, syms);

    udp_dgm_image[5] = sizeof(udp_dgm_image);

#if 0 
    rc = asn_write_value_field(template, NULL, 0,
                               "pdus.0.#ip4.pld-checksum.#disable");
    if (rc != 0)
        TEST_FAIL("set payload to template failed %X", rc);
#endif
    rc = asn_write_value_field(template, udp_dgm_image,
                               sizeof(udp_dgm_image), "payload.#bytes");
    if (rc != 0)
        TEST_FAIL("set payload to template failed %X", rc);
    rc = tapi_ip4_eth_csap_create(agt_a, sid_a, "eth2", NULL, NULL,
                                  ip_addr_a, ip_addr_b, &ip4_send_csap);
    if (rc != 0)
        TEST_FAIL("CSAP create failed, rc from module %d is %r\n", 
                    TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc)); 

    srv_listen->op = RCF_RPC_CALL;
    rpc_recvfrom(srv_listen, udp_socket,
                      rcv_buffer, sizeof(rcv_buffer), 0, 
                      SA(&from_sa), &from_len);

    rc = tapi_tad_trsend_start(agt_a, sid_a, ip4_send_csap,
                               template, RCF_MODE_BLOCKING);
    if (rc != 0) 
        TEST_FAIL("send start failed %X", rc); 

#if 0
    RPC_AWAIT_IUT_ERROR(srv_listen);
#else
    srv_listen->op = RCF_RPC_WAIT;
#endif
    rc = rpc_recvfrom(srv_listen, udp_socket,
                      rcv_buffer, sizeof(rcv_buffer), 0, 
                      SA(&from_sa), &from_len);
    if (rc <= 0)
        TEST_FAIL("wanted UDP datagram not received!");
    RING("receive %d bytes on UDP socket", rc); 
    TEST_SUCCESS;

cleanup:
#define CLEANUP_CSAP(ta_, sid_, csap_) \
    do {                                                                \
        csap_handle_t csap_tmp = csap_;                                 \
        csap_ = CSAP_INVALID_HANDLE;                                    \
                                                                        \
        if (csap_tmp != CSAP_INVALID_HANDLE &&                          \
            (rc = rcf_ta_csap_destroy(ta_, sid_, csap_tmp)) != 0)       \
            TEST_FAIL("CSAP destroy %d on agt %s failure %X",           \
                      csap_tmp, ta_, rc);                               \
    } while (0)                                                         \
                                                                    
    CLEANUP_CSAP(agt_a, sid_a, ip4_send_csap);

    if (udp_socket > 0)
        rpc_close(srv_listen, udp_socket);

    if (srv_listen != NULL && (rcf_rpc_server_destroy(srv_listen) != 0))
    {
        WARN("Cannot delete listen RPC server\n");
    }

    TEST_END;
}
