/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief IP VLAN configuration support
 *
 * Implementation of configuration nodes IP VLAN interfaces.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Unix Conf IP VLAN"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif
#include "logger_api.h"
#include "rcf_pch_tree.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "te_str.h"

#ifdef USE_LIBNETCONF

#include "conf_netconf.h"
#include "netconf.h"
#include <linux/if_link.h>

/* Linux > 3.18 */
#ifdef IFLA_IPVLAN_MAX
typedef struct ipvlan_mode_map {
    enum ipvlan_mode    val;
    const char         *str;
} ipvlan_mode_map;

typedef struct ipvlan_flag_map {
    uint32_t    val;
    const char *str;
} ipvlan_flag_map;

static ipvlan_mode_map ipvlan_modes[] =
{
    {IPVLAN_MODE_L2, "l2"},
    {IPVLAN_MODE_L3, "l3"},
/* Linux > 4.8 */
#ifdef IPVLAN_MODE_L3S
    {IPVLAN_MODE_L3S, "l3s"}
#endif
};

/* bridge is default value for ipvlan */
static ipvlan_flag_map ipvlan_flags[] =
{
    {0, "bridge"},
/* Linux > 4.14 */
#ifdef IPVLAN_F_PRIVATE
    {IPVLAN_F_PRIVATE, "private"},
#endif
/* Linux > 4.14 */
#ifdef IPVLAN_F_VEPA
    {IPVLAN_F_VEPA, "vepa"},
#endif
};
#endif /* IFLA_IPVLAN_MAX */

/**
 * Convert IP VLAN mode string @p str to value @p val.
 *
 * @return Status code
 */
static te_errno
ipvlan_mode_str2val(const char *str, uint32_t *val)
{
#ifdef IFLA_IPVLAN_MAX
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(ipvlan_modes); i++)
    {
        if (strcmp(ipvlan_modes[i].str, str) == 0)
        {
            *val = ipvlan_modes[i].val;
            return 0;
        }
    }

    ERROR("Unknown IP VLAN mode '%s'", str);
#else
    UNUSED(str);
    UNUSED(val);
#endif
    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

/**
 * Convert IP VLAN mode value @p val to string @p str.
 *
 * @return Status code
 */
static te_errno
ipvlan_mode_val2str(uint32_t val, const char **str)
{
#ifdef IFLA_IPVLAN_MAX
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(ipvlan_modes); i++)
    {
        if (val == ipvlan_modes[i].val)
        {
            *str = ipvlan_modes[i].str;
            return 0;
        }
    }

    ERROR("Unknown IP VLAN mode %"TE_PRINTF_32"u", val);
#else
    UNUSED(val);
    UNUSED(str);
#endif
    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

/**
 * Convert IP VLAN flag string @p str to value @p val.
 *
 * @return Status code
 */
static te_errno
ipvlan_flag_str2val(const char *str, uint32_t *val)
{
#ifdef IFLA_IPVLAN_MAX
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(ipvlan_flags); i++)
    {
        if (strcmp(ipvlan_flags[i].str, str) == 0)
        {
            *val = ipvlan_flags[i].val;
            return 0;
        }
    }

    ERROR("IP VLAN flag '%s' is not supported", str);
#else
    UNUSED(str);
    UNUSED(val);
#endif
    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

/**
 * Convert IP VLAN flag value @p val to string @p str.
 *
 * @return Status code
 */
static te_errno
ipvlan_flag_val2str(uint32_t val, const char **str)
{
#ifdef IFLA_IPVLAN_MAX
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(ipvlan_flags); i++)
    {
        if (val == ipvlan_flags[i].val)
        {
            *str = ipvlan_flags[i].str;
            return 0;
        }
    }

    ERROR("Unknown IP VLAN flag %"TE_PRINTF_32"u", val);
#else
    UNUSED(str);
    UNUSED(val);
#endif
    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

/**
 * Add a new IP VLAN interface or modify existing one.
 *
 * @param ctx       Request context
 * @param values    IP VLAN mode and flag (separated by ':')
 * @param cmd       Command to execute: NETCONF_CMD_ADD or NETCONF_CMD_CHANGE
 *
 * @return          Status code
 */
static te_errno
ipvlan_modify(const ta_conf_ctx *ctx, const char *values, netconf_cmd cmd)
{
    const char *link = ta_conf_ctx_inst(ctx, "interface");
    const char *ifname = ta_conf_ctx_inst(ctx, "ipvlan");
    uint32_t    mode = 0;
    uint32_t    flag = 0;
    char        str[RCF_MAX_VAL] = {0};
    char       *val = NULL;
    te_errno    rc;

    if (values == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    TE_STRLCPY(str, values, sizeof(str));

    if ((val = strtok(str, ":")) == NULL)
    {
        ERROR("%s(): unexpected mode value in '%s'", __FUNCTION__, values);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((rc = ipvlan_mode_str2val(val, &mode)) != 0)
        return rc;

    if ((val = strtok(NULL, ":")) == NULL)
    {
        ERROR("%s(): unexpected flag value in '%s'", __FUNCTION__, values);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((rc = ipvlan_flag_str2val(val, &flag)) != 0)
        return rc;

    return netconf_ipvlan_modify(nh, cmd, link, ifname, mode, flag);
}

/**
 * Add a new IP VLAN interface.
 *
 * @param ctx       Request context
 * @param val       IP VLAN mode and flag (separated by ':')
 *
 * @return          Status code
 */
static te_errno
ipvlan_add(const ta_conf_ctx *ctx, const char *val)
{
    return ipvlan_modify(ctx, val, NETCONF_CMD_ADD);
}

/**
 * Delete an IP VLAN interface.
 *
 * @param ctx       Request context
 *
 * @return          Status code
 */
static te_errno
ipvlan_del(const ta_conf_ctx *ctx)
{
    const char *link = ta_conf_ctx_inst(ctx, "interface");
    const char *ifname = ta_conf_ctx_inst(ctx, "ipvlan");

    return netconf_ipvlan_modify(nh, NETCONF_CMD_DEL, link, ifname, 0, 0);
}

/**
 * Set IP VLAN interface mode.
 *
 * @param ctx       Request context
 * @param val       IP VLAN mode and flag (separated by ':')
 *
 * @return          Status code
 */
static te_errno
ipvlan_set(const ta_conf_ctx *ctx, const char *val)
{
    return ipvlan_modify(ctx, val, NETCONF_CMD_CHANGE);
}

/**
 * Get IP VLAN interface mode.
 *
 * @param ctx   Request context
 * @param val   Location for the mode:flag string
 *
 * @return      Status code
 */
static te_errno
ipvlan_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "ipvlan");
    uint32_t    mode;
    uint32_t    flag;
    const char *mode_str;
    const char *flag_str;
    te_errno    rc;

    if ((rc = netconf_ipvlan_get_mode(nh, ifname, &mode, &flag)) != 0)
        return rc;

    if ((rc = ipvlan_mode_val2str(mode, &mode_str)) != 0)
        return rc;
    if ((rc = ipvlan_flag_val2str(flag, &flag_str)) != 0)
        return rc;

    /* Build string, for example "l2:private" */
    te_string_append(val, "%s:%s", mode_str, flag_str);
    return 0;
}

/**
 * Get IP VLAN interfaces list.
 *
 * @param ctx     Request context (parent instance OID)
 * @param names   Vector of heap-allocated names to append to
 *
 * @return        Status code
 */
static te_errno
ipvlan_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *link = ta_conf_ctx_inst(ctx, "interface");

    return netconf_ipvlan_list(nh, link, names);
}

static const ta_conf_node *const node_ipvlan =
    TA_CONF_COLL_STR_RW("ipvlan", ipvlan_get, ipvlan_set,
                        ipvlan_add, ipvlan_del, ipvlan_list);

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_ipvlan_init(void)
{
    return ta_conf_register("/agent/interface", node_ipvlan);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_ipvlan_init(void)
{
    ERROR("IP VLAN interface configuration is not supported");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
}
#endif /* !USE_LIBNETCONF */
