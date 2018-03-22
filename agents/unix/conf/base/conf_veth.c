/** @file
 * @brief Virtual Ethernet (veth) interface configuration support
 *
 * Implementation of configuration nodes VETH interfaces.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#define TE_LGR_USER     "Unix Conf VETH"

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
#include "unix_internal.h"

#include "netconf.h"

/**
 * Add a new veth interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param peer      Peer interface name
 * @param ifname    The inteface name
 *
 * @return      Status code
 */
static te_errno
veth_add(unsigned int gid, const char *oid, const char *peer,
            const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    return netconf_veth_add(nh, ifname, peer);
}

/**
 * Delete a veth interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param ifname    The interface name
 *
 * @return      Status code
 */
static te_errno
veth_del(unsigned int gid, const char *oid, const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    return netconf_veth_del(nh, ifname);
}

/**
 * Get veth peer interface name.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param peer      Peer interface name
 * @param ifname    The inteface name
 *
 * @return      Status code
 */
static te_errno
veth_get(unsigned int gid, const char *oid, char *peer, const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    return netconf_veth_get_peer(nh, ifname, peer, RCF_MAX_VAL);
}

/**
 * Check whether a given interface is grabbed by TA when creating a list of
 * veth interfaces.
 *
 * @param ifname    The interface name.
 * @param data      Unused.
 *
 * @return @c TRUE if the interface is grabbed, @c FALSE otherwise.
 */
static te_bool
veth_list_include_cb(const char *ifname, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible("/agent:%s/veth:%s", ta_name, ifname);
}

/**
 * Get veth interfaces list.
 *
 * @param gid     Group identifier (unused)
 * @param oid     Full identifier of the father instance (unused)
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    Location for the list pointer
 *
 * @return      Status code
 */
static te_errno
veth_list(unsigned int gid, const char *oid,
          const char *sub_id, char **list)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    return netconf_veth_list(nh, veth_list_include_cb, NULL, list);
}

static rcf_pch_cfg_object node_veth =
    { "veth", 0, NULL, NULL,
      (rcf_ch_cfg_get)veth_get, NULL,
      (rcf_ch_cfg_add)veth_add, (rcf_ch_cfg_del)veth_del,
      (rcf_ch_cfg_list)veth_list, NULL, NULL };

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_veth_init(void)
{
    te_errno rc;

    rc = rcf_pch_add_node("/agent/", &node_veth);
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
