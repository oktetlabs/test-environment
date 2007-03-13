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
 * $Id$
 */

#ifndef __TE_TAPI_CFG_PHY_H__
#define __TE_TAPI_CFG_PHY_H__

#include "te_errno.h"
#include "logger_api.h"
#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PHY link states */
#define TE_PHY_STATE_DOWN   (0)     /* Link down */
#define TE_PHY_STATE_UP     (1)     /* Link up */

/* PHY autonegotiation states */
#define TE_PHY_AUTONEG_OFF  (0)     /* Autonegotiation off */
#define TE_PHY_AUTONEG_ON   (1)     /* Autonegotiation on */

/* PHY duplex states */
#define TE_PHY_DUPLEX_HALF  (0)     /* Half duplex */
#define TE_PHY_DUPLEX_FULL  (1)     /* Full duplex */

/* PHY speed values */
#define TE_PHY_SPEED_10     (10)    /* 10 Mb/sec */
#define TE_PHY_SPEED_100    (100)   /* 100 Mb/sec */
#define TE_PHY_SPEED_1000   (1000)  /* 1000 Mb/sec */
#define TE_PHY_SPEED_10000  (10000) /* 10000 Mb/sec */

/**
 * Get PHY autonegotiation state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned autonegotiation state value:
 *                      TE_PHY_AUTONEG_OFF - autonegatiation OFF
 *                      TE_PHY_AUTONEG_ON  - autonegatiation ON
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
 *                      TE_PHY_AUTONEG_OFF - autonegatiation OFF
 *                      TE_PHY_AUTONEG_ON  - autonegatiation ON
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
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
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
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
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
 *                      TE_PHY_SPEED_10    - 10 Mbit/sec
 *                      TE_PHY_SPEED_100   - 100 Mbit/sec
 *                      TE_PHY_SPEED_1000  - 1000 Mbit/sec
 *                      TE_PHY_SPEED_10000 - 10000 Mbit/sec
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
 *                      TE_PHY_SPEED_10    - 10 Mbit/sec
 *                      TE_PHY_SPEED_100   - 100 Mbit/sec
 *                      TE_PHY_SPEED_1000  - 1000 Mbit/sec
 *                      TE_PHY_SPEED_10000 - 10000 Mbit/sec
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
 *                      TE_PHY_STATE_DOWN - link down
 *                      TE_PHY_STATE_UP   - link up
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
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
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
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
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
 * This function should be called after any __set__
 * operation (when autonegotiation is ON) to be sure that changes has
 * been applied at both hosts connected by physical link.
 * For example, if interface speed value
 * has been changed at first host, network adapher at the second host
 * can automatically change itself speed value. This function call
 * will reflect such changes at the configuration tree.
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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAPI_CFG_PHY_H__ */
