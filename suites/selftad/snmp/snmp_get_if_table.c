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

#define TE_TEST_NAME    "snmp/snmp_get_if_table"

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
#include "tapi_test.h"
#include "rcf_api.h"
#include "logger_api.h"
#include "tapi_snmp.h"


#include "tapi_snmp_iftable.h"



int
main(int argc, char *argv[])
{
    tapi_snmp_if_table_row_t * iftable_result;
    int snmp_csap = 0;
    int sid;
    int snmp_version;
    const char *ta;
    const char *mib_name;
    const char *snmp_agt;

    TEST_START;
    TEST_GET_STRING_PARAM(ta);
    TEST_GET_STRING_PARAM(mib_name);
    TEST_GET_STRING_PARAM(snmp_agt);
    TEST_GET_INT_PARAM(snmp_version);
    
    /* Session */
    {
        if ((rc = rcf_ta_create_session(ta, &sid)) != 0)
        {
            TEST_FAIL("Session create error %X", rc);
        }
        VERB("Session created %d", sid);
    }

    do {
        tapi_snmp_oid_t ifTable_oid;
        int num;

        rc = tapi_snmp_csap_create(ta, sid, snmp_agt, "public", 
                                   snmp_version, &snmp_csap); 
        if (rc) 
            TEST_FAIL("Csap create error %X", rc);
        VERB("New csap %d", snmp_csap); 

        
        if ((rc = tapi_snmp_load_mib_with_path("/usr/share/snmp/mibs", 
                                                mib_name)) != 0)
        {
            TEST_FAIL("snmp_load_mib(%s) failed, rc %x\n", mib_name, rc);
            return rc;
        }

        if ((rc = tapi_snmp_make_oid("ifTable", &ifTable_oid)) != 0)
        {
            TEST_FAIL("tapi_snmp_make_oid() failed, rc %x\n", rc);
            return rc;
        }
        if (rc)
            TEST_FAIL("SNMP GET NEXT failed with rc %X", rc);

        iftable_result = NULL;

        rc = tapi_snmp_get_table(ta, sid, snmp_csap, &ifTable_oid, &num, 
                                 &iftable_result);
        if (rc)
            TEST_FAIL("SNMP GET NEXT failed with rc %X", rc);

        INFO("snmp get table got num: %d, ptr %x", num, iftable_result); 

        if (num > 0)
        {
            int i;
            for (i = 0; i < num; i ++)
            {
                INFO("row %d: ", i );
                if (iftable_result[i].index_suffix)
                    INFO(" index_suffix %s", 
                            print_oid(iftable_result[i].index_suffix));
        #if 1
                if (iftable_result[i].ifDescr)
                    INFO(" ifDescr \"%s\"", iftable_result[i].ifDescr->data);
        #endif
            }
        }

        rcf_ta_csap_destroy(ta, sid, snmp_csap); 

    } while(0);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}

#define  OLD 0

#if OLD
int
main()
{
    int rc;
    int ifIndex;
    char mib_path[] = "/home/konst/work/cablehome/atmos/source/snmpr/mibs/";
    char ta[32];
    int  len = sizeof(ta);
    int  sid = 2;
    tapi_snmp_if_table_row_t * iftable_result;
    
    printf("Starting test\n");

    if (rcf_get_ta_list(ta, &len) != 0)
    {
        printf("rcf_get_ta_list failed\n");
        return 1;
    }
    printf("Agent: %s\n", ta);
    
    if (rcf_ta_create_session(ta, &sid) != 0)
    {
        printf("rcf_ta_create_session failed\n");
        return 1;
    }

    if ((rc = tapi_snmp_load_mib_with_path(mib_path, "IF-MIB")))
    {
        printf ("log IF-MIB failed with rc %d\n", rc);
        return 1;
    }


    do { 
        int csap;
        int rc;
        int errstat = 0;
        int num;

        tapi_snmp_varbind_t vb[100];
        tapi_snmp_oid_t root_oid     = {11, {1,3,6,1,4,1,4491,2,4,5,1}};
        tapi_snmp_oid_t ctp_num_pkts = {14, {1,3,6,1,4,1,4491,2,4,5,1,2,6, 0}};
        tapi_snmp_oid_t ifTable      = {9, {1,3,6,1,2,1,2,2,1}}; 
        tapi_snmp_oid_t ctp_ping_sent= {14, {1,3,6,1,4,1,4491,2,4,5,1,3,11,0}};

        memset (&vb, 0, sizeof(vb));
        
        
        printf ("before csap create\n");
        fflush(stdout);
        rc =  tapi_snmp_csap_create(ta, sid, "192.168.253.224", 
                                    "public", 2, &csap);
        printf("csap_create rc: %d\n", rc); 

        if (rc == 0) 
            rc = tapi_snmp_get_integer(ta, sid, csap, &ctp_num_pkts, &num);
        printf("snmp get integer rc: %d, got num: %d\n", rc, num); 

#if 1
        num = 50;
        if (rc == 0) 
            rc = tapi_snmp_getbulk(ta, sid, csap, &ctp_ping_sent, &num, vb);
        printf("snmp getbulk rc: %d\n", rc); 

        if (rc == 0)
        {
            int i;
            printf("number of vars: %d\n", num);
            for (i = 0; i < num; i ++)
            {
                printf ("var %d, oid: ");
                print_objid(vb[i].name.id, vb[i].name.length);
            }
        }
#endif
#if 1
        if (rc == 0) 
            rc = tapi_snmp_get_table(ta, sid, csap, &ifTable, &num, 
                                     &iftable_result);
        printf("snmp get table rc: %d; num: %d\n", rc, num); 
                fflush(stdout);
        printf ("iftable_result: %p\n",iftable_result);

        if (rc == 0)
        {
            int i;
            for (i = 0; i < num; i ++)
            {
                printf ( "row %d: ", i );
        #if 1
                if (iftable_result[i].ifIndex)
                    printf(" ifIndex %d\n", (int) *(iftable_result[i].ifIndex));
                if (iftable_result[i].ifDescr)
                    printf(" ifDescr \"%s\"\n", iftable_result[i].ifDescr->data);
        #endif
            }
        }
        
#endif
#if 1 
        printf("csap_destroy: %d\n", 
               rcf_ta_csap_destroy(ta, sid, csap)); 
#endif

    } while(0);

    return 0;
}

#endif
