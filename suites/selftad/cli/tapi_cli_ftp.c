/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Simple RCF test
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME    "cli/shell"

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

#define TE_LOG_LEVEL 0xff
#include "logger_api.h"

#if 1
#define CLI_DEBUG(args...) \
    do {                                        \
        fprintf(stdout, "\nTEST CLI " args);    \
        INFO("TEST CLI " args);                 \
    } while (0)

#undef ERROR
#define ERROR(args...) CLI_DEBUG("ERROR: " args)

#undef RING
#define RING(args...) CLI_DEBUG("RING: " args)

#undef WARN
#define WARN(args...) CLI_DEBUG("WARN: " args)

#undef VERB
#define VERB(args...) CLI_DEBUG("VERB: " args)
#endif

int
main(int argc, char *argv[])
{
    char *ta;
    char *shell_args = "ftp aros";
    char *cprompt = "ftp> ";
    char *lprompt = "): ";
    char *pprompt = "Password:";
    char *user = "anonymous";
    char *password = user;

    int   sid;
    csap_handle_t  cli_csap;
    char *ftp_result = NULL;

    int   i;
    int   try_count = 3;

    TEST_START;
    TEST_GET_STRING_PARAM(ta);

    CHECK_RC(rcf_ta_create_session(ta, &sid));

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    VERB("Try to create Shell CLI session with args %s", shell_args);

#if 1
    CHECK_RC(tapi_cli_csap_shell_create(ta, sid, shell_args,
                                        TAPI_CLI_PROMPT_TYPE_PLAIN, cprompt,
                                        TAPI_CLI_PROMPT_TYPE_PLAIN,
                                        lprompt, user,
                                        TAPI_CLI_PROMPT_TYPE_PLAIN,
                                        pprompt, password, &cli_csap));
#else
    CHECK_RC(tapi_cli_csap_shell_create(ta, 0, "ftp aros",
                 TAPI_CLI_PROMPT_TYPE_PLAIN, "ftp> ",
                 TAPI_CLI_PROMPT_TYPE_PLAIN, "): ", "anonymous",
                 TAPI_CLI_PROMPT_TYPE_PLAIN, "Password:", "anonymous",
                 &cli_csap));
#endif

    VERB("Try to send command : %s", "ls");

    CHECK_RC(tapi_cli_send(ta, sid, cli_csap, "ls"));

    VERB("Try to send_recv : %s", "ls");

    CHECK_RC(tapi_cli_send_recv(ta, sid, cli_csap, "ls", &ftp_result, 5000000));

    VERB("send_recv response : \n%s", ftp_result);

    VERB("Try to send_recv : %s", "pwd");

    CHECK_RC(tapi_cli_send_recv(ta, sid, cli_csap, "pwd", &ftp_result, 5000000));

    VERB("send_recv response : \n%s", ftp_result);

    VERB("Try to send command : %s", "bye");

    CHECK_RC(tapi_cli_send(ta, sid, cli_csap, "bye"));

    VERB("Try to destroy CLI CSAP");

    CHECK_RC(rcf_ta_csap_destroy(ta, sid, cli_csap));

    TEST_SUCCESS;

cleanup:

    free(ftp_result);

    TEST_END;

}
