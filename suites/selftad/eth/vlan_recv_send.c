/** @file
 * @brief Test Environment
 *
 * Simple RCF test
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

#define TE_TEST_NAME    "eth/vlan_recv_send"

#define TE_LOG_LEVEL 0xff

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


static void
local_eth_frame_handler(const asn_value *packet, int layer,
                        const ndn_eth_header_plain *header, 
                        const uint8_t *payload, uint16_t plen, 
                        void *userdata)
{
    int i;
    char buffer [100];
    
    UNUSED(packet);
    UNUSED(layer);
    UNUSED(payload);
    UNUSED(userdata);

    VERB ("++++ Ethernet frame received\n");
    sprintf(buffer, "dst: ");
    for (i = 0; i < ETHER_ADDR_LEN; i ++ )
        sprintf(buffer, "%02x ", header->dst_addr[i]);
    sprintf(buffer, "src: ");
    for (i = 0; i < ETHER_ADDR_LEN; i ++ )
        sprintf(buffer, "%02x ", header->src_addr[i]);
    VERB("addrs: %s", buffer);

    VERB("len_type: 0x%x = %d", header->len_type, header->len_type);

    if(header->is_tagged)
    {
        VERB("cfi:     %d", (int)header->cfi);
        VERB("prio:    %d", (int)header->priority);
        VERB("vlan-id: %d", (int)header->vlan_id);
    }

    VERB("payload len: %d", plen);
}

#define EXAMPLE_MULT_PKTS 0

int
main(int argc, char *argv[])
{
    char   ta[32];
    size_t len = sizeof(ta);
    int  sid;

    int  num;

    csap_handle_t eth_csap = CSAP_INVALID_HANDLE;
    csap_handle_t eth_listen_csap = CSAP_INVALID_HANDLE;


    TEST_START;
    
    if (rcf_get_ta_list(ta, &len) != 0)
    {
        TEST_FAIL("rcf_get_ta_list failed");
    }
    VERB("Agent: %s\n", ta);
       
    if (rcf_ta_create_session(ta, &sid) != 0)
        TEST_FAIL("rcf_ta_create_session failed");
    VERB("Test: Created session: %d\n", sid); 

    do {
        int       syms = 4;
        uint16_t  eth_type = ETH_P_IP;
        uint8_t   payload [2000];

        ndn_eth_header_plain plain_hdr;

        asn_value *csap_spec = NULL;
        asn_value *asn_eth_hdr;
        asn_value *template;
        asn_value *asn_pdus;
        asn_value *asn_pdu;
        asn_value *pattern;

        char eth_device[] = "eth0";
#if 0
        char payload_fill_method[100] = "eth_udp_payload";
#endif

#if 0
        uint8_t rem_addr[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
#else
        uint8_t rem_addr[6] = {0x20,0x03,0x20,0x04,0x14,0x30};
#endif

                    
        memset (&plain_hdr, 0, sizeof(plain_hdr));
        memcpy (plain_hdr.dst_addr, rem_addr, ETHER_ADDR_LEN);  
        memset (payload, 0, sizeof(payload));
        plain_hdr.len_type = ETH_P_IP; 

        plain_hdr.is_tagged = 1;
        plain_hdr.vlan_id = 16;

        if ((asn_eth_hdr = ndn_eth_plain_to_packet(&plain_hdr)) == NULL)
            TEST_FAIL("eth header not converted\n");

        template = asn_init_value(ndn_traffic_template);
        asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
        asn_pdu = asn_init_value(ndn_generic_pdu); 

#if EXAMPLE_MULT_PKTS
        rc = asn_parse_value_text("{simple-for:{begin 1}}",
                 ndn_template_parameter_sequence, &arg_sets, &syms);

        num_pkts = 20;
        rc = asn_write_value_field (arg_sets, &num_pkts, sizeof(num_pkts), 
                                        "0.#simple-for.end");
        if (rc == 0)
            rc = asn_write_component_value(template, arg_sets, "arg-sets");
#endif

        rc = asn_write_component_value(asn_pdu, asn_eth_hdr, "#eth");
        if (rc == 0)
            rc = asn_insert_indexed(asn_pdus, asn_pdu, -1, "");
        if (rc == 0)
            rc = asn_write_component_value(template, asn_pdus, "pdus");


#if 0
        if (rc == 0)
            rc = asn_write_value_field (template, payload_fill_method, 
                    strlen(payload_fill_method) + 1, 
                "payload.#function");
#else
        if (rc == 0)
            rc = asn_write_value_field(template, payload, 
                                       100 /* sizeof(payload) */,
                                       "payload.#bytes");
#endif

#if 1
    VERB("come data from addr: %Tm", rem_addr,  ETHER_ADDR_LEN);
#endif

        if (rc)
            TEST_FAIL("template create error %x\n", rc);
        VERB ("template created successfully \n");
        rc = tapi_eth_csap_create(ta, sid, eth_device, rem_addr, NULL, 
                              &eth_type, &eth_csap);

        if (rc)
            TEST_FAIL("csap create error: %x\n", rc);
        else 
            VERB ("csap created, id: %d\n", (int)eth_csap);

        CHECK_RC(tapi_eth_add_csap_layer(&csap_spec, eth_device,
                                         TAD_ETH_RECV_ALL, NULL, NULL,
                                         NULL));
        if ((rc = tapi_tad_csap_create(ta, sid, "eth", csap_spec,
                                       &eth_listen_csap)) != 0)
            TEST_FAIL("csap for listen create error: %x\n", rc);
        else 
            VERB("csap for listen created, id: %d\n",
                 (int)eth_listen_csap);
        asn_free_value(csap_spec); csap_spec = NULL;


        rc = asn_parse_value_text("{{ pdus { eth:{ }}}}", 
                            ndn_traffic_pattern, &pattern, &syms); 
        if (rc)
            TEST_FAIL("parse value text fails, %r", rc);

        rc = asn_write_value_field(pattern, rem_addr, sizeof(rem_addr), 
                 "0.pdus.0.#eth.dst-addr.#plain"); 
        if (rc)
            TEST_FAIL("write dst to pattern failed %r", rc);


#if 0
        do {
            asn_free_subvalue(pattern,
                              "0.pdus.0.#eth.frame-type.#tagged.vlan-id");
            asn_value *interval;
            asn_value *ints_seq = asn_init_value(ndn_interval_sequence);
            const asn_type *type = NULL;
            int parsed = 0;

            rc = asn_get_subtype(ndn_generic_pdu, &type,
                                 "#eth.frame-type.#tagged.vlan-id");
            if (rc)
            {
                ERROR("get subtype for vlan-id failed %x", rc);
                type = ndn_data_unit_int16;
                rc = 0;
            }
#if 0
            int val = 10; 

            rc = asn_write_value_field(pattern, &val, sizeof(val), 
                    "0.pdus.0.#eth.vlan-id.#intervals.0.b");
            if (rc) 
            {
               ERROR(stderr, "write intervals value field rc %x\n", rc);
               rc = 0;
            }
#endif

#if 1
            rc = asn_parse_value_text("intervals: { { b 16, e 20} }", 
                    type,  &ints_seq, &parsed);
            if (rc != 0) break;
            VERB("parse intervals ok");
            rc = asn_write_component_value(pattern, ints_seq,
                     "0.pdus.0.#eth.frame-type.#tagged.vlan-id");
            if (rc) break;
            VERB("write intervals seq ok"); 
            UNUSED(interval);
#else
            rc = asn_parse_value_text("{b 18, e 20}",
                        ndn_interval, &interval, &parsed);
            if (rc) break;
            VERB("parse intervals ok");
            rc = asn_insert_indexed(ints_seq, interval, 0, "");
            if (rc) break;
            VERB("insert indexed ok");
            rc = asn_write_component_value(pattern, ints_seq,
                     "0.pdus.0.#eth.frame-type.#tagged.vlan-id.#intervals");
            if (rc) break;
            VERB("write intervals seq ok");
#endif

        } while(0);

        if (rc)
            TEST_FAIL("write intervals to pattern failed, rc %r\n", rc);

#endif

        rc = tapi_tad_trrecv_start(ta, sid, eth_listen_csap, pattern, 
                                   0, 1, RCF_TRRECV_PACKETS);
        if (rc != 0)
            TEST_FAIL("failed %r, catched %d", rc, syms);

        rc = tapi_tad_trsend_start(ta, sid, eth_csap, template,
                                   RCF_MODE_BLOCKING);

        if (rc)
            TEST_FAIL("ETH send fails, rc %r", rc);

        VERB("Eth pkt sent");

        sleep(2);

        num = 0;
        rc = tapi_tad_trrecv_stop(ta, sid, eth_listen_csap,
                                  tapi_eth_trrecv_cb_data(
                                      local_eth_frame_handler, NULL),
                                  &num);

        if (rc != 0)
            TEST_FAIL("ETH recv_stop fails, rc %r", rc);

        INFO("trrecv stop rc: %r, num of pkts: %d", rc, num);

        if (num <= 0)
            TEST_FAIL("no received packets");

    } while (0);

    TEST_SUCCESS;

cleanup:
    if (eth_csap != CSAP_INVALID_HANDLE &&
        (rc = rcf_ta_csap_destroy(ta, sid, eth_csap) != 0) )
        ERROR("ETH csap destroy fails, rc %r", rc);

    if (eth_listen_csap != CSAP_INVALID_HANDLE &&
        (rc = rcf_ta_csap_destroy(ta, sid, eth_listen_csap) != 0) )
        ERROR("ETH listen csap destroy fails, rc %r", rc);

    TEST_END; 
}
