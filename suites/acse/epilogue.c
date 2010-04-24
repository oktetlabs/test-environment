/** @file
 * @brief ACSE Test Suite
 *
 * ACSE Test Suite epilogue.
 *
 * Copyright (C) 2010 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Konstantin Abramenko <Konstantin Abramenko@oktetlabs.ru>
 *
 * $Id: $
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "ACSE epilogue"

#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>

#include "tapi_test.h"
#include "tapi_acse.h"


int
main(int argc, char *argv[])
{
    int result = EXIT_FAILURE;
    const char *ta_acse;
    cfg_val_type type = CVT_INTEGER;
    int cfg_value;
    
    TEST_GET_STRING_PARAM(ta_acse);

    signal(SIGINT, te_test_sig_handler);
    te_lgr_entity = TE_TEST_NAME;
    TAPI_ON_JMP(TEST_ON_JMP_DO);

    CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box", ACSE_OP_DEL,
                                  VA_END_LIST));

    CHECK_RC(tapi_acse_manage_acs(ta_acse, "A", ACSE_OP_DEL,
                                  VA_END_LIST));


    CHECK_RC(tapi_acse_stop(ta_acse));

    type = CVT_INTEGER;
    CHECK_RC(cfg_get_instance_fmt(&type, &cfg_value, 
                                 "/agent:%s/acse:", ta_acse));

    RING("value of acse leaf: %d", cfg_value);

    TEST_SUCCESS;

cleanup:

    return result;
}
