/** @file
 * @brief ACSE Test Suite
 *
 * ACSE Test Suite prologue.
 *
 * Copyright (C) 2010 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Konstantin Abramenko <Konstantin Abramenko@oktetlabs.ru>
 *
 * $Id: $
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "ACSE prologue"

#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>

#include "acse_suite.h"

#include "tapi_rpc_tr069.h"
#include "acse_epc.h"
#include "cwmp_data.h"
#include "tapi_test.h"
#include "tapi_acse.h"


int
main(int argc, char *argv[])
{
    int result = EXIT_FAILURE;
    const char *ta_acse;
    cfg_val_type type = CVT_INTEGER;
    int cfg_value;
    char *cr_url = NULL;
    int cr_url_wait_count, session_finish_count;
    rcf_rpc_server *rpcs_acse = NULL; 
    te_errno te_rc;
    int cr_state;
    
    TEST_GET_STRING_PARAM(ta_acse);

    signal(SIGINT, te_test_sig_handler);
    te_lgr_entity = TE_TEST_NAME;
    TAPI_ON_JMP(TEST_ON_JMP_DO);

#if 0
    CHECK_RC(tapi_acse_start(ta_acse));

    te_rc = rcf_rpc_server_create_ta_thread(ta_acse, "acse_ctl",
                &rpcs_acse);

    type = CVT_INTEGER;
    CHECK_RC(cfg_get_instance_fmt(&type, &cfg_value, 
                                 "/agent:%s/acse:", ta_acse));

    RING("value of acse leaf: %d", cfg_value);

    CHECK_RC(tapi_acse_manage_acs(ta_acse, "A", ACSE_OP_ADD,
                                  "auth_mode", "digest",
                                  "port", 8080, VA_END_LIST));

    CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box", ACSE_OP_ADD,
          "login", "000261-Home Gateway-V601L622R1A0-1001742119", 
          "passwd", "z7cD7CTDA1DrQKUb", 
          "cr_login", "000261-Home Gateway-V601L622R1A0-1001742119", 
          "cr_passwd", "z7cD7CTDA1DrQKUb", 
          VA_END_LIST));
#endif
    CHECK_RC(tapi_acse_manage_acs(ta_acse, "A", ACSE_OP_MODIFY,
                                  "enabled", 1, VA_END_LIST));

    CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box", ACSE_OP_OBTAIN,
          "cr_url", &cr_url, VA_END_LIST));

    if (cr_url != NULL)
        RING("got ConnReq url '%s'", cr_url);
    else
        RING("got ConnReq url empty");

    cr_url_wait_count = 20;

    while (cr_url == NULL || strlen(cr_url) == 0)
    {
        sleep(1);
        CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box",
                 ACSE_OP_OBTAIN, "cr_url", &cr_url, VA_END_LIST));

        RING("got ConnReq url '%s', count %d", cr_url, cr_url_wait_count);

        if ((--cr_url_wait_count) < 0)
            break;
    }

    if (cr_url == NULL || strlen(cr_url) == 0)
        TEST_FAIL("No Conn Req url on ACSE");

    session_finish_count = 10;
    do {
        sleep(1);
        CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box", ACSE_OP_OBTAIN,
              "cwmp_state", &cr_state, VA_END_LIST));

        RING("cwmp_state on box is %d", cr_state);
        session_finish_count--;
    } while (cr_state != 0 && session_finish_count > 0);


    TEST_SUCCESS;

cleanup:

    return result;
}
