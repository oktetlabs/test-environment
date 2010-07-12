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

/** @page mc-telephony Make a call from one telephony port to another
 *
 * @objective Make a call from one telephony port to another
 *
 * @author Evgeny Omelchenko <Evgeny.Omelchenko@oktetlabs.ru>
 */

#define TE_TEST_NAME    "make_call"

#include "te_config.h"

#include "tapi_test.h"
#include "tapi_rpc_telephony.h"

#include "rcf_rpc.h"

int
main(int argc, char *argv[])
{
    int chan_iut;
    int chan_aux;
    int port_from;
    int port_to;
    char *number;
    rcf_rpc_server *pco_iut = NULL;
    rcf_rpc_server *pco_aux = NULL;

    TEST_START; 
   
    TEST_GET_INT_PARAM(port_from);
    TEST_GET_INT_PARAM(port_to);
    TEST_GET_STRING_PARAM(number);


    rcf_rpc_server_create("Agt_A", "First", &pco_iut);
    rcf_rpc_server_create("Agt_A", "Second", &pco_aux);

    chan_iut = rpc_telephony_open_channel(pco_iut, port_from);
    chan_aux = rpc_telephony_open_channel(pco_aux, port_to);
    
    rpc_telephony_pickup(pco_iut, chan_iut);
    sleep(2);
    rpc_telephony_dial_number(pco_iut, chan_iut, number);
    rpc_telephony_call_wait(pco_aux, chan_aux);
    rpc_telephony_pickup(pco_aux, chan_aux);
    rpc_telephony_hangup(pco_aux, chan_aux);
    rpc_telephony_hangup(pco_iut, chan_iut);
    rpc_telephony_close_channel(pco_iut, chan_iut);
    rpc_telephony_close_channel(pco_aux, chan_aux);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
