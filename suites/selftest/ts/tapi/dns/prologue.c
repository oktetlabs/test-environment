/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief DNS Test Suite.
 *
 * Test Suite prologue.
 */

#define TE_TEST_NAME    "prologue"

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
    CHECK_RC(tapi_cfg_net_all_assign_ip(AF_INET6));

    VSLEEP(2, "Wait for interfaces to up");

    TEST_SUCCESS;
cleanup:
    TEST_END;
}