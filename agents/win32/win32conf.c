/** @file
 * @brief Windows Test Agent
 *
 * Windows TA configuring support
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "windows.h"
#include "w32api/iphlpapi.h"
#undef _SYS_TYPES_FD_SET
#include <w32api/winsock2.h>
#include "te_defs.h"
#include "iprtrmib.h"

#undef ERROR
#define TE_LGR_USER     "Windows Conf"

#include "te_errno.h"
#include "te_defs.h"
#include "logger_ta.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

/* TA name pointer */
extern char *ta_name;

/* Auxiliary buffer */
static char  buf[2048] = {0, };

#define PRINT(msg...) \
    do {                                                \
       printf(msg); printf("\n"); fflush(stdout);       \
    } while (0)

/** Route is direct "local interface" in terms of RFC 1354 */
#define FORW_TYPE_LOCAL  3

/** Route is indirect "remote destination" in terms of RFC 1354 */
#define FORW_TYPE_REMOTE 4

/** Static ARP entry */
#define ARP_STATIC      4

/** Dynamic ARP entry */
#define ARP_DYNAMIC     3

/** Fast conversion of the network mask to prefix */
#define MASK2PREFIX(mask, prefix)            \
    switch (ntohl(mask))                     \
    {                                        \
        case 0x0: prefix = 0; break;         \
        case 0x80000000: prefix = 1; break;  \
        case 0xc0000000: prefix = 2; break;  \
        case 0xe0000000: prefix = 3; break;  \
        case 0xf0000000: prefix = 4; break;  \
        case 0xf8000000: prefix = 5; break;  \
        case 0xfc000000: prefix = 6; break;  \
        case 0xfe000000: prefix = 7; break;  \
        case 0xff000000: prefix = 8; break;  \
        case 0xff800000: prefix = 9; break;  \
        case 0xffc00000: prefix = 10; break; \
        case 0xffe00000: prefix = 11; break; \
        case 0xfff00000: prefix = 12; break; \
        case 0xfff80000: prefix = 13; break; \
        case 0xfffc0000: prefix = 14; break; \
        case 0xfffe0000: prefix = 15; break; \
        case 0xffff0000: prefix = 16; break; \
        case 0xffff8000: prefix = 17; break; \
        case 0xffffc000: prefix = 18; break; \
        case 0xffffe000: prefix = 19; break; \
        case 0xfffff000: prefix = 20; break; \
        case 0xfffff800: prefix = 21; break; \
        case 0xfffffc00: prefix = 22; break; \
        case 0xfffffe00: prefix = 23; break; \
        case 0xffffff00: prefix = 24; break; \
        case 0xffffff80: prefix = 25; break; \
        case 0xffffffc0: prefix = 26; break; \
        case 0xffffffe0: prefix = 27; break; \
        case 0xfffffff0: prefix = 28; break; \
        case 0xfffffff8: prefix = 29; break; \
        case 0xfffffffc: prefix = 30; break; \
        case 0xfffffffe: prefix = 31; break; \
        case 0xffffffff: prefix = 32; break; \
         /* Error indication */              \
        default: prefix = 33; break;         \
    }

/** Fast conversion of the prefix to network mask */
#define PREFIX2MASK(prefix) htonl(prefix == 0 ? 0 : (~0) << (32 - (prefix)))

/* TA name pointer */
extern char *ta_name;

#define METRIC_DEFAULT  20

/*
 * Access routines prototypes (comply to procedure types
 * specified in rcf_ch_api.h).
 */
static int interface_list(unsigned int, const char *, char **);

static int net_addr_add(unsigned int, const char *, const char *,
                        const char *, const char *);
static int net_addr_del(unsigned int, const char *,
                        const char *, const char *);
static int net_addr_list(unsigned int, const char *, char **,
                         const char *);

static int prefix_get(unsigned int, const char *, char *,
                       const char *, const char *);
static int prefix_set(unsigned int, const char *, const char *,
                       const char *, const char *);


static int broadcast_get(unsigned int, const char *, char *,
                         const char *, const char *);
static int broadcast_set(unsigned int, const char *, const char *,
                         const char *, const char *);

static int link_addr_get(unsigned int, const char *, char *,
                         const char *);

static int ifindex_get(unsigned int, const char *, char *,
                       const char *);

static int status_get(unsigned int, const char *, char *,
                      const char *);
static int status_set(unsigned int, const char *, const char *,
                      const char *);

static int mtu_get(unsigned int, const char *, char *,
                   const char *);

static int arp_get(unsigned int, const char *, char *,
                   const char *, const char *);
static int arp_set(unsigned int, const char *, const char *,
                   const char *, const char *);
static int arp_add(unsigned int, const char *, const char *,
                   const char *, const char *);
static int arp_del(unsigned int, const char *,
                   const char *, const char *);
static int arp_list(unsigned int, const char *, char **);

static int route_get(unsigned int, const char *, char *,
                     const char *);
static int route_set(unsigned int, const char *, const char *,
                     const char *);
static int route_add(unsigned int, const char *, const char *,
                     const char *);
static int route_del(unsigned int, const char *,
                     const char *);
static int route_list(unsigned int, const char *, char **);

/* Volatile subtree */
static rcf_pch_cfg_object node_volatile_arp =
    { "arp", 0, NULL, NULL,
      (rcf_ch_cfg_get)arp_get, (rcf_ch_cfg_set)arp_set,
      (rcf_ch_cfg_add)arp_add, (rcf_ch_cfg_del)arp_del,
      (rcf_ch_cfg_list)arp_list, NULL, NULL};

RCF_PCH_CFG_NODE_NA(node_volatile, "volatile", &node_volatile_arp, NULL);

/* win32 Test Agent configuration tree */
static rcf_pch_cfg_object node_route =
    { "route", 0, NULL, &node_volatile,
      (rcf_ch_cfg_get)route_get, (rcf_ch_cfg_set)route_set,
      (rcf_ch_cfg_add)route_add, (rcf_ch_cfg_del)route_del,
      (rcf_ch_cfg_list)route_list, NULL, NULL};

static rcf_pch_cfg_object node_arp =
    { "arp", 0, NULL, &node_route,
      (rcf_ch_cfg_get)arp_get, (rcf_ch_cfg_set)arp_set,
      (rcf_ch_cfg_add)arp_add, (rcf_ch_cfg_del)arp_del,
      (rcf_ch_cfg_list)arp_list, NULL, NULL};

RCF_PCH_CFG_NODE_RW(node_status, "status", NULL, NULL,
                    status_get, status_set);

RCF_PCH_CFG_NODE_RW(node_mtu, "mtu", NULL, &node_status,
                    mtu_get, NULL);

RCF_PCH_CFG_NODE_RO(node_link_addr, "link_addr", NULL, &node_mtu,
                    link_addr_get);


RCF_PCH_CFG_NODE_RW(node_broadcast, "broadcast", NULL, NULL,
                    broadcast_get, broadcast_set);
RCF_PCH_CFG_NODE_RW(node_prefix, "prefix", NULL, &node_broadcast,
                    prefix_get, prefix_set);


RCF_PCH_CFG_NODE_COLLECTION(node_net_addr, "net_addr",
                            &node_prefix, &node_link_addr,
                            net_addr_add, net_addr_del,
                            net_addr_list, NULL);

RCF_PCH_CFG_NODE_RO(node_ifindex, "index", NULL, &node_net_addr,
                    ifindex_get);

RCF_PCH_CFG_NODE_COLLECTION(node_interface, "interface",
                            &node_ifindex, &node_arp,
                            NULL, NULL, interface_list, NULL);

RCF_PCH_CFG_NODE_AGENT(node_agent, &node_interface);

static MIB_IFROW if_entry;


/** Update information in if_entry. Local variable ifname should exist */
#define GET_IF_ENTRY \
    do {                                                        \
        char *tmp;                                              \
                                                                \
        if (ifname == NULL ||                                   \
            strncmp(ifname, "intf", 4) != 0 ||                  \
            (if_entry.dwIndex = strtol(ifname + 4, &tmp, 10),   \
             tmp == ifname) || *tmp != 0 ||                     \
            GetIfEntry(&if_entry) != 0)                         \
        {                                                       \
            return TE_RC(TE_TA_WIN32, ENOENT);                  \
        }                                                       \
    } while (0)

#define RETURN_BUF

/**
 * Allocate memory and get some SNMP-like table; variable table of the
 * type _type should be defined.
 */
#define GET_TABLE(_type, _func) \
    do {                                                                \
        DWORD size = 0, rc;                                             \
                                                                        \
        if ((table = (_type *)malloc(sizeof(_type))) == NULL)           \
            return TE_RC(TE_TA_WIN32, ENOMEM);                          \
                                                                        \
        if ((rc = _func(table, &size, 0)) == ERROR_INSUFFICIENT_BUFFER) \
        {                                                               \
            table = (_type *)realloc(table, size);                      \
        }                                                               \
        else if (rc == NO_ERROR)                                        \
        {                                                               \
            free(table);                                                \
            table = NULL;                                               \
        }                                                               \
        else                                                            \
        {                                                               \
            ERROR("%s failed, error %d", #_func, rc);                   \
            free(table);                                                \
            return TE_RC(TE_TA_WIN32, ETEWIN);                          \
        }                                                               \
                                                                        \
        if ((rc = _func(table, &size, 0)) != NO_ERROR)                  \
        {                                                               \
            ERROR("%s failed, error %d", #_func, rc);                   \
            free(table);                                                \
            return TE_RC(TE_TA_WIN32, ETEWIN);                          \
        }                                                               \
                                                                        \
        if (table->dwNumEntries == 0)                                   \
        {                                                               \
            free(table);                                                \
            table = NULL;                                               \
        }                                                               \
    } while (0)

/** Find an interface for destination IP */
static int
find_ifindex(DWORD addr, DWORD *ifindex)
{
    MIB_IPFORWARDTABLE *table;
    
    DWORD index = 0;
    DWORD mask_max = 0;
    int   i;

    GET_TABLE(MIB_IPFORWARDTABLE, GetIpForwardTable);
    
    if (table == NULL)
        return TE_RC(TE_TA_WIN32, ENOENT);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if ((table->table[i].dwForwardDest & 
             table->table[i].dwForwardMask) !=
            (addr &  table->table[i].dwForwardMask))
        {
            continue;
        }
        
        if (ntohl(table->table[i].dwForwardMask) > mask_max || index == 0)
        {
            mask_max = table->table[i].dwForwardMask;
            index = table->table[i].dwForwardIfIndex;
            if (mask_max == 0xFFFFFFFF)
                break;
        }
    }
    free(table);

    if (index == 0)
        return TE_RC(TE_TA_WIN32, ENOENT);
        
    *ifindex = index;
    
    return 0;
}


/**
 * Get root of the tree of supported objects.
 *
 * @return root pointer
 */
rcf_pch_cfg_object *
rcf_ch_conf_root()
{
    return &node_agent;
}

/**
 * Get Test Agent name.
 *
 * @return name pointer
 */
const char *
rcf_ch_conf_agent()
{
    return ta_name;
}

/**
 * Release resources allocated for configuration support.
 */
void
rcf_ch_conf_release()
{
}

/**
 * Get instance list for object "agent/interface".
 *
 * @param gid           group identifier (unused)
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return error code
 * @retval 0            success
 * @retval ENOMEM       cannot allocate memory
 */
static int
interface_list(unsigned int gid, const char *oid, char **list)
{
    MIB_IFTABLE *table;
    char        *s = buf;
    int          i;

    UNUSED(gid);
    UNUSED(oid);

    GET_TABLE(MIB_IFTABLE, GetIfTable);
    if (table == NULL)
    {
        if ((*list = strdup(" ")) == NULL)
            return TE_RC(TE_TA_WIN32, ENOMEM);
        else
            return 0;
    }

    buf[0] = 0;

    for (i = 0; i < (int)table->dwNumEntries; i++)
        s += sprintf(s, "intf%lu ", table->table[i].dwIndex);

    free(table);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_WIN32, ENOMEM);

    return 0;
}

/**
 * Get index of the interface.
 *
 * @param gid           request group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         location for interface index
 * @param ifname        name of the interface (like "eth0")
 *
 * @return error code
 */
static int
ifindex_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

    sprintf(value, "%lu", if_entry.dwIndex);

    return 0;
}

/** Check, if IP address exist for the current if_entry */
static int
ip_addr_exist(DWORD addr, MIB_IPADDRROW *data)
{
    MIB_IPADDRTABLE *table;
    int              i;

    GET_TABLE(MIB_IPADDRTABLE, GetIpAddrTable);

    if (table == NULL)
        return TE_RC(TE_TA_WIN32, ENOENT);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwIndex == if_entry.dwIndex &&
            table->table[i].dwAddr == addr)
        {
            if (data != NULL)
                *data = table->table[i];
            free(table);
            return 0;
        }
    }

    free(table);

    return TE_RC(TE_TA_WIN32, ENOENT);
}

/**
 * Configure IPv4 address for the interface.
 * If the address does not exist, alias interface is created.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return error code
 */
static int
net_addr_add(unsigned int gid, const char *oid, const char *value,
             const char *ifname, const char *addr)
{
    DWORD a, m;

    ULONG nte_context;
    ULONG nte_instance;

    int rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if ((a = inet_addr(addr)) == INADDR_NONE ||
        (a & 0xe0000000) == 0xe0000000)
        return TE_RC(TE_TA_WIN32, EINVAL);

    m = (a & htonl(0x80000000)) == 0 ? htonl(0xFF000000) :
        (a & htonl(0xC0000000)) == htonl(0x80000000) ?
        htonl(0xFFFF0000) : htonl(0xFFFFFF00);

    GET_IF_ENTRY;

    if ((rc = AddIPAddress(*(IPAddr *)&a, *(IPAddr *)&m, if_entry.dwIndex,
                            &nte_context, &nte_instance)) != NO_ERROR)
    {
        ERROR("AddIpAddress() failed, error %x", rc);
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }

    return 0;
}

/**
 * Clear interface address of the down interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return error code
 */

static int
net_addr_del(unsigned int gid, const char *oid,
             const char *ifname, const char *addr)
{
    PIP_ADAPTER_INFO table, info; 
    DWORD            size = 0; 

    DWORD a;
    int   rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((a = inet_addr(addr)) == INADDR_NONE)
        return TE_RC(TE_TA_WIN32, EINVAL);

    GET_IF_ENTRY;

    GetAdaptersInfo(NULL, &size);
    table = (PIP_ADAPTER_INFO)malloc(size);                      
                                                                    
    if ((rc = GetAdaptersInfo(table, &size)) != NO_ERROR)                  
    {                                                               
        ERROR("GetAdaptersInfo failed, error %d", rc);
        free(table);                                                
        return TE_RC(TE_TA_WIN32, ETEWIN);                          
    }                                                               

    for (info = table; info != NULL; info = info->Next)
    {
       IP_ADDR_STRING *addrlist;
       
       if (info->Index != if_entry.dwIndex)
           continue;
       
       for (addrlist = &(info->IpAddressList); 
            addrlist != NULL;
            addrlist = addrlist->Next)
        {
            if (strcmp(addr, addrlist->IpAddress.String) == 0)
            {
                if ((rc = DeleteIPAddress(addrlist->Context)) != NO_ERROR)
                {
                    ERROR("DeleteIPAddress() failed; error %d\n", rc);
                    free(table);
                    return TE_RC(TE_TA_WIN32, ETEWIN);;
                }
                free(table);
                return 0;
            }
        }
    }
    free(table);

    return TE_RC(TE_TA_WIN32, ENOENT);
}

/**
 * Get instance list for object "agent/interface/net_addr".
 *
 * @param gid           group identifier (unused)
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 * @param ifname        interface name
 *
 * @return error code
 * @retval 0                    success
 * @retval ETENOSUCHNAME        no such instance
 * @retval ENOMEM               cannot allocate memory
 */
static int
net_addr_list(unsigned int gid, const char *oid, char **list,
              const char *ifname)
{
    MIB_IPADDRTABLE *table;
    int              i;
    char            *s = buf;

    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;
    GET_TABLE(MIB_IPADDRTABLE, GetIpAddrTable);
    if (table == NULL)
    {
        if ((*list = strdup(" ")) == NULL)
            return TE_RC(TE_TA_WIN32, ENOMEM);
        else
            return 0;
    }

    buf[0] = 0;

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwIndex != if_entry.dwIndex)
            continue;

        s += sprintf(s, "%s ",
                 inet_ntoa(*(struct in_addr *)&(table->table[i].dwAddr)));
    }

    free(table);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_WIN32, ENOMEM);

    return 0;
}

/**
 * Get netmask (prefix) of the interface address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         netmask location (netmask is presented in dotted
 *                      notation)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return error code
 */
static int
prefix_get(unsigned int gid, const char *oid, char *value,
            const char *ifname, const char *addr)
{
    MIB_IPADDRROW data;
    DWORD         a;
    int           rc;
    int           prefix;

    UNUSED(gid);
    UNUSED(oid);

    if ((a = inet_addr(addr)) == INADDR_NONE)
    {
        if (strcmp(addr, "255.255.255.255") != 0)
            return TE_RC(TE_TA_WIN32, EINVAL);
    }

    GET_IF_ENTRY;

    if ((rc = ip_addr_exist(a, &data)) != 0)
        return rc;

    MASK2PREFIX(data.dwMask, prefix);

    snprintf(value, RCF_MAX_VAL, "%d", prefix);

    return 0;
}

/**
 * Change netmask (prefix) of the interface address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         pointer to the new network mask in dotted notation
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return error code
 */
static int
prefix_set(unsigned int gid, const char *oid, const char *value,
            const char *ifname, const char *addr)
{
    DWORD    a, m;
    uint8_t  prefix;
    char    *end;
    int      rc;

    ULONG nte_context;
    ULONG nte_instance;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((a = inet_addr(addr)) == INADDR_NONE)
        return TE_RC(TE_TA_WIN32, EINVAL);

    prefix = strtol(value, &end, 10);
    if (value == end || *end != 0)
    {
        ERROR("Invalid value '%s' of prefix length", value);
        return TE_RC(TE_TA_WIN32, ETEFMT);
    }

    if (prefix > 32)
    {
        ERROR("Invalid prefix '%s' to be set", value);
        return TE_RC(TE_TA_WIN32, EINVAL);
    }

    m = PREFIX2MASK(prefix);
    
    if ((rc = net_addr_del(0, NULL, ifname, addr)) != 0)
        return rc;

    if ((rc = AddIPAddress(*(IPAddr *)&a, *(IPAddr *)&m,
                           if_entry.dwIndex, &nte_context, 
                           &nte_instance)) != NO_ERROR)
    {
        ERROR("AddIpAddr() failed, error %d", rc);
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }

    return 0;
}

/**
 * Get broadcast address of the interface address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         netmask location (netmask is presented in dotted
 *                      notation)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return error code
 */
static int
broadcast_get(unsigned int gid, const char *oid, char *value,
              const char *ifname, const char *addr)
{
    MIB_IPADDRROW data;
    DWORD         a;
    DWORD         b;
    int           rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((a = inet_addr(addr)) == INADDR_NONE)
    {
        if (strcmp(addr, "255.255.255.255") != 0)
            return TE_RC(TE_TA_WIN32, EINVAL);
    }

    GET_IF_ENTRY;

    if ((rc = ip_addr_exist(a, &data)) != 0)
        return rc;

    b = (~data.dwMask) | (a & data.dwMask);

    snprintf(value, RCF_MAX_VAL, "%s", inet_ntoa(*(struct in_addr *)&b));

    return 0;
}

/**
 * Change broadcast address of the interface address - does nothing.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         pointer to the new network mask in dotted notation
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return error code
 */
static int
broadcast_set(unsigned int gid, const char *oid, const char *value,
              const char *ifname, const char *addr)
{
    MIB_IPADDRROW data;
    DWORD         a;
    int           rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if ((a = inet_addr(addr)) == INADDR_NONE)
    {
        if (strcmp(addr, "255.255.255.255") != 0)
            return TE_RC(TE_TA_WIN32, EINVAL);
    }

    GET_IF_ENTRY;

    if ((rc = ip_addr_exist(a, &data)) != 0)
        return rc;
    return 0;
}

/**
 * Get hardware address of the interface.
 * Only MAC addresses are supported now.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         location for hardware address (address is returned
 *                      as XX:XX:XX:XX:XX:XX)
 * @param ifname        name of the interface (like "eth0")
 *
 * @return error code
 */
static int
link_addr_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    uint8_t *ptr = if_entry.bPhysAddr;

    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

    if (if_entry.dwPhysAddrLen != 6)
        snprintf(value, RCF_MAX_VAL, "00:00:00:00:00:00");
    else
        snprintf(value, RCF_MAX_VAL, "%02x:%02x:%02x:%02x:%02x:%02x",
                 ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);

    return 0;
}

/**
 * Get MTU of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return error code
 */
static int
mtu_get(unsigned int gid, const char *oid, char *value,
        const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

    sprintf(value, "%lu", if_entry.dwMtu);

    return 0;
}

/**
 * Get status of the interface ("0" - down or "1" - up).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return error code
 */
static int
status_get(unsigned int gid, const char *oid, char *value,
           const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

    sprintf(value, "%d",
            if_entry.dwOperStatus == MIB_IF_OPER_STATUS_CONNECTED ||
            if_entry.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL ?
                1 : 0);

    return 0;
}

/**
 * Change status of the interface. If virtual interface is put to down
 * state, it is de-installed and information about it is stored in the
 * list of down interfaces.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return error code
 */
static int
status_set(unsigned int gid, const char *oid, const char *value,
           const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

    if (strcmp(value, "0") == 0)
        if_entry.dwAdminStatus = MIB_IF_ADMIN_STATUS_DOWN;
    else if (strcmp(value, "1") == 0)
        if_entry.dwAdminStatus = MIB_IF_ADMIN_STATUS_UP;
    else
        return TE_RC(TE_TA_WIN32, EINVAL);

    if (SetIfEntry(&if_entry) != 0)
        return TE_RC(TE_TA_WIN32, ENOENT);

    return 0;
}

/**
 * Get ARP entry value (hardware address corresponding to IPv4).
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param value          location for the value
 *                       (XX:XX:XX:XX:XX:XX is returned)
 * @param addr           IPv4 address in the dotted notation
 * @param addr_volatile  IPv4 address in case of volatile ARP subtree,
 *                       in this case @p addr parameter points to zero
 *                       length string
 *
 * @return error code
 */
static int
arp_get(unsigned int gid, const char *oid, char *value,
        const char *addr, const char *addr_volatile)
{
    MIB_IPNETTABLE *table;
    DWORD           a;
    int             i;
    DWORD           type = ARP_STATIC;

    UNUSED(gid);
    UNUSED(oid);
    
    /* 
     * Determine which subtree we are working with
     * (volatile or non-volatile).
     */
    if (strstr(oid, node_volatile.sub_id) != NULL)
    {
        /*
         * Volatile subtree, as soon as its instance names are
         * /agent:NAME/volatile:/arp:ADDR,
         * in which case we have:
         * + addr          - "" (empty string 'volatile' instance name);
         * + addr_volatile - "ADDR" (dynamic ARP entry name).
         */
        type = ARP_DYNAMIC;
        addr = addr_volatile;
    }
    
    if ((a = inet_addr(addr)) == INADDR_NONE)
        return TE_RC(TE_TA_WIN32, EINVAL);
        
    GET_TABLE(MIB_IPNETTABLE, GetIpNetTable);
    if (table == NULL)
        return TE_RC(TE_TA_WIN32, ENOENT);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (a == table->table[i].dwAddr && table->table[i].dwType == type)
        {
            uint8_t *ptr = table->table[i].bPhysAddr;

            if (table->table[i].dwPhysAddrLen != 6)
            {
                free(table);
                return TE_RC(TE_TA_WIN32, ENOENT);
            }
            snprintf(value, RCF_MAX_VAL, "%02x:%02x:%02x:%02x:%02x:%02x",
                     ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
            free(table);
            return 0;
        }
    }

    free(table);

    return TE_RC(TE_TA_WIN32, ENOENT);
}



/**
 * Change already existing ARP entry.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param value          new value pointer ("XX:XX:XX:XX:XX:XX")
 * @param addr           IPv4 address in the dotted notation
 * @param addr_volatile  IPv4 address in case of volatile ARP subtree,
 *                       in this case @p addr parameter points to zero
 *                       length string
 *
 * @return error code
 */
static int
arp_set(unsigned int gid, const char *oid, const char *value,
        const char *addr, const char *addr_volatile)
{
    int rc;

    if ((rc = arp_del(gid, oid, addr, addr_volatile)) != 0)
        return rc;

    return arp_add(gid, oid, value, addr, addr_volatile);
}

/**
 * Add a new ARP entry.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param value          new entry value pointer ("XX:XX:XX:XX:XX:XX")
 * @param addr           IPv4 address in the dotted notation
 * @param addr_volatile  IPv4 address in case of volatile ARP subtree,
 *                       in this case @p addr parameter points to zero
 *                       length string
 *
 * @return error code
 */
static int
arp_add(unsigned int gid, const char *oid, const char *value,
        const char *addr, const char *addr_volatile)
{
    char           val[32];
    MIB_IPNETROW   entry;
    int            int_mac[6];
    int            rc;
    int            i;

    UNUSED(gid);
    UNUSED(oid);
    
    if (arp_get(0, oid, val, addr, addr_volatile) == 0)
        return TE_RC(TE_TA_WIN32, EEXIST);

    memset(&entry, 0, sizeof(entry));
    
    if (strstr(oid, node_volatile.sub_id) != NULL)
    {
        entry.dwType = ARP_DYNAMIC;
        addr = addr_volatile;
    }
    else
        entry.dwType = ARP_STATIC;

    if (sscanf(value, "%2x:%2x:%2x:%2x:%2x:%2x%s", int_mac, int_mac + 1,
            int_mac + 2, int_mac + 3, int_mac + 4, int_mac + 5, buf) != 6)
    {
        return TE_RC(TE_TA_WIN32, EINVAL);
    }

    for (i = 0; i < 6; i++)
        entry.bPhysAddr[i] = (unsigned char)int_mac[i];

    entry.dwAddr = inet_addr(addr);
    if ((rc = find_ifindex(entry.dwAddr, &entry.dwIndex)) != 0)
        return rc;

    entry.dwPhysAddrLen = 6;
    if ((rc = CreateIpNetEntry(&entry)) != NO_ERROR)
    {
        ERROR("CreateIpNetEntry() failed, error %d", rc);
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }
    return 0;
}

/**
 * Delete ARP entry.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param addr           IPv4 address in the dotted notation
 * @param addr_volatile  IPv4 address in case of volatile ARP subtree,
 *                       in this case @p addr parameter points to zero
 *                       length string
 *
 * @return error code
 */
static int
arp_del(unsigned int gid, const char *oid, const char *addr, 
        const char *addr_volatile)
{
    MIB_IPNETTABLE *table;
    int             i;
    DWORD           a;
    int             rc;
    DWORD           type = ARP_STATIC;
    te_bool         found = FALSE;

    UNUSED(gid);
    UNUSED(oid);

    if (strstr(oid, node_volatile.sub_id) != NULL)
    {
        type = ARP_DYNAMIC;
        addr = addr_volatile;
    }

    if ((a = inet_addr(addr)) == INADDR_NONE)
        return TE_RC(TE_TA_WIN32, EINVAL);

    GET_TABLE(MIB_IPNETTABLE, GetIpNetTable);
    if (table == NULL)
        return TE_RC(TE_TA_WIN32, ENOENT);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwAddr == a && table->table[i].dwType == type)
        {
            if ((rc = DeleteIpNetEntry(table->table + i)) != 0)
            {
                if (type == ARP_STATIC)
                {
                    ERROR("DeleteIpNetEntry() failed, error %d", rc);
                    free(table);
                    return TE_RC(TE_TA_WIN32, ETEWIN);
                }
            }
            found = TRUE;
            /* Continue to delete entries on other interfaces */
        }
    }
    free(table);
    return (!found && type == ARP_STATIC) ? TE_RC(TE_TA_WIN32, ENOENT) : 0;
}

/**
 * Get instance list for object "agent/arp".
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param list          location for the list pointer
 *
 * @return error code
 */
static int
arp_list(unsigned int gid, const char *oid, char **list)
{
    MIB_IPNETTABLE *table;
    int             i;
    char           *s = buf;
    DWORD           type = ARP_STATIC;

    UNUSED(gid);
    UNUSED(oid);

    if (strstr(oid, node_volatile.sub_id) != NULL)
        type = ARP_DYNAMIC;

    GET_TABLE(MIB_IPNETTABLE, GetIpNetTable);
    if (table == NULL)
    {
        if ((*list = strdup(" ")) == NULL)
            return TE_RC(TE_TA_WIN32, ENOMEM);
        else
            return 0;
    }

    buf[0] = 0;

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwPhysAddrLen != 6 ||
            table->table[i].dwType != type)
            continue;
        s += sprintf(s, "%s ",
                 inet_ntoa(*(struct in_addr *)&(table->table[i].dwAddr)));
    }

    free(table);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_WIN32, ENOMEM);

    return 0;
}


/** Route entry data structure */
typedef struct route_entry {
    DWORD dst; /**< Destination address */
    int   prefix; /**< Destination address prefix */
    DWORD gw; /**< Gwateway address, in case 'forw_type' is 
                   FORW_TYPE_REMOTE */
    DWORD if_index; /**< Interface index, in case 'forw_type' is 
                         FORW_TYPE_LOCAL */
    DWORD forw_type; /**< Forward type value () */
    DWORD metric; /**< Primary route metric */
} route_entry_t;

/** 
 * Parse route instance name and returns data structure of 
 * type 'route_entry_t'
 *
 * @param inst_name  route instance name
 * @param rt         routing entry (OUT)
 */
static int
route_parse_inst_name(const char *inst_name, route_entry_t *rt)
{
    static char  inst_copy[RCF_MAX_VAL];
    int          int_val;
    char        *tmp;
    char        *tmp1;
    char        *term_byte;
    char        *ptr;
    char        *end_ptr;

    memset(rt, 0, sizeof(*rt));
    strncpy(inst_copy, inst_name, sizeof(inst_copy));
    inst_copy[sizeof(inst_copy) - 1] = '\0';

    if ((tmp = strchr(inst_copy, '|')) == NULL)
        return TE_RC(TE_TA_WIN32, ETENOSUCHNAME);

    *tmp = 0;

    if ((rt->dst = inet_addr(inst_copy)) == INADDR_NONE)
    {
        if (strcmp(inst_copy, "255.255.255.255") != 0)
           return TE_RC(TE_TA_WIN32, ETENOSUCHNAME);
    }

    *tmp++ = '|';
    if (*tmp == '-' ||
        (rt->prefix = strtol(tmp, &tmp1, 10), tmp == tmp1 ||
         rt->prefix > 32))
    {
        return TE_RC(TE_TA_WIN32, ETENOSUCHNAME);
    }
    tmp = tmp1;
    term_byte = (char *)(tmp + strlen(tmp));

    if ((ptr = strstr(tmp, "gw=")) != NULL)
    {
        end_ptr = ptr += strlen("gw=");
        while (*end_ptr != ',' && *end_ptr != '\0')
            end_ptr++;
        *end_ptr = '\0';

        if ((rt->gw = inet_addr(ptr)) == INADDR_NONE)
        {
            return TE_RC(TE_TA_WIN32, ETENOSUCHNAME);
        }
        if (term_byte != end_ptr)
            *end_ptr = ',';
        rt->forw_type = FORW_TYPE_REMOTE;

        assert(strstr(tmp, "dev=") == NULL);
    }
    else if ((ptr = strstr(tmp, "dev=")) != NULL)
    {
        end_ptr = ptr += strlen("dev=");
        while (*end_ptr != ',' && *end_ptr != '\0')
            end_ptr++;
        *end_ptr = '\0';

        if (sscanf(ptr, "intf%d", (int *)&rt->if_index) != 1)
        {
            return TE_RC(TE_TA_WIN32, ETENOSUCHNAME);
        }

        if (term_byte != end_ptr)
            *end_ptr = ',';
            
        rt->forw_type = FORW_TYPE_LOCAL;
    }
    else
    {
        /* 
         * Route can be direct (via interface),
         * or indirect (via gateway) 
         */
        assert(0);
    }

    if ((ptr = strstr(tmp, "metric=")) != NULL)
    {
        end_ptr = ptr += strlen("metric=");
        while (*end_ptr != ',' && *end_ptr != '\0')
            end_ptr++;
        *end_ptr = '\0';
        
        if (*ptr == '\0' || *ptr == '-' ||
            (int_val = strtol(ptr, &end_ptr, 10), *end_ptr != '\0'))
        {
            return TE_RC(TE_TA_WIN32, ETENOSUCHNAME);
        }
        if (term_byte != end_ptr)
            *end_ptr = ',';
        rt->metric = int_val;
    }

    if (strstr(tmp, "mss=") != NULL || strstr(tmp, "window=") != NULL ||
        strstr(tmp, "irtt=") != NULL || strstr(tmp, "reject") != NULL)
    {
        return TE_RC(TE_TA_WIN32, EOPNOTSUPP);
    }

    return 0;
}

/**
 * Get route value (gateway IP address).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location (IPv4 address is returned in
 *                      dotted notation)
 * @param route         route instance name:
 *                      <IPv4 address in dotted notation>'|'<prefix length>
 *
 * @return error code
 */
static int
route_get(unsigned int gid, const char *oid, char *value,
          const char *route)
{
    MIB_IPFORWARDTABLE *table;
    route_entry_t       rt;
    int                 rc;
    int                 i;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = route_parse_inst_name(route, &rt)) != 0)
        return rc;

    if (rt.metric == 0)
        rt.metric = METRIC_DEFAULT;

    GET_TABLE(MIB_IPFORWARDTABLE, GetIpForwardTable);
    if (table == NULL)
        return TE_RC(TE_TA_WIN32, ENOENT);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        int p;

        if (table->table[i].dwForwardType != FORW_TYPE_LOCAL &&
            table->table[i].dwForwardType != FORW_TYPE_REMOTE)
        {
            continue;
        }

        MASK2PREFIX(table->table[i].dwForwardMask, p);
        if (table->table[i].dwForwardDest != rt.dst || p != rt.prefix ||
            table->table[i].dwForwardMetric1 != rt.metric || 
            (table->table[i].dwForwardType == FORW_TYPE_LOCAL &&
             table->table[i].dwForwardIfIndex != rt.if_index) ||
            (table->table[i].dwForwardType == FORW_TYPE_REMOTE &&
             table->table[i].dwForwardNextHop != rt.gw))
        {
            continue;
        }

        /*
         * win32 agent does not support values defined for routes
         * in configuration model.
         */
        value[0] = 0;
        free(table);
        return 0;
    }

    free(table);

    return TE_RC(TE_TA_WIN32, ENOENT);
}

/**
 * Change already existing route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param route         route instance name:
 *                      <IPv4 address in dotted notation>'|'<prefix length>
 *
 * @return error code
 */
static int
route_set(unsigned int gid, const char *oid, const char *value,
          const char *route)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(route);
    return TE_RC(TE_TA_WIN32, ENOTSUP);
}

/**
 * Add a new route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param route         route instance name:
 *                      <IPv4 address in dotted notation>'|'<prefix length>
 *
 * @return error code
 */
static int
route_add(unsigned int gid, const char *oid, const char *value,
          const char *route)
{
    MIB_IPFORWARDROW  entry;
    route_entry_t     rt;
    int               rc;
    int               i;
    
    MIB_IPFORWARDTABLE *table;
    
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if ((rc = route_parse_inst_name(route, &rt)) != 0)
        return rc;
        
    if (rt.metric == METRIC_DEFAULT)
    {
        ERROR("Failed to explicitly assign default metric to the route");
        return TE_RC(TE_TA_WIN32, EINVAL);
    }
        
    if (rt.metric == 0)
    {
        WARN("Default metric for the route to %s is assigned", route);
        rt.metric = METRIC_DEFAULT;
    }
        
    memset(&entry, 0, sizeof(entry));
    entry.dwForwardDest = rt.dst;
    entry.dwForwardNextHop = rt.gw;
    entry.dwForwardMask = PREFIX2MASK(rt.prefix);
    entry.dwForwardType = rt.forw_type;
    entry.dwForwardNextHop = rt.gw;
    entry.dwForwardIfIndex = rt.if_index;
    entry.dwForwardMetric1 = rt.metric;
    
    if (entry.dwForwardNextHop == 0)
        entry.dwForwardNextHop = rt.dst;
        
    if (entry.dwForwardIfIndex == 0)
    {
        if ((rc = find_ifindex(entry.dwForwardNextHop,
                               &entry.dwForwardIfIndex)) != 0)
        {
            return rc;
        }
    }
    
    entry.dwForwardProto = 3;
    
    /* Check, if exactly the same route except metric exists */
    GET_TABLE(MIB_IPFORWARDTABLE, GetIpForwardTable);
    if (table == NULL)
        return TE_RC(TE_TA_WIN32, ENOMEM);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwForwardDest == entry.dwForwardDest &&
            table->table[i].dwForwardMask == entry.dwForwardMask &&
            table->table[i].dwForwardType == entry.dwForwardType &&
            table->table[i].dwForwardIfIndex == entry.dwForwardIfIndex &&
            (table->table[i].dwForwardType != FORW_TYPE_REMOTE ||
             table->table[i].dwForwardNextHop == entry.dwForwardNextHop))
        {
            free(table);
            return TE_RC(TE_TA_WIN32, EEXIST);
        }
    }
    free(table);
    
    if ((rc = CreateIpForwardEntry(&entry)) != NO_ERROR)
    {
        ERROR("CreateIpForwardEntry() failed, error %d", rc);
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }
    
    return 0;
}


/**
 * Delete a route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param route         route instance name:
 *                      <IPv4 address in dotted notation>'|'<prefix length>
 *
 * @return error code
 */
static int
route_del(unsigned int gid, const char *oid, const char *route)
{
    MIB_IPFORWARDTABLE *table;

    int            i;
    int            rc;
    route_entry_t  rt;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = route_parse_inst_name(route, &rt)) != 0)
        return rc;

    GET_TABLE(MIB_IPFORWARDTABLE, GetIpForwardTable);
    if (table == NULL)
        return TE_RC(TE_TA_WIN32, ENOENT);
        
    if (rt.metric == 0)
        rt.metric = METRIC_DEFAULT;

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        int p;

        if (table->table[i].dwForwardType != FORW_TYPE_LOCAL &&
            table->table[i].dwForwardType != FORW_TYPE_REMOTE)
        {
            continue;
        }
        MASK2PREFIX(table->table[i].dwForwardMask, p);
        if (table->table[i].dwForwardDest != rt.dst || p != rt.prefix ||
            table->table[i].dwForwardMetric1 != rt.metric ||
            (table->table[i].dwForwardType == FORW_TYPE_LOCAL &&
             table->table[i].dwForwardIfIndex != rt.if_index) ||
            (table->table[i].dwForwardType == FORW_TYPE_REMOTE &&
             table->table[i].dwForwardNextHop != rt.gw))
        {
            continue;
        }
        
        if ((rc = DeleteIpForwardEntry(table->table + i)) != 0)
        {
            ERROR("DeleteIpForwardEntry() failed, error %d", rc);
            free(table);
            return TE_RC(TE_TA_WIN32, ETEWIN);
        }
        free(table);
        return 0;
    }

    return TE_RC(TE_TA_WIN32, ENOENT);
}

/**
 * Get instance list for object "agent/route".
 *
 * @param gid           group identifier (unused)
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return error code
 * @retval 0                    success
 * @retval ETENOSUCHNAME      no such instance
 * @retval ENOMEM               cannot allocate memory
 */
static int
route_list(unsigned int gid, const char *oid, char **list)
{
    MIB_IPFORWARDTABLE *table;
    int                 i;
    char               *end_ptr = buf + sizeof(buf);
    char               *ptr = buf;

    UNUSED(gid);
    UNUSED(oid);

    GET_TABLE(MIB_IPFORWARDTABLE, GetIpForwardTable);
    if (table == NULL)
    {
        if ((*list = strdup(" ")) == NULL)
            return TE_RC(TE_TA_WIN32, ENOMEM);
        else
            return 0;
    }

    buf[0] = 0;

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        int prefix;

        if (table->table[i].dwForwardType != FORW_TYPE_LOCAL &&
            table->table[i].dwForwardType != FORW_TYPE_REMOTE)
        {
            continue;
        }

        MASK2PREFIX(table->table[i].dwForwardMask, prefix);
        snprintf(ptr, end_ptr - ptr, "%s|%d",
                 inet_ntoa(*(struct in_addr *)
                     &(table->table[i].dwForwardDest)), prefix);
        ptr += strlen(ptr);
        if (table->table[i].dwForwardType == FORW_TYPE_REMOTE)
        {
            /* Route via gateway */
            snprintf(ptr, end_ptr - ptr, ",gw=%s",
                     inet_ntoa(*(struct in_addr *)
                         &(table->table[i].dwForwardNextHop)));
        }
        else
        {
            snprintf(ptr, end_ptr - ptr, ",dev=intf%d",
                     (int)(table->table[i].dwForwardIfIndex));
        }
        ptr += strlen(ptr);
        if (table->table[i].dwForwardMetric1 != METRIC_DEFAULT)
        {
            snprintf(ptr, end_ptr - ptr, ",metric=%d",
                     (int)(table->table[i].dwForwardMetric1));
            ptr += strlen(ptr);
        }
        snprintf(ptr, end_ptr - ptr, " ");
        ptr += strlen(ptr);
    }

    free(table);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_WIN32, ENOMEM);

    return 0;
}
