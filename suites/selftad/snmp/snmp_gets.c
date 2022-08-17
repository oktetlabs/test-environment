/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Simple RCF test
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME    "snmp_gets"

#define TE_LOG_LEVEL 0x0f

#include "te_config.h"

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
main(int argc, char *argv[])
{
    csap_handle_t snmp_csap = CSAP_INVALID_HANDLE;
    int sid;
    int err;
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
            TEST_FAIL("Session create error %r", rc);
        }
        VERB("Session created %d", sid);
    }

    do {
        tapi_snmp_varbind_t vb;
        tapi_snmp_oid_t oid;
        int value;
        int errstat, errindex;

        rc = tapi_snmp_csap_create(ta, sid, snmp_agt, "public",
                                   snmp_version, &snmp_csap);
        if (rc)
            TEST_FAIL("Csap create error %r", rc);
        VERB("New csap %d", snmp_csap);


        if ((rc = tapi_snmp_load_mib_with_path("/usr/share/snmp/mibs",
                                                mib_name)) != 0)
            TEST_FAIL("snmp_load_mib(%s) failed, rc %r", mib_name, rc);

        if ((rc = tapi_snmp_make_oid(mib_object, &oid)) != 0)
            TEST_FAIL("tapi_snmp_make_oid() failed, rc %r", rc);

        rc = tapi_snmp_get(ta, sid, snmp_csap, &oid, TAPI_SNMP_NEXT,
                           &vb, &err);
        if (rc)
            TEST_FAIL("SNMP GET NEXT failed with rc %r", rc);

        INFO("getnext for object %s got oid %s",
             mib_object, print_oid(&oid));


        oid = vb.name;
        rc = tapi_snmp_get(ta, sid, snmp_csap, &oid, TAPI_SNMP_EXACT,
                           &vb, &err);
        if (rc)
            TEST_FAIL("SNMP GET failed with rc %r", rc);

        INFO("get for object %s got oid %s", mib_object, print_oid(&oid));

        oid.length = 1;
        oid.id[0] = 0;
        value = 0;
        rc = tapi_snmp_get_row(ta, sid, snmp_csap, &errstat, &errindex,
                               &oid, "ifNumber", &value, NULL);
        if (rc)
            TEST_FAIL("SNMP get_row method failed with rc %r", rc);

        INFO("get row for ifNumber got value %d", value);

    } while(0);

    TEST_SUCCESS;

cleanup:
    if (snmp_csap != CSAP_INVALID_HANDLE)
        rcf_ta_csap_destroy(ta, sid, snmp_csap);

    TEST_END;
}
