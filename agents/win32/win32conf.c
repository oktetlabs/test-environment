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
 * $Id: win32conf.c 3612 2004-07-19 10:13:31Z arybchik $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "windows.h"
#include "iprtrmib.h"
#include "w32api/iphlpapi.h"
#include <w32api/winsock2.h>

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

/* Fast conversion of the network mask to prefix */
#define MASK2PREFIX(mask, prefix)            \
    switch (mask)                            \
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

/* Fast conversion of the prefix to network mask */
#define PREFIX2MASK(prefix) (prefix == 0 ? 0 : (~0) << (32 - (prefix)))

/* TA name pointer */
extern char *ta_name;

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

static int netmask_get(unsigned int, const char *, char *,
                       const char *, const char *);
static int netmask_set(unsigned int, const char *, const char *,
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
                   const char *);
static int arp_set(unsigned int, const char *, const char *,
                   const char *);
static int arp_add(unsigned int, const char *, const char *,
                   const char *);
static int arp_del(unsigned int, const char *,
                   const char *);
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

/* Linux Test Agent configuration tree */
static rcf_pch_cfg_object node_route =
    { "route", 0, NULL, NULL,
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

RCF_PCH_CFG_NODE_RW(node_netmask, "netmask", NULL, NULL,
                    netmask_get, netmask_set);

RCF_PCH_CFG_NODE_COLLECTION(node_net_addr, "net_addr",
                            &node_netmask, &node_link_addr,
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
    
    DWORD size = 0;
    char *s = buf;
    int   i;

    UNUSED(gid);
    UNUSED(oid);

    table = (MIB_IFTABLE *)malloc(sizeof(MIB_IFTABLE));

    if (GetIfTable(table, &size, 0) == ERROR_INSUFFICIENT_BUFFER) 
        table = (MIB_IFTABLE *)realloc(table, size);
    else
    {
        ERROR("GetIfTable() failed, error %x", GetLastError());
        free(table);
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }
    
    if (GetIfTable(table, &size, 0) != NO_ERROR) 
    {
        ERROR("GetIfTable() failed, error %x", GetLastError());
        free(table);
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }
    
    if (table->dwNumEntries == 0)
    {
        ERROR("GetIfTable() returned");
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

typedef struct added_ip_addr {
    struct added_ip_addr *next;
    struct in_addr        addr;
    uint32_t              ifindex;
    ULONG                 nte_context;
} added_ip_addr;

static added_ip_addr *list = NULL;

/** Check, if IP address exist for the current if_entry */
static te_bool
ip_addr_exist(struct in_addr addr, MIB_IPADDRROW *data)
{
    MIB_IPADDRTABLE *table;
    DWORD            size = 0;
    int              i;
    
    table = (MIB_IPADDRTABLE *)malloc(sizeof(MIB_IFTABLE));

    if (GetIpAddrTable(table, &size, 0) == ERROR_INSUFFICIENT_BUFFER) 
        table = (MIB_IPADDRTABLE *)realloc(table, size);
    else
    {
        ERROR("GetIpAddrTable() failed, error %x", GetLastError());
        free(table);
        return FALSE;
    }
    
    if (GetIpAddrTable(table, &size, 0) != NO_ERROR) 
    {
        ERROR("GetIpAddrTable() failed, error %x", GetLastError());
        free(table);
        return FALSE;
    }
    
    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwIndex == if_entry.dwIndex &&
            table->table[i].dwAddr == addr.s_addr)
        {
            if (data != NULL)
                *data = table->table[i];
            free(table);
            return TRUE;
        }
    }

    free(table);

    return FALSE;
}

/**
 * Configure IPv4 address for the interface.
 * If the address does not exist, alias interface is created.
 *
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
    struct in_addr a, m;

    ULONG nte_context;
    ULONG nte_instance;
    
    added_ip_addr *tmp;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    
    if ((a.s_addr = inet_addr(addr)) == 0 || 
        (a.s_addr & 0xe0000000) == 0xe0000000)
    {
        return TE_RC(TE_TA_WIN32, EINVAL);
    }
    m.s_addr = (a.s_addr & htonl(0x80000000)) == 0 ? htonl(0xFF000000) :
               (a.s_addr & htonl(0xC0000000)) == htonl(0x80000000) ? 
               htonl(0xFFFF0000) : htonl(0xFFFFFF00);
    
    GET_IF_ENTRY;
    
    if (AddIPAddress(*(IPAddr *)&a, *(IPAddr *)&m, 
                     if_entry.dwIndex, &nte_context, &nte_instance) != 0)
    {
        ERROR("AddIpAddr() failed, error %x", GetLastError());
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }
    
    if ((tmp = (added_ip_addr *)calloc(sizeof(*tmp), 1)) == NULL)
    {
        DeleteIPAddress(nte_context);
        return TE_RC(TE_TA_WIN32, ENOMEM);
    }
    
    tmp->addr = a;
    tmp->ifindex = if_entry.dwIndex;
    tmp->nte_context = nte_context;
    tmp->next = list;
    list = tmp;
    
    return 0;
}

/**
 * Clear interface address of the down interface.
 *
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
    added_ip_addr *cur, *prev;
    
    struct in_addr a;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((a.s_addr = inet_addr(addr)) == 0)
        return TE_RC(TE_TA_WIN32, EINVAL);
    
    GET_IF_ENTRY;

    for (prev = NULL, cur = list; 
         cur != NULL && 
         !(cur->addr.s_addr == a.s_addr && cur->ifindex == if_entry.dwIndex);
         prev = cur, cur = cur->next);
         
    if (cur == NULL)
        return ip_addr_exist(a, NULL) ? TE_RC(TE_TA_WIN32, EPERM) :
                                        TE_RC(TE_TA_WIN32, ENOENT);
                                  
    if (DeleteIPAddress(cur->nte_context) != 0)
    {
        ERROR("DeleteIPAddr() failed, error %x", GetLastError());
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }
    
    if (prev)
        prev->next = cur->next;
    else
        list = cur->next;
    free(cur);
    
    return 0;
}

/**
 * Get instance list for object "agent/interface/net_addr".
 *
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
    DWORD            size = 0;
    int              i;
    struct in_addr   addr;
    
    UNUSED(gid);
    UNUSED(oid);
    
    GET_IF_ENTRY;

    table = (MIB_IPADDRTABLE *)malloc(sizeof(MIB_IFTABLE));

    if (GetIpAddrTable(table, &size, 0) == ERROR_INSUFFICIENT_BUFFER) 
        table = (MIB_IPADDRTABLE *)realloc(table, size);
    else
    {
        ERROR("GetIpAddrTable() failed, error %x", GetLastError());
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }
    
    if (GetIpAddrTable(table, &size, 0) != NO_ERROR) 
    {
        ERROR("GetIpAddrTable() failed, error %x", GetLastError());
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }
    
    buf[0] = 0;
    
    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwIndex != if_entry.dwIndex)
            continue;
            
        addr.s_addr = table->table[i].dwAddr;
        
        sprintf("%s ", inet_ntoa(addr));
    }

    free(table);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_WIN32, ENOMEM);

    return 0;
}

/**
 * Get netmask of the interface.
 *
 * @param oid           full object instence identifier (unused)
 * @param value         netmask location (netmask is presented in dotted
 *                      notation)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return error code
 */
static int
netmask_get(unsigned int gid, const char *oid, char *value,
            const char *ifname, const char *addr)
{
    MIB_IPADDRROW data;
    struct in_addr a;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((a.s_addr = inet_addr(addr)) == 0)
        return TE_RC(TE_TA_WIN32, EINVAL);
    
    GET_IF_ENTRY;
    
    if (!ip_addr_exist(a, &data))
        return TE_RC(TE_TA_WIN32, ENOENT);
    
    snprintf(value, RCF_MAX_VAL, "%s", 
            inet_ntoa(*(struct in_addr *)&(data.dwMask)));

    return 0;
}

/**
 * Change netmask of the interface.
 *
 * @param oid           full object instence identifier (unused)
 * @param value         pointer to the new network mask in dotted notation
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return error code
 */
static int
netmask_set(unsigned int gid, const char *oid, const char *value,
            const char *ifname, const char *addr)
{
    added_ip_addr *cur;
    struct in_addr a, m;
    uint8_t        prefix;
    ULONG          nte_instance;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((a.s_addr = inet_addr(addr)) == 0)
        return TE_RC(TE_TA_WIN32, EINVAL);

    if ((m.s_addr = inet_addr(value)) == 0)
        return TE_RC(TE_TA_WIN32, EINVAL);

    MASK2PREFIX(ntohl(m.s_addr), prefix);
    if (prefix > 32)
        return TE_RC(TE_TA_LINUX, EINVAL);

    GET_IF_ENTRY;
    
    for (cur = list; 
         cur != NULL && 
         !(cur->addr.s_addr == a.s_addr && cur->ifindex == if_entry.dwIndex);
         cur = cur->next);
         
    if (cur == NULL)
        return ip_addr_exist(a, NULL) ? TE_RC(TE_TA_WIN32, EPERM) :
                                        TE_RC(TE_TA_WIN32, ENOENT);
                                  
    if (DeleteIPAddress(cur->nte_context) != 0)
    {
        ERROR("DeleteIPAddr() failed, error %x", GetLastError());
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }
    
    if (AddIPAddress(*(IPAddr *)&a, *(IPAddr *)&m, 
                     cur->ifindex, &cur->nte_context, &nte_instance) != 0)
    {
        ERROR("AddIpAddr() failed, error %x", GetLastError());
        return TE_RC(TE_TA_WIN32, ETEWIN);
    }
    
    return 0;
}

/**
 * Get hardware address of the interface. Only MAC addresses are supported now.
 *
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
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);

    snprintf(value, RCF_MAX_VAL, "%02x:%02x:%02x:%02x:%02x:%02x",
             ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
    
    return 0;
}

/**
 * Get MTU of the interface.
 *
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
            if_entry.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL ? 1 : 0);
    
    return 0;
}

/**
 * Change status of the interface. If virtual interface is put to down state,
 * it is de-installed and information about it is stored in the list
 * of down interfaces.
 *
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
        return TE_RC(TE_TA_LINUX, EINVAL);

    if (SetIfEntry(&if_entry) != 0)
        return TE_RC(TE_TA_WIN32, ENOENT);
        
    return 0;
}

/**
 * Get ARP entry value (hardware address corresponding to IPv4).
 *
 * @param oid           full object instence identifier (unused)
 * @param value         location for the value (XX:XX:XX:XX:XX:XX is returned)
 * @param addr          IPv4 address in the dotted notation
 *
 * @return error code
 */
static int
arp_get(unsigned int gid, const char *oid, char *value,
        const char *addr)
{
    UNUSED(gid);
    UNUSED(oid);
    
}

/**
 * Change already existing ARP entry.
 *
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer ("XX:XX:XX:XX:XX:XX")
 * @param addr          IPv4 address in the dotted notation
 *
 * @return error code
 */
static int
arp_set(unsigned int gid, const char *oid, const char *value,
        const char *addr)
{
    UNUSED(gid);
    UNUSED(oid);
    
}

/**
 * Add a new ARP entry.
 *
 * @param oid           full object instence identifier (unused)
 * @param value         new entry value pointer ("XX:XX:XX:XX:XX:XX")
 * @param addr          IPv4 address in the dotted notation
 *
 * @return error code
 */
static int
arp_add(unsigned int gid, const char *oid, const char *value,
        const char *addr)
{
    UNUSED(gid);
    UNUSED(oid);
    
}

/**
 * Delete ARP entry.
 *
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param addr          IPv4 address in the dotted notation
 *
 * @return error code
 */
static int
arp_del(unsigned int gid, const char *oid, const char *addr)
{
    UNUSED(gid);
    UNUSED(oid);
    
}

/**
 * Get instance list for object "agent/arp".
 *
 * @param oid           full object instence identifier (unused)
 * @param list          location for the list pointer
 *
 * @return error code
 */
static int
arp_list(unsigned int gid, const char *oid, char **list)
{
    UNUSED(gid);
    UNUSED(oid);
    
}

/**
 * Get route value (gateway IP address).
 *
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
    UNUSED(gid);
    UNUSED(oid);
    
}

/**
 * Change already existing route.
 *
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
    
}

/**
 * Add a new route.
 *
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
    UNUSED(gid);
    UNUSED(oid);
    
}


/**
 * Delete a route.
 *
 * @param oid           full object instence identifier (unused)
 * @param route         route instance name:
 *                      <IPv4 address in dotted notation>'|'<prefix length>
 *
 * @return error code
 */
static int
route_del(unsigned int gid, const char *oid, const char *route)
{
    UNUSED(gid);
    UNUSED(oid);
    
}

/**
 * Get instance list for object "agent/route".
 *
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
    UNUSED(gid);
    UNUSED(oid);
    
}
