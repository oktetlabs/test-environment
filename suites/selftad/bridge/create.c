/** @file
 * @brief Test Environment
 *
 * Simple BPDU CSAP test: create.
 * from first ('eth0') network card.
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

#undef TE_LOG_LEVEL
#define TE_LOG_LEVEL 255
#include "logger_ten.h"

#define TE_LOG_LEVEL 255

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
        csap_handle_t csap;
        ndn_eth_header_plain plain_hdr;
        asn_value *asn_eth_hdr;
        asn_value *pattern;
        char eth_device[] = "eth0"; 

        uint8_t own_addr[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
        uint8_t out_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff}; 

        rc = tapi_stp_plain_csap_create(ta, sid, eth_device, own_addr, NULL, &csap);

        printf ("tapi_stp_plain_csap_create rc: %x, csap: %d\n", rc, csap);

        if (rc == 0)
            rc = rcf_ta_csap_destroy(ta,sid, csap);

        return rc;

    } while (0);

    return 0;
}
