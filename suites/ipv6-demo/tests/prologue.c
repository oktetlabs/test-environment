/** @file
 * @brief IPv6 Demo Test Suite
 *
 * Test Suite prologue.
 *
 * Copyright (C) 2007 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME    "prologue"

#include "ipv6-demo-test.h"

#include "tapi_cfg_base.h"
#include "tapi_cfg_net.h"
#include "logger_ten.h"
#include "tapi_test.h"
#include "tapi_cfg.h"

/**
 * Configures IPv4 and IPv6 subnetworks for network configurations.
 *
 * @retval EXIT_SUCCESS     success
 * @retval EXIT_FAILURE     failure
 */
int
main(int argc, char **argv)
{
    unsigned int i;
    cfg_nets_t   nets;

/* Redefine as empty to avoid environment processing here */
#undef TEST_START_SPECIFIC
#define TEST_START_SPECIFIC
    TEST_START;
#if 0
    tapi_env_init(&env);
#endif

    if ((rc = tapi_cfg_net_remove_empty()) != 0)
        TEST_FAIL("Failed to remove /net instances with empty interfaces");

    rc = tapi_cfg_net_reserve_all();
    if (rc != 0)
    {
        TEST_FAIL("Failed to reserve all interfaces mentioned in network "
                  "configuration: %r", rc);
    }

    rc = tapi_cfg_net_all_up(FALSE);
    if (rc != 0)
    {
        TEST_FAIL("Failed to up all interfaces mentioned in networks "
                  "configuration: %r", rc);
    }

    rc = tapi_cfg_net_delete_all_ip4_addresses();
    if (rc != 0)
    {
        TEST_FAIL("Failed to delete all IPv4 addresses from all "
                  "interfaces mentioned in networks configuration: %r",
                  rc);
    }

    /* Get available networks */
    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        TEST_FAIL("Failed to get networks from Configurator: %r", rc);
    }

    /* Assign IPv4 and IPv6 subnets for available networks */
    for (i = 0; i < nets.n_nets; ++i)
    {
        cfg_net_t  *net = nets.nets + i;

        rc = tapi_cfg_net_assign_ip(AF_INET, net, NULL);
        if (rc != 0)
        {
            ERROR("Failed to assign IPv4 subnet to net #%u: %r", i, rc);
            break;
        }

        rc = tapi_cfg_net_assign_ip(AF_INET6, net, NULL);
        if (rc != 0)
        {
            ERROR("Failed to assign IPv6 subnet to net #%u: %r", i, rc);
            break;
        }
    }
    tapi_cfg_net_free_nets(&nets);
    if (rc != 0)
    {
        TEST_FAIL("Failed to prepare testing networks");
    }

    CFG_WAIT_CHANGES;
    CHECK_RC(rc = cfg_synchronize("/:", TRUE));

#if 0
    TEST_START_ENV;
#endif

    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/:"));
    
    SLEEP(2);

    TEST_SUCCESS;

cleanup:

#undef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC
    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
