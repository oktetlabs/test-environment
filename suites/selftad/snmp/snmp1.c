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
#include "tapi_snmp.h"

int 
test_walk_callback(const tapi_snmp_varbind_t *varbind, void *userdata)
{
    printf ("walk CALLBACK: ");
    print_objid(varbind->name.id, varbind->name.length);
    return 0;
}


int
main(int argc, char *argv[])
{
    char ta[32];
    int  len = sizeof(ta);
    int  sid = 2;
    
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
    if (rcf_ta_create_session(ta, &sid) != 0)
    {
        printf("rcf_ta_create_session failed\n");
        return 1;
    }


    do {
        tapi_snmp_oid_t walk_oid     =
            {11, {1,3,6,1,4,1,4491,2,4,5,1}};
        tapi_snmp_oid_t ctp_num_pkts =
            {14, {1,3,6,1,4,1,4491,2,4,5,1,2,6, 0}};

        int errstat;
        int csap;
        int rc;
        tapi_snmp_varbind_t vb;
        tapi_snmp_oid_t root_oid;

        memset (&vb, 0, sizeof(vb));
        root_oid.length = 1;
        root_oid.id[0] = 1;
        
        
        printf ("before csap create\n");
        fflush(stdout);
        rc =  tapi_snmp_csap_create(ta, sid, "192.168.253.224", 
                                    "public", 2, &csap);
        printf("csap_create rc: %d\n", rc); 

        if (rc == 0) 
            rc = tapi_snmp_get(ta, sid, csap, &root_oid, TAPI_SNMP_NEXT,
                               &vb);
        printf("snmp get next rc: %d\n", rc); 

        if (rc == 0) 
            rc = tapi_snmp_walk(ta, sid, csap, &walk_oid, NULL, 
                                test_walk_callback);
        printf("snmp walk rc: %d\n", rc); 
        
        if (rc == 0) 
            rc = tapi_snmp_set_integer(ta, sid, csap, &ctp_num_pkts, 
                                        100, &errstat);
        printf("snmp set rc: %d; errstat: %d\n", rc, errstat); 
#if 1 
        printf("csap_destroy: %d\n", 
               rcf_ta_csap_destroy(ta, sid, csap)); 
#endif

    } while(0);

    return 0;
}
