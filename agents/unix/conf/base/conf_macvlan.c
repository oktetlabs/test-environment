/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief MAC VLAN configuration support
 *
 * Implementation of configuration nodes MAC VLAN interfaces.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Unix Conf MAC VLAN"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(USE_LIBNETCONF)

#include "conf_netconf.h"
#include "logger_api.h"
#include "rcf_pch_tree.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"

#include "netconf.h"

/**
 * Add a new MAC VLAN interface.
 *
 * @param ctx       Request context
 * @param val       Macvlan mode
 *
 * @return      Status code
 */
static te_errno
macvlan_add(const ta_conf_ctx *ctx, const char *val)
{
    const char *link = ta_conf_ctx_inst(ctx, "interface");
    const char *ifname = ta_conf_ctx_inst(ctx, "macvlan");

    return netconf_macvlan_modify(nh, NETCONF_CMD_ADD, link, ifname, val);
}

/**
 * Delete a MAC VLAN interface.
 *
 * @param ctx       Request context
 *
 * @return      Status code
 */
static te_errno
macvlan_del(const ta_conf_ctx *ctx)
{
    const char *link = ta_conf_ctx_inst(ctx, "interface");
    const char *ifname = ta_conf_ctx_inst(ctx, "macvlan");

    return netconf_macvlan_modify(nh, NETCONF_CMD_DEL, link, ifname, 0);
}

/**
 * Set MAC VLAN interface mode.
 *
 * @param ctx       Request context
 * @param val       Macvlan mode
 *
 * @return      Status code
 */
static te_errno
macvlan_set(const ta_conf_ctx *ctx, const char *val)
{
    const char *link = ta_conf_ctx_inst(ctx, "interface");
    const char *ifname = ta_conf_ctx_inst(ctx, "macvlan");

    return netconf_macvlan_modify(nh, NETCONF_CMD_CHANGE, link, ifname, val);
}

/**
 * Get MAC VLAN interface mode.
 *
 * @param ctx   Request context
 * @param val   Location for the MAC VLAN mode
 *
 * @return      Status code
 */
static te_errno
macvlan_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "macvlan");
    const char *mode_str;
    te_errno    rc;

    rc = netconf_macvlan_get_mode(nh, ifname, &mode_str);
    if (rc != 0)
        return rc;

    te_string_append(val, "%s", mode_str);

    return 0;
}

/**
 * Get MAC VLAN interfaces list.
 *
 * @param ctx     Request context (parent instance OID)
 * @param names   Vector of heap-allocated names to append to
 *
 * @return      Status code
 */
static te_errno
macvlan_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *link = ta_conf_ctx_inst(ctx, "interface");

    return netconf_macvlan_list(nh, link, names);
}

static const ta_conf_node *const node_macvlan =
    TA_CONF_COLL_STR_RW("macvlan", macvlan_get, macvlan_set,
                        macvlan_add, macvlan_del, macvlan_list);

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_macvlan_init(void)
{
    return ta_conf_register("/agent/interface", node_macvlan);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_macvlan_init(void)
{
    INFO("MAC VLAN interface configuration is not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF */
