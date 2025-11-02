/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for PHY configuration model
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2004-2025 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Configuration PHY"

#include "te_defs.h"
#include "tapi_cfg_phy.h"
#include "te_ethernet_phy.h"
#include "te_time.h"
#include "te_sleep.h"

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_autoneg_get(const char *ta, const char *if_name, int *state)
{
    return cfg_get_instance_int_sync_fmt(state,
                                         "/agent:%s/interface:%s/phy:/autoneg:",
                                         ta, if_name);
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_autoneg_set(const char *ta, const char *if_name, int state)
{
    return cfg_set_instance_local_fmt(
                            CFG_VAL(INT32, state),
                            "/agent:%s/interface:%s/phy:/autoneg:",
                            ta, if_name);
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_autoneg_oper_get(const char *ta, const char *if_name,
                              int *state)
{
    WARN("%s() is outdated, use tapi_cfg_phy_autoneg_get()",
         __FUNCTION__);

    return tapi_cfg_phy_autoneg_get(ta, if_name, state);
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_autoneg_admin_get(const char *ta, const char *if_name,
                               int *state)
{
    WARN("%s() is outdated, use tapi_cfg_phy_autoneg_get()",
         __FUNCTION__);

    return tapi_cfg_phy_autoneg_get(ta, if_name, state);
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_autoneg_admin_set(const char *ta, const char *if_name,
                               int state)
{
    WARN("%s() is outdated, use tapi_cfg_phy_autoneg_set()",
         __FUNCTION__);

    return tapi_cfg_phy_autoneg_set(ta, if_name, state);
}
/* See description in tapi_cfg_phy.h */
extern int
tapi_cfg_phy_autoneg_str2id(const char *name)
{
    return te_enum_map_from_str(te_phy_autoneg_map, name, -1);
}

/* See description in tapi_cfg_phy.h */
extern int
tapi_cfg_phy_duplex_str2id(const char *name)
{
    return te_enum_map_from_str(te_phy_duplex_map, name, -1);
}

/* See description in tapi_cfg_phy.h */
extern enum te_phy_port
tapi_cfg_phy_port_str2id(const char *name)
{
    return te_enum_map_from_str(te_phy_port_map, name, -1);
}

/* See description in tapi_cfg_phy.h */
extern int
tapi_cfg_phy_speed_str2id(const char *name)
{
    return te_enum_map_from_str(te_phy_speed_map, name, 0);
}

/* See description in tapi_cfg_phy.h */
extern const char *
tapi_cfg_phy_autoneg_id2str(int autoneg)
{
    return te_enum_map_from_value(te_phy_autoneg_map, autoneg);
}

/* See description in tapi_cfg_phy.h */
extern const char *
tapi_cfg_phy_duplex_id2str(int duplex)
{
    return te_enum_map_from_value(te_phy_duplex_map, duplex);
}

/* See description in tapi_cfg_phy.h */
extern const char *
tapi_cfg_phy_port_id2str(enum te_phy_port port)
{
    return te_enum_map_from_value(te_phy_port_map, port);
}

/* See description in tapi_cfg_phy.h */
extern const char *
tapi_cfg_phy_speed_id2str(int speed)
{
    return te_enum_map_from_value(te_phy_speed_map, speed);
}

/* See description in tapi_cfg_phy.h */
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

/* See description in tapi_cfg_phy.h */
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

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_duplex_admin_set(const char *ta, const char *if_name,
                              int state)
{
    te_errno  rc = 0;
    const char *duplex;

    duplex = tapi_cfg_phy_duplex_id2str(state);

    if (duplex == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_set_instance_local_fmt(CFG_VAL(STRING, duplex),
                            "/agent:%s/interface:%s/phy:/duplex_admin:",
                            ta, if_name);

    return rc;
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_port_get(const char *ta, const char *if_name,
                      enum te_phy_port *state)
{
    te_errno  rc = 0;
    char *port;

    rc = cfg_get_instance_sync_fmt(NULL, (void *)&port,
                            "/agent:%s/interface:%s/phy:/port:",
                            ta, if_name);
    if (rc != 0)
        return rc;

    *state = tapi_cfg_phy_port_str2id(port);

    free(port);

    if (*state == -1)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return 0;
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_speed_oper_get(const char *ta, const char *if_name, int *speed)
{
    te_errno rc = 0;

    rc = cfg_get_instance_int_sync_fmt(speed,
                                   "/agent:%s/interface:%s/phy:/speed_oper:",
                                   ta, if_name);
    if (rc != 0)
        return rc;

    return rc;
}

/* See description in tapi_cfg_phy.h */
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

    return rc;
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_speed_admin_set(const char *ta, const char *if_name, int speed)
{
    te_errno rc = 0;

    rc = cfg_set_instance_local_fmt(CFG_VAL(INT32, speed),
                            "/agent:%s/interface:%s/phy:/speed_admin:",
                            ta, if_name);

    return rc;

}

/* See description in tapi_cfg_phy.h */
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

/* See description in tapi_cfg_phy.h */
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

/* See description in tapi_cfg_phy.h */
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

/* See description in tapi_cfg_phy.h */
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

    while (true)
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
 * Enable/disable/check advertisement of specific speed/duplex.
 *
 * @note If there are multiple link modes matching supplied parameters,
 *       advertising state will be changed for all of them if a change is
 *       requested. In case of get operation, if any of the matching
 *       modes are advertised, this function reports enabled state.
 *
 * @param ta                  Test agent name
 * @param if_name             Interface name
 * @param speed               Speed (if <= 0, all speeds match)
 * @param duplex              Duplex state (if #TE_PHY_DUPLEX_UNKNOWN, all
 *                            states match)
 * @param state               If @p do_set is @c true, this tells whether
 *                            advertising should be enabled. Otherwise the
 *                            current advertising state will be saved here
 * @param do_set              If @c true, perform state change. Otherwise
 *                            get the current state
 * @param unset_not_matched   If @c true and @p do_set is @c true, disable
 *                            all not matching link modes which specify
 *                            speed/duplex
 *
 * @return Status code.
 */
static te_errno
speed_duplex_advertise_op(const char *ta, const char *if_name,
                          int speed, int duplex, bool *state,
                          bool do_set, bool unset_not_matched)
{
    te_errno rc = 0;
    unsigned int modes_num = 0;
    cfg_handle *modes = NULL;
    char *mode_name = NULL;
    unsigned int i;
    cfg_val_type val_type;

    char *endptr;
    long unsigned int parsed_speed;
    char *str_duplex;
    int parsed_duplex;
    int state_int;
    bool not_matched;

    if (!do_set)
        *state = false;

    rc = cfg_find_pattern_fmt(&modes_num, &modes,
                              "/agent:%s/interface:%s/phy:/mode:*",
                              ta, if_name);
    if (rc != 0)
        return rc;

    for (i = 0; i < modes_num; i++)
    {
        free(mode_name);
        mode_name = NULL;
        not_matched = false;

        rc = cfg_get_inst_name(modes[i], &mode_name);
        if (rc != 0)
            break;

        parsed_speed = strtoul(mode_name, &endptr, 10);
        if (endptr == NULL || strcmp_start("base", endptr) != 0)
            continue;
        if (speed > 0 && parsed_speed != speed)
        {
            if (!unset_not_matched)
                continue;

            not_matched = true;
        }

        str_duplex = strrchr(mode_name, '_');
        if (str_duplex == NULL)
            continue;

        if (strcmp(str_duplex, "_Full") == 0)
            parsed_duplex = TE_PHY_DUPLEX_FULL;
        else if (strcmp(str_duplex, "_Half") == 0)
            parsed_duplex = TE_PHY_DUPLEX_HALF;
        else
            continue;

        if (duplex != TE_PHY_DUPLEX_UNKNOWN && duplex != parsed_duplex)
        {
            if (!unset_not_matched)
                continue;

            not_matched = true;
        }

        if (do_set)
        {
            if (not_matched)
                state_int = 0;
            else
                state_int = (state ? 1 : 0);

            rc = cfg_set_instance_local(modes[i], CVT_INTEGER, state_int);
        }
        else
        {
            val_type = CVT_INT32;
            rc = cfg_get_instance(modes[i], &val_type, &state_int);
            if (rc == 0)
            {
                *state = (state_int == 0 ? false : true);
                if (*state)
                    break;
            }
        }
        if (rc != 0)
            break;
    }

    free(modes);
    free(mode_name);

    return rc;
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_is_mode_advertised(const char *ta, const char *if_name,
                                int speed, int duplex,
                                bool *state)
{
    return speed_duplex_advertise_op(ta, if_name, speed, duplex, state,
                                     false, false);
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_advertise_mode(const char *ta, const char *if_name,
                            int speed, int duplex, bool state)
{

    return speed_duplex_advertise_op(ta, if_name, speed, duplex, &state,
                                     true, false);
}

/* See description in tapi_cfg_phy.h */
extern te_errno
tapi_cfg_phy_commit(const char *ta, const char *if_name)
{
    return cfg_commit_fmt("/agent:%s/interface:%s/phy:", ta, if_name);
}

/* See description in tapi_cfg_phy.h */
extern te_errno
tapi_cfg_phy_advertise_one(const char *ta, const char *if_name,
                           int advert_speed, int advert_duplex)
{
    te_errno rc;
    bool state = true;

    rc = speed_duplex_advertise_op(ta, if_name, advert_speed,
                                   advert_duplex, &state, true, true);
    if (rc != 0)
        return rc;

    return tapi_cfg_phy_commit(ta, if_name);
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_lp_advertised(const char *ta,
                           const char *if_name,
                           const char *mode_name,
                           bool *advertised)
{
    te_errno rc;
    cfg_handle handle;

    rc = cfg_find_fmt(&handle,
                      "/agent:%s/interface:%s/phy:/lp_advertised:%s",
                      ta, if_name, mode_name);
    if (rc == 0)
        *advertised = true;
    else if (rc == TE_RC(TE_CS, TE_ENOENT))
        *advertised = false;
    else
        return rc;

    return 0;
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_pause_lp_adv_get(const char *ta, const char *if_name,
                              int *state)
{
    te_errno rc;
    bool pause = false;
    bool asym_pause = false;

    rc = tapi_cfg_phy_lp_advertised(ta, if_name, "Pause", &pause);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_phy_lp_advertised(ta, if_name, "Asym_Pause", &asym_pause);
    if (rc != 0)
        return rc;

    if (pause)
    {
        if (asym_pause)
            *state = TE_PHY_PAUSE_SYMMETRIC_RX_ONLY;
        else
            *state = TE_PHY_PAUSE_SYMMETRIC;
    }
    else
    {
        if (asym_pause)
            *state = TE_PHY_PAUSE_TX_ONLY;
        else
            *state = TE_PHY_PAUSE_NONE;
    }

    return 0;
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_autoneg_lp_adv_get(const char *ta, const char *if_name,
                                int *state)
{
    te_errno rc;
    bool autoneg;

    rc = tapi_cfg_phy_lp_advertised(ta, if_name, "Autoneg", &autoneg);
    if (rc != 0)
        return rc;

    if (autoneg)
        *state = TE_PHY_AUTONEG_ON;
    else
        *state = TE_PHY_AUTONEG_OFF;

    return 0;
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_mode_supported(const char *ta,
                            const char *if_name,
                            const char *mode_name,
                            bool *supported)
{
    te_errno rc;
    cfg_handle handle;

    rc = cfg_find_fmt(&handle,
                      "/agent:%s/interface:%s/phy:/mode:%s",
                      ta, if_name, mode_name);
    if (rc == 0)
        *supported = true;
    else if (rc == TE_RC(TE_CS, TE_ENOENT))
        *supported = false;
    else
        return rc;

    return 0;
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_mode_adv_get(const char *ta,
                          const char *if_name,
                          const char *mode_name,
                          bool *state)
{
    int value;
    te_errno rc;

    rc = cfg_get_instance_int_fmt(
                              &value,
                              "/agent:%s/interface:%s/phy:/mode:%s",
                              ta, if_name, mode_name);
    if (rc != 0)
        return rc;

    if (value == 0)
        *state = false;
    else
        *state = true;

    return 0;
}

/* See description in tapi_cfg_phy.h */
te_errno
tapi_cfg_phy_mode_adv_set(const char *ta,
                          const char *if_name,
                          const char *mode_name,
                          bool state)
{
    int value = state ? 1 : 0;

    return cfg_set_instance_local_fmt(
                                CVT_INTEGER, &value,
                                "/agent:%s/interface:%s/phy:/mode:%s",
                                ta, if_name, mode_name);
}
