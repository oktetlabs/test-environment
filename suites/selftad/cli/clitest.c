/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Simple RCF test
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME    "cli/simple"

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "te_stdint.h"
#include "te_errno.h"
#include "tapi_test.h"
#include "rcf_api.h"

#include "logger_api.h"

void
cli_msg_handler(char *fn, void *p)
{
    UNUSED(p);
    VERB("CLI message handler, file with NDS: %s\n", fn);
}

int
main(int argc, char *argv[])
{
    char *ta;
    int   sid;

    TEST_START;
    TEST_GET_STRING_PARAM(ta);

    /* Session */
    {
        if (rcf_ta_create_session(ta, &sid) != 0)
        {
            ERROR("rcf_ta_create_session failed\n");
            return 1;
        }
        INFO("Test: Created session: %d", sid);
    }

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
            INFO("te_suites: %s\n", te_suites);
        else
            break;

        strcpy(path, te_suites);
        strcat(path, "/selftad/cli/");
        path_prefix = strlen(path);
        strcpy(path + path_prefix, "cli-csap.asn");
        VERB("csap full path: %s\n", path);

        VERB("let's create csap for listen\n");
        rc =   rcf_ta_csap_create(ta, sid, "cli", path, &handle);
        VERB("csap_create rc: %r, csap id %d\n", rc, handle);
        if (rc)
            TEST_FAIL("CLI CSAP create failed %X", rc);

        strcpy(path + path_prefix, "cli-filter.asn");
        VERB("send template full path: %s\n", path);
        rc = rcf_ta_trsend_recv(ta, sid, handle, path, cli_msg_handler,
                                NULL, &timeout, num);
        VERB("trsend_recv: %r, timeout: %d, num: %d\n",
                    rc, timeout, num);
        if (rc)
            TEST_FAIL("CLI CSAP send_recv failed %X", rc);


        rc = rcf_ta_csap_destroy(ta, sid, handle);
        if (rc)
            TEST_FAIL("CLI CSAP destroy failed %X", rc);

    } while(0);

    TEST_SUCCESS;

cleanup:
    TEST_END;

}
