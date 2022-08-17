/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Simple RCF test
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME    "cli/serial"

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
#include "tapi_cli.h"
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
    char *device;
    char *user;
    char *passwd;
    int   sid;
    csap_handle_t  cli_csap;

    int   i;
    int   try_count = 10;
    char *cprompt = "--> ";

    TEST_START;
    TEST_GET_STRING_PARAM(ta);
    TEST_GET_STRING_PARAM(device);
    TEST_GET_STRING_PARAM(user);
    TEST_GET_STRING_PARAM(passwd);

    CHECK_RC(rcf_ta_create_session(ta, &sid));

    VERB("Try to create CLI session on the local device %s", device);

    TAPI_CLI_CSAP_CREATE_SERIAL(ta, sid, device, user,
                                passwd, cprompt, &cli_csap);

    VERB("Try to send command");

    CHECK_RC(tapi_cli_send(ta, sid, cli_csap, "snmp list trapdestinations"));

    for (i = 0; i < try_count; i++)
    {
        VERB("Try to send command");

        CHECK_RC(tapi_cli_send(ta, sid, cli_csap, "snmp send trap abs2200"));
    }

    VERB("Try to destroy CLI CSAP");

    CHECK_RC(rcf_ta_csap_destroy(ta, sid, cli_csap));

    TEST_SUCCESS;

cleanup:
    TEST_END;

}
