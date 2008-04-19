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

/* Ensure PATH_MAX is defined */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <limits.h>

/* Nevertheless PATH_MAX can still be undefined here yet */
#ifndef PATH_MAX
#define PATH_MAX 108
#endif

#endif /* STDC_HEADERS */

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
    te_errno     rc;

    if (ta == NULL || path == NULL)
    {
        ERROR("Failed to get XEN path: Invalid params");
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &value,
                                   "/agent:%s/xen:", ta)) == 0)
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
    te_errno rc;

    if (ta == NULL || path == NULL)
    {
        ERROR("Failed to set XEN path: Invalid params");
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, path),
                                   "/agent:%s/xen:", ta)) != 0)
    {
        ERROR("Failed to set XEN path to '%s' on %s", path, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_get_rcf_port(char const *ta, unsigned int *port)
{
    cfg_val_type  type  = CVT_INTEGER;
    te_errno      rc;

    if (ta == NULL)
    {
        ERROR("Failed to get RCF port on %s: Invalid params", ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, port,
                                   "/agent:%s/xen:/rcf_port:", ta)) != 0)
    {
        ERROR("Failed to get RCF port on %s", ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_set_rcf_port(char const *ta, unsigned int port)
{
    te_errno rc;

    if (ta == NULL)
    {
        ERROR("Failed to set RCF port on %s: Invalid params", ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, port),
                                   "/agent:%s/xen:/rcf_port:", ta)) != 0)
    {
        ERROR("Failed to set RCF port on %s", ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_get_rpc_br(char const *ta, char *br_name)
{
    cfg_val_type type  = CVT_STRING;
    char const  *value;
    te_errno     rc;

    if (ta == NULL || br_name == NULL)
    {
        ERROR("Failed to get RCF/RPC bridge name on %s: "
              "Invalid params", ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &value,
                                   "/agent:%s/xen:/rpc_br:", ta)) != 0)
    {
        ERROR("Failed to get RCF/RPC bridge name on %s", ta);
    }
    else
    {
        strcpy(br_name, value);
        free((void *)value);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_set_rpc_br(char const *ta, char const *br_name)
{
    te_errno rc;

    if (ta == NULL || br_name == NULL)
    {
        ERROR("Failed to set \"%s\" RCF/RPC bridge name on %s: "
              "Invalid params", br_name, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, br_name),
                                   "/agent:%s/xen:/rpc_br:", ta)) != 0)
    {
        ERROR("Failed to set \"%s\" RCF/RPC bridge name on %s: %r",
              br_name, ta, rc);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_get_rpc_if(char const *ta, char *if_name)
{
    cfg_val_type type  = CVT_STRING;
    char const  *value;
    te_errno     rc;

    if (ta == NULL || if_name == NULL)
    {
        ERROR("Failed to get RCF/RPC interface name on %s: "
              "Invalid params", ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &value,
                                   "/agent:%s/xen:/rpc_if:", ta)) != 0)
    {
        ERROR("Failed to get RCF/RPC interface name on %s", ta);
    }
    else
    {
        strcpy(if_name, value);
        free((void *)value);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_set_rpc_if(char const *ta, char const *if_name)
{
    te_errno rc;

    if (ta == NULL || if_name == NULL)
    {
        ERROR("Falied to set \"%s\" RCF/RPC interface name on %s: "
              "Invalid params", if_name, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, if_name),
                                   "/agent:%s/xen:/rpc_if:", ta)) != 0)
    {
        ERROR("Failed to set \"%s\" RCF/RPC interface name on %s: %r",
              if_name, ta, rc);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_get_base_mac_addr(char const *ta, uint8_t *mac)
{
    cfg_val_type     type = CVT_ADDRESS;
    struct sockaddr *addr;
    te_errno         rc;

    if (ta == NULL || mac == NULL)
    {
        ERROR("Failed to get base MAC address on %s: Invalid params", ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &addr,
                                   "/agent:%s/xen:/base_mac_addr:",
                                   ta)) != 0)
    {
        ERROR("Failed to get base MAC address on %s", ta);
    }
    else
    {
        memcpy(mac, addr->sa_data, ETHER_ADDR_LEN);
        free(addr);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_set_base_mac_addr(char const *ta, uint8_t const *mac)
{
    struct sockaddr addr = { .sa_family = AF_LOCAL };
    te_errno        rc;

    if (ta == NULL || mac == NULL)
    {
        ERROR("Failed to set base MAC address on %s: Invalid params", ta);
        return TE_EINVAL;
    }

    memcpy(addr.sa_data, mac, ETHER_ADDR_LEN);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, &addr),
                                   "/agent:%s/xen:/base_mac_addr:",
                                   ta)) != 0)
    {
        ERROR("Failed to set base MAC address on %s", ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_get_accel(char const *ta, te_bool *accel)
{
    cfg_val_type type = CVT_INTEGER;
    int          acceleration;
    te_errno     rc;

    if (ta == NULL || accel == NULL)
    {
        ERROR("Failed to get acceleration on %s: Invalid params", ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &acceleration,
                                   "/agent:%s/xen:/accel:",
                                   ta)) != 0)
    {
        ERROR("Failed to get acceleration on %s", ta);
    }
    else
    {
        *accel = acceleration ? TRUE : FALSE;
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_set_accel(char const *ta, te_bool accel)
{
    te_errno rc;

    if (ta == NULL)
    {
        ERROR("Failed to set acceleration to %s on %s: Invalid params",
              accel ? "TRUE" : "FALSE", ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, accel ? 1 : 0),
                                   "/agent:%s/xen:/accel:",
                                   ta)) != 0)
    {
        ERROR("Failed to set acceleration to %s on %s",
              accel ? "TRUE" : "FALSE", ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_set_init(char const *ta, te_bool init)
{
    te_errno rc;

    if (ta == NULL)
    {
        ERROR("Failed to perform XEN %s on %s: Invalid params",
              init ? "initialization" : "clean up", ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, init ? 1 : 0),
                                   "/agent:%s/xen:/init:",
                                   ta)) != 0)
    {
        ERROR("Failed to perform XEN %s on %s",
              init ? "initialization" : "clean up", ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_create_dom_u(char const *ta, char const *dom_u)
{
    /* Create domU destroying old  directory/disk images in XEN storage */
    te_errno rc;

    if (ta == NULL || dom_u == NULL)
    {
        ERROR("Failed to create '%s' domU on %s: Invalid params",
              dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 0),
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to create '%s' domU on %s destroying old "
              "directory and images in XEN storage", dom_u, ta);
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to create '%s' domU on %s creating new "
              "directory and images in XEN storage", dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_destroy_dom_u(char const *ta, char const *dom_u)
{
    /* Ensure that domU is in "non-running" state */
    te_errno rc;

    if (ta == NULL || dom_u == NULL)
    {
        ERROR("Failed to destroy '%s' domU on %s: Invalid params",
              dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, "non-running"),
                                   "/agent:%s/xen:/dom_u:%s/status:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to shutdown '%s' domU on %s: %r", dom_u, ta, rc);
        goto cleanup0;
    }

    /* Remove directory/disk images of domU from XEN storage */
    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to remove directory/images of '%s' domU on %s",
              dom_u, ta);
        goto cleanup0;
    }

    /* Destroy domU */
    if ((rc = cfg_del_instance_fmt(FALSE,
                                   "/agent:%s/xen:/dom_u:%s",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to destroy '%s' domU on %s", dom_u, ta);
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
    te_errno     rc;

    if (ta == NULL || dom_u == NULL || status == NULL)
    {
        ERROR("Failed to get status for '%s' domU on %s: Invalid params",
              dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &value,
                                   "/agent:%s/xen:/dom_u:%s/status:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to get status for '%s' domU on %s", dom_u, ta);
    }
    else
    {
        strcpy(status, value);
        free((void *)value);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_set_status(char const *ta, char const *dom_u,
                              char const *status)
{
    unsigned int v;
    te_bool      started;
    te_errno     rc;

    cfg_val_type           type  = CVT_ADDRESS;
    struct sockaddr const *value;
    struct sockaddr        ip_addr;

    if (ta == NULL || dom_u == NULL || status == NULL)
    {
        ERROR("Failed to set status for '%s' domU on %s: Invalid params",
              dom_u, ta);
        return TE_EINVAL;
    }

    /* Needed for transition to "running" status */
    if ((rc = cfg_get_instance_fmt(&type, &value,
                                   "/agent:%s/xen:/dom_u:%s/ip_addr:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to get IP address for '%s' domU on %s", dom_u, ta);
        return rc;
    }

    memcpy(&ip_addr, value, sizeof(ip_addr));
    free((void *)value);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, status),
                                   "/agent:%s/xen:/dom_u:%s/status:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to set \"%s\" status for '%s' domU on %s: %r",
              status, dom_u, ta, rc);
        return rc;
    }

    if (strcmp(status, "running") != 0)
        return 0;

    /* Check up to 120-150 seconds whether SSH server is up */
    for (v = 0, started = FALSE; !started && v < 20; v++)
    {
        char buf[256];
        char const *const chk_str = "BOPOHA ECT KYCOK CbIPA";
        FILE *f;

        TE_SPRINTF(buf,
                   "/usr/bin/ssh -qxTno StrictHostKeyChecking=no "
                   "%s echo '%s' 2> /dev/null",
                   inet_ntoa(SIN(&ip_addr)->sin_addr), chk_str);

        if ((f = popen(buf, "r")) == NULL)
        {
            ERROR("popen(%s) failed with errno %d", buf, rc);
            return errno;
        }

        if (fgets(buf, sizeof(buf), f) != NULL &&
            strncmp(buf, chk_str, strlen(chk_str)) == 0)
        {
            started = TRUE;
        }

        pclose(f);

        sleep(5);
    }

    if (!started)
    {
        ERROR("Failed to detect running SSH daemon within '%s' domU",
              dom_u);
        return TE_EFAIL;
    }

    RING("Running SSH daemon within '%s' domU is detected", dom_u);
    return 0;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_get_memory_size(char const *ta, char const *dom_u,
                                   unsigned int *size)
{
    cfg_val_type  type  = CVT_INTEGER;
    te_errno      rc;

    if (ta == NULL || dom_u == NULL || size == NULL)
    {
        ERROR("Failed to get memory size for '%s' domU on %s: "
              "Invalid params", dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, size,
                                   "/agent:%s/xen:/dom_u:%s/memory:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to get memory size for '%s' domU on %s", dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_set_memory_size(char const *ta, char const *dom_u,
                                   unsigned int size)
{
    te_errno rc;

    if (ta == NULL || dom_u == NULL)
    {
        ERROR("Failed to set memory size for '%s' domU on %s: "
              "Invalid params", dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, size),
                                   "/agent:%s/xen:/dom_u:%s/memory:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to set memory size for '%s' domU on %s", dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_get_ip_addr(char const *ta, char const *dom_u,
                               struct sockaddr *ip_addr)
{
    cfg_val_type           type  = CVT_ADDRESS;
    struct sockaddr const *value;
    te_errno               rc;

    if (ta == NULL || dom_u == NULL || ip_addr == NULL)
    {
        ERROR("Failed to get IP address for '%s' domU on %s: "
              "Invalid params", dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &value,
                                   "/agent:%s/xen:/dom_u:%s/ip_addr:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to get IP address for '%s' domU on %s", dom_u, ta);
    }
    else
    {
        memcpy(ip_addr, value, sizeof(*value));
        free((void *)value);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_set_ip_addr(char const *ta, char const *dom_u,
                               struct sockaddr const *ip_addr)
{
    te_errno rc;

    if (ta == NULL || dom_u == NULL || ip_addr == NULL)
    {
        ERROR("Failed to set IP address for '%s' domU on %s: "
              "Invalid params", dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, ip_addr),
                                   "/agent:%s/xen:/dom_u:%s/ip_addr:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to set IP address for '%s' domU on %s", dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_get_mac_addr(char const *ta, char const *dom_u, uint8_t *mac)
{
    cfg_val_type     type = CVT_ADDRESS;
    struct sockaddr *addr;
    te_errno         rc;

    if (ta == NULL || dom_u == NULL || mac == NULL)
    {
        ERROR("Failed to get MAC address for '%s' domU on %s: "
              "Invalid params", dom_u, ta);
        ERROR("Invalid params");
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &addr,
                                   "/agent:%s/xen:/dom_u:%s/mac_addr:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to get MAC address of '%s' domU on %s", dom_u, ta);
    }
    else
    {
        memcpy(mac, addr->sa_data, ETHER_ADDR_LEN);
        free(addr);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_set_mac_addr(char const *ta, char const *dom_u, uint8_t const *mac)
{
    struct sockaddr addr = { .sa_family = AF_LOCAL };
    te_errno        rc;

    if (ta == NULL || dom_u == NULL || mac == NULL)
    {
        ERROR("Failed to set MAC address for '%s' domU on %s: "
              "Invalid params", dom_u, ta);
        return TE_EINVAL;
    }

    memcpy(addr.sa_data, mac, ETHER_ADDR_LEN);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, &addr),
                                   "/agent:%s/xen:/dom_u:%s/mac_addr:",
                                   ta, dom_u)) != 0)
    {
        ERROR("Failed to set MAC address of '%s' domU on %s", dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_add_bridge(char const *ta, char const *dom_u,
                              char const *bridge, char const *if_name)
{
    te_errno rc;

    if (ta == NULL || dom_u == NULL || bridge == NULL || if_name == NULL)
    {
        ERROR("Failed to add '%s' bridge for '%s' domU on %s: "
              "Invalid params", bridge, dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, if_name),
                                   "/agent:%s/xen:/dom_u:%s"
                                   "/bridge:%s", ta, dom_u, bridge)) != 0)
    {
        ERROR("Failed to add '%s' bridge for '%s' domU on %s",
              bridge, dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_del_bridge(char const *ta, char const *dom_u,
                              char const *bridge)
{
    te_errno rc;

    if (ta == NULL || dom_u == NULL || bridge == NULL)
    {
        ERROR("Failed to delete '%s' bridge for '%s' domU on %s: "
              "Invalid params", bridge, dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/xen:/dom_u:%s"
                                   "/bridge:%s", ta, dom_u, bridge)) != 0)
    {
        ERROR("Failed to delete '%s' bridge for '%s' domU on %s",
              bridge, dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_bridge_get_if_name(char const *ta, char const *dom_u,
                                      char const *bridge, char *if_name)
{
    cfg_val_type type  = CVT_STRING;
    char const  *value;
    te_errno     rc;

    if (ta == NULL || dom_u == NULL || bridge == NULL || if_name == NULL)
    {
        ERROR("Failed to get RCF/RPC interface name for '%s' bridge "
              "on '%s' domU on %s: Invalid params", bridge, dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &value,
                                   "/agent:%s/xen:/dom_u:%s"
                                   "/bridge:%s", ta, dom_u, bridge)) != 0)
    {
        ERROR("Failed to get RCF/RPC interface name for '%s' "
              "bridge on '%s' domU on %s", bridge, dom_u, ta);
    }
    else
    {
        strcpy(if_name, value);
        free((void *)value);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_bridge_set_if_name(char const *ta, char const *dom_u,
                                      char const *bridge,
                                      char const *if_name)
{
    te_errno rc;

    if (ta == NULL || dom_u == NULL || bridge == NULL || if_name == NULL)
    {
        ERROR("Failed to set RCF/RPC interface name for '%s' bridge "
              "on '%s' domU on %s: Invalid params", bridge, dom_u, ta);
        return TE_EINVAL;
    }

    if((rc = cfg_set_instance_fmt(CFG_VAL(STRING, if_name),
                                  "/agent:%s/xen:/dom_u:%s"
                                  "/bridge:%s", ta, dom_u, bridge)) != 0)
    {
        ERROR("Failed to set \"%s\" RCF/RPC interface name for '%s' "
              "bridge on '%s' domU on %s: %r", if_name, bridge, dom_u,
              ta, rc);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_bridge_get_ip_addr(char const *ta, char const *dom_u,
                                      char const *bridge,
                                      struct sockaddr *ip_addr)
{
    cfg_val_type           type  = CVT_ADDRESS;
    struct sockaddr const *value;
    te_errno               rc;

    if (ta == NULL || dom_u == NULL || bridge == NULL || ip_addr == NULL)
    {
        ERROR("Failed to get IP address for '%s' bridge interface "
              "on '%s' domU on %s: Invalid params", bridge, dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &value,
                                   "/agent:%s/xen:/dom_u:%s"
                                   "/bridge:%s/ip_addr:",
                                   ta, dom_u, bridge)) != 0)
    {
        ERROR("Failed to get IP address for '%s' bridge "
              "interface on '%s' domU on %s", bridge, dom_u, ta);
    }
    else
    {
        memcpy(ip_addr, value, sizeof(*value));
        free((void *)value);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_bridge_set_ip_addr(char const *ta, char const *dom_u,
                                      char const *bridge,
                                      struct sockaddr const *ip_addr)
{
    te_errno rc;

    if (ta == NULL || dom_u == NULL || bridge == NULL || ip_addr == NULL)
    {
        ERROR("Failed to set IP address for '%s' bridge interface "
              "on '%s' domU on %s: Invalid params", bridge, dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, ip_addr),
                                   "/agent:%s/xen:/dom_u:%s"
                                   "/bridge:%s/ip_addr:",
                                   ta, dom_u, bridge)) != 0)
    {
        ERROR("Failed to set IP address for '%s' bridge "
              "interface on '%s' domU on %s", bridge, dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_bridge_get_mac_addr(char const *ta, char const *dom_u,
                                       char const *bridge, uint8_t *mac)
{
    cfg_val_type     type = CVT_ADDRESS;
    struct sockaddr *addr;
    te_errno         rc;

    if (ta == NULL || dom_u == NULL || bridge == NULL || mac == NULL)
    {
        ERROR("Failed to get MAC address for '%s' bridge interface "
              "on '%s' domU on %s: Invalid params", bridge, dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &addr,
                                   "/agent:%s/xen:/dom_u:%s"
                                   "/bridge:%s/mac_addr:",
                                   ta, dom_u, bridge)) != 0)
    {
        ERROR("Failed to get MAC address for '%s' bridge "
              "interface on '%s' domU on %s", bridge, dom_u, ta);
    }
    else
    {
        memcpy(mac, addr->sa_data, ETHER_ADDR_LEN);
        free(addr);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_bridge_set_mac_addr(char const *ta, char const *dom_u,
                                       char const *bridge,
                                       uint8_t const *mac)
{
    struct sockaddr addr = { .sa_family = AF_LOCAL };
    te_errno        rc;

    if (ta == NULL || dom_u == NULL || bridge == NULL || mac == NULL)
    {
        ERROR("Failed to set MAC address for '%s' bridge interface "
              "on '%s' domU on %s: Invalid params", bridge, dom_u, ta);
        return TE_EINVAL;
    }

    memcpy(addr.sa_data, mac, ETHER_ADDR_LEN);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, &addr),
                                   "/agent:%s/xen:/dom_u:%s"
                                   "/bridge:%s/mac_addr:",
                                   ta, dom_u, bridge)) != 0)
    {
        ERROR("Failed to set MAC address for '%s' bridge "
              "interface on '%s' domU on %s", bridge, dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_bridge_get_accel(char const *ta, char const *dom_u,
                                    char const *bridge, te_bool *accel)
{
    cfg_val_type type = CVT_INTEGER;
    int          acceleration;
    te_errno     rc;

    if (ta == NULL || dom_u == NULL || bridge == NULL || accel == NULL)
    {
        ERROR("Failed to get acceleration sign for '%s' bridge interface "
              "on '%s' domU on %s: Invalid params", bridge, dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_get_instance_fmt(&type, &acceleration,
                                   "/agent:%s/xen:/dom_u:%s"
                                   "/bridge:%s/accel:",
                                   ta, dom_u, bridge)) != 0)
    {
        ERROR("Failed to get acceleration sign for '%s' bridge "
              "interface on '%s' domU on %s", bridge, dom_u, ta);
    }
    else
    {
        *accel = acceleration ? TRUE : FALSE;
        free(acceleration);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_bridge_set_accel(char const *ta, char const *dom_u,
                                    char const *bridge, te_bool accel)
{
    int      acceleration = accel ? 1 : 0;
    te_errno rc;

    if (ta == NULL || dom_u == NULL || bridge == NULL)
    {
        ERROR("Failed to set acceleration sign for '%s' bridge interface "
              "on '%s' domU on %s: Invalid params", bridge, dom_u, ta);
        return TE_EINVAL;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, acceleration),
                                   "/agent:%s/xen:/dom_u:%s"
                                   "/bridge:%s/accel:",
                                   ta, dom_u, bridge)) != 0)
    {
        ERROR("Failed to set acceleration sign for '%s' bridge "
              "interface on '%s' domU on %s", bridge, dom_u, ta);
    }

    return rc;
}

/* See description in tapi_cfg_xen.h */
te_errno
tapi_cfg_xen_dom_u_migrate(char const *from_ta, char const *to_ta,
                           char const *dom_u, char const *host,
                           te_bool live)
{
    te_bool running;
    te_bool saved;

    char xen_path1[PATH_MAX];
    char xen_path2[PATH_MAX];
    char status[PATH_MAX];

    unsigned        memory_size = 0;
    uint8_t         mac[ETHER_ADDR_LEN];
    struct sockaddr ip;

    te_errno rc;

    if (from_ta == NULL || to_ta == NULL || dom_u == NULL || host == NULL)
    {
        ERROR("Failed to migrate: Invalid params");
        return TE_EINVAL;
    }

    if ((rc = tapi_cfg_xen_dom_u_get_status(from_ta, dom_u, status)) != 0)
        goto cleanup0;

    running = strcmp(status, "running") == 0 ||
              strcmp(status, "migrated-running") == 0;
    saved   = strcmp(status, "saved") == 0 ||
              strcmp(status, "migrated-saved") == 0;

    /* Check the status of domU on 'from_ta' agent */
    if (!running && !saved)
    {
        ERROR("Failed to migrate since '%s' domU is in \"%s\" status "
              "(neither in \"running\" nor in \"saved\" one)",
              dom_u, status);
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup0;
    }

    /* Cannot migrate to itself */
    if (strcmp(from_ta, to_ta) == 0)
    {
        ERROR("Failed to migrate from %s to itself", from_ta);
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup0;
    }

    /* Check that XEN paths are identical for both dom0 agents */
    if ((rc = tapi_cfg_xen_get_path(from_ta, xen_path1)) != 0 ||
        (rc = tapi_cfg_xen_get_path(  to_ta, xen_path2)) != 0)
    {
        goto cleanup0;
    }

    if (strcmp(xen_path1, xen_path2) != 0)
    {
        ERROR("XEN path differs between %s and %s", from_ta, to_ta);
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup0;
    }

    /* Save memory size and MAC and IP addresses */
    if ((rc = tapi_cfg_xen_dom_u_get_memory_size(from_ta, dom_u,
                                                 &memory_size)) != 0 ||
        (rc = tapi_cfg_xen_dom_u_get_mac_addr(from_ta, dom_u, mac)) != 0 ||
        (rc = tapi_cfg_xen_dom_u_get_ip_addr( from_ta, dom_u, &ip)) != 0)
    {
        goto cleanup0;
    }

    if (running)
    {
        /* Set kind of migration (live/non-live) */
        if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, live),
                                       "/agent:%s/xen:/dom_u:%s/"
                                       "migrate:/kind:",
                                       from_ta, dom_u)) != 0)
        {
            ERROR("Failed to set migration kind for '%s' domU on %s",
                  dom_u, from_ta);
            goto cleanup0;
        }

        /* Perform migration */
        if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, host),
                                       "/agent:%s/xen:/dom_u:%s/migrate:",
                                       from_ta, dom_u)) != 0)
        {
            ERROR("Failed to perform migration itself");
            goto cleanup0;
        }
    }

    /* Delete domU item from the source agent configurator tree */
    if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/xen:/dom_u:%s",
                                   from_ta, dom_u)) != 0)
    {
        ERROR("Failed to destroy '%s' domU on %s", dom_u, from_ta);
        goto cleanup0;
    }

    /* Create domU on target agent (domU will have "non-running" state) */
    if ((rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 1),
                                   "/agent:%s/xen:/dom_u:%s",
                                   to_ta, dom_u)) != 0)
    {
        ERROR("Failed to accept '%s' domU just migrated to %s",
              dom_u, to_ta);
        goto cleanup0;
    }

    /* Set MAC and IP addresses saved previously */
    if ((rc = tapi_cfg_xen_dom_u_set_memory_size(to_ta, dom_u,
                                                 memory_size)) != 0 ||
        (rc = tapi_cfg_xen_dom_u_set_mac_addr(to_ta, dom_u, mac)) != 0 ||
        (rc = tapi_cfg_xen_dom_u_set_ip_addr( to_ta, dom_u, &ip)) != 0)
    {
        goto cleanup0;
    }

    /* Set "migrated-running" or "migrated-saved" status */
    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING,
                                           running ? "migrated-running" :
                                                      "migrated-saved"),
                                   "/agent:%s/xen:/dom_u:%s/status:",
                                   to_ta, dom_u)) != 0)
    {
        ERROR("Failed to set migrated %s status for '%s' domU on %s",
              running ? "running" : "saved", dom_u, to_ta);
    }

cleanup0:
    if (rc != 0)
    {
        ERROR("Failed to migrate '%s' domU from %s to %s (to host '%s')",
              dom_u, from_ta, to_ta, host);
    }

    return rc;
}

