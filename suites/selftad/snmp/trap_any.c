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
#define TE_TEST_NAME    "snmp/trap_any"

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


void
trap_handler(char *fn, void *p)
{ 
    RING("snmp TRAP handler, file: %s", fn);
}

int
main(int argc, char *argv[])
{
    int sid;
    int snmp_version;

    const char *ta;

    csap_handle_t trap_csap = CSAP_INVALID_HANDLE;

    TEST_START;
    TEST_GET_STRING_PARAM(ta);
    TEST_GET_INT_PARAM(snmp_version);

   
    INFO("Agent: %s", ta);
   
    
    {
        if (rcf_ta_create_session(ta, &sid) != 0)
            TEST_FAIL("rcf_ta_create_session failed");
        
        INFO("Test: Created session: %d", sid); 
    }

    do { 
        asn_value *pattern;

#if 0
        pattern = asn_parse_value_text("{{}}", const asn_type *type, );

#endif
        rc = tapi_snmp_gen_csap_create(ta, sid, NULL, "public",
                                       snmp_version,
                                       0, 162, 2000, &trap_csap);
        if (rc != 0)
            TEST_FAIL("CSAP for trap recv creation fails %X", rc); 

#if 0
        rc = tapi_snmp_trap_recv_start(ta, sid, trap_csap, pattern, );
#endif

    } while(0);

    TEST_SUCCESS;

cleanup:
    if (trap_csap != CSAP_INVALID_HANDLE)
        rcf_ta_csap_destroy(ta, sid, trap_csap); 

    TEST_END;
}
