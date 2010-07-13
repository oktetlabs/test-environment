/** @file
 * @brief Test Environment
 *
 * Telephony test
 * 
 * Copyright (C) 2007 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * $Id: $
 */

/** @page cdt-telephony Check dial tone on some telephony port
 *
 * @objective Check dial tone on some telephony port
 *
 * @param port      Telephony port
 *
 * @author Evgeny Omelchenko <Evgeny.Omelchenko@oktetlabs.ru>
 */

#define TE_TEST_NAME    "check_dial_tone"

#include "te_config.h"

#include "tapi_test.h"
#include "tapi_rpc_telephony.h"

#include "rcf_rpc.h"

int
main(int argc, char *argv[])
{
    int chan;
    int port;
    int state;
    rcf_rpc_server *pco = NULL;

    TEST_START; 
   
    TEST_GET_INT_PARAM(port);

    rcf_rpc_server_create("Agt_A", "First", &pco);

    chan = rpc_telephony_open_channel(pco, port);
    rpc_telephony_pickup(pco, chan);
    rpc_telephony_check_dial_tone(pco, chan, &state);
    
    if (state != TRUE)
        TEST_FAIL("there is no dial tone on port %d", port);

    TEST_SUCCESS;

cleanup:
    rpc_telephony_hangup(pco, chan);
    rpc_telephony_close_channel(pco, chan);

    TEST_END;
}
