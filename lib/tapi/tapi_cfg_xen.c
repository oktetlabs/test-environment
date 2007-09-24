/** @file
 * @brief Test API to configure XEN.
 *
 * Implementation of API to configure XEN.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id:
 */

#define TE_LGR_USER     "TAPI CFG XEN"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_sockaddr.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_xen.h"

#define DHCP_MAGIC_SIZE        4

#define SERVER_ID_OPTION       54
#define REQUEST_IP_ADDR_OPTION 50

/* Ethernet address length */
#define ETHER_ADDR_LEN 6

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_get_path(char const *ta, char *path)
{
    cfg_val_type type  = CVT_STRING;
    char const  *value;
    te_errno     rc    = cfg_get_instance_fmt(&type, &value,
                                              "/agent:%s/xen:", ta);

    if (rc == 0)
    {
        strcpy(path, value);
        free((void *)value);
    }
    else
        ERROR("Failed to get XEN path on %s", ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_set_path(char const *ta, char const *path)
{
    te_errno rc = cfg_set_instance_fmt(CFG_VAL(STRING, path),
                                       "/agent:%s/xen:",
                                       ta);

    if (rc != 0)
        ERROR("Failed to set XEN path on %s", ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_create_dom_u(char const *ta, char const *dom_u)
{
    /* Create domU and its directory/disk images in XEN storage */
    te_errno rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 1),
                                       "/agent:%s/xen:/dom_u:%s",
                                       ta, dom_u);

    if (rc != 0)
        ERROR("Failed to create domU %s on %s", dom_u, ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_destroy_dom_u(char const *ta, char const *dom_u)
{
    /* Ensure that domU is in "non-running" state */
    te_errno rc = cfg_set_instance_fmt(CFG_VAL(STRING, "non-running"),
                                       "/agent:%s/xen:/dom_u:%s/status:",
                                       ta, dom_u);

    if (rc != 0)
    {
        ERROR("Failed to shutdown domU %s on %s", dom_u, ta);
        goto cleanup0;
    }

    /* Remove directory/disk images of domU from XEN storage */
    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to remove directory/images of domU %s on %s",
              dom_u, ta);
        goto cleanup0;
    }

    /* Destroy domU */
    if ((rc = cfg_del_instance_fmt(FALSE,
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to destroy domU %s on %", dom_u, ta);
    }

cleanup0:
    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_get_status(char const *ta, char const *dom_u,
                              char *status)
{
    cfg_val_type type  = CVT_STRING;
    char const  *value;

    te_errno rc = cfg_get_instance_fmt(&type, &value,
                                       "/agent:%s/xen:/dom_u:%s/status:",
                                       ta, dom_u);

    if (rc == 0)
    {
        strcpy(status, value);
        free((void *)value);
    }
    else
        ERROR("Failed to get status for domU %s on %s", dom_u, ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_set_status(char const *ta, char const *dom_u,
                              char const *status)
{
    te_errno rc = cfg_set_instance_fmt(CFG_VAL(STRING, status),
                                       "/agent:%s/xen:/dom_u:%s/status:",
                                       ta, dom_u);

    if (rc != 0)
        ERROR("Failed to set status for domU %s on %s, rc = %d: %r", dom_u, ta, rc, rc);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_get_ip_addr(char const *ta, char const *dom_u,
                               struct sockaddr *ip_addr)
{
    cfg_val_type           type  = CVT_ADDRESS;
    struct sockaddr const *value;

    te_errno rc = cfg_get_instance_fmt(&type, &value,
                                       "/agent:%s/xen:/dom_u:%s/ip_addr:",
                                       ta, dom_u);

    if (rc == 0)
    {
        memcpy(ip_addr, value, sizeof(*value));
        free((void *)value);
    }
    else
        ERROR("Failed to get IP address for domU %s on %s", dom_u, ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_set_ip_addr(char const *ta, char const *dom_u,
                               struct sockaddr const *ip_addr)
{
    te_errno rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, ip_addr),
                                       "/agent:%s/xen:/dom_u:%s/ip_addr:",
                                       ta, dom_u);

    if (rc != 0)
        ERROR("Failed to set IP address for domU %s on %s", dom_u, ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_get_mac_addr(char const *ta, char const *dom_u, uint8_t *mac)
{
    cfg_val_type     type = CVT_ADDRESS;
    struct sockaddr *addr;

    te_errno rc = cfg_get_instance_fmt(&type, &addr,
                                       "/agent:%s/xen:/dom_u:%s/mac_addr:",
                                       ta, dom_u);

    if (rc == 0)
    {
        memcpy(mac, addr->sa_data, ETHER_ADDR_LEN);
        free(addr);
    }
    else
        ERROR("Failed to get MAC address of domU %s on %s", dom_u, ta);

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_set_mac_addr(char const *ta, char const *dom_u, uint8_t const *mac)
{
    struct sockaddr addr = { .sa_family = AF_LOCAL };
    te_errno        rc;

    memcpy(addr.sa_data, mac, ETHER_ADDR_LEN);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, &addr),
                                   "/agent:%s/xen:/dom_u:%s/mac_addr:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to set MAC address of domU %s on %s", dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_migrate(char const *from_ta, char const *to_ta,
                           char const *dom_u, char const *host,
                           te_bool live)
{
    cfg_val_type  type1 = CVT_STRING;
    char const   *xen_path1;
    char const   *xen_path2;

    uint8_t         mac[ETHER_ADDR_LEN];
    struct sockaddr ip;

    te_errno rc;

    /* Cannot migrate to itself */
    if (strcmp(from_ta, to_ta) == 0)
    {
        ERROR("Failed to migrate from %s to itself", from_ta);
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup0;
    }

    /* Check that XEN paths are identical for both dom0 agents */
    if ((rc = cfg_get_instance_fmt(&type1, &xen_path1,
                                   "/agent:%s/xen:", from_ta)) != 0)
    {
        ERROR("Failed to get XEN path on %s", from_ta);
        goto cleanup0;
    }

    if ((rc = cfg_get_instance_fmt(&type1, &xen_path2,
                                   "/agent:%s/xen:", to_ta)) != 0)
    {
        ERROR("Failed to get XEN path on %s", to_ta);
        goto cleanup1;
    }

    if (strcmp(xen_path1, xen_path2) != 0)
    {
        ERROR("XEN path differs between %s and %s", from_ta, to_ta);
        goto cleanup2;
    }

    /* Save MAC address */
    if ((rc = tapi_cfg_xen_dom_u_get_mac_addr(from_ta, dom_u, mac)) != 0)
    {
        ERROR("Failed to get domU %s MAC address on %s", dom_u, from_ta);
        goto cleanup2;
    }

    /* Save IP address */
    if ((rc = tapi_cfg_xen_dom_u_get_ip_addr(from_ta, dom_u, &ip)) != 0)
    {
        ERROR("Failed to get domU %s IP address on %s", dom_u, from_ta);
        goto cleanup2;
    }

    /* Set kind of migration (live/non-live) */
    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, live),
                                   "/agent:%s/xen:/dom_u:%s/"
                                   "migrate:/kind:",
                                   from_ta, dom_u)) != 0)
    {
        ERROR("Failed to set migrate kind for domU %s on %s",
              dom_u, from_ta);
        goto cleanup2;
    }

    /* Perform migration */
    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, host),
                                   "/agent:%s/xen:/dom_u:%s/migrate:",
                                   from_ta, dom_u)) != 0)
    {
        ERROR("Failed to migrate domU %s from %s to %s (host %s)",
              dom_u, from_ta, to_ta, host);
        goto cleanup2;
    }

    /* Delete domU item from the source agent configurator tree */
    if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/xen:/dom_u:%s",
                                   from_ta, dom_u)) != 0)
    {
        ERROR("Failed to destroy domU %s on %s", dom_u, from_ta);
        goto cleanup2;
    }

    /* Create domU on target agent (domU will have "non-running" state) */
    if ((rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 1),
                                   "/agent:%s/xen:/dom_u:%s",
                                   to_ta, dom_u)) != 0)
    {
        ERROR("Failed to accept domU %s migrated from %s to %s "
              "(host %s)", dom_u, from_ta, to_ta, host);
        goto cleanup2;
    }

    /* Set MAC address saved previously */
    if ((rc = tapi_cfg_xen_dom_u_set_mac_addr(to_ta, dom_u, mac)) != 0)
    {
        ERROR("Failed to set MAC address for domU %s on %s",
              dom_u, to_ta);
        goto cleanup2;
    }

    /* Set IP address saved previously */
    if ((rc = tapi_cfg_xen_dom_u_set_ip_addr(to_ta, dom_u, &ip)) != 0)
    {
        ERROR("Failed to set IP address " "%d.%d.%d.%d for domU %s on %s",
              ((ntohl(SIN(&ip)->sin_addr.s_addr)) >> 24) % 256,
              ((ntohl(SIN(&ip)->sin_addr.s_addr)) >> 16) % 256,
              ((ntohl(SIN(&ip)->sin_addr.s_addr)) >> 8) % 256,
              ((ntohl(SIN(&ip)->sin_addr.s_addr)) >> 0) % 256,
              dom_u, to_ta);
        goto cleanup2;
    }

    /**
     * Set "migrated" pseudo-status; that is set domU status to
     * "running" in case migration has proceeded successfully and
     * domU exists within dom0, that is controlled by the target agent
     */
    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, "migrated"),
                                   "/agent:%s/xen:/dom_u:%s/status:",
                                   to_ta, dom_u)) != 0)
    {
        ERROR("Failed to set migrated pseudo-status for domU %s on %s",
              dom_u, to_ta);
    }

cleanup2:
    free((void *)xen_path2);

cleanup1:
    free((void *)xen_path1);

cleanup0:
    return rc;
}

