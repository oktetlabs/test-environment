/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for basic configuration model
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "conf_api.h"

#include "tapi_cfg_base.h"

#define TE_LGR_USER     "Configuration TAPI"
#include "logger_api.h"


/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_ipv4_fw(const char *ta, te_bool *enabled)
{
    int             rc;
    cfg_val_type    val_type = CVT_INTEGER;
    int             val;

    rc = cfg_get_instance_fmt(&val_type, &val,
                              "/agent:%s/ip4_fw:", ta);
    if (rc != 0)
    {
        ERROR("Failed to get IPv4 forwarding state on '%s': %X", ta, rc);
        return rc;
    }

    if (val != *enabled)
    {
        int new_val = *enabled;

        rc = cfg_set_instance_fmt(val_type, (void *)new_val,
                                  "/agent:%s/ip4_fw:", ta);
        if (rc != 0)
        {
            ERROR("Failed to configure IPv4 forwarding on '%s': %X", ta, rc);
            return rc;
        }
        *enabled = val;
    }

    return 0;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_get_mac(const char *oid, uint8_t *mac)
{
    int                 rc;
    char                buf[strlen(oid) + strlen("/link_addr:") + 1];
    cfg_handle          handle;
    cfg_val_type        type = CVT_ADDRESS;
    struct sockaddr    *addr = NULL;

    sprintf(buf, "%s/link_addr:", oid);
    rc = cfg_find_str(buf, &handle);
    if (rc != 0)
    {
        ERROR("Failed to find MAC address OID handle for %s", oid);
        return rc;
    }

    rc = cfg_get_instance(handle, &type, &addr);
    if (rc != 0)
    {
        ERROR("Failed to get MAC address using OID %s", buf);
        return rc;
    }
    memcpy(mac, addr->sa_data, ETHER_ADDR_LEN);

    free(addr);

    return rc;
}


/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_get_mtu(const char *oid, unsigned int *p_mtu)
{
    int                 rc;
    char                buf[strlen(oid) + strlen("/mtu:") + 1];
    cfg_handle          handle;
    cfg_val_type        type = CVT_INTEGER;
    int                 mtu;

    sprintf(buf, "%s/mtu:", oid);
    rc = cfg_find_str(buf, &handle);
    if (rc != 0)
    {
        ERROR("Failed to find MTU OID handle for %s", oid);
        return rc;
    }

    rc = cfg_get_instance(handle, &type, &mtu);
    if (rc != 0)
    {
        ERROR("Failed to get MTU using OID %s. %X", buf, rc);
        return rc;
    }
    assert(mtu >= 0);
    *p_mtu = (unsigned int)mtu;

    return rc;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_add_net_addr(const char *oid, const struct sockaddr *sa,
                           cfg_handle *cfg_hndl)
{
    char    buf[INET6_ADDRSTRLEN];
    int     rc;


    if (sa->sa_family != AF_INET)
    {
        ERROR("AF_INET address family is supported only.");
        return TE_RC(TE_TAPI, EAFNOSUPPORT);
    }

    rc = cfg_add_instance_fmt(cfg_hndl, CVT_NONE, NULL,
                              "%s/net_addr:%s", oid,
                              inet_ntop(sa->sa_family,
                                        &SIN(sa)->sin_addr,
                                        buf, sizeof(buf)));
    if (rc == 0)
    {
        RING("Address %s added to %s",
             inet_ntop(sa->sa_family, &SIN(sa)->sin_addr, buf, sizeof(buf)),
             oid);
    }
    else if (TE_RC_GET_ERROR(rc) == EEXIST)
    {
        WARN("%s already has address %s", oid,
             inet_ntop(sa->sa_family, &SIN(sa)->sin_addr, buf, sizeof(buf)));
    }
    else
    {
        ERROR("Failed to add address for %s: %X", oid, rc);
    }

    return rc;
}
