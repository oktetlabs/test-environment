/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for PHY configuration model
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Configuration PHY"

#include "te_defs.h"
#include "tapi_cfg_phy.h"
#include "te_ethernet_phy.h"
#include "te_time.h"
#include "te_sleep.h"

/**
 * Get PHY autonegotiation oper state.
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
tapi_cfg_phy_autoneg_oper_get(const char *ta, const char *if_name,
                              int *state)
{
    te_errno rc = 0;

    rc = cfg_get_instance_int_sync_fmt(state,
                                   "/agent:%s/interface:%s/phy:/autoneg_oper:",
                                   ta, if_name);

    /* Check that option is supported */
    if (*state == -1)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return rc;
}

/**
 * Get PHY autonegotiation admin state.
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
tapi_cfg_phy_autoneg_admin_get(const char *ta, const char *if_name,
                         int *state)
{
    te_errno rc = 0;

    rc = cfg_get_instance_int_sync_fmt(state,
                                   "/agent:%s/interface:%s/phy:/autoneg_admin:",
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
tapi_cfg_phy_autoneg_admin_set(const char *ta, const char *if_name,
                               int state)
{
    te_errno rc = 0;

    rc = cfg_set_instance_local_fmt(CFG_VAL(INTEGER, state),
                            "/agent:%s/interface:%s/phy:/autoneg_admin:",
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
 *         TE_PHY_DUPLEX_UNKNOWN - unknown duplex;
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
    else if (strcmp(name, TE_PHY_DUPLEX_STRING_UNKNOWN) == 0)
        return TE_PHY_DUPLEX_UNKNOWN;

    return -1;
}

/**
 * Get PHY duplex state by id.
 *
 * @param duplex        Duplex state id
 *
 * @return half - half duplex;
 *         full - full duplex;
 *         unknown - unknown duplex;
 *         or NULL if id does not recognized
 */
extern char *
tapi_cfg_phy_duplex_id2str(int duplex)
{
    switch (duplex)
    {
        case TE_PHY_DUPLEX_HALF: return TE_PHY_DUPLEX_STRING_HALF;
        case TE_PHY_DUPLEX_FULL: return TE_PHY_DUPLEX_STRING_FULL;
        case TE_PHY_DUPLEX_UNKNOWN: return TE_PHY_DUPLEX_STRING_UNKNOWN;
    }

    return NULL;
}

/**
 * Get PHY duplex oper state.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned duplex state value:
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *                      TE_PHY_DUPLEX_UNKNOWN - unknown duplex state
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_duplex_oper_get(const char *ta, const char *if_name,
                             int *state)
{
    te_errno  rc = 0;
    char     *duplex;

    rc = cfg_get_instance_sync_fmt(NULL, (void *)&duplex,
                            "/agent:%s/interface:%s/phy:/duplex_oper:",
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
 * Get PHY duplex admin state.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned duplex state value:
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *                      TE_PHY_DUPLEX_UNKNOWN - unknown duplex state
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_duplex_admin_get(const char *ta, const char *if_name,
                        int *state)
{
    te_errno  rc = 0;
    char     *duplex;

    rc = cfg_get_instance_sync_fmt(NULL, (void *)&duplex,
                            "/agent:%s/interface:%s/phy:/duplex_admin:",
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
tapi_cfg_phy_duplex_admin_set(const char *ta, const char *if_name,
                              int state)
{
    te_errno  rc = 0;
    char     *duplex;

    duplex = tapi_cfg_phy_duplex_id2str(state);

    if (duplex == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_set_instance_local_fmt(CFG_VAL(STRING, duplex),
                            "/agent:%s/interface:%s/phy:/duplex_admin:",
                            ta, if_name);

    return rc;
}

/**
 * Get PHY speed oper value.
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
tapi_cfg_phy_speed_oper_get(const char *ta, const char *if_name, int *speed)
{
    te_errno rc = 0;

    rc = cfg_get_instance_int_sync_fmt(speed,
                                   "/agent:%s/interface:%s/phy:/speed_oper:",
                                   ta, if_name);
    if (rc != 0)
        return rc;

    /* Check that option is supported */
    if (*speed == -1)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return rc;
}

/**
 * Get PHY speed admin value.
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
tapi_cfg_phy_speed_admin_get(const char *ta, const char *if_name,
                             int *speed)
{
    te_errno rc = 0;

    rc = cfg_get_instance_int_sync_fmt(speed,
                                   "/agent:%s/interface:%s/phy:/speed_admin:",
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
tapi_cfg_phy_speed_admin_set(const char *ta, const char *if_name, int speed)
{
    te_errno rc = 0;

    rc = cfg_set_instance_local_fmt(CFG_VAL(INTEGER, speed),
                            "/agent:%s/interface:%s/phy:/speed_admin:",
                            ta, if_name);

    return rc;

}

/**
 * Get PHY interface mode: speed and duplex oper state.
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
 *                      TE_PHY_DUPLEX_UNKNOWN - unknown duplex
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_phy_mode_oper_get(const char *ta, const char *if_name,
                           int *speed, int *duplex)
{
    te_errno rc;

    rc = tapi_cfg_phy_speed_oper_get(ta, if_name, speed);
    if (rc != 0)
    {
        ERROR("failed to get interface speed value on %s at %s",
              ta, if_name);
        return rc;
    }

    rc = tapi_cfg_phy_duplex_oper_get(ta, if_name, duplex);
    if (rc != 0)
    {
        ERROR("failed to get interface duplex state on %s at %s",
              ta, if_name);
        return rc;
    }

    return 0;
}

/**
 * Get PHY interface mode: speed and duplex admin state.
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
 *                      TE_PHY_DUPLEX_UNKNOWN - unknown duplex
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_phy_mode_admin_get(const char *ta, const char *if_name,
                            int *speed, int *duplex)
{
    te_errno rc;

    rc = tapi_cfg_phy_speed_admin_get(ta, if_name, speed);
    if (rc != 0)
    {
        ERROR("failed to get interface speed value on %s at %s",
              ta, if_name);
        return rc;
    }

    rc = tapi_cfg_phy_duplex_admin_get(ta, if_name, duplex);
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
tapi_cfg_phy_mode_admin_set(const char *ta, const char *if_name,
                            int speed, int duplex)
{
    te_errno rc;

    rc = tapi_cfg_phy_speed_admin_set(ta, if_name, speed);
    if (rc != 0)
    {
        ERROR("failed to set interface speed value on %s at %s",
              ta, if_name);
        return rc;
    }

    rc = tapi_cfg_phy_duplex_admin_set(ta, if_name, duplex);
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
    te_errno rc = 0;

    rc = cfg_get_instance_int_sync_fmt(state,
                                       "/agent:%s/interface:%s/phy:/state:",
                                       ta, if_name);

    if (rc != 0)
        return rc;

    /* Check that option is supported */
    if (*state == -1)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return rc;
}

/* See tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_state_wait_up(const char *ta,
                           const char *if_name,
                           int timeout)
{
    struct timeval tv_start;
    struct timeval tv_cur;
    te_errno rc;
    int state;

    rc = te_gettimeofday(&tv_start, NULL);
    if (rc != 0)
        return rc;

    while (TRUE)
    {
        rc = tapi_cfg_phy_state_get(ta, if_name, &state);
        if (rc != 0)
            return rc;
        if (state == TE_PHY_STATE_UP)
            return 0;

        rc = te_gettimeofday(&tv_cur, NULL);
        if (rc != 0)
            return rc;
        if (TIMEVAL_SUB(tv_cur, tv_start) >= TE_MS2US(timeout))
            return TE_RC(TE_TAPI, TE_ETIMEDOUT);

        (void)usleep(500000);
    }
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
    te_errno  rc = 0;
    int       advertised = -1;
    char     *duplex_string;

    duplex_string = tapi_cfg_phy_duplex_id2str(duplex);

    if (duplex_string == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    /* Get mode state */
    rc = cfg_get_instance_int_sync_fmt(&advertised,
                                       "/agent:%s/interface:%s/phy:"
                                       "/modes:/speed:%d/duplex:%s",
                                       ta, if_name, speed, duplex_string);

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

/**
 * Turn off all advertised modes and advertise only one.
 *
 * @param ta            Test agent
 * @param ifname        Interface name
 * @param advert_speed  Speed to advertise
 * @param advert_duplex Duplex to advertise
 *
 * @return Operation status code.
 */
extern te_errno
tapi_cfg_phy_advertise_one(const char *ta, const char *if_name,
                           int advert_speed, int advert_duplex)
{
    unsigned int  snum, dnum, i, j;
    cfg_handle   *speeds, *duplexes;
    char         *speed;
    char         *duplex;

    /* Get a list of all supported speeds */
    if (cfg_find_pattern_fmt(&snum, &speeds,
                             "/agent:%s/interface:%s/phy:/modes:/speed:*",
                             ta, if_name) != 0)
        ERROR("Failed to find any supported modes");

    /* Walk through all supported speeds values for this interface name */
    for (i = 0; i < snum; i++)
    {
         /* Get the instance name of current speed node */
         if (cfg_get_inst_name(speeds[i], &speed) != 0)
            continue;

         /* Get a list of all supported duplex states */
         if (cfg_find_pattern_fmt(&dnum, &duplexes,
                                  "/agent:%s/interface:%s/phy:/modes:"
                                  "/speed:%s/duplex:*", ta,
                                  if_name, speed) != 0)
            ERROR("Failed to find any supported modes");

        /*
         * Walk through all supported duplex values for this interface
         * name and current speed value */
        for (j = 0; j < dnum; j++)
        {
            /* Get the instance name of current duplex node */
            if (cfg_get_inst_name(duplexes[j], &duplex) != 0)
                continue;

            /* Leave mode advertised */
            if (atoi(speed) == advert_speed &&
                tapi_cfg_phy_duplex_str2id(duplex) == advert_duplex)
                continue;

            /* Turn off advertising for this mode */
            if (tapi_cfg_phy_advertise_mode(ta, if_name, atoi(speed),
                                        tapi_cfg_phy_duplex_str2id(duplex),
                                            FALSE) != 0)
                ERROR("Failed to turn off mode [%s,%s] advertising on "
                      "%s at %s", speed, duplex, ta, if_name);

            free(duplex);
        }

        free(duplexes);
        free(speed);
    }

    free(speeds);

    return tapi_cfg_phy_commit(ta, if_name);
}

/**
 * Get PHY link partner advertised pause frame use.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned pause frame use value:
 *                      TE_PHY_PAUSE_NONE               - no pause frame use
 *                      TE_PHY_PAUSE_TX_ONLY            - transmit only
 *                      TE_PHY_PAUSE_SYMMETRIC          - symmetric
 *                      TE_PHY_PAUSE_SYMMETRIC_RX_ONLY  - symmetric or receive only
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_pause_lp_adv_get(const char *ta, const char *if_name,
                              int *state)
{
    te_errno rc = 0;

    rc = cfg_get_instance_int_sync_fmt(state,
                                   "/agent:%s/interface:%s/phy:/pause_lp_adv:",
                                   ta, if_name);

    /* Check that option is supported */
    if (*state == TE_PHY_PAUSE_UNKNOWN)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return rc;
}

/**
 * Get PHY link partner advertised autonegotiation state.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned autonegotiation state value:
 *                      TE_PHY_AUTONEG_OFF      - autonegotiation OFF
 *                      TE_PHY_AUTONEG_ON       - autonegotiation ON
 *
 * @return Status code
 */
te_errno
tapi_cfg_phy_autoneg_lp_adv_get(const char *ta, const char *if_name,
                                int *state)
{
    te_errno rc = 0;

    rc = cfg_get_instance_int_sync_fmt(state,
                               "/agent:%s/interface:%s/phy:/autoneg_lp_adv:",
                               ta, if_name);

    /* Check that option is supported */
    if (*state == TE_PHY_AUTONEG_UNKNOWN)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return rc;
}
