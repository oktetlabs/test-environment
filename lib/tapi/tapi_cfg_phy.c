/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for PHY configuration model
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2007 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "Configuration PHY"

#include "tapi_cfg_phy.h"
#include "te_ethernet_phy.h"

/**
 * Get PHY autonegotiation state.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned autonegotiation state value:
 *                      TE_PHY_AUTONEG_OFF - autonegotiation OFF
 *                      TE_PHY_AUTONEG_ON  - autonegotiation ON
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_autoneg_get(const char *ta, const char *if_name,
                         int *state)
{
    te_errno     rc = 0;
    cfg_val_type type = CVT_INTEGER;
    
    rc = cfg_get_instance_sync_fmt(&type, state,
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
 *                      TE_PHY_AUTONEG_OFF - autonegotiation OFF
 *                      TE_PHY_AUTONEG_ON  - autonegotiation ON
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_autoneg_set(const char *ta, const char *if_name,
                         int state)
{
    te_errno rc = 0;
    
    rc = cfg_set_instance_local_fmt(CFG_VAL(INTEGER, state),
                                    "/agent:%s/interface:%s/phy:/autoneg:",
                                    ta, if_name);
    
    return rc;
}

/**
 * Get PHY duplex state by name string.
 *
 * @param name          Duplex state name string
 *
 * @return TE_PHY_DUPLEX_HALF - half duplex;
 *         TE_PHY_DUPLEX_FULL - full duplex;
 *         or -1 if name string does not recognized
 */
extern int
tapi_cfg_phy_duplex_str2id(char *name)
{
    if (name == NULL)
        return -1;
    
    if (strcmp(name, TE_PHY_DUPLEX_STRING_HALF) == 0)
        return TE_PHY_DUPLEX_HALF;
    else if (strcmp(name, TE_PHY_DUPLEX_STRING_FULL) == 0)
        return TE_PHY_DUPLEX_FULL;
    
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
extern char *
tapi_cfg_phy_duplex_id2str(int duplex)
{
    switch (duplex)
    {
        case TE_PHY_DUPLEX_HALF: return TE_PHY_DUPLEX_STRING_HALF;
        case TE_PHY_DUPLEX_FULL: return TE_PHY_DUPLEX_STRING_FULL;
    }
    
    return NULL;
}

/**
 * Get PHY duplex state.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned duplex state value:
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_duplex_get(const char *ta, const char *if_name,
                        int *state)
{
    te_errno  rc = 0;
    char     *duplex;
    
    rc = cfg_get_instance_sync_fmt(NULL, (void *)&duplex,
                                   "/agent:%s/interface:%s/phy:/duplex:",
                                   ta, if_name);
    if (rc != 0)
        return rc;
    
    *state = tapi_cfg_phy_duplex_str2id(duplex);
    
    free(duplex);
    
    if (*state == -1)
        return TE_RC(TE_TAPI, TE_EINVAL);
    
    return 0;
}

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
te_errno
tapi_cfg_phy_duplex_set(const char *ta, const char *if_name, int state)
{
    te_errno  rc = 0;
    char     *duplex;
    
    duplex = tapi_cfg_phy_duplex_id2str(state);
    
    if (duplex == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    
    rc = cfg_set_instance_local_fmt(CFG_VAL(STRING, duplex),
                                    "/agent:%s/interface:%s/phy:/duplex:",
                                    ta, if_name);
    
    return rc;
}

/**
 * Get PHY speed value.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Pointer to the returned speed value
 *                      TE_PHY_SPEED_10    - 10 Mbit/sec
 *                      TE_PHY_SPEED_100   - 100 Mbit/sec
 *                      TE_PHY_SPEED_1000  - 1000 Mbit/sec
 *                      TE_PHY_SPEED_10000 - 10000 Mbit/sec
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_speed_get(const char *ta, const char *if_name, int *speed)
{
    te_errno     rc = 0;
    cfg_val_type type = CVT_INTEGER;
    
    rc = cfg_get_instance_sync_fmt(&type, speed,
                                   "/agent:%s/interface:%s/phy:/speed:",
                                   ta, if_name);
    if (rc != 0)
        return rc;
    
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
 *                      TE_PHY_SPEED_10    - 10 Mbit/sec
 *                      TE_PHY_SPEED_100   - 100 Mbit/sec
 *                      TE_PHY_SPEED_1000  - 1000 Mbit/sec
 *                      TE_PHY_SPEED_10000 - 10000 Mbit/sec
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_speed_set(const char *ta, const char *if_name, int speed)
{
    te_errno rc = 0;
    
    rc = cfg_set_instance_local_fmt(CFG_VAL(INTEGER, speed),
                                    "/agent:%s/interface:%s/phy:/speed:",
                                     ta, if_name);
    
    return rc;

}

/**
 * Get PHY interface mode: speed and duplex state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value
 *                      TE_PHY_SPEED_10    - 10 Mbit/sec
 *                      TE_PHY_SPEED_100   - 100 Mbit/sec
 *                      TE_PHY_SPEED_1000  - 1000 Mbit/sec
 *                      TE_PHY_SPEED_10000 - 10000 Mbit/sec
 * @param duplex        Duplex state value
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_phy_mode_get(const char *ta, const char *if_name,
                      int *speed, int *duplex)
{
    te_errno rc;
    
    rc = tapi_cfg_phy_speed_get(ta, if_name, speed);
    if (rc != 0)
    {
        ERROR("failed to get interface speed value on %s at %s",
              ta, if_name);
        return rc;
    }
    
    rc = tapi_cfg_phy_duplex_get(ta, if_name, duplex);
    if (rc != 0)
    {
        ERROR("failed to get interface duplex state on %s at %s",
              ta, if_name);
        return rc;
    }
    
    return 0;
}

/**
 * Set PHY interface mode: speed and duplex state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value
 *                      TE_PHY_SPEED_10    - 10 Mbit/sec
 *                      TE_PHY_SPEED_100   - 100 Mbit/sec
 *                      TE_PHY_SPEED_1000  - 1000 Mbit/sec
 *                      TE_PHY_SPEED_10000 - 10000 Mbit/sec
 * @param duplex        Duplex state value
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_phy_mode_set(const char *ta, const char *if_name,
                      int speed, int duplex)
{
    te_errno rc;
    
    rc = tapi_cfg_phy_speed_set(ta, if_name, speed);
    if (rc != 0)
    {
        ERROR("failed to set interface speed value on %s at %s",
              ta, if_name);
        return rc;
    }
    
    rc = tapi_cfg_phy_duplex_set(ta, if_name, duplex);
    if (rc != 0)
    {
        ERROR("failed to set interface duplex state on %s at %s",
              ta, if_name);
        return rc;
    }
    
    return 0;
}

/**
 * Get PHY link state.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned link state value:
 *                      TE_PHY_STATE_DOWN - link down
 *                      TE_PHY_STATE_UP   - link up
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_state_get(const char *ta, const char *if_name, int *state)
{
    te_errno     rc = 0;
    cfg_val_type type = CVT_INTEGER;
    
    rc = cfg_get_instance_sync_fmt(&type, state,
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
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 * @param state         Pointer to mode state:
 *                      TRUE - the mode is advertised
 *                      FALSE - the mode is not advertised
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_is_mode_advertised(const char *ta, const char *if_name,
                                int speed, int duplex,
                                te_bool *state)
{
    te_errno      rc = 0;
    int           advertised = -1;
    char         *duplex_string;
    cfg_val_type  type = CVT_INTEGER;
    
    duplex_string = tapi_cfg_phy_duplex_id2str(duplex);
    
    if (duplex_string == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    
    /* Get mode state */
    rc = cfg_get_instance_sync_fmt(&type, &advertised,
                                   "/agent:%s/interface:%s/phy:"
                                   "/modes:/speed:%d/duplex:%s",
                                   ta, if_name, speed,
                                   duplex_string);
    
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
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 * @param state         Mode state:
 *                      0 - the mode is not advertised
 *                      1 - the mode is advertised
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_advertise_mode(const char *ta, const char *if_name,
                            int speed, int duplex, te_bool state)
{
    te_errno  rc = 0;
    char     *duplex_string;
    
    duplex_string = tapi_cfg_phy_duplex_id2str(duplex);
    
    if (duplex_string == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    
    rc = cfg_set_instance_local_fmt(CFG_VAL(INTEGER, (int)state),
                                    "/agent:%s/interface:%s/phy:"
                                    "/modes:/speed:%d/duplex:%s",
                                    ta, if_name, speed,
                                    duplex_string);
    
    return rc;
}

/**
 * Commit PHY interface changes to TAs.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_phy_commit(const char *ta, const char *if_name)
{
    return cfg_commit_fmt("/agent:%s/interface:%s/phy:", ta, if_name);
}
