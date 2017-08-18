/** @file
 * @brief Virtual Ethernet (veth) interface configuration support
 *
 * Implementation of configuration nodes VETH interfaces.
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
 * Get veth interfaces list.
 *
 * @param gid   Group identifier (unused)
 * @param oid   Full identifier of the father instance (unused)
 * @param list  Location for the list pointer
 *
 * @return      Status code
 */
static te_errno
veth_list(unsigned int gid, const char *oid, char **list)
{
    UNUSED(gid);
    UNUSED(oid);

    return netconf_veth_list(nh, list);
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
    return rcf_pch_add_node("/agent/", &node_veth);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_veth_init(void)
{
    INFO("VETH interface configuration is not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF */
