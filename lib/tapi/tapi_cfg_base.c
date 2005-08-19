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

#include "te_config.h"

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
        ERROR("Failed to get IPv4 forwarding state on '%s': %r", ta, rc);
        return rc;
    }

    if (val != *enabled)
    {
        int new_val = *enabled;

        rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, new_val),
                                  "/agent:%s/ip4_fw:", ta);
        if (rc != 0)
        {
            ERROR("Failed to configure IPv4 forwarding on '%s': %r",
                  ta, rc);
            return rc;
        }
        *enabled = val;
    }

    return 0;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_get_link_addr(const char *ta, const char *dev,
                               struct sockaddr *link_addr)
{
    char             inst_name[CFG_OID_MAX];
    cfg_handle       handle;
    cfg_val_type     type = CVT_ADDRESS;
    struct sockaddr *addr = NULL;
    int              rc;

    snprintf(inst_name, sizeof(inst_name), 
             "/agent:%s/interface:%s/link_addr:",
             ta, dev);
    rc = cfg_find_str(inst_name, &handle);
    if (rc != 0)
    {
        ERROR("Failed to find MAC address OID handle for %s", inst_name);
        return rc;
    }

    rc = cfg_get_instance(handle, &type, &addr);
    if (rc != 0)
    {
        ERROR("Failed to get MAC address using OID %s", inst_name);
        return rc;
    }
    memcpy(link_addr, addr, sizeof(struct sockaddr));
    free(addr);
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
        ERROR("Failed to get MTU using OID %s. %r", buf, rc);
        return rc;
    }
    assert(mtu >= 0);
    *p_mtu = (unsigned int)mtu;

    return rc;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_add_net_addr(const char *oid, const struct sockaddr *addr,
                           int prefix, te_bool set_bcast,
                           cfg_handle *cfg_hndl)
{
    char    buf[INET6_ADDRSTRLEN];
    int     rc;


    if (addr->sa_family != AF_INET)
    {
        ERROR("AF_INET address family is supported only.");
        return TE_RC(TE_TAPI, TE_EAFNOSUPPORT);
    }

    if (prefix == -1)
    {
        rc = cfg_add_instance_fmt(cfg_hndl, CFG_VAL(NONE, NULL),
                                  "%s/net_addr:%s", oid,
                                  inet_ntop(addr->sa_family,
                                            &SIN(addr)->sin_addr,
                                            buf, sizeof(buf)));
    }
    else
    {
        rc = cfg_add_instance_fmt(cfg_hndl, CFG_VAL(INTEGER, prefix),
                                  "%s/net_addr:%s", oid,
                                  inet_ntop(addr->sa_family,
                                            &SIN(addr)->sin_addr,
                                            buf, sizeof(buf)));
    }
    if (rc == 0)
    {
        if (set_bcast)
        {
            struct sockaddr_in  bcast;
            uint32_t            nmask;

            if (prefix > 0)
            {
                nmask = (1 << ((sizeof(struct in_addr) << 3) - prefix)) - 1;
            }
            else
            {
                uint32_t    inaddr = ntohl(SIN(addr)->sin_addr.s_addr);

                if (IN_CLASSA(inaddr))
                    nmask = IN_CLASSA_HOST;
                else if (IN_CLASSB(inaddr))
                    nmask = IN_CLASSA_HOST;
                else if (IN_CLASSC(inaddr))
                    nmask = IN_CLASSA_HOST;
                else
                {
                    ERROR("Invalid IPv4 address - unknown class");
                    return TE_EINVAL;
                }
            }

            memcpy(&bcast, addr, sizeof(bcast));
            bcast.sin_addr.s_addr |= htonl(nmask);

            /* Set broadcast address */
            rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, &bcast),
                                      "%s/net_addr:%s/broadcast:", oid,
                                      inet_ntop(addr->sa_family,
                                                &SIN(addr)->sin_addr,
                                                buf, sizeof(buf)));
            if (rc != 0)
            {
                int rc2;

                ERROR("Failed to set broadcast address: %r", rc);
                rc2 = cfg_del_instance_fmt(TRUE, 
                                           "%s/net_addr:%s", oid,
                                           inet_ntop(addr->sa_family,
                                                     &SIN(addr)->sin_addr,
                                                     buf, sizeof(buf)));
                if (rc2 != 0)
                {
                    ERROR("Failed to delete address to rollback: %r", rc2);
                }
                return rc;
            }
        }
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EEXIST)
    {
        WARN("%s already has address %s", oid,
             inet_ntop(addr->sa_family, &SIN(addr)->sin_addr,
                       buf, sizeof(buf)));
    }
    else
    {
        ERROR("Failed to add address for %s: %r", oid, rc);
    }

    return rc;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_del_if_addresses(const char *ta,
                          const char *if_name,
                          const struct sockaddr *addr_to_save)
{
    cfg_handle   *addrs = NULL;
    cfg_handle   *ptr; 
    int           addr_num = 0;
    int           i;
    char          buf[INET_ADDRSTRLEN];
    char         *addr_to_save_str = NULL;
    char         *addr_str;
    int           rc;
    
    if ((rc = cfg_find_pattern_fmt(&addr_num, &addrs,
                                   "/agent:%s/interface:%s/net_addr:*",
                                   ta, if_name)) != 0)
    {    
        ERROR("Failed to get net_addr list for /agent:%s/interface:%s/", 
              ta, if_name);
        return rc;
    }
    if (addr_to_save != NULL)
    {
        if ((addr_to_save_str = 
                (char *)inet_ntop(AF_INET, 
                        &(SIN(addr_to_save)->sin_addr),
                        buf, INET_ADDRSTRLEN)) == NULL)
        {
            ERROR("Failed to convert address to string, errno %s",
                  strerror(errno));
            return -1;
        }
    }
    else
    {
        if ((rc = cfg_get_inst_name(*addrs, &addr_to_save_str)) != 0)
        {
            ERROR("Failed to get instance name");
            return rc;
        }
    }
    VERB("addr_to_save_str is %s", addr_to_save_str);
    
    for (i = 0, ptr = addrs; i < addr_num; i++, ptr++)
    {
        if ((rc = cfg_get_inst_name(*ptr, &addr_str)) != 0)
        {
            ERROR("Failed to get instance name");
            return rc;
        }
        if (strncmp(addr_str, addr_to_save_str, 
                    strlen(addr_to_save_str) + 1) == 0)
            continue;
        if ((rc = cfg_del_instance(*ptr, FALSE)) != 0)
        {
            ERROR("Failed to delete address %s", addr_str);
            return rc;
        }
    }
    return 0;
}
