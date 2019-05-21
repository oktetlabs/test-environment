/** @file
 * @brief Virtual eXtensible Local Area Network (vxlan)
 * interface configuration support
 *
 * Implementation of configuration nodes VXLAN interfaces.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Ian Dolzhansky <Ian.Dolzhansky@oktetlabs.ru>
 */

#define TE_LGR_USER     "Unix Conf VXLAN"

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
 * vxlan interfaces.
 *
 * @param ifname    The interface name.
 * @param data      Unused.
 *
 * @return @c TRUE if the interface is grabbed, @c FALSE otherwise.
 */
static te_bool
vxlan_list_include_cb(const char *ifname, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname);
}

/**
 * Get vxlan interfaces list.
 *
 * @param gid     Group identifier (unused)
 * @param oid     Full identifier of the father instance (unused)
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    Location for the list pointer
 *
 * @return      Status code
 */
static te_errno
vxlan_list(unsigned int gid, const char *oid,
           const char *sub_id, char **list)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    return netconf_vxlan_list(nh, vxlan_list_include_cb, NULL, list);
}

RCF_PCH_CFG_NODE_RO_COLLECTION(node_vxlan, "vxlan", NULL, NULL,
                               NULL, vxlan_list);

RCF_PCH_CFG_NODE_NA(node_tunnel, "tunnel", &node_vxlan, NULL);

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_vxlan_init(void)
{
    return rcf_pch_add_node("/agent", &node_tunnel);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_vxlan_init(void)
{
    INFO("VXLAN interface configuration is not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF */

