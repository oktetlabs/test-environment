/** @file
 * @brief Test Environment
 *
 * Telephony test
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * $Id: $
 */

/** @page cdt-telephony Check dial tone on some telephony port
 *
 * @objective Check dial tone on some telephony port
 *
 * @param port      Telephony port
 * @param plan      Numbering plan
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
    TEST_GET_INT_PARAM(plan);

    rcf_rpc_server_create("Agt_A", "First", &pco);

    chan = rpc_telephony_open_channel(pco, port);
    rpc_telephony_pickup(pco, chan);

    RPC_AWAIT_IUT_ERROR(pco);
    if (rpc_telephony_check_dial_tone(pco, chan, plan, &state) < 0)
        TEST_FAIL("unable to check dial tone on port %d", port);
    
    if (state != TRUE)
        TEST_VERDICT("there is no %d dial tone on port %d", plan, port);

    TEST_SUCCESS;

cleanup:
    rpc_telephony_hangup(pco, chan);
    rpc_telephony_close_channel(pco, chan);

    TEST_END;
}
