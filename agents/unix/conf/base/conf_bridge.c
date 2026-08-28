/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Bridge interface configuration support.
 *
 * Implementation of configuration nodes Bridge interfaces.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Unix Conf Bridge"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(USE_LIBNETCONF)

#include "conf_netconf.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"
#include "te_defs.h"
#include "te_string.h"
#include "te_vector.h"
#include "unix_internal.h"

#include "netconf.h"

/**
 * Check whether a given interface is grabbed by TA when creating a list of
 * port interfaces.
 *
 * @param ifname    The interface name.
 * @param data      Unused.
 *
 * @return @c true if the interface is grabbed, @c false otherwise.
 */
static bool
port_list_include_cb(const char *ifname, void *data)
{
    UNUSED(ifname);
    UNUSED(data);

    return 0;
}


/**
 * Get port interfaces list.
 *
 * @param ctx     Request context (parent instance OID)
 * @param names   Vector of heap-allocated names to append to
 *
 * @return      Status code
 */
static te_errno
port_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *brname = ta_conf_ctx_inst(ctx, "bridge");

    return netconf_port_list(nh, brname, port_list_include_cb, NULL, names);
}

/**
 * Get port interface OID.
 *
 * @param ctx       Request context
 * @param val       Location for the retrieved value
 *
 * @return      Status code
 */
static te_errno
port_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "port");

    if (rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname))
    {
        te_string_append(val, "/agent:%s/interface:%s", ta_name, ifname);
    }
    return 0;
}

/**
 * Add a new bridge port interface.
 *
 * @param ctx       Request context
 * @param data      OID of the interface to add as a port
 *
 * @return      Status code
 */
static te_errno
port_add(const ta_conf_ctx *ctx, const char *data)
{
    const char *brname = ta_conf_ctx_inst(ctx, "bridge");
    const char *ifname = ta_conf_ctx_inst(ctx, "port");
    char if_oid[RCF_MAX_VAL];

    snprintf(if_oid, sizeof(if_oid), "/agent:%s/interface:%s", ta_name, ifname);
    if (strcmp(data, if_oid) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    if (!rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname))
        return TE_RC(TE_TA_UNIX, TE_EACCES);

    return netconf_port_add(nh, brname, ifname);
}

/**
 * Delete a bridge interface.
 *
 * @param ctx       Request context
 *
 * @return      Status code
 */
static te_errno
port_del(const ta_conf_ctx *ctx)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "port");

    if (!rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname))
        return TE_RC(TE_TA_UNIX, TE_EACCES);

    return netconf_port_del(nh, ifname);
}

/**
 * Add a new bridge interface.
 *
 * @param ctx       Request context
 *
 * @return      Status code
 */
static te_errno
bridge_add(const ta_conf_ctx *ctx)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "bridge");

    return netconf_bridge_add(nh, ifname);
}

/**
 * Delete a bridge interface.
 *
 * @param ctx       Request context
 *
 * @return      Status code
 */
static te_errno
bridge_del(const ta_conf_ctx *ctx)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "bridge");

    return netconf_bridge_del(nh, ifname);
}

/**
 * Check whether a given interface is grabbed by TA when creating a list of
 * bridge interfaces.
 *
 * @param ifname    The interface name.
 * @param data      Unused.
 *
 * @return @c true if the interface is grabbed, @c false otherwise.
 */
static bool
bridge_list_include_cb(const char *ifname, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname);
}


/**
 * Get bridge interfaces list.
 *
 * @param ctx     Request context (unused)
 * @param names   Vector of heap-allocated names to append to
 *
 * @return      Status code
 */
static te_errno
bridge_list(const ta_conf_ctx *ctx, te_vec *names)
{
    UNUSED(ctx);

    return netconf_bridge_list(nh, bridge_list_include_cb, NULL, names);
}

static const ta_conf_node *const node_bridge =
    TA_CONF_COLL("bridge", bridge_add, bridge_del, bridge_list,
        TA_CONF_COLL_STR("port", port_get, port_add, port_del, port_list));

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_bridge_init(void)
{
    return ta_conf_register("/agent", node_bridge);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_bridge_init(void)
{
    INFO("Bridge interface configuration is not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF */
