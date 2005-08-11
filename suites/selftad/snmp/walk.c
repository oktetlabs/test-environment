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

#define TE_TEST_NAME    "snmp_gets"

#define TE_LOG_LEVEL 0x0f

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

int
walk_cb(const tapi_snmp_varbind_t *vb, void *userdata)
{
    int *counter = (int *)userdata;

    if (vb == NULL)
    {
        ERROR("%s: zero varbind ptr passed!", __FUNCTION__);
        return TE_EWRONGPTR;
    }
    INFO("%s: oid %s received, type %d(%s), len %d", 
         __FUNCTION__, print_oid(&vb->name), 
         vb->type, tapi_snmp_val_type_h2str(vb->type), vb->v_len);

    if (counter != NULL)
        (*counter)++;

    return 0;
}

int
main(int argc, char *argv[])
{
    csap_handle_t snmp_csap = CSAP_INVALID_HANDLE;
    int sid;
    int err;
    int snmp_version;
    const char *ta;
    const char *mib_table;
    const char *mib_name;
    const char *snmp_agt;

    TEST_START;
    TEST_GET_STRING_PARAM(ta);
    TEST_GET_STRING_PARAM(mib_table);
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
        tapi_snmp_oid_t oid;

        int timeout = 30;
        int number;

        rc = tapi_snmp_csap_create(ta, sid, snmp_agt, "public", 
                                   snmp_version, &snmp_csap); 
        if (rc) 
            TEST_FAIL("Csap create error %X", rc);
        VERB("New csap %d", snmp_csap); 

        
        if ((rc = tapi_snmp_load_mib_with_path("/usr/share/snmp/mibs", 
                                                mib_name)) != 0)
            TEST_FAIL("snmp_load_mib(%s) failed, rc %x\n", mib_name, rc);

        if ((rc = tapi_snmp_make_oid(mib_table, &oid)) != 0)
            TEST_FAIL("tapi_snmp_make_oid() failed, rc %x\n", rc);

        number = 0;
        rc = tapi_snmp_walk(ta, sid, snmp_csap, &oid, &number, walk_cb);

        if (rc)
            TEST_FAIL("SNMP WALK failed with rc %X", rc); 

        INFO("SNMP walk passed, got %d varbinds", number);

    } while(0);

    TEST_SUCCESS;

cleanup:
    if (snmp_csap != CSAP_INVALID_HANDLE)
        rcf_ta_csap_destroy(ta, sid, snmp_csap); 

    TEST_END;
}
