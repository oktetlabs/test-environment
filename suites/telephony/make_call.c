/** @file
 * @brief Test Environment
 *
 * Telephony test
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
    int chan_src;
    int chan_dst;
    int port_src;
    int port_dst;
    int timeout;
    char *number;
    te_errno errno;
    rcf_rpc_server *pco_src = NULL;
    rcf_rpc_server *pco_dst = NULL;

    TEST_START; 
   
    TEST_GET_INT_PARAM(port_src);
    TEST_GET_INT_PARAM(port_dst);
    TEST_GET_STRING_PARAM(number);
    TEST_GET_INT_PARAM(timeout);


    rcf_rpc_server_create("Agt_A", "First", &pco_src);
    rcf_rpc_server_create("Agt_A", "Second", &pco_dst);

    chan_src = rpc_telephony_open_channel(pco_src, port_src);
    chan_dst = rpc_telephony_open_channel(pco_dst, port_dst);
    
    rpc_telephony_pickup(pco_src, chan_src);
    rpc_telephony_dial_number(pco_src, chan_src, number);
    
    errno = rpc_telephony_call_wait(pco_dst, chan_dst, timeout);

    if (errno == TE_ERPCTIMEOUT)
        TEST_VERDICT("Internal call have been maked but not catched");
    else if (errno != 0)
        TEST_VERDICT("Can't make internal call");

    rpc_telephony_pickup(pco_dst, chan_dst);

    TEST_SUCCESS;

cleanup:
    rpc_telephony_hangup(pco_dst, chan_dst);
    rpc_telephony_hangup(pco_src, chan_src);
    rpc_telephony_close_channel(pco_dst, chan_dst);
    rpc_telephony_close_channel(pco_src, chan_src);

    TEST_END;
}
