/** @file
 * @brief Test Environment
 *
 * Check PHY support in Configurator
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

/** @page cs-ts-phy Check configurator PHY management support
 *
 * @objective Check configurator PHY management support
 *
 * @param ta            Test agent name
 * @param iface_name    Interface name
 * @param speed         Interface speed
 * @param duplex        Interface duplex state
 * @param speed_adver   Interface speed to advertise
 * @param duplex_adver  Interface duplex state to advertise
 *
 * @par Scenario
 * -# Get PHY state value
 * -# Check that PHY is at state ON
 * -# Get PHY autonegotiation state
 * -# Turn PHY autonegotiation to state OFF
 * -# Check that PHY autonegotiation is at state OFF
 * -# Set PHY speed value equal to @p speed
 * -# Check that PHY speed has been set correctly
 * -# Set PHY duplex value equal to @p duplex
 * -# Check that PHY duplex state has been set correctly
 * -# Turn PHY autonegotiation to state ON
 * -# Check that PHY autonegotiation is at state ON
 * -# Check advertising support:
 *        a) Check that mode (@p speed_adver, @p duplex_adver)
 *           is advertised
 *        b) Turn off advertising for this mode
 *        c) Check that mode is not advertised
 *        d) Turn on advertising for this mode
 *        e) Check that mode is advertised
 * -# Restart PHY autonegotiation
 *
 */

#define TE_TEST_NAME    "phy"

#include "te_config.h"
#include "te_defs.h"
#include "conf_api.h"
#include "tapi_cfg_base.h"
#include "tapi_test.h"
#include "tapi_cfg.h"
#include "tapi_rpc.h"
#include "tapi_env.h"
#include "tapi_cfg_phy.h"

int
main(int argc, char *argv[])
{
    const char    *ta = NULL;
    const char    *iface_name = getenv("TE_AGT1_TA_IF");
    int      link_state = -1;
    int      autoneg = -1;
    int      duplex = -1;
    int      tmp_duplex = -1;
    int      speed = -1;
    int      tmp_speed = -1;
    int      speed_adver = -1;
    int      duplex_adver = -1;
    te_bool  mode = FALSE;
    te_bool  tmp_mode = FALSE;

    TEST_START;

    /* Get test parameters */
    TEST_GET_STRING_PARAM(ta);
    TEST_GET_INT_PARAM(speed);
    TEST_GET_INT_PARAM(duplex);
    TEST_GET_INT_PARAM(speed_adver);
    TEST_GET_INT_PARAM(duplex_adver);

    CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, "/:"));

    if (iface_name == NULL)
        TEST_FAIL("Interface name for agent '%s' is required", ta);

    /* Check link state */
    CHECK_RC(tapi_cfg_phy_state_get(ta, iface_name, &link_state));
    if (link_state != TE_PHY_STATE_UP)
        TEST_FAIL("link down at TA `%s' for interface `%s'", ta,
                  iface_name);

    /* Get autonegotiation state */
    CHECK_RC(tapi_cfg_phy_autoneg_oper_get(ta, iface_name, &autoneg));
    RING("Autonegatiation state: %d", autoneg);

    autoneg = TE_PHY_AUTONEG_OFF;
    CHECK_RC(tapi_cfg_phy_autoneg_admin_set(ta, iface_name, autoneg));

    CHECK_RC(tapi_cfg_phy_speed_admin_set(ta, iface_name, speed));

    CHECK_RC(tapi_cfg_phy_duplex_admin_set(ta, iface_name, duplex));

    CHECK_RC(tapi_cfg_phy_commit(ta, iface_name));

    CFG_WAIT_CHANGES;

    /*
     * Check the result
     */

    /* Check that PHY duplex state has been set correctly */
    CHECK_RC(tapi_cfg_phy_duplex_oper_get(ta, iface_name, &tmp_duplex));
    if (tmp_duplex != duplex)
        TEST_FAIL("failed to set duplex to value: %d", duplex);

    /* Check that PHY speed has been set correctly */
    CHECK_RC(tapi_cfg_phy_speed_oper_get(ta, iface_name, &tmp_speed));
    if (tmp_speed != speed)
        TEST_FAIL("failed to set speed to value: %d", speed);

    /* Reset value */
    autoneg = -1;

    /* Check that PHY autonegotiation is at state OFF */
    CHECK_RC(tapi_cfg_phy_autoneg_oper_get(ta, iface_name, &autoneg));
    if (autoneg != TE_PHY_AUTONEG_OFF)
        TEST_FAIL("failed to set autonegotiation to state OFF");


    /*
     * Autonegotiation
     */

    /* Turn PHY autonegotiation to state ON */
    autoneg = TE_PHY_AUTONEG_ON;
    CHECK_RC(tapi_cfg_phy_autoneg_admin_set(ta, iface_name, autoneg));
    CHECK_RC(tapi_cfg_phy_commit(ta, iface_name));
    CFG_WAIT_CHANGES;

    /*
     * Advertising
     */

    /* Check that mode is advertised */
    CHECK_RC(tapi_cfg_phy_is_mode_advertised(ta, iface_name,
                                             speed_adver, duplex_adver,
                                             &mode));
    if (!mode)
        TEST_FAIL("mode is not advertised on %s at %s", ta, iface_name);

    /* Turn off advertising for this mode */
    CHECK_RC(tapi_cfg_phy_advertise_mode(ta, iface_name,
                                         speed_adver, duplex_adver,
                                         0));

    CHECK_RC(tapi_cfg_phy_commit(ta, iface_name));
    CFG_WAIT_CHANGES;

    /* Check that mode is not advertised */
    CHECK_RC(tapi_cfg_phy_is_mode_advertised(ta, iface_name,
                                             speed_adver, duplex_adver,
                                             &tmp_mode));
    if (tmp_mode)
        TEST_FAIL("failed to turn off mode advertising");

    /* Turn on advertising for this mode */
    CHECK_RC(tapi_cfg_phy_advertise_mode(ta, iface_name,
                                         speed_adver, duplex_adver, 1));

    CHECK_RC(tapi_cfg_phy_commit(ta, iface_name));
    CFG_WAIT_CHANGES;

    /* Check that mode is advertised */
    CHECK_RC(tapi_cfg_phy_is_mode_advertised(ta, iface_name,
                                             speed_adver, duplex_adver,
                                             &tmp_mode));
    if (!tmp_mode)
        TEST_FAIL("failed to advertise mode");

    /*
     * Restart PHY autonegotiation
     */

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
