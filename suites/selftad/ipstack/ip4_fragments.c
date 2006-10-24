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

#define TE_TEST_NAME    "ipstack/ip4_fragments"

#define TE_LOG_LEVEL 0xff

#include "te_config.h"

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

#include "conf_api.h"
#include "tapi_rpc.h"
#include "tapi_tad.h"
#include "tapi_cfg.h"

#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_ip4.h"
#include "tapi_udp.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"


int
main(int argc, char *argv[])
{
    int  udp_socket = -1;
    int  syms;
    int  sid;
    char *agt_a = NULL;

    const struct sockaddr *csap_addr;
    const struct sockaddr *pco_addr;

    struct sockaddr_in from_sa;
    size_t from_len = sizeof(from_sa);

    tapi_ip_frag_spec frags[] = {
            { hdr_offset:0, real_offset:0,
                hdr_length:44, real_length:24, 1, 0},
#if 1
            { hdr_offset:24, real_offset:24,
                hdr_length:40, real_length:20, 0, 0},
#endif
        };

    csap_handle_t ip4_send_csap = CSAP_INVALID_HANDLE;

    rcf_rpc_server *pco = NULL;

    asn_value *template = NULL; /* template for traffic generation */ 
    asn_value *ip4_pdu = NULL;

    /* src port = 20000, dst port = 20001, checksum = 0 */
    uint8_t udp_dgm_image[] = {0x4e, 0x20, 0x4e, 0x21,
                               0x00, 0x00, 0x00, 0x00, /* end UDP header */
                               0x00, 0x00, 0x00, 0x00,
                               0x03, 0x04, 0x05, 0x06,
                               0x00, 0x00, 0x00, 0x00,
                               0x07, 0x08, 0x08, 0x09,
                               0x00, 0x00, 0x00, 0x00,
                               0x01, 0x01, 0x02, 0x02};

    tapi_env_host *host_csap;

    uint8_t rcv_buffer[2000];

    uint8_t pco_mac[ETHER_ADDR_LEN];
    size_t  pco_mac_len = sizeof(pco_mac);
    const struct if_nameindex *pco_if;
    const struct if_nameindex *csap_if;


    TEST_START; 
    TEST_GET_HOST(host_csap);
    TEST_GET_PCO(pco);
    TEST_GET_IF(pco_if);
    TEST_GET_IF(csap_if);
    TEST_GET_ADDR(pco_addr);
    TEST_GET_ADDR(csap_addr);

    agt_a = host_csap->ta;

    CHECK_RC(tapi_cfg_get_hwaddr(pco->ta, pco_if->if_name, 
                                 pco_mac, &pco_mac_len));

    /* set SRC UDP port in datagram image, both are in NET byte order */
    *((uint16_t *)udp_dgm_image)       = SIN(csap_addr)->sin_port;

    /* set DST UDP port in datagram image, both are in NET byte order */
    *((uint16_t *)(udp_dgm_image + 2)) = SIN(pco_addr) ->sin_port;

    /******** Create RCF sessions *************/
    if (rcf_ta_create_session(agt_a, &sid) != 0)
    {
        TEST_FAIL("rcf_ta_create_session failed");
    }
    INFO("Test: Created session for A agt: %d", sid); 



    /******** Init UDP socket *************/

    udp_socket = rpc_socket(pco, RPC_PF_INET,
                            RPC_SOCK_DGRAM, RPC_PROTO_DEF);
    if (udp_socket < 0)
    {
        TEST_FAIL("create socket failed");
    }

    rc = rpc_bind(pco, udp_socket, pco_addr);
    if (rc != 0)
        TEST_FAIL("bind failed");

    /******** Create Traffic Template *************/
    udp_dgm_image[5] = sizeof(udp_dgm_image); 

    frags[1].hdr_length = 
        20 + (frags[1].real_length = 
              (sizeof(udp_dgm_image) - frags[0].real_length));

    rc = tapi_ip4_add_pdu(&template, &ip4_pdu, FALSE,
                          SIN(csap_addr)->sin_addr.s_addr,
                          SIN(pco_addr)->sin_addr.s_addr,
                          IPPROTO_UDP, -1 /* default TTL */,
                          -1 /* default TOS */);
    if (rc != 0)
        TEST_FAIL("Failed to add IPv4 PDU to template: %r", rc);

    rc = tapi_ip4_pdu_tmpl_fragments(NULL, &ip4_pdu,
                                     frags, TE_ARRAY_LEN(frags));
    if (rc != 0)
        TEST_FAIL("Failed to fragments specification to IPv4 PDU: %r", rc); 

    rc = tapi_eth_add_pdu(&template, NULL, FALSE /* template */, 
                          pco_mac, NULL, NULL,
                          TE_BOOL3_FALSE /* untagged */,
                          TE_BOOL3_FALSE /* Ethernet2 */);
    if (rc != 0)
        TEST_FAIL("Failed to add Ethernet PDU to template: %r", rc);

    rc = asn_write_value_field(template, udp_dgm_image,
                               sizeof(udp_dgm_image), "payload.#bytes");
    if (rc != 0)
        TEST_FAIL("set payload to template failed %r", rc);

    /***************** Create 'ip4.eth' CSAP *****************/
    rc = tapi_ip4_eth_csap_create(agt_a, sid, csap_if->if_name,
                                  TAD_ETH_RECV_DEF & ~TAD_ETH_RECV_OTHER,
                                  NULL, NULL, INADDR_ANY, INADDR_ANY,
                                  -1 /* unspecified protocol */,
                                  &ip4_send_csap);
    if (rc != 0)
        TEST_FAIL("CSAP create failed, rc from module %d is %r\n", 
                    TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc)); 


    /***************** Issue datagram via CSAP *****************/
    rc = tapi_tad_trsend_start(agt_a, sid, ip4_send_csap,
                               template, RCF_MODE_BLOCKING);
    if (rc != 0) 
        TEST_FAIL("send start failed %X", rc); 
    RPC_AWAIT_IUT_ERROR(pco);
    te_sleep(1);
    from_len = sizeof(from_sa);
    rc = rpc_recvfrom(pco, udp_socket,
                      rcv_buffer, sizeof(rcv_buffer), RPC_MSG_DONTWAIT, 
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
                                                                    
    CLEANUP_CSAP(agt_a, sid, ip4_send_csap);

    if (udp_socket > 0)
        rpc_close(pco, udp_socket);

    TEST_END;
}
