/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Virtual Ethernet (veth) interface configuration support
 *
 * Implementation of configuration nodes VETH interfaces.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Unix Conf VETH"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(USE_LIBNETCONF)

#include "conf_netconf.h"
#include "logger_api.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "unix_internal.h"

#include "netconf.h"

/**
 * Add a new veth interface.
 *
 * @param ctx       Request context
 * @param val       Peer interface name
 *
 * @return      Status code
 */
static te_errno
veth_add(const ta_conf_ctx *ctx, const char *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "veth");

    return netconf_veth_add(nh, ifname, val);
}

/**
 * Delete a veth interface.
 *
 * @param ctx       Request context
 *
 * @return      Status code
 */
static te_errno
veth_del(const ta_conf_ctx *ctx)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "veth");

    return netconf_veth_del(nh, ifname);
}

/**
 * Get veth peer interface name.
 *
 * @param ctx       Request context
 * @param val       Location for the peer interface name
 *
 * @return      Status code
 */
static te_errno
veth_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "veth");
    char peer[RCF_MAX_VAL];
    te_errno rc;

    rc = netconf_veth_get_peer(nh, ifname, peer, sizeof(peer));
    if (rc != 0)
        return rc;

    te_string_append(val, "%s", peer);
    return 0;
}

/**
 * Check whether a given interface is grabbed by TA when creating a list of
 * veth interfaces.
 *
 * @param ifname    The interface name.
 * @param data      Unused.
 *
 * @return @c true if the interface is grabbed, @c false otherwise.
 */
static bool
veth_list_include_cb(const char *ifname, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible("/agent:%s/veth:%s", ta_name, ifname);
}

/**
 * Get veth interfaces list.
 *
 * @param ctx       Request context (unused)
 * @param names     Vector of heap-allocated names to append to
 *
 * @return      Status code
 */
static te_errno
veth_list(const ta_conf_ctx *ctx, te_vec *names)
{
    UNUSED(ctx);

    return netconf_veth_list(nh, veth_list_include_cb, NULL, names);
}

static const ta_conf_node *const node_veth =
    TA_CONF_COLL_STR("veth", veth_get, veth_add, veth_del, veth_list);

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_veth_init(void)
{
    te_errno rc;

    rc = ta_conf_register("/agent", node_veth);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/veth",
                             rcf_pch_rsrc_grab_dummy,
                             rcf_pch_rsrc_release_dummy);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_veth_init(void)
{
    INFO("VETH interface configuration is not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF */
