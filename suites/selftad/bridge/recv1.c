/** @file
 * @brief Test Environment
 *
 * Simple STP BPDU test
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
 * $Id $
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"

#include "ndn_eth.h"
#include "ndn_bridge.h"
#include "tapi_stp.h"

#include "logger_ten.h"

#define CHECK_STATUS(_rc, x...) \
    do if (_rc) {\
        printf (x);\
        VERB(x);\
        if (bpdu_csap) \
            rcf_ta_csap_destroy(ta, sid, bpdu_csap);\
        if (bpdu_listen_csap) \
            rcf_ta_csap_destroy(ta, sid, bpdu_listen_csap);\
        return (_rc);\
    } while (0)



int
main()
{
    char ta[32];
    int  len = sizeof(ta);
    int  sid;
    
    VERB("Starting test\n");
    if (rcf_get_ta_list(ta, &len) != 0)
    {
        fprintf(stderr, "rcf_get_ta_list failed\n");
        return 1;
    }
    VERB(" Using agent: %s\n", ta);
    
    /* Type test */
    {
        char type[16];
        if (rcf_ta_name2type(ta, type) != 0)
        {
            fprintf(stderr, "rcf_ta_name2type failed\n");
            VERB("rcf_ta_name2type failed\n"); 
            return 1;
        }
        VERB("TA type: %s\n", type); 
    }
    
    /* Session */
    {
        if (rcf_ta_create_session(ta, &sid) != 0)
        {
            fprintf(stderr, "rcf_ta_create_session failed\n");
            VERB("rcf_ta_create_session failed\n"); 
            return 1;
        }
        VERB("Test: Created session: %d\n", sid); 
    }

    /* CSAP tests */
    do {
        int rc, syms = 4;
        uint8_t payload [2000];
        int p_len = 100; /* for test */
        csap_handle_t bpdu_csap = 0, bpdu_listen_csap = 0;
        asn_value *pattern;
        asn_value *template;
        asn_value *asn_pdus;
        asn_value *asn_pdu;
        asn_value *asn_bpdu;
        asn_value *asn_eth_hdr;
        char eth_device[] = "eth1"; 

        uint8_t own_addr[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
        uint8_t rem_addr[6] = {0x01,0x02,0x03,0x04,0x05,0x06};

        uint8_t root_id[] = {0x12, 0x13, 0x14, 0x15, 0, 0, 0, 0}; 
        ndn_stp_bpdu_t plain_bpdu;

        memset (&plain_bpdu, 0, sizeof (plain_bpdu));

        plain_bpdu.cfg.root_path_cost = 10;
        plain_bpdu.cfg.port_id = 0x1122;
        memcpy (plain_bpdu.cfg.root_id, root_id, sizeof(root_id));

        asn_bpdu = ndn_bpdu_plain_to_asn(&plain_bpdu);
        CHECK_STATUS(asn_bpdu == NULL,
                     "Create ASN bpdu from plain fails\n");

        template = asn_init_value(ndn_traffic_template);
        asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
        asn_pdu = asn_init_value(ndn_generic_pdu); 
        asn_eth_hdr = asn_init_value(ndn_eth_header); 
        
        rc = asn_write_component_value(asn_pdu, asn_bpdu, "#bridge");
        if (rc == 0)
            rc = asn_insert_indexed(asn_pdus, asn_pdu, 0, "");
 
        asn_free_value(asn_pdu);
        asn_pdu = asn_init_value(ndn_generic_pdu); 
        if (rc == 0)
            rc = asn_write_component_value(asn_pdu, asn_eth_hdr, "#eth");
        if (rc == 0)
            rc = asn_insert_indexed(asn_pdus, asn_pdu, 1, "");

        if (rc == 0)
            rc = asn_write_component_value(template, asn_pdus, "pdus"); 

        CHECK_STATUS(rc, "Template create failed with rc %x\n", rc);

        rc = tapi_stp_plain_csap_create(ta, sid, eth_device, own_addr, 
                                        NULL, &bpdu_csap); 
        CHECK_STATUS(rc, "csap create failed with rc %x\n", rc);


        rc = tapi_stp_plain_csap_create(ta, sid, eth_device, NULL, 
                                        own_addr, &bpdu_listen_csap); 
        CHECK_STATUS(rc, "csap listen create failed with rc %x\n", rc);
                    
        rc = asn_parse_value_text(
                "{{ pdus {  bridge:{ version-id plain:0,"
                                "    content cfg:{port-id "
                                "             mask:{v '0023'H, m '00ff'H}}"
                                "  },"
                          " eth:{ }}}}", 
                            ndn_traffic_pattern, &pattern, &syms); 
        CHECK_STATUS(rc, "parse pattern fails:  %x on sym %d\n", rc, syms); 

        {
            uint8_t p_mask[2]; /* number of bytes in port-id STP field */
            uint8_t port_num = 0x22;

            rc = asn_free_subvalue(pattern,
                                   "0.pdus.0.content.#cfg.port-id");

            p_mask[0] = 0;
            p_mask[1] = port_num;

            if (rc == 0)
                rc = asn_write_value_field(pattern, p_mask, sizeof(p_mask),
                         "0.pdus.0.content.#cfg.port-id.#mask.v");

            p_mask[0] = 0;
            p_mask[1] = 0xff; 
            if (rc == 0)
                rc = asn_write_value_field(pattern, p_mask, sizeof(p_mask),
                         "0.pdus.0.content.#cfg.port-id.#mask.m"); 
            CHECK_STATUS(rc, "bpdu set port-id mask rc: %x\n", rc); 
        }

        rc = tapi_stp_bpdu_recv_start(ta, sid, bpdu_listen_csap, pattern, 
                NULL, NULL, 20000, 1);
        CHECK_STATUS(rc, "bpdu recv start rc: %x\n", rc); 


        rc = tapi_stp_bpdu_send(ta, sid, bpdu_csap, template);
        CHECK_STATUS(rc, "BDPU send failed with rc %x\n", rc); 

        sleep(1);

        syms = 0;
        rc = rcf_ta_trrecv_stop(ta, bpdu_listen_csap, &syms);
        printf ("trrecv stop rc: %x, num: %d\n", rc, syms);
        CHECK_STATUS(rc, "trrecv stop rc: %x, num of pkts: %d\n", rc, syms);

        rc = rcf_ta_csap_destroy(ta, sid, bpdu_csap); 
        CHECK_STATUS(rc, "csap destroy failed with rc %x\n", rc); 

        rcf_ta_csap_destroy(ta, sid, bpdu_listen_csap);

    } while (0);

    return 0;
}
