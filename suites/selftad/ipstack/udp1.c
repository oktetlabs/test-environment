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

#define TE_TEST_NAME    "ipstack/udp1"

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
#include "logger_api.h"

#include "tapi_test.h"
#include "tapi_ipstack.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"

void
udp_handler(char *fn, void *p)
{ 
    int rc, s_parsed;
    asn_value_p packet, eth_header;

    UNUSED(p);

    VERB("ETH handler, file: %s\n", fn);

    rc = asn_parse_dvalue_in_file(fn, ndn_raw_packet, &packet, &s_parsed);

    if (rc == 0)
    {
        ndn_eth_header_plain eh;
        VERB("parse file OK!\n");

        eth_header = asn_read_indexed(packet, 0, "pdus");
        rc = ndn_eth_packet_to_plain(eth_header, &eh);
        if (rc)
            VERB("eth_packet to plain fail: %x\n", rc);
        else
        {
            int i; 
            VERB("dst - %02x", eh.dst_addr[0]);
            for (i = 1; i < 6; i++)
                VERB(":%02x", eh.dst_addr[i]);

            VERB("\nsrc - %02x", eh.src_addr[0]);
            for (i = 1; i < 6; i++)
                VERB (":%02x", eh.src_addr[i]);
            VERB("\ntype - %04x\n", eh.eth_type_len);
        } 
    }
    else
        VERB("parse file failed, rc = %x, symbol %d\n", rc, s_parsed); 

}

int
main(int argc, char *argv[])
{
    int  sid;
    const char *ta;

    TEST_START;
    TEST_GET_STRING_PARAM(ta);
    
    INFO("Starting test\n");
    
    /* Session */
    {
        if (rcf_ta_create_session(ta, &sid) != 0)
        {
            ERROR("rcf_ta_create_session failed\n");
            return 1;
        }
        INFO("Test: Created session: %d\n", sid); 
    }

    do {
        struct timeval to;

        char val[64];
        int csap;
        int num;
        int timeout = 30;
        int rc, rc_mod, rc_code;

        INFO("let's create Ethernet csap \n"); 
        rc = tapi_udp4_csap_create(ta, sid, NULL, "127.0.0.1", 
                                    5678, 6789, &csap); 
        INFO("csap_create rc: %d, csap id %d\n", rc, csap); 
        if ((rc_mod = TE_RC_GET_MODULE(rc)) != 0)
        {
            INFO ("rc from module %d is 0x%x\n", 
                        rc_mod, TE_RC_GET_ERROR(rc));
        } 
        if (rc) break;

#if 0
        rc = rcf_ta_trrecv_start(ta, sid, handle, path, 0, eth_handler, NULL, 0);
        INFO("trrecv_start: 0x%x \n", rc);
        if (rc) break;

#if 1
        sleep(1);
        INFO ("try to get\n");
        rc = rcf_ta_trrecv_get(ta, handle, &num);
        INFO("trrecv_get: 0x%x num: %d\n", rc, num);

#endif
        num = 1;
        INFO ("sleep %d secs before stop\n", num);
        sleep (num);

        INFO ("try to stop\n");
        rc = rcf_ta_trrecv_stop(ta, handle, &num);
        INFO("trrecv_stop: 0x%x num: %d\n", rc, num);

#endif


    } while(0);
    TEST_SUCCESS;

cleanup:
    TEST_END;
}
