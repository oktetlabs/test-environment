/** @file
 * @brief TAD Test Suite
 *
 * Test Suite prologue.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 */

/** @page prologue Prepare testing networks
 *
 * @objective Prepare testing networks.
 *
 * @par Scenario:
 *
 * -# Up all interfaces mentioned in /net/node configuration.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

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

    CHECK_RC(tapi_cfg_net_all_up(FALSE));
    CHECK_RC(tapi_cfg_net_all_assign_ip(AF_INET));

    TEST_SUCCESS;

cleanup:
    CFG_WAIT_CHANGES;

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
