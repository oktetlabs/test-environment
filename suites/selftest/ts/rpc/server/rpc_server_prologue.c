/** @file
 * @brief RPC Test Suite
 *
 * Test Suite prologue.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "rpc_server_prologue"

#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>

#include "tapi_test.h"
#include "tapi_cfg_net.h"


int
main(int argc, char **argv)
{
    TEST_START;

    CHECK_RC(tapi_cfg_net_all_up(FALSE));
    CHECK_RC(tapi_cfg_net_all_assign_ip(AF_INET));

    TEST_SUCCESS;
cleanup:
    TEST_END;
}
