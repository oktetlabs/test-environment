/** @file
 * @brief Test API to configure ethernet interface.
 *
 * Definition of API to configure ethernet interface.
 * 
 * Copyright (C) 2004-20013 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrey A. Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 * 
 * $Id$
 */

#define TE_LGR_USER     "Ethernet configuration TAPI"

#include "te_config.h"
#include "conf_api.h"
#include "tapi_cfg_eth.h"
#include "logger_api.h"
#include "te_ethernet.h"

#define TE_CFG_TA_ETH_FMT "/agent:%s/interface:%s"

/**
 * Get integer value of an interface field
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param field     Field name
 * @param val       The value location
 * 
 * @return Status code
 */
static te_errno
tapi_eth_common_get(const char *ta, const char *ifname, const char *field,
                    int *val)
{
    cfg_val_type type = CVT_INTEGER;
    int          rc;

    if ((rc = cfg_get_instance_fmt(&type, val, TE_CFG_TA_ETH_FMT "/%s:",
                                   ta, ifname, field)) != 0)
        ERROR("Failed to get %s value: %r", field, rc);

    return rc;
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_link_get(const char *ta, const char *ifname, int *link)
{
    return tapi_eth_common_get(ta, ifname, "link", link);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_gro_get(const char *ta, const char *ifname, int *gro)
{
    return tapi_eth_common_get(ta, ifname, "gro", gro);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_gso_get(const char *ta, const char *ifname, int *gso)
{
    return tapi_eth_common_get(ta, ifname, "gso", gso);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_tso_get(const char *ta, const char *ifname, int *tso)
{
    return tapi_eth_common_get(ta, ifname, "tso", tso);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_flags_get(const char *ta, const char *ifname, int *flags)
{
    return tapi_eth_common_get(ta, ifname, "flags", flags);
}

/**
 * Set integer value of an interface field
 * 
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param field     Field name
 * @param val       The new value
 * 
 * @return Status code
 */
static te_errno
tapi_eth_common_set(const char *ta, const char *ifname, const char *field,
                    int val)
{
    te_errno rc;

    if ((rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)val,
                                   TE_CFG_TA_ETH_FMT "/%s:", ta,
                                   ifname, field)) != 0)
        ERROR("Failed to set %s value: %r", field, rc);

    return rc;
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_gro_set(const char *ta, const char *ifname, int gro)
{
    return tapi_eth_common_set(ta, ifname, "gro", gro);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_gso_set(const char *ta, const char *ifname, int gso)
{
    return tapi_eth_common_set(ta, ifname, "gso", gso);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_tso_set(const char *ta, const char *ifname, int tso)
{
    return tapi_eth_common_set(ta, ifname, "tso", tso);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_flags_set(const char *ta, const char *ifname, int flags)
{
    return tapi_eth_common_set(ta, ifname, "flags", flags);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_reset(const char *ta, const char *ifname)
{
    return tapi_eth_common_set(ta, ifname, "reset", 1);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_permaddr_get(const char *ta, const char *ifname,
                      unsigned char *o_addr)
{
    cfg_val_type        type = CVT_ADDRESS;
    int                 rc;
    struct sockaddr    *addr = NULL;

    if ((rc = cfg_get_instance_fmt(&type, &addr,
                                   TE_CFG_TA_ETH_FMT "/permaddr:",
                                   ta, ifname)) != 0)
        ERROR("Failed to get hardware address: %r", rc);
    else
    {
        memcpy(o_addr, addr->sa_data, ETHER_ADDR_LEN);
        free(addr);
    }

    return rc;
}
