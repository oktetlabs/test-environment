/** @file
 * @brief Test Environment
 *
 * Simple IPv4 CSAP test
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

#define TE_TEST_NAME    "ipstack/ip4_simple"

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

#include "rcf_rpc.h"
#include "conf_api.h"
#include "tapi_tad.h"
#include "tapi_eth.h"

#include "tapi_test.h"
#include "tapi_ipstack.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"


int
main(int argc, char *argv[])
{
    int  syms;
    int  sid_a;
    int  sid_b;
    char ta[32];
    char *agt_a = ta;
    char *agt_b;
    unsigned  len = sizeof(ta);

    csap_handle_t ip4_send_csap = CSAP_INVALID_HANDLE;
    csap_handle_t ip4_listen_csap = CSAP_INVALID_HANDLE;
    csap_handle_t eth_listen_csap_1 = CSAP_INVALID_HANDLE;
    csap_handle_t eth_listen_csap_2 = CSAP_INVALID_HANDLE;

    asn_value *template;/*  iteration template for traffic generation */ 
    asn_value *eth_pattern;

    int num_pkts;
    int pld_len;
    int delay;

    TEST_START; 

    TEST_GET_INT_PARAM(num_pkts);
    TEST_GET_INT_PARAM(pld_len);
    
    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed: %X", rc);

    INFO("Found first TA: %s; len %d", ta, len);

    agt_a = ta;
    if (strlen(ta) + 1 >= len) 
        TEST_FAIL("There is no second Test Agent");

    agt_b = ta + strlen(ta) + 1;

    INFO("Found second TA: %s", agt_b, len);

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

    rc = asn_parse_value_text("{{ pdus {  eth:{}} }}",
                              ndn_traffic_pattern,
                              &eth_pattern, &syms);
    if (rc != 0)
        TEST_FAIL("parse of pattern failed %X, syms %d", rc, syms);

    /* Fill in value for iteration */
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

    delay = 100;
    rc = asn_write_value_field(template, &delay, sizeof(delay),
                               "delays.#plain");
    if (rc != 0)
        TEST_FAIL("write delay failed %X", rc); 
  
    do {
        uint8_t mac_a[6] = {
            0x01,0x02,0x03,0x04,0x05,0x06};
        uint8_t mac_b[6] = {
            0x16,0x15,0x14,0x13,0x12,0x11};
        in_addr_t ip_a = inet_addr("192.168.123.231");
        in_addr_t ip_b = inet_addr("192.168.123.232");

        int num;

        rc = tapi_ip4_eth_csap_create(agt_a, sid_a, "eth0", mac_a, mac_b,
                                      (uint8_t *)&ip_a, (uint8_t *)&ip_b,
                                      &ip4_send_csap);
        if (rc != 0)
            TEST_FAIL("CSAP create failed, rc from module %d is 0x%x\n", 
                        TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc)); 

        rc = tapi_ip4_eth_csap_create(agt_b, sid_b, "eth0", mac_b, mac_a,
                                      (uint8_t *)&ip_b, (uint8_t *)&ip_a,
                                      &ip4_listen_csap);
        if (rc != 0)
            TEST_FAIL("CSAP create failed, rc from mod %d is 0x%x\n", 
                        TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc)); 

        rc = tapi_eth_csap_create(agt_b, sid_b, "eth0", mac_a, mac_b,
                                  NULL, &eth_listen_csap_1);
        if (rc != 0)
            TEST_FAIL("ETH CSAP create failed, rc from mod %d is 0x%x\n",
                        TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc)); 

        rc = tapi_eth_csap_create(agt_b, sid_b, "eth0", mac_a, mac_b,
                                  NULL, &eth_listen_csap_2);
        if (rc != 0)
            TEST_FAIL("ETH CSAP create failed, rc from mod %d is 0x%x\n",
                        TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc)); 
        
        rc = tapi_ip4_eth_recv_start(agt_b, sid_b, ip4_listen_csap,
                                     NULL, NULL, NULL, NULL, 5000,
                                     num_pkts);
        if (rc != 0) 
            TEST_FAIL("recv start failed %X", rc); 

        rc = tapi_eth_recv_start(agt_b, sid_b, eth_listen_csap_2,
                                 eth_pattern, NULL, NULL, 5000, num_pkts);

        rc = tapi_eth_recv_start(agt_b, sid_b, eth_listen_csap_1,
                                 eth_pattern, NULL, NULL, 5000, num_pkts);
        if (rc != 0) 
            TEST_FAIL("Eth recv start failed %X", rc); 

        rc = tapi_tad_trsend_start(agt_a, sid_a, ip4_send_csap,
                                   template, RCF_MODE_NONBLOCKING);
        if (rc != 0) 
            TEST_FAIL("send start failed %X", rc); 

        INFO ("try to wait\n");
        rc = rcf_ta_trrecv_wait(agt_b, sid_b, ip4_listen_csap, &num);
        RING("trrecv_wait: 0x%X num: %d\n", rc, num);

        rc = rcf_ta_trrecv_stop(agt_b, sid_b, eth_listen_csap_2, &num);
        RING("Eth trrecv_stop: 0x%X num: %d\n", rc, num);

        rc = rcf_ta_trrecv_stop(agt_b, sid_b, eth_listen_csap_1, &num);
        RING("Eth trrecv_stop: 0x%X num: %d\n", rc, num);

    } while(0);

    if (rc)
        TEST_FAIL("Failed, rc %X", rc);

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
    CLEANUP_CSAP(agt_b, sid_b, ip4_listen_csap);
    CLEANUP_CSAP(agt_b, sid_b, eth_listen_csap_1);
    CLEANUP_CSAP(agt_b, sid_b, eth_listen_csap_2);

    TEST_END;
}
