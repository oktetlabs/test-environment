/** @file
 * @brief Network Configuration Model TAPI
 *
 * Definition of test API for PHY configuration model
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Vadim V. Galitsyn <Vadim.Galitsyn@oktetlabs.ru>
 *
 * $Id: tapi_cfg_phy.h 36739 2007-02-26 09:19:57Z galitsyn $
 */

#ifndef __TE_TAPI_CFG_PHY_H__
#define __TE_TAPI_CFG_PHY_H__

#include "te_errno.h"
#include "logger_api.h"
#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIGURATOR_PHY_SUPPORT

/* PHY link states */
#define PHY_STATE_DOWN   (0)    /* Link down */
#define PHY_STATE_UP     (1)    /* Link up */

/* PHY autonegotiation states */
#define PHY_AUTONEG_OFF  (0)
#define PHY_AUTONEG_ON   (1)

/* PHY duplex states */
#define PHY_DUPLEX_HALF  (0)
#define PHY_DUPLEX_FULL  (1)

/* PHY speed values */
#define PHY_SPEED_10     (10)
#define PHY_SPEED_100    (100)
#define PHY_SPEED_1000   (1000)
#define PHY_SPEED_10000  (10000)

/**
 * Get PHY autonegotiation state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned autonegotiation state value:
 *                      PHY_AUTONEG_OFF - autonegatiation OFF
 *                      PHY_AUTONEG_ON  - autonegatiation ON
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_autoneg_get(const char *ta, const char *if_name,
                              int *state);

/**
 * Set PHY autonegotiation state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Autonegotiation state value:
 *                      PHY_AUTONEG_OFF - autonegatiation OFF
 *                      PHY_AUTONEG_ON  - autonegatiation ON
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_autoneg_set(const char *ta, const char *if_name,
                              int state);

/**
 * Get PHY duplex state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_duplex_get(const char *ta, const char *if_name,
                             int *state);

/**
 * Set PHY duplex state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_duplex_set(const char *ta, const char *if_name,
                             int state);

/**
 * Get PHY speed value.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Pointer to returned speed value
 *                      PHY_SPEED_10    - 10 Mbit/sec
 *                      PHY_SPEED_100   - 100 Mbit/sec
 *                      PHY_SPEED_1000  - 1000 Mbit/sec
 *                      PHY_SPEED_10000 - 10000 Mbit/sec
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_speed_get(const char *ta, const char *if_name,
                            int *speed);

/**
 * Set PHY speed.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value
 *                      PHY_SPEED_10    - 10 Mbit/sec
 *                      PHY_SPEED_100   - 100 Mbit/sec
 *                      PHY_SPEED_1000  - 1000 Mbit/sec
 *                      PHY_SPEED_10000 - 10000 Mbit/sec
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_speed_set(const char *ta, const char *if_name,
                            int speed);

/**
 * Get PHY link state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned link state value:
 *                      PHY_STATE_DOWN - link down
 *                      PHY_STATE_UP   - link up
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_state_get(const char *ta, const char *if_name,
                            int *state);

/**
 * Check that PHY mode is advertised.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (see linux/ethtool.h for more details)
 * @param duplex        Duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 * @param state         Pointer to mode state:
 *                      1 - mode is advertised
 *                      0 - mode is not advertised
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_is_mode_advertised(const char *ta, const char *if_name,
                                     int speed, int duplex,
                                     te_bool *state);

/**
 * Set PHY mode to advertising state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (see linux/ethtool.h for more details)
 * @param duplex        Duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 * @param state         Mode state:
 *                      0 - mode is not advertised
 *                      1 - mode is advertised 
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_advertise_mode(const char *ta, const char *if_name,
                                 int speed, int duplex, int state);

/**
 * Reset link.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param unused        Unused parameter;
 *                      should be set to any value (just formality)
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_reset(const char *ta, const char *if_name,
                        int unused);

/**
 * Synchonize PHY interface changes between remote hosts.
 *
 * @param first_ta      First Test Agent name
 * @param first_ifname  Interface name of first TA
 * @param second_ta     Second Test Agent name
 * @param second_ifname Interface name of second TA
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_sync(const char *first_ta, const char *first_ifname,
                       const char *second_ta, const char *second_ifname);

#endif /* CONFIGURATOR_PHY_SUPPORT */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAPI_CFG_PHY_H__ */
