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
#include "tapi_eth.h"
#include "logger_api.h"


void
local_eth_frame_handler(const ndn_eth_header_plain *header, 
                   const uint8_t *payload, uint16_t plen, 
                   void *userdata)
{
    int i;
    UNUSED(payload);
    UNUSED(userdata);

    printf ("++++ Ethernet frame received\n");
    printf ("dst: ");
    for (i = 0; i < ETH_ALEN; i ++ )
        printf ("%02x ", header->dst_addr[i]);
    printf ("\nsrc: ");
    for (i = 0; i < ETH_ALEN; i ++ )
        printf ("%02x ", header->src_addr[i]);

    printf ("\neth_len_type: 0x%x = %d\n", header->eth_type_len,  header->eth_type_len);

    printf ("payload len: %d\n", plen);
}

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
        uint16_t eth_type = ETH_P_IP;
        uint8_t payload [2000];
        int p_len = 100; /* for test */
        csap_handle_t eth_csap;
        csap_handle_t eth_listen_csap;
        ndn_eth_header_plain plain_hdr;
        asn_value *asn_eth_hdr;
        asn_value *template;
        asn_value *asn_pdus;
        asn_value *asn_pdu;
        asn_value *pattern;
        char eth_device[] = "lo";


        uint8_t rem_addr[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
        uint8_t loc_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

                    
        memset (&plain_hdr, 0, sizeof(plain_hdr));
        memcpy (plain_hdr.dst_addr, rem_addr, ETH_ALEN);  
        memset (payload, 0, sizeof(payload));
        plain_hdr.eth_type_len = ETH_P_IP; 

        if ((asn_eth_hdr = ndn_eth_plain_to_packet(&plain_hdr)) == NULL)
            return 2;

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
        {
            printf ("template create error %x\n", rc);
            VERB("template create error %x\n", rc);
            return rc;
        }
        VERB("template created successfully \n");
        rc = tapi_eth_csap_create(ta, sid, eth_device, rem_addr, loc_addr, 
                              &eth_type, &eth_csap);

        if (rc)
        {
            fprintf (stderr, "csap create error: %x\n", rc);
            VERB("csap create error: %x\n", rc);
            return rc;
        }
        else 
            VERB("csap created, id: %d\n", (int)eth_csap);


        rc = tapi_eth_csap_create(ta, sid, eth_device, rem_addr, loc_addr, 
                              &eth_type, &eth_listen_csap); 
        if (rc)
        {
            fprintf (stderr, "csap for listen create error: %x\n", rc);
            VERB("csap for listen create error: %x\n", rc);
            return rc;
        }
        else 
            VERB("csap for listen created, id: %d\n", (int)eth_listen_csap);


        rc = asn_parse_value_text("{{ pdus { eth:{ }}}}", 
                            ndn_traffic_pattern, &pattern, &syms); 
        if (rc)
        {
            fprintf (stderr, "parse value text fails\n");
            return rc;
        }

        rc = asn_write_value_field(pattern, rem_addr, sizeof(rem_addr), 
                "0.pdus.0.#eth.dst-addr.#plain"); 
        if (rc)
        {
            fprintf (stderr, "write dst to pattern failed\n");
            return rc;
        } 

        rc = tapi_eth_send(ta, sid, eth_csap, template);

        VERB("tapi_eth_send rc: %x\n", rc);

        if (rc)
        {
            fprintf (stderr, "Eth frame send error: %x\n", rc);
            return rc;
        }

#if 1
        rc = tapi_eth_recv_start(ta, sid, eth_listen_csap, pattern, 
                local_eth_frame_handler, NULL, 0, 10);
        VERB("eth recv start rc: %x\n", rc);
#else
        syms = 4;
        rc = tapi_eth_recv_wait(ta, sid, eth_listen_csap, pattern, 
                local_eth_frame_handler, NULL, 120000, &syms);
        VERB("eth recv wait rc: %x, num: %d\n", rc, syms);
#endif

        if (rc)
        {
            fprintf (stderr, "tapi_eth_recv_start failed 0x%x, catched %d\n", 
                        rc, syms);
            return rc;
        } 

        sleep(5);

        rc = rcf_ta_trrecv_stop(ta, eth_listen_csap, &syms);

        VERB("trrecv stop rc: %x, num of pkts: %d\n", rc, syms);

    } while (0);

    return 0;
}
