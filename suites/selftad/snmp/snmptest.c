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

#define TEST_NAME "snmp_gets"

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "tapi_test.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"
#include "tapi_snmp.h"


int
main(int argc, char *argv[])
{
    int snmp_csap = 0;
    int sid;
    int snmp_version;
    const char *ta;
    const char *mib_object;
    const char *mib_name;
    const char *snmp_agt;

    TEST_START;
    TEST_GET_STRING_PARAM(ta);
    TEST_GET_STRING_PARAM(mib_object);
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
        tapi_snmp_varbind_t vb;
        tapi_snmp_oid_t oid;
        int timeout = 30;
        int value;

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

        if ((rc = tapi_snmp_make_oid(mib_object, &oid)) != 0)
        {
            TEST_FAIL("tapi_snmp_make_oid() failed, rc %x\n", rc);
	    return rc;
	}

        rc = tapi_snmp_get(ta, sid, snmp_csap, &oid, TAPI_SNMP_NEXT, &vb);
        if (rc)
            TEST_FAIL("SNMP GET NEXT failed with rc %X", rc);

        INFO("getnext for object %s got oid %s", mib_object, print_oid(&oid));


        oid = vb.name;
        rc = tapi_snmp_get(ta, sid, snmp_csap, &oid, TAPI_SNMP_EXACT, &vb);
        if (rc)
            TEST_FAIL("SNMP GET failed with rc %X", rc);

        INFO("get for object %s got oid %s", mib_object, print_oid(&oid));

        oid.length = 1;
        oid.id[0] = 0; 
        value = 0;
        rc = tapi_snmp_get_row(ta, sid, snmp_csap, &oid, "ifNumber", &value, NULL);
        if (rc)
            TEST_FAIL("SNMP get_row method failed with rc %X", rc);

        INFO("get row for ifNumber got value %d", value);



        rcf_ta_csap_destroy(ta, sid, snmp_csap); 

    } while(0);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
