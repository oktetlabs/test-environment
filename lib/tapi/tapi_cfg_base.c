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

#define TE_LGR_USER     "Configuration TAPI"

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
#include "logger_api.h"
#include "conf_api.h"

#include "tapi_cfg_base.h"
#include "tapi_sockaddr.h"


/* See the description in tapi_cfg_base.h */
int
tapi_cfg_base_ip_fw(const char *ta, te_bool *enabled, const char *vrsn)
{
    int             rc;
    cfg_val_type    val_type = CVT_INTEGER;
    int             val;
    char            *val_str;
    char            obj_id[CFG_OID_MAX];
    

    snprintf(obj_id, sizeof(obj_id), "/agent/ip%s_fw", vrsn);
    
    if (cfg_get_instance_fmt(NULL, &val_str,
                             "/agent:%s/rsrc:ip%s_fw", ta, vrsn) != 0)
    {
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, obj_id),
                                  "/agent:%s/rsrc:ip%s_fw", ta, vrsn);
        if (rc != 0)
            return rc;
    }        
    else
        free(val_str);                     

    rc = cfg_get_instance_fmt(&val_type, &val,
                              "/agent:%s/ip%s_fw:", ta, vrsn);
    if (rc != 0)
    {
        ERROR("Failed to get IPv%s forwarding state on '%s': %r",
              vrsn, ta, rc);
        return rc;
    }

    if (val != *enabled)
    {
        int new_val = *enabled;

        rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, new_val),
                                  "/agent:%s/ip%s_fw:", ta, vrsn);
        if (rc != 0)
        {
            ERROR("Failed to configure IPv%s forwarding on '%s': %r",
                  vrsn, ta, rc);
            return rc;
        }
        *enabled = val;
    }

    return 0;
}   /* tapi_cfg_base_ip_fw() */

/* See the description in tapi_cfg_base.h */
int
tapi_cfg_base_ipv4_fw(const char *ta, te_bool *enabled)
{
    return tapi_cfg_base_ip_fw(ta, enabled, "4");
}

/* See the description in tapi_cfg_base.h */
int
tapi_cfg_base_ipv6_fw(const char *ta, te_bool *enabled)
{
    return tapi_cfg_base_ip_fw(ta, enabled, "6");
}

/* See the description in tapi_cfg_base.h */
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
tapi_cfg_base_if_set_mac(const char *oid, const uint8_t *mac)
{
    int                 rc;
    char                buf[strlen(oid) + strlen("/link_addr:") + 1];
    cfg_handle          handle;
    struct sockaddr     addr;

    memset(&addr, 0, sizeof(struct sockaddr));
    addr.sa_family = AF_LOCAL;
    sprintf(buf, "%s/link_addr:", oid);
    rc = cfg_find_str(buf, &handle);
    if (rc != 0)
    {
        ERROR("Failed to find MAC address OID handle for %s", oid);
        return rc;
    }

    memcpy(addr.sa_data, mac, ETHER_ADDR_LEN);

    rc = cfg_set_instance(handle, CVT_ADDRESS, &addr);
    if (rc != 0)
    {
        ERROR("Failed to set MAC address using OID %s", buf);
        return rc;
    }

    return rc;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_get_bcast_mac(const char *oid, uint8_t *bcast_mac)
{
    int                 rc;
    char                buf[strlen(oid) + strlen("/bcast_link_addr:") + 1];
    cfg_handle          handle;
    cfg_val_type        type = CVT_ADDRESS;
    struct sockaddr    *addr = NULL;

    sprintf(buf, "%s/bcast_link_addr:", oid);
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
    memcpy(bcast_mac, addr->sa_data, ETHER_ADDR_LEN);

    free(addr);

    return rc;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_set_bcast_mac(const char *oid,
                               const uint8_t *bcast_mac)
{
    int                 rc;
    char                buf[strlen(oid) +
                             strlen("/bcast_link_addr:") + 1];
    cfg_handle          handle;
    struct sockaddr     addr;

    memset(&addr, 0, sizeof(struct sockaddr));
    addr.sa_family = AF_LOCAL;
    sprintf(buf, "%s/bcast_link_addr:", oid);
    rc = cfg_find_str(buf, &handle);
    if (rc != 0)
    {
        ERROR("Failed to find broadcast MAC "
              "address OID handle for %s", oid);
        return rc;
    }

    memcpy(addr.sa_data, bcast_mac, ETHER_ADDR_LEN);

    rc = cfg_set_instance(handle, CVT_ADDRESS, &addr);
    if (rc != 0)
    {
        ERROR("Failed to set bcast MAC address using OID %s", buf);
        return rc;
    }

    return rc;
}


static const char *
tapi_cfg_mac2str(const uint8_t *mac)
{
    static char macbuf[ETHER_ADDR_LEN * 3];

    int  i;
    
    for (i = 0; i < ETHER_ADDR_LEN; i++)
    {
        sprintf(macbuf + 3 * i, "%2.2x:", mac[i]);
    }
    macbuf[ETHER_ADDR_LEN * 3 - 1] = '\0';
    
    return macbuf;
}

/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_add_mcast_mac(const char *oid,
                               const uint8_t *mcast_mac)
{
    return cfg_add_instance_fmt(NULL, CFG_VAL(NONE, NULL),
                                "%s/mcast_link_addr:%s", oid,
                                tapi_cfg_mac2str(mcast_mac));
}


/* See description in tapi_cfg_base.h */
int
tapi_cfg_base_if_del_mcast_mac(const char *oid,
                               const uint8_t *mcast_mac)
{
    if (mcast_mac != NULL)
    {
        return cfg_del_instance_fmt(TRUE, "%s/mcast_link_addr:%s", oid,
                                    tapi_cfg_mac2str(mcast_mac));
    }
    else
    {
        te_errno        rc = 0;
        cfg_handle     *addrs;
        unsigned int    addr_num;
        unsigned int    i;
        
        if ((rc = cfg_find_pattern_fmt(&addr_num, &addrs,
                                       "%s/mcast_link_addr:*",
                                       oid)) != 0)
        {    
            ERROR("Failed to get mcast_link_addr list for %s", oid);
            return rc;
        }
        for (i = 0; i < addr_num; i++)
        {
            char *inst_name;
            rc = cfg_get_inst_name(addrs[i], &inst_name);
            if (rc != 0)
            {
                ERROR("Unable to enumerate multicast addresses: %r", rc);
                break;
            }

            if ((rc = cfg_del_instance(addrs[i], TRUE)) != 0)
            {
                ERROR("Failed to delete address with handle %#x: %r",
                      addrs[i], rc);
                break;
            }
        }
        free(addrs);
        
        return rc;
    }
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


    if (addr->sa_family != AF_INET && addr->sa_family != AF_INET6)
    {
        ERROR("AF_INET and AF_INET6 address families are supported only.");
        return TE_RC(TE_TAPI, TE_EAFNOSUPPORT);
    }

    if (prefix == -1)
    {
        rc = cfg_add_instance_fmt(cfg_hndl, CFG_VAL(NONE, NULL),
                                  "%s/net_addr:%s", oid,
                                  inet_ntop(addr->sa_family,
                                            te_sockaddr_get_netaddr(addr),
                                            buf, sizeof(buf)));
    }
    else
    {
        rc = cfg_add_instance_fmt(cfg_hndl, CFG_VAL(INTEGER, prefix),
                                  "%s/net_addr:%s", oid,
                                  inet_ntop(addr->sa_family,
                                            te_sockaddr_get_netaddr(addr),
                                            buf, sizeof(buf)));
    }
    if (rc == 0)
    {
        if (addr->sa_family == AF_INET && set_bcast)
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
             inet_ntop(addr->sa_family, te_sockaddr_get_netaddr(addr),
                       buf, sizeof(buf)));
    }
    else
    {
        ERROR("Failed to add address for %s: %r", oid, rc);
    }

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_del_if_ip4_addresses(const char *ta,
                              const char *if_name,
                              const struct sockaddr *addr_to_save)
{
    cfg_handle                 *addrs = NULL;
    unsigned int                addr_num = 0;
    unsigned int                i;
    char                       *addr_str;
    struct sockaddr_storage     addr;
    te_errno                    rc;

    if (addr_to_save != NULL && addr_to_save->sa_family != AF_INET)
    {
        ERROR("%s(): Invalid family %d of the address to save",
              __FUNCTION__, (int)addr_to_save->sa_family);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    
    if ((rc = cfg_find_pattern_fmt(&addr_num, &addrs,
                                   "/agent:%s/interface:%s/net_addr:*",
                                   ta, if_name)) != 0)
    {    
        ERROR("Failed to get net_addr list for /agent:%s/interface:%s/", 
              ta, if_name);
        return rc;
    }
    
    for (i = 0; i < addr_num; i++)
    {
        if ((rc = cfg_get_inst_name(addrs[i], &addr_str)) != 0)
        {
            ERROR("Failed to get instance name: %r", rc);
            break;
        }
        if ((rc = te_sockaddr_netaddr_from_string(addr_str,
                                                  SA(&addr))) != 0)
        {
            ERROR("Failed to convert address from string '%s': %r",
                  addr_str, rc);
            free(addr_str);
            break;
        }
        free(addr_str);
        if (addr.ss_family != AF_INET)
        {
            continue;
        }
        if (addr_to_save == NULL)
        {
            /* Just to mark that one address is saved */
            addr_to_save = SA(&addr);
            continue;
        }
        if (addr_to_save != SA(&addr) &&
            memcmp(&SIN(addr_to_save)->sin_addr,
                   &SIN(&addr)->sin_addr,
                   sizeof(SIN(&addr)->sin_addr)) == 0)
        {
            continue;
        }

        if ((rc = cfg_del_instance(addrs[i], FALSE)) != 0)
        {
            ERROR("Failed to delete address with handle %#x: %r",
                  addrs[i], rc);
            break;
        }
    }

    free(addrs);

    return rc;
}

/* See description in tapi_cfg_base.h */
te_errno
tapi_cfg_base_if_add_vlan(const char *ta, const char *if_name,
                          uint16_t vid, char **vlan_ifname)
{
    cfg_val_type val = CVT_STRING;
    te_errno     rc = 0;

    if ((rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                                   "/agent:%s/interface:%s/vlans:%d",
                                   ta, if_name, vid)) != 0)
    {
        ERROR("Failed to add VLAN with VID=%d to %s", vid, if_name);
        return rc;
    }
    if ((rc = cfg_get_instance_fmt(&val, vlan_ifname,
                         "/agent:%s/interface:%s/vlans:%d/ifname:",
                         ta, if_name, vid)) != 0)
    {
        ERROR("Failed to get interface name for VLAN interface "
              "with VID=%d on %s", vid, if_name);
        return rc;
    }

    return rc;
}

/*
 * Configurator PHY support
 */
  
#if CONFIGURATOR_PHY_SUPPORT
#if HAVE_LINUX_ETHTOOL_H

/**
 * Get PHY autonegotiation state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned autonegotiation state value:
 *                      PHY_AUTONEG_OFF - autonegatiation OFF
 *                      PHY_AUTONEG_ON  - autonegatiation ON
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_autoneg_get(const char *ta, const char *if_name,
    int *state)
{
    return cfg_get_instance_sync_fmt(CFG_VAL(INTEGER, state),
                                     "/agent:%s/interface:%s/phy:/autoneg:",
                                     ta,
                                     if_name);
}

/**
 * Set PHY autonegotiation state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Autonegotiation state value:
 *                      PHY_AUTONEG_OFF - autonegatiation OFF
 *                      PHY_AUTONEG_ON  - autonegatiation ON
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_autoneg_set(const char *ta, const char *if_name,
    int state)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, state),
                                "/agent:%s/interface:%s/phy:/autoneg:",
                                ta,
                                if_name);
}

#define PHY_DUPLEX_STRING_HALF "half"
#define PHY_DUPLEX_STRING_FULL "full"

/**
 * Get PHY duplex state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_duplex_get(const char *ta, const char *if_name,
    int *state)
{
    te_errno  rc = 0;
    char     *duplex;
    
    rc = cfg_get_instance_sync_fmt(NULL,
                                   (void *)&duplex,
                                   "/agent:%s/interface:%s/phy:/duplex:",
                                   ta,
                                   if_name);
    if (rc != 0)
        return rc;
    
    if (strcmp(duplex, PHY_DUPLEX_STRING_HALF) == 0)
        *state = PHY_DUPLEX_HALF;
    else
        if (strcmp(duplex, PHY_DUPLEX_STRING_FULL) == 0)
            *state = PHY_DUPLEX_FULL;
        else
            ERROR("unknown duplex value");
    
    free(duplex);
    
    return 0;
}

/**
 * Set PHY duplex state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_duplex_set(const char *ta, const char *if_name, int state)
{
    te_errno rc = 0;
    
    switch (state)
    {
        case PHY_DUPLEX_HALF:
        {
            rc = cfg_set_instance_fmt(CFG_VAL(STRING,
                                              PHY_DUPLEX_STRING_HALF),
                                      "/agent:%s/interface:%s/phy:/duplex:",
                                      ta,
                                      if_name);
            break;
        }
        
        case PHY_DUPLEX_FULL:
        {
            rc = cfg_set_instance_fmt(CFG_VAL(STRING,
                                              PHY_DUPLEX_STRING_FULL),
                                      "/agent:%s/interface:%s/phy:/duplex:",
                                      ta,
                                      if_name);
            break;
        }
        
        default: ERROR("unknown duplex value");
    }
    
    return rc;
}



/**
 * Get PHY speed value.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Pointer to returned speed value
 *                      (see linux/ethtool.h for more details)
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_speed_get(const char *ta, const char *if_name, int *speed)
{
    return cfg_get_instance_sync_fmt(CFG_VAL(INTEGER, speed),
                                     "/agent:%s/interface:%s/phy:/speed:",
                                     ta,
                                     if_name);
}

/**
 * Set PHY speed.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (see linux/ethtool.h for more details)
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_speed_set(const char *ta, const char *if_name, int speed)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, speed),
                                "/agent:%s/interface:%s/phy:/speed:",
                                ta,
                                if_name);
}

/**
 * Get PHY link state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned link state value:
 *                      0 - link down
 *                      1 - link up
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_state_get(const char *ta, const char *if_name, int *state)
{
    return cfg_get_instance_sync_fmt(CFG_VAL(INTEGER, state),
                                     "/agent:%s/interface:%s/phy:/state:",
                                     ta,
                                     if_name);
}

/**
 * Check that PHY mode is advertised.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (see linux/ethtool.h for more details)
 * @param duplex        Duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 * @param state         Pointer to mode state:
 *                      TRUE - mode is advertised
 *                      FALSE - mode is not advertised
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_is_mode_advertised(const char *ta, const char *if_name,
    int speed, int duplex, te_bool *state)
{
    te_errno rc = 0;
    int      advertised = -1;
    
    /* Get mode state */
    rc = cfg_get_instance_sync_fmt(CFG_VAL(INTEGER, &advertised),
                                   "/agent:%s/interface:%s/phy:"
                                   "/modes:/speed:%d/duplex:%s",
                                   ta,
                                   if_name,
                                   speed,
                                   (duplex) ?
                                   PHY_DUPLEX_STRING_FULL :
                                   PHY_DUPLEX_STRING_HALF);
    
    /* Store state */
    if (advertised)
        *state = TRUE;
    else
        *state = FALSE;
    
    return rc;
}

/**
 * Set PHY mode to advertising state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (see linux/ethtool.h for more details)
 * @param duplex        Duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 * @param state         Mode state:
 *                      0 - mode is not advertised
 *                      1 - mode is advertised
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_advertise_mode(const char *ta, const char *if_name,
    int speed, int duplex, int state)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, state),
                                "/agent:%s/interface:%s/phy:"
                                "/modes:/speed:%d/duplex:%s",
                                ta,
                                if_name,
                                speed,
                                (duplex) ?
                                PHY_DUPLEX_STRING_FULL :
                                PHY_DUPLEX_STRING_HALF);
}

#undef PHY_DUPLEX_STRING_HALF
#undef PHY_DUPLEX_STRING_FULL

/**
 * Restart autonegotiation.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param unused        Unused parameter;
 *                      should be set to any value (just formality)
 *
 * @return Status code
 */
te_errno
tapi_cfg_base_phy_restart_autoneg(const char *ta, const char *if_name,
    int unused)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, unused),
                                "/agent:%s/interface:%s/phy:/reset:",
                                ta,
                                if_name);
}

#endif /* HAVE_LINUX_ETHTOOL_H */ 
#endif /* CONFIGURATOR_PHY_SUPPORT */
