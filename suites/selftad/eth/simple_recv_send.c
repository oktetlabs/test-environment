/** @file
 * @brief Test Environment
 *
 * Simple RAW Ethernet test: send packet to loopback interface and catch it
 * on another CSAP. 
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

#define TE_TEST_NAME    "eth/simple_recv_send"

#define TE_LOG_LEVEL 255

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"

#include "tapi_test.h"

#include "ndn_eth.h"
#include "tapi_eth.h"
#include "logger_api.h"


void
local_eth_frame_handler(const ndn_eth_header_plain *header, 
                   const uint8_t *payload, uint16_t plen, 
                   void *userdata)
{
    UNUSED(payload);
    UNUSED(userdata);

    INFO("++++ Ethernet frame received\n");
    INFO("dst: %Tm", header->dst_addr, ETH_ALEN);
    INFO("src: %Tm", header->src_addr, ETH_ALEN); 
    INFO("eth_len_type: 0x%x = %d",
           header->eth_type_len,  header->eth_type_len); 
    INFO("payload len: %d", plen);
}

int
main(int argc, char *argv[])
{
    char   ta[64];
    size_t len = sizeof(ta);

    int  sid_a;
    int  sid_b;

    char *ta_A;
    char *ta_B;

    csap_handle_t eth_csap = CSAP_INVALID_HANDLE;
    csap_handle_t eth_listen_csap = CSAP_INVALID_HANDLE;
   
    TEST_START;
 
    if (rcf_get_ta_list(ta, &len) != 0)
        TEST_FAIL("rcf_get_ta_list failed");

    ta_A = ta;
    ta_B = ta + strlen(ta) + 1;
    if (ta_B - ta > (int)len)
        TEST_FAIL("Second TA not found, at least two agents required");

    INFO("Using agent A: '%s', agent B: '%s'", ta_A, ta_B);
    
    /* Session */
    if (rcf_ta_create_session(ta_A, &sid_a) != 0)
        TEST_FAIL("rcf_ta_create_session failed");
    VERB("Test: Created A session: %d\n", sid_a); 

    if (rcf_ta_create_session(ta_B, &sid_b) != 0)
        TEST_FAIL("rcf_ta_create_session failed");
    VERB("Test: Created B session: %d\n", sid_b); 

    /* ETH CSAP tests */
    do {
        int syms = 4;
        int num_pkts;
        uint16_t eth_type = ETH_P_IP;
        uint8_t payload [2000];
        int p_len = 100; /* for test */
        ndn_eth_header_plain plain_hdr;
        asn_value *asn_eth_hdr;
        asn_value *template;
        asn_value *asn_pdus;
        asn_value *asn_pdu;
        asn_value *pattern;
        char eth_device[] = "eth0"; 

        uint8_t mac_a[6] = {
            0x01,0x02,0x03,0x04,0x05,0x06};
        uint8_t mac_b[6] = {
            0x16,0x15,0x14,0x13,0x12,0x11};

                    
        memset(&plain_hdr, 0, sizeof(plain_hdr));
        memcpy(plain_hdr.dst_addr, mac_b, ETH_ALEN);  
        memcpy(plain_hdr.src_addr, mac_a, ETH_ALEN);  
        memset(payload, 0, sizeof(payload));
        plain_hdr.eth_type_len = ETH_P_IP; 

        if ((asn_eth_hdr = ndn_eth_plain_to_packet(&plain_hdr)) == NULL)
            TEST_FAIL("make eth pkt fails");

        template = asn_init_value(ndn_traffic_template);
        asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
        asn_pdu = asn_init_value(ndn_generic_pdu); 
        
        rc = asn_write_component_value(asn_pdu, asn_eth_hdr, "#eth");
        if (rc == 0)
            rc = asn_insert_indexed(asn_pdus, asn_pdu, -1, "");
        if (rc == 0)
            rc = asn_write_component_value(template, asn_pdus, "pdus");

        if (rc == 0)
            rc = asn_write_value_field (template, payload, p_len, 
                "payload.#bytes");

        if (rc)
            TEST_FAIL("template create error %X", rc);
        VERB("template created successfully");

        rc = tapi_eth_csap_create(ta_A, sid_a, eth_device, mac_b, mac_a, 
                                  &eth_type, &eth_csap);

        if (rc)
            TEST_FAIL("csap create error: %x", rc);

        VERB("csap created, id: %d\n", (int)eth_csap);


        rc = tapi_eth_csap_create(ta_B, sid_b, eth_device, mac_a, mac_b, 
                                  &eth_type, &eth_listen_csap); 
        if (rc)
            TEST_FAIL("csap for listen create error: %x", rc);

        VERB("csap for listen created, id: %d\n", (int)eth_listen_csap);


        rc = asn_parse_value_text("{{ pdus { eth:{ }}}}", 
                            ndn_traffic_pattern, &pattern, &syms); 
        if (rc)
            TEST_FAIL("parse value text fails %X, sym %d", rc, syms);

        rc = tapi_tad_trrecv_start(ta_B, sid_b, eth_listen_csap, pattern, 
                                   5000, 1, RCF_TRRECV_PACKETS);
        VERB("eth recv start rc: %x", rc);

        if (rc)
            TEST_FAIL("tapi_tad_trrecv_start failed %r", rc);

        rc = tapi_tad_trsend_start(ta_A, sid_a, eth_csap, template,
                                   RCF_MODE_BLOCKING);

        VERB("tapi_tad_trsend_start rc: %x\n", rc);

        if (rc)
            TEST_FAIL("Eth frame send error: %x", rc);

        sleep(2);

        num_pkts = 0;
        rc = tapi_tad_trrecv_wait(ta_B, sid_b, eth_listen_csap,
                                  tapi_eth_trrecv_cb_data(
                                      local_eth_frame_handler, NULL),
                                  &num_pkts);

        if (rc)
            TEST_FAIL("tapi_eth_recv_wait failed %r", rc);

        INFO("trrecv wait rc: %x, num of pkts: %d\n", rc, num_pkts);

        if (num_pkts != 1)
            TEST_FAIL("Wrong number of packets caught");

    } while (0);

    TEST_SUCCESS;

cleanup:
    if (eth_csap != CSAP_INVALID_HANDLE && 
        (rc = rcf_ta_csap_destroy(ta_A, sid_a, eth_csap)) != 0)
        ERROR("CSAP destroy %d on agt %s failure %X", eth_csap, ta_A, rc);

    if (eth_listen_csap != CSAP_INVALID_HANDLE && 
        (rc = rcf_ta_csap_destroy(ta_B, sid_b, eth_listen_csap)) != 0)
        ERROR("CSAP destroy %d on agt %s failure %X", eth_listen_csap,
              ta_B, rc);

    TEST_END; 
}
