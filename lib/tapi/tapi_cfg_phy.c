/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for PHY configuration model
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
 * $Id: tapi_cfg_phy.c 36776 2007-02-27 14:36:00Z galitsyn $
 */

#define TE_LGR_USER     "Configuration PHY"

#include "tapi_cfg_phy.h"

#if CONFIGURATOR_PHY_SUPPORT

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
te_errno
tapi_cfg_base_phy_autoneg_get(const char *ta, const char *if_name,
                              int *state)
{
    int rc = 0;
    
    rc =  cfg_get_instance_sync_fmt(CFG_VAL(INTEGER, state),
                                    "/agent:%s/interface:%s/phy:/autoneg:",
                                    ta, if_name);
    
    /* Check that option is supported */
    if (*state == -1)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);
    
    return rc;
}

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
te_errno
tapi_cfg_base_phy_autoneg_set(const char *ta, const char *if_name,
                              int state)
{
    te_errno rc = 0;
    
    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, state),
                              "/agent:%s/interface:%s/phy:/autoneg:",
                              ta, if_name);
    
    return rc;
}

#define PHY_DUPLEX_STRING_HALF "half"
#define PHY_DUPLEX_STRING_FULL "full"

/**
 * Get PHY duplex state by name string.
 *
 * @param name          Duplex state name string
 *
 * @return PHY_DUPLEX_HALF - half duplex;
 *         PHY_DUPLEX_FULL - full duplex;
 *         or -1 if name string does not recognized
 */
static inline int
tapi_cfg_base_phy_get_duplex_by_name(char *name)
{
#if HAVE_LINUX_ETHTOOL_H
    if (strcmp(name, PHY_DUPLEX_STRING_HALF) == 0)
        return PHY_DUPLEX_HALF;
    else if (strcmp(name, PHY_DUPLEX_STRING_FULL) == 0)
        return PHY_DUPLEX_FULL;
#endif
    return -1;
}

/**
 * Get PHY duplex state by id.
 *
 * @param duplex        Duplex state id
 *
 * @return half - half duplex;
 *         full - full duplex;
 *         or NULL if id does not recognized
 */
static inline char *
tapi_cfg_base_phy_get_duplex_by_id(int duplex)
{
#if HAVE_LINUX_ETHTOOL_H
    switch (duplex)
    {
        case PHY_DUPLEX_HALF: return PHY_DUPLEX_STRING_HALF;
        case PHY_DUPLEX_FULL: return PHY_DUPLEX_STRING_FULL;
    }
#endif
    return NULL;
}

#undef PHY_DUPLEX_STRING_HALF
#undef PHY_DUPLEX_STRING_FULL

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
te_errno
tapi_cfg_base_phy_duplex_get(const char *ta, const char *if_name,
                             int *state)
{
    te_errno  rc = 0;
    char     *duplex;
    
    rc = cfg_get_instance_sync_fmt(NULL,
                                   (void *)&duplex,
                                   "/agent:%s/interface:%s/phy:/duplex:",
                                   ta, if_name);
    if (rc != 0)
        return rc;
    
    *state = tapi_cfg_base_phy_get_duplex_by_name(duplex);
    
    free(duplex);
    
    return 0;
}

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
te_errno
tapi_cfg_base_phy_duplex_set(const char *ta, const char *if_name, int state)
{
    te_errno  rc = 0;
    char     *duplex;
    
    duplex = tapi_cfg_base_phy_get_duplex_by_id(state);
    
    if (duplex == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    
    rc = cfg_set_instance_fmt(CFG_VAL(STRING, duplex),
                                      "/agent:%s/interface:%s/phy:/duplex:",
                                      ta, if_name);
    
    return rc;
}

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
te_errno
tapi_cfg_base_phy_speed_get(const char *ta, const char *if_name, int *speed)
{
    int rc = 0;
    
    rc = cfg_get_instance_sync_fmt(CFG_VAL(INTEGER, speed),
                                   "/agent:%s/interface:%s/phy:/speed:",
                                   ta, if_name);
    
    /* Check that option is supported */
    if (*speed == -1)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);
    
    return rc;
}

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
te_errno
tapi_cfg_base_phy_speed_set(const char *ta, const char *if_name, int speed)
{
    te_errno rc = 0;
    
    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, speed),
                              "/agent:%s/interface:%s/phy:/speed:",
                              ta, if_name);
    
    return rc;

}

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
te_errno
tapi_cfg_base_phy_state_get(const char *ta, const char *if_name, int *state)
{
    int rc = 0;
    
    rc = cfg_get_instance_sync_fmt(CFG_VAL(INTEGER, state),
                                   "/agent:%s/interface:%s/phy:/state:",
                                   ta, if_name);
    
    if (rc != 0)
        return rc;
    
    /* Check that option is supported */
    if (*state == -1)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);
    
    return rc;
}

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
 *                      TRUE - mode is advertised
 *                      FALSE - mode is not advertised
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_is_mode_advertised(const char *ta, const char *if_name,
                                     int speed, int duplex,
                                     te_bool *state)
{
    te_errno  rc = 0;
    int       advertised = -1;
    char     *duplex_string;
    
    duplex_string = tapi_cfg_base_phy_get_duplex_by_id(duplex);
    
    if (duplex_string == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    
    /* Get mode state */
    rc = cfg_get_instance_sync_fmt(CFG_VAL(INTEGER, &advertised),
                                   "/agent:%s/interface:%s/phy:"
                                   "/modes:/speed:%d/duplex:%s",
                                   ta, if_name, speed,
                                   duplex_string);
    
    /* Store state */
    if (advertised == 1)
        *state = TRUE;
    else
        *state = FALSE;
    
    return rc;
}

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
te_errno
tapi_cfg_base_phy_advertise_mode(const char *ta, const char *if_name,
                                 int speed, int duplex, int state)
{
    te_errno  rc = 0;
    char     *duplex_string;
    
    duplex_string = tapi_cfg_base_phy_get_duplex_by_id(duplex);
    
    if (duplex_string == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    
    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, state),
                              "/agent:%s/interface:%s/phy:"
                              "/modes:/speed:%d/duplex:%s",
                              ta, if_name, speed,
                              duplex_string);
    
    return rc;
}

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
te_errno
tapi_cfg_base_phy_reset(const char *ta, const char *if_name,
                        int unused)
{
    te_errno rc = 0;
    
    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, unused),
                              "/agent:%s/interface:%s/phy:/reset:",
                              ta, if_name);
    
    return rc;
}

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
                       const char *second_ta, const char *second_ifname)
{
    te_errno rc = 0;
    
    /* Reset first link */
    rc = tapi_cfg_base_phy_reset(first_ta, first_ifname, 0);
    if (rc != 0)
    {
        ERROR("failed to reset first link");
        return rc;
    }
    
    /* Reset second link */
    rc = tapi_cfg_base_phy_reset(second_ta, second_ifname, 0);
    if (rc != 0)
    {
        ERROR("failed to reset second link");
        return rc;
    }
    
    /* Wait changes */
    rc = cfg_wait_changes();
    if (rc != 0)
    {
        ERROR("wait changes fails");
        return rc;
    }
    
    /* Synchronize first link leaf */
    rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s/phy:",
                             first_ta, first_ifname);
    if (rc != 0)
    {
        ERROR("failed to sync first link leaf");
        return rc;
    }
    
    /* Synchronize second link leaf */
    rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s/phy:",
                             second_ta, second_ifname);
    if (rc != 0)
    {
        ERROR("failed to sync second link leaf");
        return rc;
    }
    
    return rc;
}

#endif /* CONFIGURATOR_PHY_SUPPORT */
