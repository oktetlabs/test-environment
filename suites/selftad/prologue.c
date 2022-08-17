/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief SelfTAD Test Suite
 *
 * Test Suite prologue.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "prologue"

#include "te_config.h"
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "tapi_test.h"
#include "tapi_cfg_net.h"


int
main(int argc, char *argv[])
{
    TEST_START;

    CHECK_RC(tapi_cfg_net_all_assign_ip(AF_INET));

    TEST_SUCCESS;

cleanup:
    CFG_WAIT_CHANGES;

    TEST_END;
}
