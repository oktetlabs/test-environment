/** @file
 * @brief Test API to access Configurator
 *
 * Implementation
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Configuration TAPI"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "conf_api.h"
#include "logger_api.h"

#include "tapi_sockaddr.h"
#include "tapi_cfg_base.h"


/** Operations with routing/ARP table */
enum tapi_cfg_oper {
    OP_ADD, /**< Add operation */
    OP_DEL, /**< Delete operation */
};

/* Forward declarations */
static int tapi_cfg_route_op(enum tapi_cfg_oper op, const char *ta,
                             int addr_family,
                             const void *dst_addr, int prefix,
                             const void *gw_addr, const char *dev,
                             uint32_t flags, int metric, int mss, 
                             int win, int irtt, cfg_handle *cfg_hndl);

static int tapi_cfg_arp_op(enum tapi_cfg_oper op, const char *ta,
                           const void *net_addr, const void *link_addr);


/* See description in tapi_cfg.h */
int
tapi_cfg_get_son_mac(const char *father, const char *subid,
                     const char *name, uint8_t *p_mac)
{
    int                 rc;
    cfg_handle          handle;
    cfg_val_type        type;
    struct sockaddr    *p_addr;


    rc = cfg_find_fmt(&handle, "%s/%s:%s", father, subid, name);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get handle of '%s:' son of %s",
            rc, subid, father);
        return rc;
    }
    
    type = CVT_ADDRESS;
    rc = cfg_get_instance(handle, &type, &p_addr);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get MAC address using OID %s/%s:%s",
            rc, father, subid, name);
        return rc;
    }
    if (p_addr->sa_family != AF_LOCAL)
    {
        ERROR("Unexpected address family %d", p_addr->sa_family);
    }
    else
    {
        memcpy(p_mac, p_addr->sa_data, ETHER_ADDR_LEN);
    }
    free(p_addr);

    return rc;
}



/*
 * Temporaraly located here
 */

const char * const cfg_oid_ta_port_admin_status_fmt =
    "/agent:%s/port:%u/admin:/status:";
const char * const cfg_oid_ta_port_oper_status_fmt =
    "/agent:%s/port:%u/state:/status:";
const char * const cfg_oid_ta_port_oper_speed_fmt =
    "/agent:%s/port:%u/state:/speed:";

const char * const cfg_oid_oper_status_fmt =
    "%s/state:/status:";
const char * const cfg_oid_oper_speed_fmt =
    "%s/state:/speed:";

/** Format string for Switch VLAN OID */
static const char * const cfg_oid_ta_vlan_fmt =
    "/agent:%s/vlan:%u";
/** Format string for Switch VLAN port OID */
static const char * const cfg_oid_ta_vlan_port_fmt =
    "/agent:%s/vlan:%u/port:%u";


/**
 * Add VLAN on switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 *
 * @return Status code.
 */
int
tapi_cfg_switch_add_vlan(const char *ta_name, uint16_t vid)
{
    /* 
     * Format string without specifiers + TA name + maximum length
     * of printed VID + '\0'
     */
    char        oid[strlen(cfg_oid_ta_vlan_fmt) - 4 +
                    strlen(ta_name) + 5 + 1];
    int         rc;
    cfg_handle  handle;

    
    ENTRY("ta_name=%s vid=%u", ta_name, vid);
    /* Prepare OID */
    sprintf(oid, cfg_oid_ta_vlan_fmt, ta_name, vid);

    rc = cfg_find_str(oid, &handle);
    if (rc == 0)
    {
        int             state;
        cfg_val_type    type;

        VERB("VLAN %u already exists on TA %s", vid, ta_name);
        type = CVT_INTEGER;
        rc = cfg_get_instance(handle, &type, &state);
        if (rc != 0)
        {
            VERB("cfg_get_instance() failed(0x%x)", rc);
            EXIT("0x%x", rc);
            return rc;
        }
        if (state != 1)
        {
            ERROR("VLAN %u disabled on TA %s", vid, ta_name);
            EXIT("ETEENV");
            return ETEENV;
        }
        rc = EEXIST;
        EXIT("EEXIST");
    }
    else
    {
        VERB("Add instance '%s'", oid);
        rc = cfg_add_instance_str(oid, &handle, CVT_INTEGER, 1);
        if (rc != 0)
        {
            ERROR("Addition of VLAN %u on TA %s failed(0x%x)",
                vid, ta_name, rc);
        }
        EXIT("0x%x", rc);
    }

    return rc;
}


/**
 * Delete VLAN from switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 *
 * @return Status code.
 */
int
tapi_cfg_switch_del_vlan(const char *ta_name, uint16_t vid)
{
    /* 
     * Format string without specifiers + TA name + maximum length
     * of printed VID + '\0'
     */
    char        oid[strlen(cfg_oid_ta_vlan_fmt) - 4 + 
                    strlen(ta_name) + 5 + 1];
    int         rc;
    cfg_handle  handle;


    /* Prepare OID */
    sprintf(oid, cfg_oid_ta_vlan_fmt, ta_name, vid);

    rc = cfg_find_str(oid, &handle);
    if (rc == 0)
    {
        rc = cfg_del_instance(handle, FALSE);
        if (rc != 0)
        {
            ERROR("Delete of VLAN %u on TA %s failed(0x%x)",
                vid, ta_name, rc);
        }
    }
    else
    {
        ERROR("VLAN %u on TA %s not found (error 0x%x)",
            vid, ta_name, rc);
    }

    return rc;
}

/**
 * Add port to VLAN on switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 * @param port      - port number
 *
 * @return Status code.
 */
int
tapi_cfg_switch_vlan_add_port(const char *ta_name, uint16_t vid,
                              unsigned int port)
{
    /* 
     * Format string without specifiers + TA name + maximum length
     * of printed VID + maximum length of printed port + \0'
     */
    char        oid[strlen(cfg_oid_ta_vlan_port_fmt) - 4 +
                    strlen(ta_name) + 5 + 10 + 1];
    int         rc;
    cfg_handle  handle;


    /* Prepare OID */
    sprintf(oid, cfg_oid_ta_vlan_port_fmt, ta_name, vid, port);

    rc = cfg_find_str(oid, &handle);
    if (rc == 0)
    {
        VERB("Port %u already in VLAN %u on TA %s", port, vid, ta_name);
        rc = EEXIST;
    }
    else
    {
        rc = cfg_add_instance_str(oid, &handle, CVT_NONE);
        if (rc != 0)
        {
            ERROR("Addition of port %u to VLAN %u on TA %s failed(0x%x)",
                port, vid, ta_name, rc);
        }
    }

    return rc;
}

/**
 * Delete port from VLAN on switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 * @param port      - port number
 *
 * @return Status code.
 */
int
tapi_cfg_switch_vlan_del_port(const char *ta_name, uint16_t vid,
                              unsigned int port)
{
    /* 
     * Format string without specifiers + TA name + maximum length
     * of printed VID + maximum length of printed port + \0'
     */
    char        oid[strlen(cfg_oid_ta_vlan_port_fmt) - 4 +
                    strlen(ta_name) + 5 + 10 + 1];
    int         rc;
    cfg_handle  handle;


    /* Prepare OID */
    sprintf(oid, cfg_oid_ta_vlan_port_fmt, ta_name, vid, port);

    rc = cfg_find_str(oid, &handle);
    if (rc == 0)
    {
        rc = cfg_del_instance(handle, FALSE);
        if (rc != 0)
        {
            ERROR("Delete of port %u from VLAN %u on TA %s failed(0x%x)",
                port, vid, ta_name, rc);
        }
    }
    else
    {
        ERROR("Port %u not in VLAN %u on TA %s (error 0x%x)",
            port, vid, ta_name, rc);
    }

    return rc;
}

/*
 * END of Temporaraly located here
 */

/* See the description in tapi_cfg.h */
int
tapi_cfg_add_route(const char *ta, int addr_family,
                   const void *dst_addr, int prefix,
                   const void *gw_addr, const char *dev,
                   uint32_t flags, int metric, int mss, int win, int irtt,
                   cfg_handle *cfg_hndl)
{
    return tapi_cfg_route_op(OP_ADD, ta, addr_family,
                             dst_addr, prefix, gw_addr, dev,
                             flags, metric, mss, win, irtt, cfg_hndl);
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_del_route_tmp(const char *ta, int addr_family,
                       const void *dst_addr, int prefix, 
                       const void *gw_addr, const char *dev,
                       uint32_t flags, int metric,
                       int mss, int win, int irtt)
{
    return tapi_cfg_route_op(OP_DEL, ta, addr_family,
                             dst_addr, prefix, gw_addr, dev,
                             flags, metric, mss, win, irtt, NULL);
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_del_route(cfg_handle *rt_hndl)
{
    int           rc;
    char         *ta;
    cfg_val_type  type = CVT_STRING;
    char         *rt_name;
    char         *rt_val;
    cfg_handle    agt_hndl;

    if (rt_hndl == NULL)
        return EINVAL;

    if (*rt_hndl == CFG_HANDLE_INVALID)
        return 0;
    
    if ((rc = cfg_get_father(*rt_hndl, &agt_hndl)) != 0)
    {
        ERROR("%s: Cannot get handle of the parent node", __FUNCTION__);
        return rc;
    }
    if ((rc = cfg_get_inst_name(*rt_hndl, &rt_name)) != 0)
    {
        ERROR("%s: Route handle cannot be processed", __FUNCTION__);
        return rc;
    }
    if ((rc = cfg_get_inst_name(agt_hndl, &ta)) != 0)
    {
        ERROR("%s: Cannot get Test Agent name", __FUNCTION__);
        free(rt_name);
        return rc;
    }
    if ((rc = cfg_get_instance(*rt_hndl, &type, &rt_val)) != 0)
    {
        ERROR("%s: Cannot get the value of the route", __FUNCTION__);
        free(ta);
        free(rt_name);
        return rc;
    }
    
    RING("Deleting route on TA %s: %s %s", ta, rt_name, rt_val);
    free(rt_val);
    free(ta);
    free(rt_name);

    rc = cfg_del_instance(*rt_hndl, FALSE);
    if (rc == 0)
    {
        *rt_hndl = CFG_HANDLE_INVALID;
    }

    return rc;
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_add_arp_entry(const char *ta,
                       const void *net_addr, const void *link_addr)
{
    return tapi_cfg_arp_op(OP_ADD, ta, net_addr, link_addr);
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_del_arp_entry(const char *ta,
                       const void *net_addr, const void *link_addr)
{
    return tapi_cfg_arp_op(OP_DEL, ta, net_addr, link_addr);
}

/**
 * Perform specified operation with routing table
 * 
 * @param op            Operation
 * @param ta            Test agent
 * @param addr_family   Address family 
 * @param dst_addr      Destination address of the route
 * @param prefix        Prefix for dst_addr
 * @param gw_addr       Gateway address of the route
 * @param cfg_hndl      Handle of the route in Configurator DB (OUT)
 *
 * @return Status code
 *
 * @retval 0  on success
 */                   
static int
tapi_cfg_route_op(enum tapi_cfg_oper op, const char *ta, int addr_family,
                  const void *dst_addr, int prefix, const void *gw_addr,
                  const char *dev, uint32_t flags,
                  int metric, int mss, int win, int irtt,
                  cfg_handle *cfg_hndl)
{
    cfg_handle  handle;
    char        dst_addr_str[INET6_ADDRSTRLEN];
    char        gw_addr_str[INET6_ADDRSTRLEN];
    char        route_inst_name[1024];
    char        rt_val[128];
    int         rc;
    int         netaddr_size = netaddr_get_size(addr_family);
    uint8_t    *dst_addr_copy;
    int         i;
    int         diff;
    uint8_t     mask;

    if (netaddr_size < 0)
    {
        ERROR("%s() unknown address family value", __FUNCTION__);
        return TE_RC(TE_TAPI, EINVAL);
    }
    
    if (prefix < 0 || prefix > (netaddr_size << 3))
    {
        ERROR("%s() fails: Incorrect prefix value specified %d", prefix);
        return TE_RC(TE_TAPI, EINVAL);
    }

    if ((dst_addr_copy = (uint8_t *)malloc(netaddr_size)) == NULL)
    {
        ERROR("%s() cannot allocate %d bytes for the copy of "
              "the network address", __FUNCTION__,
              netaddr_get_size(addr_family));

        return TE_RC(TE_TAPI, ENOMEM);
    }
    memcpy(dst_addr_copy, dst_addr, netaddr_size);
    
    /* Check that dst_addr & netmask == dst_addr */
    for (i = 0; i < netaddr_size; i++)
    {
        diff = ((i + 1) << 3) - prefix;
        
        if (diff < 0)
        {
            /* i-th byte is fully under the mask, so skip it */
            continue;
        }
        if (diff < 8)
            mask = 0xff << diff;
        else
            mask = 0;
        
        if ((dst_addr_copy[i] & mask) != dst_addr_copy[i])
        {
            WARN("%d-th byte of destination address specified in "
                 "the route does not cleared according to the prefix "
                 "prefix length %d, addr[%d] %x expected to be %x.\n"
                 "Clear these bits.",
                 i, prefix, i, dst_addr_copy[i], (dst_addr_copy[i] & mask));
            dst_addr_copy[i] &= mask;
        }
    }

    if (inet_ntop(addr_family, dst_addr_copy, dst_addr_str, 
                  sizeof(dst_addr_str)) == NULL)
    {
        ERROR("%s() fails converting binary destination address "
              "into a character string", __FUNCTION__);
        return TE_RC(TE_TAPI, errno);
    }
    
#define PUT_INTO_BUF(buf_, args...) \
    snprintf((buf_) + strlen(buf_), sizeof(buf_) - strlen(buf_), args)

    route_inst_name[0] = '\0';
    PUT_INTO_BUF(route_inst_name, "%s|%d", dst_addr_str, prefix);

    if (gw_addr != NULL)
    {
        if (inet_ntop(addr_family, gw_addr, gw_addr_str,
                      sizeof(gw_addr_str)) == NULL)
        {
            ERROR("%s() fails converting binary gateway address "
                  "into a character string", __FUNCTION__);
            return TE_RC(TE_TAPI, errno);
        }
        PUT_INTO_BUF(route_inst_name, ",gw=%s", gw_addr_str);
    }
    if (dev != NULL)
        PUT_INTO_BUF(route_inst_name, ",dev=%s", dev);
    if (metric != 0)
        PUT_INTO_BUF(route_inst_name, ",metric=%d", metric);
    if (mss != 0)
        PUT_INTO_BUF(route_inst_name, ",mss=%d", mss);
    if (win != 0)
        PUT_INTO_BUF(route_inst_name, ",window=%d", win);
    if (irtt != 0)
        PUT_INTO_BUF(route_inst_name, ",irtt=%d", irtt);
    if (flags & RTF_REJECT)
        PUT_INTO_BUF(route_inst_name, ",reject");

    rt_val[0] = '\0';

    if (flags & RTF_MODIFIED)
        PUT_INTO_BUF(rt_val, " mod");
    if (flags & RTF_DYNAMIC)
        PUT_INTO_BUF(rt_val, " dyn");
    if (flags & RTF_REINSTATE)
        PUT_INTO_BUF(rt_val, " reinstate");

    RING("%s route on TA %s: %s %s", op == OP_ADD ? "Adding" : "Deleting",
         ta, route_inst_name, rt_val);

    switch (op)
    {
        case OP_ADD:
            if ((rc = cfg_add_instance_fmt(&handle, CVT_STRING, rt_val,
                                           "/agent:%s/route:%s",
                                           ta, route_inst_name)) != 0)
            {
                ERROR("%s() fails adding a new route %s %s on '%s' Agent "
                      "errno = %X", __FUNCTION__, route_inst_name,
                      rt_val, ta, rc);

                return TE_RC(TE_TAPI, rc);
            }
            if (cfg_hndl != NULL)
                *cfg_hndl = handle;

            break;

        case OP_DEL:
            if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/route:%s",
                                           ta, route_inst_name)) != 0)
            {
                ERROR("%s() fails deleting route %s on '%s' Agent "
                      "errno = %X", __FUNCTION__, route_inst_name, ta, rc);
                return TE_RC(TE_TAPI, rc);
            }
            break;

        default:
            ERROR("%s(): Operation %d is not supported", __FUNCTION__, op);
            break;
    }

#undef PUT_INTO_BUF

    return 0;
}

/**
 * Perform specified operation with ARP table
 * 
 * @param op            Operation
 * @param ta            Test agent
 * @param net_addr      Network address
 * @param link_addr     Link-leyer address
 *
 * @return Status code
 *
 * @retval 0  on success
 */                   
static int
tapi_cfg_arp_op(enum tapi_cfg_oper op, const char *ta,
                const void *net_addr, const void *link_addr)
{
    struct sockaddr lnk_addr;
    cfg_handle      handle;
    char            net_addr_str[INET_ADDRSTRLEN];
    int             rc;

    if (inet_ntop(AF_INET, net_addr, net_addr_str, 
                  sizeof(net_addr_str)) == NULL)
    {
        ERROR("%s() fails converting binary IPv4 address  "
              "into a character string", __FUNCTION__);
        return TE_RC(TE_TAPI, errno);
    }

    memset(&lnk_addr, 0, sizeof(lnk_addr));
    lnk_addr.sa_family = AF_LOCAL;
    memcpy(&(lnk_addr.sa_data), link_addr, IFHWADDRLEN);

    switch (op)
    {
        case OP_ADD:
            if ((rc = cfg_add_instance_fmt(&handle, CVT_ADDRESS, &lnk_addr,
                                           "/agent:%s/arp:%s",
                                           ta, net_addr_str)) != 0)
            {
                /* TODO Correct formating */
                ERROR("%s() fails adding a new ARP entry "
                      "%s -> %x:%x:%x:%x:%x:%x on TA '%s'",
                      __FUNCTION__, net_addr_str,
                      *(((uint8_t *)link_addr) + 0),
                      *(((uint8_t *)link_addr) + 1),
                      *(((uint8_t *)link_addr) + 2),
                      *(((uint8_t *)link_addr) + 3),
                      *(((uint8_t *)link_addr) + 4),
                      *(((uint8_t *)link_addr) + 5), ta);
                return TE_RC(TE_TAPI, rc);
            }
            break;
            
        case OP_DEL:
            if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/arp:%s",
                                           ta, net_addr_str)) != 0)
            {
                ERROR("%s() fails deleting ARP entry for %s host "
                      "on TA '%s'", __FUNCTION__, net_addr_str, ta);
                return TE_RC(TE_TAPI, rc);
            }
            break;

        default:
            ERROR("%s(): Operation %d is not supported", __FUNCTION__, op);
            break;
    }
    
    return 0;
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_get_hwaddr(const char *ta,
                    const char *ifname,
                    void *hwaddr, unsigned int *hwaddr_len)
{
    char     buf[1024];
    int      rc;
    char   *ifname_bkp;
    char   *ptr;

    if (hwaddr == NULL || hwaddr_len == NULL)
    {
        ERROR("%s(): It is not allowed to have NULL 'hwaddr' or "
              "'hwaddr_len' parameter", __FUNCTION__);
        return TE_RC(TE_TAPI, EINVAL);
    }
    if (*hwaddr_len < IFHWADDRLEN)
    {
        ERROR("%s(): 'hwaddr' is too short");
        return TE_RC(TE_TAPI, EMSGSIZE);
    }

    /**
     * Configuration model does not support alias interfaces,
     * so that we should truncate trailing :XX part from the interface name.
     */
    if ((ifname_bkp = (char *)malloc(strlen(ifname) + 1)) == NULL)
    {
        return TE_RC(TE_TAPI, ENOMEM);
    }
    memcpy(ifname_bkp, ifname, strlen(ifname) + 1);

    if ((ptr = strchr(ifname_bkp, ':')) != NULL)
        *ptr = '\0';

    snprintf(buf, sizeof(buf), "/agent:%s/interface:%s",
             ta, ifname_bkp);
    if ((rc = tapi_cfg_base_if_get_mac(buf, hwaddr)) != 0)
    {
        return rc;
    }

    return 0;
}


/* See the description in tapi_cfg.h */
int
tapi_cfg_alloc_entry(const char *parent_oid, cfg_handle *entry)
{
    int             rc;
    cfg_handle      handle;
    cfg_val_type    type = CVT_INTEGER;
    int             value;

    rc = cfg_find_str(parent_oid, &handle);
    if (rc != 0)
    {
        ERROR("%s: Failed to convert OID '%s' to handle: %X",
              __FUNCTION__, parent_oid, rc);
        return rc;
    }
    
    for (rc = cfg_get_son(handle, &handle);
         rc == 0;
         rc = cfg_get_brother(handle, &handle))
    {
        rc = cfg_get_instance(handle, &type, &value);
        if (rc != 0)
        {
            ERROR("%s: Failed to get integer value by handle 0x%x: %X",
                  __FUNCTION__, handle, rc);
            break;
        }
        if (value == 0)
        {
            rc = cfg_set_instance(handle, type, 1);
            if (rc != 0)
            {
                ERROR("%s: Failed to set value of handle 0x%x to 1: %X",
                      __FUNCTION__, handle, rc);
            }
            break;
        }
    }

    if (rc == 0)
    {
        *entry = handle;
        INFO("Pool '%s' entry with Cfgr handle 0x%x allocated",
             parent_oid, *entry);
    }
    else
    {
        ERROR("Failed to allocate entry in '%s': %X", parent_oid, rc);
    }

    return rc;
}

/* See the description in tapi_cfg.h */
int
tapi_cfg_free_entry(cfg_handle *entry)
{
    int rc;

    if (entry == NULL)
    {
        ERROR("%s(): Invalid Cfgr handle pointer", __FUNCTION__);
        return TE_RC(TE_TAPI, EINVAL);
    }
    if (*entry == CFG_HANDLE_INVALID)
    {
        return 0;
    }

    rc = cfg_set_instance(*entry, CVT_INTEGER, 0);
    if (rc != 0)
    {
        ERROR("Failed to free entry by handle 0x%x: %X", *entry, rc);
    }
    else
    {
        INFO("Pool entry with Cfgr handle 0x%x freed", *entry);
        *entry= CFG_HANDLE_INVALID;
    }
    return rc;
}

