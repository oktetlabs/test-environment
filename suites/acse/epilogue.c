/** @file
 * @brief ACSE Test Suite
 *
 * ACSE Test Suite epilogue.
 *
 * Copyright (C) 2004-2018 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Konstantin Abramenko <Konstantin Abramenko@oktetlabs.ru>
 *
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "ACSE epilogue"

#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>

#include "acse_suite.h"
#include "tapi_test.h"
#include "tapi_acse.h"


int
main(int argc, char *argv[])
{
    const char *ta_acse;
    int cfg_value;

    TEST_START;

    TEST_GET_STRING_PARAM(ta_acse);

    CHECK_RC(tapi_acse_stop(ta_acse));

    CHECK_RC(cfg_get_instance_int_fmt(&cfg_value, "/agent:%s/acse:", ta_acse));


    RING("value of acse leaf: %d", cfg_value);

    if (cfg_value != 0)
        TEST_FAIL("value of ACSE leaf should be zero");

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
