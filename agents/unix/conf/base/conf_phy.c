/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix TA PHY interface support
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "PHY Conf"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "te_string.h"
#include "te_alloc.h"
#include "te_vector.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"
#include "unix_internal.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

#if HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#include "te_ethernet_phy.h"
#include "conf_ethtool.h"

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H

/* Get value of a field in link settings structure */
static te_errno
phy_field_get(unsigned int gid, const char *if_name,
              ta_ethtool_lsets_field field,
              unsigned int *value, bool admin)
{
    ta_ethtool_lsets *lsets_ptr;
    te_errno rc;

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_LINKSETTINGS,
                           (void **)&lsets_ptr);
    if (rc != 0)
    {
        if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
        {
            /*
             * This will cause Configurator to ignore absence of
             * value silently; it simply will not show not supported
             * node in the tree.
             */
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        return rc;
    }

    if (admin)
    {
        unsigned int autoneg;

        /*
         * When autonegotiation is enabled, operative speed/duplex are
         * considered to be volatile; related administrative nodes are set
         * to unknown value so that Configurator will not try to set
         * specific speed/duplex when trying to restore configuration
         * from backup.
         *
         * When driver does not support changing link settings,
         * administrative speed/duplex should be set to unknown values
         * for the same reason.
         */

        rc = ta_ethtool_lsets_field_get(lsets_ptr,
                                        TA_ETHTOOL_LSETS_AUTONEG,
                                        &autoneg);
        if (rc != 0)
            return rc;

        if (autoneg != 0 || !lsets_ptr->set_supported)
        {
            if (field == TA_ETHTOOL_LSETS_SPEED)
            {
                *value = SPEED_UNKNOWN;
                return 0;
            }
            if (field == TA_ETHTOOL_LSETS_DUPLEX)
            {
                *value = DUPLEX_UNKNOWN;
                return 0;
            }
        }
        else
        {
            rc = ta_ethtool_lsets_field_get(lsets_ptr, field, value);
            if (rc != 0)
                return rc;

            if ((field == TA_ETHTOOL_LSETS_SPEED &&
                 (*value == SPEED_UNKNOWN || *value == 0)) ||
                (field == TA_ETHTOOL_LSETS_DUPLEX &&
                 *value == DUPLEX_UNKNOWN))
            {
                unsigned int best_speed;
                unsigned int best_duplex;

                /*
                 * If returned speed or duplex value is UNKNOWN while
                 * autonegotiation is disabled, report maximum supported
                 * values for administrative speed/duplex instead.
                 * So that if Configurator tries to restore the current
                 * state, it will use values that can be set.
                 * See https://redmine.oktetlabs.ru/issues/12521.
                 */

                rc = ta_ethtool_get_max_speed(lsets_ptr, &best_speed,
                                              &best_duplex);
                if (rc == 0)
                {
                    if (field == TA_ETHTOOL_LSETS_SPEED)
                        *value = best_speed;
                    else
                        *value = best_duplex;
                }
            }

            return 0;
        }
    }

    return ta_ethtool_lsets_field_get(lsets_ptr, field, value);
}

/* Common function to set link settings structure field */
static te_errno
phy_field_set(unsigned int gid, const char *if_name,
              ta_ethtool_lsets_field field, unsigned int value)
{
    ta_ethtool_lsets *lsets_ptr;
    te_errno rc;

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_LINKSETTINGS,
                           (void **)&lsets_ptr);
    if (rc != 0)
        return rc;

    return ta_ethtool_lsets_field_set(lsets_ptr, field, value);
}

/*
 * Get value of agent/interface/phy/port telling physical connector type.
 */
static te_errno
phy_port_get(const ta_conf_ctx *ctx, int *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    unsigned int port;
    te_errno rc;

    rc = phy_field_get(ta_conf_ctx_gid(ctx), if_name,
                       TA_ETHTOOL_LSETS_PORT, &port, false);
    if (rc != 0)
        return rc;

    *val = (int)port;
    return 0;
}

/*
 * Get value of agent/interface/phy/autoneg telling whether
 * autonegotiation is enabled.
 */
static te_errno
phy_autoneg_get(const ta_conf_ctx *ctx, bool *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    unsigned int autoneg;
    te_errno rc;

    rc = phy_field_get(ta_conf_ctx_gid(ctx), if_name,
                       TA_ETHTOOL_LSETS_AUTONEG, &autoneg, false);
    if (rc != 0)
        return rc;

    *val = (autoneg != 0);
    return 0;
}

/* Set autonegotiation state */
static te_errno
phy_autoneg_set(const ta_conf_ctx *ctx, bool val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");

    return phy_field_set(ta_conf_ctx_gid(ctx), if_name,
                         TA_ETHTOOL_LSETS_AUTONEG, val ? 1 : 0);
}

/* Common function to process get request for speed_oper/speed_admin */
static te_errno
phy_speed_get_common(const ta_conf_ctx *ctx, bool admin, int32_t *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    unsigned int speed;
    te_errno rc;

    rc = phy_field_get(ta_conf_ctx_gid(ctx), if_name,
                       TA_ETHTOOL_LSETS_SPEED, &speed, admin);
    if (rc != 0)
        return rc;

    /*
     * Currently maximum known speed value is 400000 (SPEED_400000).
     * It fits into signed int32 (INT32_MAX=2147483647).
     */
    *val = (speed == (unsigned int)SPEED_UNKNOWN) ? -1 : (int32_t)speed;
    return 0;
}

/* Get value of agent/interface/phy/speed_oper */
static te_errno
phy_speed_oper_get(const ta_conf_ctx *ctx, int32_t *val)
{
    return phy_speed_get_common(ctx, false, val);
}

/*
 * Get value of agent/interface/phy/speed_admin. It is equal to
 * speed_oper if autonegotiation is disabled, and is unknown
 * otherwise.
 */
static te_errno
phy_speed_admin_get(const ta_conf_ctx *ctx, int32_t *val)
{
    return phy_speed_get_common(ctx, true, val);
}

/* Set administrative speed value */
static te_errno
phy_speed_admin_set(const ta_conf_ctx *ctx, int32_t val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");

    if (val < 0)
    {
        ERROR("%s(): invalid speed value '%jd'", __FUNCTION__, val);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return phy_field_set(ta_conf_ctx_gid(ctx), if_name,
                         TA_ETHTOOL_LSETS_SPEED, (unsigned int)val);
}

/* Common function to process get request for duplex_oper/duplex_admin */
static te_errno
phy_duplex_get_common(const ta_conf_ctx *ctx, bool admin, int *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    unsigned int duplex;
    te_errno rc;

    rc = phy_field_get(ta_conf_ctx_gid(ctx), if_name,
                       TA_ETHTOOL_LSETS_DUPLEX, &duplex, admin);
    if (rc != 0)
        return rc;

    *val = (int)duplex;
    return 0;
}

/* Get value of agent/interface/phy/duplex_oper */
static te_errno
phy_duplex_oper_get(const ta_conf_ctx *ctx, int *val)
{
    return phy_duplex_get_common(ctx, false, val);
}

/*
 * Get value of agent/interface/phy/duplex_admin. It is equal to
 * duplex_oper if autonegotiation is disabled, and is unknown
 * otherwise.
 */
static te_errno
phy_duplex_admin_get(const ta_conf_ctx *ctx, int *val)
{
    return phy_duplex_get_common(ctx, true, val);
}

/* Set administrative duplex value */
static te_errno
phy_duplex_admin_set(const ta_conf_ctx *ctx, int val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");

    return phy_field_set(ta_conf_ctx_gid(ctx), if_name,
                         TA_ETHTOOL_LSETS_DUPLEX, (unsigned int)val);
}

/*
 * Check whether changing link settings is supported for the interface.
 */
static te_errno
phy_set_supported_get(const ta_conf_ctx *ctx, bool *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    ta_ethtool_lsets *lsets_ptr = NULL;
    te_errno rc;

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx),
                           TA_ETHTOOL_LINKSETTINGS, (void **)&lsets_ptr);
    if (rc != 0)
    {
        if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
        {
            /*
             * This will cause Configurator to ignore absence of
             * value silently; it simply will not show not supported
             * node in the tree.
             */
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        return rc;
    }

    *val = lsets_ptr->set_supported;
    return 0;
}

/**
 * Restart autonegotiation.
 *
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_reset(const char *ifname)
{
    struct ifreq         ifr;
    int                  rc = -1;
    struct ethtool_value edata;

    memset(&edata, 0, sizeof(edata));
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);

    edata.cmd = ETHTOOL_NWAY_RST;
    ifr.ifr_data = (caddr_t)&edata;
    rc = ioctl(cfg_socket, SIOCETHTOOL, &ifr);

    if (rc < 0)
    {
        VERB("failed to restart autonegotiation at %s, errno=%d (%s)",
             ifname, errno, strerror(errno));

        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/**
 * Get PHY state value.
 *
 * @param ctx           request context
 * @param val           location of value
 *
 * @return              Status code
 */
static te_errno
phy_state_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    struct ifreq         ifr;
    struct ethtool_value edata;

    memset(&edata, 0, sizeof(edata));

    /* Initialize control structure */
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);

    edata.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (caddr_t)&edata;

    /* Get link state */
    if (ioctl(cfg_socket, SIOCETHTOOL, &ifr) != 0)
    {
        switch (errno)
        {
            case EOPNOTSUPP:
                /*
                 * Check for option support: if option is not
                 * supported the leaf value should be set to -1
                 */
            case ENODEV:
                /*
                 * It can return ENODEV for some interfaces if the last ones
                 * are not active, and this case should not prevent
                 * agent/interface initialization
                 */
                *val = TE_PHY_STATE_UNKNOWN;
                return 0;

            default:
                ERROR("failed to get interface state value");
                return TE_RC(TE_TA_UNIX, errno);
        }
    }

    *val = edata.data ? TE_PHY_STATE_UP : TE_PHY_STATE_DOWN;

    return 0;
}

/*
 * Get list of link modes which are supported by network interface
 * or advertised by its link partner.
 */
static te_errno
mode_list_common(bool link_partner, const ta_conf_ctx *ctx, te_vec *names)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    ta_ethtool_lsets *lsets_ptr;
    te_string list_str = TE_STRING_INIT;
    te_errno rc = 0;

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx),
                           TA_ETHTOOL_LINKSETTINGS, (void **)&lsets_ptr);
    if (rc != 0)
    {
        if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
        {
            /*
             * This will cause Configurator to ignore absence of
             * value silently; it simply will not show not supported
             * node in the tree.
             */
            return 0;
        }

        return rc;
    }

    rc = ta_ethtool_lmode_list_names(lsets_ptr, link_partner, &list_str);
    if (rc != 0)
    {
        te_string_free(&list_str);
        return rc;
    }

    if (list_str.ptr != NULL)
    {
        char *saveptr;
        char *tok;

        for (tok = strtok_r(list_str.ptr, " ", &saveptr); tok != NULL;
             tok = strtok_r(NULL, " ", &saveptr))
        {
            char *name = TE_STRDUP(tok);

            TE_VEC_APPEND(names, name);
        }
    }
    te_string_free(&list_str);

    return 0;
}

/* Get list of link modes supported by network interface */
static te_errno
phy_mode_list(const ta_conf_ctx *ctx, te_vec *names)
{
    return mode_list_common(false, ctx, names);
}

/* Get advertising state for a supported link mode */
static te_errno
phy_mode_get(const ta_conf_ctx *ctx, bool *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *mode_name = ta_conf_ctx_inst(ctx, "mode");
    ta_ethtool_lsets *lsets_ptr;
    ta_ethtool_link_mode mode;
    te_errno rc;

    rc = ta_ethtool_lmode_parse(mode_name, &mode);
    if (rc != 0)
        return rc;

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx),
                           TA_ETHTOOL_LINKSETTINGS, (void **)&lsets_ptr);
    if (rc != 0)
        return rc;

    return ta_ethtool_lmode_advertised(lsets_ptr, mode, val);
}

/* Set advertising state for a supported link mode */
static te_errno
phy_mode_set(const ta_conf_ctx *ctx, bool val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *mode_name = ta_conf_ctx_inst(ctx, "mode");
    ta_ethtool_lsets *lsets_ptr;
    ta_ethtool_link_mode mode;
    te_errno rc;

    rc = ta_ethtool_lmode_parse(mode_name, &mode);
    if (rc != 0)
        return rc;

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx),
                           TA_ETHTOOL_LINKSETTINGS, (void **)&lsets_ptr);
    if (rc != 0)
        return rc;

    return ta_ethtool_lmode_advertise(lsets_ptr, mode, val);
}

/* Get list of link modes advertised by link partner */
static te_errno
phy_lp_advertised_list(const ta_conf_ctx *ctx, te_vec *names)
{
    return mode_list_common(true, ctx, names);
}

/* Commit all changes made to link settings */
static te_errno
phy_commit(const ta_conf_ctx *ctx)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    unsigned int gid = ta_conf_ctx_gid(ctx);
    unsigned int autoneg;
    te_errno rc;

    rc = commit_ethtool_value(if_name, gid, TA_ETHTOOL_LINKSETTINGS);
    if (rc != 0)
        return rc;

    rc = phy_field_get(gid, if_name, TA_ETHTOOL_LSETS_AUTONEG,
                       &autoneg, false);
    if (rc != 0)
        return rc;

    if (autoneg)
        phy_reset(if_name);

    return 0;
}

static const ta_conf_node *const node_phy =
    TA_CONF_NA_COMMIT("phy", phy_commit,
        TA_CONF_RO_BOOL("set_supported", phy_set_supported_get),
        TA_CONF_RO_ENUM("duplex_oper", te_phy_duplex_map,
                        phy_duplex_oper_get),
        TA_CONF_RW_ENUM("duplex_admin", te_phy_duplex_map,
                        phy_duplex_admin_get, phy_duplex_admin_set),
        TA_CONF_RO_INT32("speed_oper", phy_speed_oper_get),
        TA_CONF_RW_INT32("speed_admin", phy_speed_admin_get,
                       phy_speed_admin_set),
        TA_CONF_RW_BOOL("autoneg", phy_autoneg_get, phy_autoneg_set),
        TA_CONF_RO_ENUM("port", te_phy_port_map, phy_port_get),
        TA_CONF_RW_COLL_BOOL("mode", phy_mode_get, phy_mode_set,
                             phy_mode_list),
        TA_CONF_LIST("lp_advertised", phy_lp_advertised_list),
        TA_CONF_RO_INT32("state", phy_state_get));

/**
 * Add /agent/interface/phy node for link settings.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_phy_init(void)
{
    return ta_conf_register("/agent/interface", node_phy);
}

#else

/* See description above */
extern te_errno
ta_unix_conf_if_phy_init(void)
{
    WARN("Interface PHY settings are not supported");
    return 0;
}

#endif
