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
#include "rcf_pch.h"
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
/*
 * Get value of agent/interface/phy/port telling physical connector type.
 */
static te_errno
phy_port_get(unsigned int gid, const char *oid, char *value,
             const char *if_name)
{
    te_errno rc;
    unsigned int port;
    const char *port_str;

    UNUSED(oid);

    rc = phy_field_get(gid, if_name, TA_ETHTOOL_LSETS_PORT, &port, false);
    if (rc != 0)
        return rc;

    port_str = te_enum_map_from_value(te_phy_port_map, port);
    if (port_str == NULL)
    {
            ERROR("%s(): unknown port value %u", __FUNCTION__, port);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = te_snprintf(value, RCF_MAX_VAL, "%s", port_str);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

/*
 * Get value of agent/interface/phy/autoneg telling whether
 * autonegotiation is enabled.
 */
static te_errno
phy_autoneg_get(unsigned int gid, const char *oid, char *value,
                const char *if_name)
{
    te_errno rc;
    unsigned int autoneg;

    UNUSED(oid);

    rc = phy_field_get(gid, if_name,
                       TA_ETHTOOL_LSETS_AUTONEG, &autoneg, false);
    if (rc != 0)
        return rc;

    rc = te_snprintf(value, RCF_MAX_VAL, "%u", autoneg);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

/* Common function to process get request for speed_oper/speed_admin */
static te_errno
phy_speed_get_common(unsigned int gid, const char *oid, char *value,
                     const char *if_name, bool admin)
{
    te_errno rc;
    unsigned int speed;

    UNUSED(oid);

    rc = phy_field_get(gid, if_name,
                       TA_ETHTOOL_LSETS_SPEED, &speed, admin);
    if (rc != 0)
        return rc;

    /*
     * Currently maximum known speed value is 400000 (SPEED_400000).
     * It fits into signed int32 (INT32_MAX=2147483647).
     */
    rc = te_snprintf(value, RCF_MAX_VAL, "%d",
                     (speed == (unsigned int)SPEED_UNKNOWN ?
                                                      -1 : (int)speed));
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

/* Get value of agent/interface/phy/speed_oper */
static te_errno
phy_speed_oper_get(unsigned int gid, const char *oid, char *value,
                   const char *if_name)
{
    return phy_speed_get_common(gid, oid, value, if_name, false);
}

/*
 * Get value of agent/interface/phy/speed_admin. It is equal to
 * speed_oper if autonegotiation is disabled, and is unknown
 * otherwise.
 */
static te_errno
phy_speed_admin_get(unsigned int gid, const char *oid, char *value,
                    const char *if_name)
{
    return phy_speed_get_common(gid, oid, value, if_name, true);
}

/* Common function to process get request for duplex_oper/duplex_admin */
static te_errno
phy_duplex_get_common(unsigned int gid, const char *oid, char *value,
                      const char *if_name, bool admin)
{
    te_errno rc;
    unsigned int duplex;
    const char *duplex_str = NULL;

    UNUSED(oid);

    rc = phy_field_get(gid, if_name,
                       TA_ETHTOOL_LSETS_DUPLEX, &duplex, admin);
    if (rc != 0)
        return rc;

    duplex_str = te_enum_map_from_value(te_phy_duplex_map, duplex);
    if (duplex_str == NULL)
    {
            ERROR("%s(): unknown duplex value %u", __FUNCTION__, duplex);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = te_snprintf(value, RCF_MAX_VAL, "%s", duplex_str);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

/* Get value of agent/interface/phy/duplex_oper */
static te_errno
phy_duplex_oper_get(unsigned int gid, const char *oid, char *value,
                    const char *if_name)
{
    return phy_duplex_get_common(gid, oid, value, if_name, false);
}

/*
 * Get value of agent/interface/phy/duplex_admin. It is equal to
 * duplex_oper if autonegotiation is disabled, and is unknown
 * otherwise.
 */
static te_errno
phy_duplex_admin_get(unsigned int gid, const char *oid, char *value,
                     const char *if_name)
{
    return phy_duplex_get_common(gid, oid, value, if_name, true);
}

/*
 * Check whether changing link settings is supported for the interface.
 */
static te_errno
phy_set_supported_get(unsigned int gid, const char *oid, char *value,
                      const char *if_name)
{
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);
    ta_ethtool_lsets *lsets_ptr = NULL;
    te_errno rc;

    UNUSED(oid);

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

    te_string_append(&str_val, "%d", lsets_ptr->set_supported ? 1 : 0);
    return 0;
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

/* Set autonegotiation state */
static te_errno
phy_autoneg_set(unsigned int gid, const char *oid, const char *value,
                const char *if_name)
{
    te_errno rc;
    unsigned long int parsed_val;

    UNUSED(oid);

    rc = te_strtoul(value, 10, &parsed_val);
    if (rc != 0 || (parsed_val != 0 && parsed_val != 1))
    {
        ERROR("%s(): invalid value '%s'", __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, rc != 0 ? rc : TE_EINVAL);
    }

    return phy_field_set(gid, if_name, TA_ETHTOOL_LSETS_AUTONEG,
                         parsed_val);
}

/* Set administrative speed value */
static te_errno
phy_speed_admin_set(unsigned int gid, const char *oid, const char *value,
                    const char *if_name)
{
    te_errno rc;
    unsigned long int parsed_val;

    UNUSED(oid);

    rc = te_strtoul(value, 10, &parsed_val);
    if (rc != 0)
    {
        ERROR("%s(): invalid speed value '%s'", __FUNCTION__, value);
        return rc;
    }

    return phy_field_set(gid, if_name, TA_ETHTOOL_LSETS_SPEED,
                         parsed_val);
}

/* Set administrative duplex value */
static te_errno
phy_duplex_admin_set(unsigned int gid, const char *oid, const char *value,
                     const char *if_name)
{
    unsigned int duplex;

    UNUSED(oid);

    duplex = te_enum_map_from_str(te_phy_duplex_map, value, UINT_MAX);
    if (duplex == UINT_MAX)
    {
        ERROR("%s(): duplex value '%s' is not supported", __FUNCTION__,
              value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return phy_field_set(gid, if_name, TA_ETHTOOL_LSETS_DUPLEX, duplex);
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
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_state_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

#if defined __linux__
    struct ifreq         ifr;
    struct ethtool_value edata;
    int                  state = -1;

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
                snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_STATE_UNKNOWN);
                return 0;

            default:
                ERROR("failed to get interface state value");
                return TE_RC(TE_TA_UNIX, errno);
        }
    }

    state = edata.data ? TE_PHY_STATE_UP : TE_PHY_STATE_DOWN;

    snprintf(value, RCF_MAX_VAL, "%d", state);

    return 0;
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    return 0;
}

/*
 * Get list of link modes which are supported by network interface
 * or advertised by its link partner.
 */
static te_errno
mode_list_common(bool link_partner,
                 unsigned int gid, const char *oid,
                 const char *sub_id, char **list,
                 const char *if_name)
{
    ta_ethtool_lsets *lsets_ptr;
    te_errno rc = 0;

    te_string list_str = TE_STRING_INIT;

    UNUSED(oid);
    UNUSED(sub_id);

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
            *list = NULL;
            return 0;
        }

        return rc;
    }

    rc = ta_ethtool_lmode_list_names(lsets_ptr, link_partner, &list_str);

    if (rc != 0)
    {
        te_string_free(&list_str);
        *list = NULL;
    }
    else
    {
        *list = list_str.ptr;
    }

    return rc;
}

/* Get list of link modes supported by network interface */
static te_errno
phy_mode_list(unsigned int gid, const char *oid,
              const char *sub_id, char **list,
              const char *if_name)
{
    return mode_list_common(false, gid, oid, sub_id, list, if_name);
}

/* Get advertising state for a supported link mode */
static te_errno
phy_mode_get(unsigned int gid, const char *oid, char *value,
             const char *if_name, const char *phy_name,
             const char *mode_name)
{
    ta_ethtool_lsets *lsets_ptr;
    te_errno rc;
    ta_ethtool_link_mode mode;
    bool advertised;

    UNUSED(oid);
    UNUSED(phy_name);

    rc = ta_ethtool_lmode_parse(mode_name, &mode);
    if (rc != 0)
        return rc;

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_LINKSETTINGS,
                           (void **)&lsets_ptr);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_lmode_advertised(lsets_ptr, mode, &advertised);
    if (rc != 0)
        return rc;

    rc = te_snprintf(value, RCF_MAX_VAL, "%u", advertised ? 1 : 0);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

/* Set advertising state for a supported link mode */
static te_errno
phy_mode_set(unsigned int gid, const char *oid, const char *value,
             const char *if_name, const char *phy_name,
             const char *mode_name)
{
    ta_ethtool_lsets *lsets_ptr;
    te_errno rc = 0;
    ta_ethtool_link_mode mode;
    unsigned long int parsed_val;

    UNUSED(oid);
    UNUSED(phy_name);

    rc = te_strtoul(value, 10, &parsed_val);
    if (rc != 0 || (parsed_val != 0 && parsed_val != 1))
    {
        ERROR("%s(): invalid value '%s'", __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, rc != 0 ? rc : TE_EINVAL);
    }

    rc = ta_ethtool_lmode_parse(mode_name, &mode);
    if (rc != 0)
        return rc;

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_LINKSETTINGS,
                           (void **)&lsets_ptr);
    if (rc != 0)
        return rc;

    return ta_ethtool_lmode_advertise(lsets_ptr, mode,
                                      parsed_val == 0 ? false : true);
}

/* Get list of link modes advertised by link partner */
static te_errno
phy_lp_advertised_list(unsigned int gid, const char *oid,
                       const char *sub_id, char **list,
                       const char *if_name)
{
    return mode_list_common(true, gid, oid, sub_id, list, if_name);
}

/* Commit all changes made to link settings */
static te_errno
phy_commit(unsigned int gid, const cfg_oid *p_oid)
{
    char *if_name;
    unsigned int autoneg;
    te_errno rc;

    UNUSED(gid);
    if_name = CFG_OID_GET_INST_NAME(p_oid, 2);

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


static rcf_pch_cfg_object node_phy;

RCF_PCH_CFG_NODE_RO(node_phy_state, "state", NULL,
                    NULL, phy_state_get);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_phy_lp_advertised, "lp_advertised",
                               NULL, &node_phy_state,
                               NULL, phy_lp_advertised_list);

RCF_PCH_CFG_NODE_RWC_COLLECTION(node_phy_mode, "mode",
                                NULL, &node_phy_lp_advertised,
                                phy_mode_get, phy_mode_set,
                                NULL, NULL,
                                phy_mode_list, &node_phy);

RCF_PCH_CFG_NODE_RO(node_phy_port, "port", NULL,
                    &node_phy_mode, phy_port_get);

RCF_PCH_CFG_NODE_RWC(node_phy_autoneg, "autoneg", NULL,
                     &node_phy_port, phy_autoneg_get, phy_autoneg_set,
                     &node_phy);

RCF_PCH_CFG_NODE_RWC(node_phy_speed_admin, "speed_admin", NULL,
                     &node_phy_autoneg, phy_speed_admin_get,
                     phy_speed_admin_set, &node_phy);

RCF_PCH_CFG_NODE_RO(node_phy_speed_oper, "speed_oper", NULL,
                    &node_phy_speed_admin, phy_speed_oper_get);

RCF_PCH_CFG_NODE_RWC(node_phy_duplex_admin, "duplex_admin", NULL,
                     &node_phy_speed_oper, phy_duplex_admin_get,
                     phy_duplex_admin_set, &node_phy);

RCF_PCH_CFG_NODE_RO(node_phy_duplex_oper, "duplex_oper", NULL,
                    &node_phy_duplex_admin, phy_duplex_oper_get);

RCF_PCH_CFG_NODE_RO(node_phy_set_supported, "set_supported", NULL,
                    &node_phy_duplex_oper, phy_set_supported_get);

RCF_PCH_CFG_NODE_NA_COMMIT(node_phy, "phy", &node_phy_set_supported, NULL,
                           phy_commit);

/**
 * Add /agent/interface/phy node for link settings.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_phy_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_phy);
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
