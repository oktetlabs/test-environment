/** @file
 * @brief RPC Test Suite
 *
 * Test Suite prologue.
 *
 * Copyright (C) 2005 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "prologue"

#include <stdlib.h>
#include <signal.h>

#include "tapi_test.h"
#include "tapi_cfg_net.h"


int
main(void)
{
    int result = EXIT_FAILURE;

    signal(SIGINT, sigint_handler);
    te_lgr_entity = TE_TEST_NAME;
    TAPI_ON_JMP(TEST_ON_JMP_DO);

    CHECK_RC(tapi_cfg_net_all_assign_ip4());

    TEST_SUCCESS;

cleanup:
    CFG_WAIT_CHANGES;

    return result;
}
