/** @file
 * @brief XEN Test Suite
 *
 * Test Suite prologue.
 *
 * Copyright (C) 2003-2018 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id: prologue.c $
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "prologue"

#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>

#include "tapi_test.h"
#include "tapi_cfg_net.h"


int
main(void)
{
    int result = EXIT_FAILURE;

    signal(SIGINT, te_test_sig_handler);
    te_lgr_entity = TE_TEST_NAME;
    TAPI_ON_JMP(TEST_ON_JMP_DO);

    CHECK_RC(tapi_cfg_net_all_assign_ip(AF_INET));

    TEST_SUCCESS;

cleanup:

    return result;
}
