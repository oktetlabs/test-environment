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


void
trap_handler(char *fn, void *p)
{ 
    printf ("snmp TRAP handler, file: %s\n", fn);
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

#if 0
        strcpy(path, "/tmp/csap_file");
#else
        strcpy (path, te_suites);
        strcat (path, "/selftest/snmp_nds/");
        path_prefix = strlen(path);


        strcpy(path + path_prefix, "trap-csap.asn");
#endif
        printf("let's create csap for traps\n"); 
        rc =   rcf_ta_csap_create(ta, sid, "snmp", path, &handle);
        printf("csap_create rc: %d, csap id %d\n", rc, handle); 
        if (rc) break;
        sleep (2);


#if 0
        strcpy(path, "/tmp/csap_file");
#else
        strcpy(path + path_prefix, "trap-filter.asn");
#endif
        printf ("send template full path: %s\n", path);

#if 1
        rc = rcf_ta_trrecv_start(ta, sid, handle, path, 0,
                                 trap_handler, NULL, 0);
        printf("trrecv_start: 0x%x \n", rc);
        if (rc) break;

#if 1
        sleep(1);
        printf ("try to get\n");
        rc = rcf_ta_trrecv_get(ta, sid, handle, &num);
        printf("trrecv_get: 0x%x num: %d\n", rc, num);

#endif
        num = 10;
        printf ("sleep %d secs before stop\n", num);
        sleep (num);

        printf ("try to stop\n");
        rc = rcf_ta_trrecv_stop(ta, sid, handle, &num);
        printf("trrecv_stop: 0x%x num: %d\n", rc, num);

#endif

#if 1
        printf ("wait for exactly 1 traps more:\n");
        rc = rcf_ta_trrecv_start(ta, sid, handle, path, 1,
                                 trap_handler, NULL, 1);
        printf("trrecv_start: 0x%x \n", rc);

#endif
        num = 1;
        printf ("sleep %d secs before destroy\n", num);
        sleep (num);


        printf ("try to  destroy\n");
        printf("csap_destroy: %x\n", 
               rcf_ta_csap_destroy(ta, sid, handle)); 

    } while(0);

    return 0;
}
