/** @file
 * @brief Test Environment
 *
 * Simple UDP CSAP test
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

#include "eth_ndn.h"
#include "ndn_ipstack.h"

void
udp_handler(char *fn, void *p)
{ 
    int rc, s_parsed;
    asn_value_p packet, eth_header;
    printf ("ETH handler, file: %s\n", fn);

    rc = asn_parse_dvalue_in_file(fn, ndn_raw_packet, &packet, &s_parsed);

    if (rc == 0)
    {
        ndn_eth_header_plain eh;
        printf ("parse file OK!\n");

        eth_header = asn_read_indexed (packet, 0, "pdus");
        rc = ndn_eth_packet_to_plain (eth_header, &eh);
        if (rc)
            printf ("eth_packet to plain fail: %x\n", rc);
        else
        {
            int i; 
            printf ("dst - %02x", eh.dst_addr[0]);
            for (i = 1; i < 6; i++)
                printf (":%02x", eh.dst_addr[i]);

            printf ("\nsrc - %02x", eh.src_addr[0]);
            for (i = 1; i < 6; i++)
                printf (":%02x", eh.src_addr[i]);
            printf ("\ntype - %04x\n", eh.eth_type);
        } 
    }
    else
        printf ("parse file failed, rc = %x, symbol %d\n", rc, s_parsed); 

}

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

    do {
        struct timeval to;

        char val[64];
        int csap;
        int num;
        int timeout = 30;
        int rc, rc_mod, rc_code;

        printf("let's create Ethernet csap \n"); 
        rc = tapi_udp4_csap_create(ta, sid, NULL, "127.0.0.1", 
                                    5678, 6789, &csap); 
        printf("csap_create rc: %d, csap id %d\n", rc, csap); 
        if ((rc_mod = TE_RC_GET_MODULE(rc)) != 0)
        {
            printf ("rc from module %d is 0x%x\n", 
                        rc_mod, TE_RC_GET_ERROR(rc));
        } 
        if (rc) break;

#if 0
        rc = rcf_ta_trrecv_start(ta, sid, handle, path, 0, eth_handler, NULL, 0);
        printf("trrecv_start: 0x%x \n", rc);
        if (rc) break;

#if 1
        sleep(1);
        printf ("try to get\n");
        rc = rcf_ta_trrecv_get(ta, handle, &num);
        printf("trrecv_get: 0x%x num: %d\n", rc, num);

#endif
        num = 1;
        printf ("sleep %d secs before stop\n", num);
        sleep (num);

        printf ("try to stop\n");
        rc = rcf_ta_trrecv_stop(ta, handle, &num);
        printf("trrecv_stop: 0x%x num: %d\n", rc, num);

#endif


    } while(0);

    return 0;
}
