/** @file
 * @brief Unix Test Agent
 *
 * Extra ethernet interface configurations
 *
 * Copyright (C) 2004-2013 Test Environment authors (see file AUTHORS
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
 * @author Andrey A. Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 * 
 * $Id$
 */

#define TE_LGR_USER     "Extra eth Conf"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#include "logger_api.h"
#include "unix_internal.h"

#if HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#include "te_rpc_sys_socket.h"

#ifdef HAVE_LINUX_ETHTOOL_H
/**
 * Get single ethernet interface configuration value.
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier
 * @param value        Location to save value
 * @param ifname       Interface name
 *
 * @return Status code
 */
static te_errno
eth_cmd_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    struct ifreq            ifr;
    struct ethtool_value    eval;

    UNUSED(gid);

    memset(&ifr, 0, sizeof(ifr));
    memset(&eval, 0, sizeof(eval));

    ifr.ifr_data = (void *)&eval;
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    if (strstr(oid, "/gro:") != NULL)
        eval.cmd = ETHTOOL_GGRO;
    else if (strstr(oid, "/gso:") != NULL)
        eval.cmd = ETHTOOL_GGSO;
    else if (strstr(oid, "/tso:") != NULL)
        eval.cmd = ETHTOOL_GTSO;
    else if (strstr(oid, "/flags:") != NULL)
        eval.cmd = ETHTOOL_GFLAGS;
    else
        return TE_EINVAL;

    if (ioctl(cfg_socket, SIOCETHTOOL, &ifr) != 0)
    {
        ERROR("ioctl failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    if (snprintf(value, RCF_MAX_VAL, "%d", eval.data) < 0)
    {
        ERROR("failed to write value to buffer: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/**
 * Set single ethernet interface configuration value.
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier
 * @param value        New value
 * @param ifname       Interface name
 *
 * @return Status code
 */
static te_errno
eth_cmd_set(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    struct ifreq            ifr;
    struct ethtool_value    eval;
    char                   *endp;

    UNUSED(gid);

    memset(&ifr, 0, sizeof(ifr));
    memset(&eval, 0, sizeof(eval));

    ifr.ifr_data = (void *)&eval;
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    eval.data = strtoul(value, &endp, 0);
    if (endp == value || *endp != '\0')
    {
        ERROR("Couldn't convert value '%s' to number, errno %r", value,
              TE_OS_RC(TE_TA_UNIX, errno));
        return TE_EINVAL;
    }

    if (strstr(oid, "/gro:") != NULL)
        eval.cmd = ETHTOOL_SGRO;
    else if (strstr(oid, "/gso:") != NULL)
        eval.cmd = ETHTOOL_SGSO;
    else if (strstr(oid, "/tso:") != NULL)
        eval.cmd = ETHTOOL_STSO;
    else if (strstr(oid, "/flags:") != NULL)
        eval.cmd = ETHTOOL_SFLAGS;
    else if (strstr(oid, "/reset:") != NULL)
    {
        if (eval.data == 0)
            return 0;
        eval.cmd = RPC_ETHTOOL_RESET;
        eval.data = RPC_ETH_RESET_ALL;
    }
    else
        return TE_EINVAL;

    if (ioctl(cfg_socket, SIOCETHTOOL, &ifr) != 0)
    {
        ERROR("ioctl failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/**
 * Get reset value (dummy)
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier
 * @param value        Location to save value
 * @param ifname       Interface name
 *
 * @return Status code
 */
static te_errno
eth_reset_get(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);

    *value = 0;

    return 0;
}

RCF_PCH_CFG_NODE_RW(eth_reset, "reset", NULL, NULL,
                    eth_reset_get, eth_cmd_set);

RCF_PCH_CFG_NODE_RW(eth_gro, "gro", NULL, &eth_reset,
                    eth_cmd_get, eth_cmd_set);

RCF_PCH_CFG_NODE_RW(eth_gso, "gso", NULL, &eth_gro,
                    eth_cmd_get, eth_cmd_set);

RCF_PCH_CFG_NODE_RW(eth_tso, "tso", NULL, &eth_gso,
                    eth_cmd_get, eth_cmd_set);

RCF_PCH_CFG_NODE_RW(eth_flags, "flags", NULL, &eth_tso,
                    eth_cmd_get, eth_cmd_set);

/**
 * Initialize ethernet interface configuration nodes
 */
te_errno
ta_unix_conf_eth_init(void)
{
    return rcf_pch_add_node("/agent/interface", &eth_flags);
}
#else
te_errno
ta_unix_conf_eth_init(void)
{
    INFO("Extra ethernet interface configurations are not supported");
    return 0;
}
#endif /* !HAVE_LINUX_ETHTOOL_H */

