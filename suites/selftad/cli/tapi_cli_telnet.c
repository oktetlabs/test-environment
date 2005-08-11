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

#define TE_TEST_NAME    "cli/telnet"

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

    char *host;
    char *user;
    char *passwd;
    int   sid;
    csap_handle_t  cli_csap;

    int   i;
    int   try_count = 10;
    char *cprompt = "--> ";
   
    TEST_START;
    TEST_GET_STRING_PARAM(ta);
    TEST_GET_STRING_PARAM(host);
    TEST_GET_STRING_PARAM(user);
    TEST_GET_STRING_PARAM(passwd);

    VERB("Try to create RCF session");

    CHECK_RC(rcf_ta_create_session(ta, &sid));

    VERB("Try to create Telnet CLI session on the %s", host);
    
    TAPI_CLI_CSAP_CREATE_TELNET(ta, sid, host, user, passwd,
                                cprompt, &cli_csap);

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
