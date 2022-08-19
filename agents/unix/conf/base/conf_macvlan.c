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
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"

#include "netconf.h"

/**
 * Add a new MAC VLAN interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param mode      Macvlan mode
 * @param link      Parent (link) interface name
 * @param ifname    The inteface name
 *
 * @return      Status code
 */
static te_errno
macvlan_add(unsigned int gid, const char *oid, const char *mode,
            const char *link, const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    return netconf_macvlan_modify(nh, NETCONF_CMD_ADD, link, ifname, mode);
}

/**
 * Delete a MAC VLAN interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param link      Parent (link) interface name
 * @param ifname    The interface name
 *
 * @return      Status code
 */
static te_errno
macvlan_del(unsigned int gid, const char *oid, const char *link,
            const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    return netconf_macvlan_modify(nh, NETCONF_CMD_DEL, link, ifname, 0);
}

/**
 * Set MAC VLAN interface mode.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param mode      Macvlan mode
 * @param link      Parent (link) interface name
 * @param ifname    The inteface name
 *
 * @return      Status code
 */
static te_errno
macvlan_set(unsigned int gid, const char *oid, const char *mode,
            const char *link, const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    return netconf_macvlan_modify(nh, NETCONF_CMD_CHANGE, link, ifname,
                                  mode);
}

/**
 * Get MAC VLAN interface mode.
 *
 * @param gid   Group identifier (unused)
 * @param oid   Full object instance identifier (unused)
 * @param mode  MAC VLAN mode
 * @param link  Parent (link) interface name
 * @param ifname  The inteface name
 *
 * @return      Status code
 */
static te_errno
macvlan_get(unsigned int gid, const char *oid, char *mode,
            const char *link, const char *ifname)
{
    const char *mode_str;
    te_errno    rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(link);

    rc = netconf_macvlan_get_mode(nh, ifname, &mode_str);
    if (rc != 0)
        return rc;

    strcpy(mode, mode_str);

    return 0;
}

/**
 * Get MAC VLAN interfaces list.
 *
 * @param gid     Group identifier (unused)
 * @param oid     Full identifier of the father instance (unused)
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    Location for the list pointer
 * @param link    Parent (link) interface name
 *
 * @return      Status code
 */
static te_errno
macvlan_list(unsigned int gid, const char *oid,
             const char *sub_id, char **list,
             const char *link)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    return netconf_macvlan_list(nh, link, list);
}

static rcf_pch_cfg_object node_macvlan =
    { "macvlan", 0, NULL, NULL,
      (rcf_ch_cfg_get)macvlan_get, (rcf_ch_cfg_set)macvlan_set,
      (rcf_ch_cfg_add)macvlan_add, (rcf_ch_cfg_del)macvlan_del,
      (rcf_ch_cfg_list)macvlan_list, NULL, NULL, NULL };

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_macvlan_init(void)
{
    return rcf_pch_add_node("/agent/interface/", &node_macvlan);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_macvlan_init(void)
{
    INFO("MAC VLAN interface configuration is not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF */
