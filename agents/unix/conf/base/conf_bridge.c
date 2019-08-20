/** @file
 * @brief Bridge interface configuration support.
 *
 * Implementation of configuration nodes Bridge interfaces.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Georgiy Levashov <Georgiy.Levashov@oktetlabs.ru>
 */

#define TE_LGR_USER     "Unix Conf Bridge"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(USE_LIBNETCONF)

#include "conf_netconf.h"
#include "rcf_pch.h"
#include "te_defs.h"
#include "unix_internal.h"

#include "netconf.h"

/**
 * Check whether a given interface is grabbed by TA when creating a list of
 * port interfaces.
 *
 * @param ifname    The interface name.
 * @param brname    The bridge name
 * @param data      Unused.
 *
 * @return @c TRUE if the interface is grabbed, @c FALSE otherwise.
 */
static te_bool
port_list_include_cb(const char *ifname, const char *brname, void *data)
{
    UNUSED(ifname);
    UNUSED(brname);
    UNUSED(data);

    return 0;
}


/**
 * Get port interfaces list.
 *
 * @param gid     Group identifier (unused)
 * @param oid     Full identifier of the father instance (unused)
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    Location for the list pointer
 *
 * @return      Status code
 */
static te_errno
port_list(unsigned int gid, const char *oid,
          const char *sub_id, char **list)
{
    const char *brname;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    brname = CFG_OID_GET_INST_NAME(cfg_convert_oid_str(oid), 2);
    return netconf_port_list(nh, brname, port_list_include_cb, NULL, list);
}

/**
 * Get port interface OID.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param if_oid    Buffer for port interface OID
 * @param brname    The bridge name (unused)
 * @param ifname    The inteface name
 *
 * @return      Status code
 */
static te_errno
port_get(unsigned int gid, char *oid, char *if_oid, char *brname, char *ifname)
{
    int rc;
    te_string str = TE_STRING_EXT_BUF_INIT(if_oid, RCF_MAX_VAL);

    UNUSED(gid);
    UNUSED(oid);

    if (rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname))
    {
        rc = te_string_append(&str, "/agent:%s/interface:%s", ta_name, ifname);
        if (rc != 0)
        {
            return TE_RC(TE_TA_UNIX, rc);
        }
    }
    return 0;
}

/**
 * Add a new bridge port interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @paran data      Unused
 * @param brname    The bridge inteface name
 * @param ifname    The interface name
 *
 * @return      Status code
 */
static te_errno
port_add(unsigned int gid, const char *oid, const char *data, const char *brname, const char *ifname)
{
    char if_oid[RCF_MAX_VAL];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(data);

    snprintf(if_oid, sizeof(if_oid), "/agent:%s/ interface:%s", ta_name, ifname);
    if (strcmp(data, if_oid) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    if (!rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname))
        return TE_RC(TE_TA_UNIX, TE_EACCES);

    return netconf_port_add(nh, brname, ifname);
}

/**
 * Delete a bridge interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param brname    The bridge interface name (unused)
 * @param ifname    The interface name
 *
 * @return      Status code
 */
static te_errno
port_del(unsigned int gid, const char *oid,
         const char *brname, const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(brname);

    if (!rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname))
        return TE_RC(TE_TA_UNIX, TE_EACCES);

    return netconf_port_del(nh, ifname);
}

RCF_PCH_CFG_NODE_RW_COLLECTION(node_port, "port", NULL, NULL,
                               port_get, NULL,
                               port_add, port_del, port_list,
                               NULL);

/**
 * Add a new bridge interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param ifname    The inteface name
 *
 * @return      Status code
 */
static te_errno
bridge_add(unsigned int gid, const char *oid, const char *data, const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    return netconf_bridge_add(nh, ifname);
}

/**
 * Delete a bridge interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param ifname    The interface name
 *
 * @return      Status code
 */
static te_errno
bridge_del(unsigned int gid, const char *oid, const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    return netconf_bridge_del(nh, ifname);
}

/**
 * Check whether a given interface is grabbed by TA when creating a list of
 * bridge interfaces.
 *
 * @param ifname    The interface name.
 * @param data      Unused.
 *
 * @return @c TRUE if the interface is grabbed, @c FALSE otherwise.
 */
static te_bool
bridge_list_include_cb(const char *ifname, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname);
}


/**
 * Get bridge interfaces list.
 *
 * @param gid     Group identifier (unused)
 * @param oid     Full identifier of the father instance (unused)
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    Location for the list pointer
 *
 * @return      Status code
 */
static te_errno
bridge_list(unsigned int gid, const char *oid,
            const char *sub_id, char **list)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    return netconf_bridge_list(nh, bridge_list_include_cb, NULL, list);
}

RCF_PCH_CFG_NODE_RW_COLLECTION(node_bridge, "bridge", &node_port, NULL,
                              NULL, NULL,
                              bridge_add, bridge_del, bridge_list,
                              NULL);

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_bridge_init(void)
{
    return rcf_pch_add_node("/agent", &node_bridge);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_bridge_init(void)
{
    INFO("Bridge interface configuration is not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF */
