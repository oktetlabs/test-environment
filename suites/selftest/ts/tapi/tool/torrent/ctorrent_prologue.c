/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Torrent Test Suite
 *
 * Test Suite prologue.
 *
 * Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME "ctorrent/prologue"

#include "tapi_cfg_net.h"
#include "tapi_test.h"
#include "tapi_cfg.h"

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Up all interfaces");
    CHECK_RC(tapi_cfg_net_all_up(FALSE));

    TEST_STEP("Assign IP for all networks");
    CHECK_RC(tapi_cfg_net_all_assign_ip(AF_INET));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
