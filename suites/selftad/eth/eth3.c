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

    printf ("\neth_len_type: 0x%x = %d\n", header->eth_type_len,
                header->eth_type_len);

    printf ("payload len: %d\n", plen);
}

#define EXAMPLE_MULT_PKTS 0
int
main()
{
    char ta[32];
    int  len = sizeof(ta);
    int  sid;
    
    printf("Starting test\n");
    if (rcf_get_ta_list(ta, &len) != 0)
    {
        printf("rcf_get_ta_list failed\n");
        return 1;
    }
    printf("Agent: %s\n", ta);
    
    /* Type test */
    {
        char type[16];
        if (rcf_ta_name2type(ta, type) != 0)
        {
            printf("rcf_ta_name2type failed\n");
            return 1;
        }
        printf("TA type: %s\n", type); 
    }
    
    /* Session */
    {
        if (rcf_ta_create_session(ta, &sid) != 0)
        {
            printf("rcf_ta_create_session failed\n");
            return 1;
        }
        printf("Test: Created session: %d\n", sid); 
    }

    /* CSAP tests */
    do {
        int rc, syms = 0;
        csap_handle_t eth_csap;
        csap_handle_t eth_listen_csap;
        asn_value *template;
        asn_value *pattern;
        char eth_device[] = "lo"; 

        rc = asn_parse_dvalue_in_file("template_bug.asn", 
                        ndn_traffic_template, &template, &syms);
        if (rc)
        {
            printf ("parse dvalue from  template failed %x, syms %d\n",
                        rc, syms);
            return 1;
        }

        rc = asn_parse_dvalue_in_file("pattern_bug.asn", 
                        ndn_traffic_pattern, &pattern, &syms);
        if (rc)
        {
            printf ("parse dvalue from  pattern failed %x, syms %d\n",
                        rc, syms);
            return 1;
        }

        rc = tapi_eth_csap_create(ta, sid, eth_device, NULL, NULL, NULL, 
                                    &eth_csap);

        if (rc)
        {
            printf ("csap create error: %x\n", rc);
            return rc;
        }
        else 
            printf ("csap created, id: %d\n", (int)eth_csap);


        rc = tapi_eth_csap_create(ta, sid, eth_device, NULL, NULL, 
                              NULL, &eth_listen_csap); 
        if (rc)
        {
            printf ("csap for listen create error: %x\n", rc);
            return rc;
        }
        else 
            printf ("csap for listen created, id: %d\n",
                    (int)eth_listen_csap);



        rc = tapi_eth_recv_start(ta, sid, eth_listen_csap, pattern, 
                local_eth_frame_handler, NULL, 0, 1);

        if (rc)
        {
            fprintf(stderr, "tapi_eth_recv_start failed 0x%x, "
                    "catched %d\n", 
                    rc, syms);
            return rc;
        } 

        rc = tapi_eth_send(ta, sid, eth_csap, template);

        if (rc)
        {
            printf ("Eth frame send error: %x\n", rc);
            return rc;
        }

        sleep(2);

        rc = rcf_ta_trrecv_stop(ta, sid, eth_listen_csap, &syms);

        printf ("trrecv stop rc: %x, num of pkts: %d\n", rc, syms);

    } while (0);

    return 0;
}
