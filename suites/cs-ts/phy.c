/** @file
 * @brief Test Environment
 *
 * Check PHY support in Configurator
 * 
 * Copyright (C) 2007 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * $Id: phy.c 34433 2006-12-23 14:00:41Z arybchik $
 */

/** @page cs-ts-phy Check configurator PHY management support
 *
 * @objective Check configurator PHY management support
 *
 * @param ta            Test agent name
 * @param iface_name    Interface name
 * @param second_ta     The name of a test agent connected
 *                      with @p ta by physical link
 * @param second_iface  @p second_ta interface name
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
 * @author Vadim V. Galitsyn <Vadim.Galitsyn@oktetlabs.ru>
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
    char    *ta = NULL;
    char    *iface_name = NULL;
    char    *second_ta = NULL;
    char    *second_iface = NULL;
    int      link_state = -1;
    int      autoneg = -1;
    int      duplex = -1;
    int      tmp_duplex = -1;
    int      speed = -1;
    int      tmp_speed = -1;
    int      speed_adver = -1;
    int      duplex_adver = -1;
    int      tmp_speed_adver = -1;
    int      tmp_duplex_adver = -1;
    te_bool  mode = FALSE;
    te_bool  tmp_mode = FALSE;
    
    TEST_START; 
    
    /* Get test parameters */
    TEST_GET_STRING_PARAM(ta);
    TEST_GET_STRING_PARAM(iface_name);
    TEST_GET_STRING_PARAM(second_ta);
    TEST_GET_STRING_PARAM(second_iface);
    TEST_GET_INT_PARAM(speed);
    TEST_GET_INT_PARAM(duplex);
    TEST_GET_INT_PARAM(speed_adver);
    TEST_GET_INT_PARAM(duplex_adver);
    
    CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, "/:"));
    
    /* Check link state */
    CHECK_RC(tapi_cfg_phy_state_get(ta, iface_name, &link_state));
    if (link_state != TE_PHY_STATE_UP)
        TEST_FAIL("link down at TA `%s' for interface `%s'", ta,
                  iface_name);
    
    /* Get autonegotiation state */
    CHECK_RC(tapi_cfg_phy_autoneg_get(ta, iface_name, &autoneg));
    RING("Autonegatiation state: %d", autoneg);
    
    autoneg = TE_PHY_AUTONEG_OFF;
    CHECK_RC(tapi_cfg_phy_autoneg_set(ta, iface_name, autoneg));
    
    CHECK_RC(tapi_cfg_phy_speed_set(ta, iface_name, speed));
    
    CHECK_RC(tapi_cfg_phy_duplex_set(ta, iface_name, duplex));
    
    CHECK_RC(tapi_cfg_phy_commit(ta, iface_name));
    
    /*
     * Check the result
     */
    
    /* Check that PHY duplex state has been set correctly */
    CHECK_RC(tapi_cfg_phy_duplex_get(ta, iface_name, &tmp_duplex));
    if (tmp_duplex != duplex)
        TEST_FAIL("failed to set duplex to value: %d", duplex);

    /* Check that PHY speed has been set correctly */
    CHECK_RC(tapi_cfg_phy_speed_get(ta, iface_name, &tmp_speed));
    if (tmp_speed != speed)
        TEST_FAIL("failed to set speed to value: %d", speed);

    /* Reset value */
    autoneg = -1;
    
    /* Check that PHY autonegotiation is at state OFF */
    CHECK_RC(tapi_cfg_phy_autoneg_get(ta, iface_name, &autoneg));
    if (autoneg != TE_PHY_AUTONEG_OFF)
        TEST_FAIL("failed to set autonegotiation to state OFF");

    
    /*
     * Autonegotiation
     */
    
    /* Turn PHY autonegotiation to state ON */
    autoneg = TE_PHY_AUTONEG_ON;
    CHECK_RC(tapi_cfg_phy_autoneg_set(ta, iface_name, autoneg));
    
    /*
     * Advertising
     */
    
    /* Check that mode is advertised */
    CHECK_RC(tapi_cfg_phy_is_mode_advertised(ta, iface_name,
                                             speed_adver, duplex_adver,
                                             &mode));
    if (!mode)
        TEST_FAIL("mode is not advertised");
    
    /* Turn off advertising for this mode */
    CHECK_RC(tapi_cfg_phy_advertise_mode(ta, iface_name,
                                         speed_adver, duplex_adver,
                                         0));
    
    CHECK_RC(tapi_cfg_phy_commit(ta, iface_name));
    
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
