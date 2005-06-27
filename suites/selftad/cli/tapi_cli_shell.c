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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * 
 * $Id$
 */

#define TE_TEST_NAME    "cli/shell"

#include "config.h"

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

#if 0
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
    char *shell_args;
    char *cprompt;
    int   sid;
    csap_handle_t  cli_csap;
    char *gdb_result = NULL;

    int   i;
    int   try_count = 3;
   
    TEST_START;
    TEST_GET_STRING_PARAM(ta);
    TEST_GET_STRING_PARAM(shell_args);
    TEST_GET_STRING_PARAM(cprompt);

    CHECK_RC(rcf_ta_create_session(ta, &sid));

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    VERB("Try to create Shell CLI session with args %s", shell_args);

    TAPI_CLI_CSAP_CREATE_SHELL(ta, sid, shell_args, cprompt,
                               NULL, NULL, NULL, NULL, &cli_csap);

    VERB("Try to send command : %s", "p /x 100");

    CHECK_RC(tapi_cli_send(ta, sid, cli_csap, "p /x 100"));

    for (i = 0; i < try_count; i++)
    {
        VERB("Try to send command : %s", "p $1 * 100");

        CHECK_RC(tapi_cli_send(ta, sid, cli_csap, "p $1 * 100"));
    }

    VERB("Try to send_recv : %s", "p /x 137 * 193");

    CHECK_RC(tapi_cli_send_recv(ta, sid, cli_csap, "p /x 137 * 193", &gdb_result, 5000000));

    VERB("send_recv returns %x", gdb_result);

#if 1
    VERB("send_recv response : %s", gdb_result);
#endif

#if 1
    VERB("Try to send command : %s", "q");

    CHECK_RC(tapi_cli_send(ta, sid, cli_csap, "q"));
#endif

#if 0
    fprintf(stdout, "\nTry to destroy CLI CSAP\n");
#endif

    VERB("Try to destroy CLI CSAP");

    CHECK_RC(rcf_ta_csap_destroy(ta, sid, cli_csap));

    TEST_SUCCESS;

cleanup:
    TEST_END;

}
