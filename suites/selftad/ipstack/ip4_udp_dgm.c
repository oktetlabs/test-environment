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

#include "ipstack-ts.h" 

#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

#include "tapi_cfg.h"
#include "tapi_rpc.h"
#include "tapi_tad.h"
#include "tapi_eth.h"

#include "tapi_env.h"
#include "tapi_ip.h"
#include "tapi_udp.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"


int
main(int argc, char *argv[])
{
    tapi_env_host *host_csap;
    const struct if_nameindex *pco_if;
    const struct if_nameindex *csap_if;

    uint8_t pco_mac[ETHER_ADDR_LEN];
    size_t  pco_mac_len = sizeof(pco_mac);
    int  udp_socket = -1;
    int  syms;
    int  sid_a;
    char ta[32];
    char *agt_a;
    size_t  len = sizeof(ta);


    csap_handle_t ip4_send_csap = CSAP_INVALID_HANDLE;

    rcf_rpc_server *pco = NULL;

    asn_value *template; /* template for traffic generation */ 

    /* checksum = 0 */
    uint8_t udp_dgm_image[] = {0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00,
                               0x03, 0x04, 0x05, 0x06,
                               0x00, 0x00, 0x00, 0x00,
                               0x01, 0x01, 0x02, 0x02, 0x02};

    struct sockaddr_in from_sa;
    const struct sockaddr *csap_addr;
    size_t csap_addr_len;

    const struct sockaddr *pco_addr;
    size_t pco_addr_len;
    size_t from_len = sizeof(from_sa);

    uint8_t rcv_buffer[2000];

    TEST_START; 
    TEST_GET_HOST(host_csap);
    TEST_GET_PCO(pco);
    TEST_GET_IF(pco_if);
    TEST_GET_IF(csap_if);
    TEST_GET_ADDR(pco_addr, pco_addr_len);
    TEST_GET_ADDR(csap_addr, csap_addr_len);

    CHECK_RC(tapi_cfg_get_hwaddr(pco->ta, pco_if->if_name, 
                                 pco_mac, &pco_mac_len));
    

    /******** Find TA names *************/
    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed: %r", rc);

    INFO("Found first TA: %s; len %d", ta, len);

    agt_a = host_csap->ta;
    if (strlen(ta) + 1 >= len) 
        TEST_FAIL("There is no second Test Agent");

    /******** Create RCF sessions *************/
    if (rcf_ta_create_session(agt_a, &sid_a) != 0)
    {
        TEST_FAIL("rcf_ta_create_session failed");
    }
    INFO("Test: Created session for A agt: %d", sid_a); 


    /******** Init RPC server and UDP socket *************/

    udp_socket = rpc_socket(pco, RPC_PF_INET,
                            RPC_SOCK_DGRAM, RPC_PROTO_DEF);
    if (udp_socket < 0)
    {
        TEST_FAIL("create socket failed");
    }

    rc = rpc_bind(pco, udp_socket, pco_addr, pco_addr_len);
    if (rc != 0)
        TEST_FAIL("bind failed");

    /******** Create Traffic Template *************/
    rc = asn_parse_value_text("{ pdus { ip4:{ protocol plain:17 }, "
                              "         eth:{}} }",
                              ndn_traffic_template,
                              &template, &syms);
    if (rc != 0)
        TEST_FAIL("parse of template failed %r, syms %d", rc, syms);

    if ((rc = asn_write_value_field(template, pco_mac, pco_mac_len, 
                                    "pdus.1.#eth.dst-addr.#plain")) != 0)
        TEST_FAIL("write pco MAC to template failed %r", rc);

    udp_dgm_image[5] = sizeof(udp_dgm_image);
    *((uint16_t *)udp_dgm_image) = SIN(pco_addr)->sin_port;
    *((uint16_t *)(udp_dgm_image + 2)) = SIN(csap_addr)->sin_port;


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
    rc = tapi_ip4_eth_csap_create(agt_a, sid_a, csap_if->if_name, 
                                  NULL, NULL,
                                  SIN(csap_addr)->sin_addr.s_addr, 
                                  SIN(pco_addr)->sin_addr.s_addr,
                                  &ip4_send_csap);
    if (rc != 0)
        TEST_FAIL("CSAP create failed, rc from module %d is %r\n", 
                    TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc)); 

    pco->op = RCF_RPC_CALL;
    rpc_recvfrom(pco, udp_socket,
                      rcv_buffer, sizeof(rcv_buffer), 0, 
                      SA(&from_sa), &from_len);

    rc = tapi_tad_trsend_start(agt_a, sid_a, ip4_send_csap,
                               template, RCF_MODE_BLOCKING);
    if (rc != 0) 
        TEST_FAIL("send start failed %X", rc); 

    TEST_SUCCESS;
#if 0
    RPC_AWAIT_IUT_ERROR(pco);
#else
    pco->op = RCF_RPC_WAIT;
#endif
    rc = rpc_recvfrom(pco, udp_socket,
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
        rpc_close(pco, udp_socket);

    TEST_END;
}
