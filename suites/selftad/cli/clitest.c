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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
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


const char *te_lgr_entity = "CLI TEST";

void
trap_handler(char *fn, void *p)
{ 
    printf ("CLI message handler, file with NDS: %s\n", fn);
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

    /* CSAP tests */
    do {
        struct timeval to;
        char path[1000];
        int  path_prefix;

        char val[64];
        int handle = 20;
        int num;
        int timeout = 30;
        int rc;
        char *te_suites = getenv("TE_INSTALL_SUITE");

        if (te_suites)
            printf ("te_suites: %s\n", te_suites);
        else 
            break;

        strcpy(path, te_suites);
        strcat(path, "/selftest/cli_nds/");
        path_prefix = strlen(path);
        strcpy(path + path_prefix, "cli-csap.asn");
        printf("csap full path: %s\n", path);

        printf("let's create csap for listen\n"); 
        rc =   rcf_ta_csap_create(ta, sid, "cli", path, &handle);
        printf("csap_create rc: %d, csap id %d\n", rc, handle); 
        if (rc) break;

        strcpy(path + path_prefix, "cli-filter.asn");
        printf("send template full path: %s\n", path);
        rc = rcf_ta_trsend_recv(ta, sid, handle, path, trap_handler, NULL, &timeout, num);
        printf("trsend_recv: 0x%x, timeout: %d, num: %d\n", 
                    rc, timeout, num);
        if (rc) break;

        printf ("try to  destroy\n");

        printf("csap_destroy: %x\n", 
               rcf_ta_csap_destroy(ta, sid, handle)); 

    } while(0);

    return 0;
}
