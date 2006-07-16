/** @file
 * @brief Test API to access configuration model
 *
 * Definition of API.
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

#ifndef __TE_TAPI_CFG_H_
#define __TE_TAPI_CFG_H_

#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get son of configuration node with MAC address value.
 *
 * @param father    father's OID
 * @param subid     son subidentifier
 * @param name      name of the son or "" (empty string)
 * @param p_mac     location for MAC address (at least ETHER_ADDR_LEN)
 *
 * @return Status code.
 */
extern int tapi_cfg_get_son_mac(const char *father, const char *subid,
                                const char *name, uint8_t *p_mac);


/**
 * Add VLAN on switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 *
 * @return Status code.
 */
extern int tapi_cfg_switch_add_vlan(const char *ta_name, uint16_t vid);

/**
 * Delete VLAN from switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 *
 * @return Status code.
 */
extern int tapi_cfg_switch_del_vlan(const char *ta_name, uint16_t vid);

/**
 * Add port to VLAN on switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 * @param port      - port number
 *
 * @return Status code.
 */
extern int tapi_cfg_switch_vlan_add_port(const char *ta_name,
                                         uint16_t vid, unsigned int port);
                                         
/**
 * Delete port from VLAN on switch.
 *
 * @param ta_name   - name of the test agent
 * @param vid       - VLAN identifier
 * @param port      - port number
 *
 * @return Status code.
 */
extern int tapi_cfg_switch_vlan_del_port(const char *ta_name,
                                         uint16_t vid, unsigned int port);

/** Routing entry data structure */
typedef struct tapi_rt_entry {
    struct sockaddr_storage dst; /**< Route for this destination address */
    unsigned int            prefix; /**< Destination address prefix */
    struct sockaddr_storage gw; /**< Gateway address
                                     (in case RTF_GATEWAY flag is set) */

    uint16_t flags; /**< Route flags */
    uint32_t metric; /**< Route metric */
    char     dev[IF_NAMESIZE]; /**< Output interface name */
    uint32_t mtu; /**< Route MTU value (for TCP) */
    uint32_t win; /**< Route Window value (for TCP) */
    uint32_t irtt; /**< Route IRTT value (for TCP) */

    cfg_handle hndl; /**< Handle of the entry in configurator */
} tapi_rt_entry_t;

/** @name TAPI Route flags */
/** Route is indirect and has gateway address */
#define TAPI_RT_GW      0x0001
/** Route is direct, so interface name is specified */
#define TAPI_RT_IF      0x0002
/** Metric is specified for the route */
#define TAPI_RT_METRIC  0x0004
/** Type of service is specified for the route */
#define TAPI_RT_TOS     0x0008
/*@}*/

/**
 * Gets routing table on the specified Test Agent
 *
 * @param ta           Test Agent name
 * @param addr_family  Address family of the routes (AF_INET)
 * @param rt_tbl       Pointer to the routing table (OUT)
 * @param n            The number of entries in the table (OUT)
 *
 * @return 0 on success, and TE errno in case of failure.
 *
 * @note Function allocates memory with malloc(), which should be freed
 * with free() by the caller.
 */
extern int tapi_cfg_get_route_table(const char *ta, int addr_family,
                                    tapi_rt_entry_t **rt_tbl,
                                    unsigned int *n);

/**
 * Add a new route to some destination address with a lot of additional 
 * route attributes
 *
 * @param ta           Test agent name
 * @param addr_family  Address family of destination and gateway addresses
 * @param dst_addr     Destination address
 * @param prefix       Prefix for destination address
 * @param gw_addr      Gateway address
 * @param dev          Interface name (for direct route)
 * @param flags        Flags to be added for the route
 *                     (see route flags in net/route.h system header)
 * @param metric       Route metric
 * @param tos          Type of service
 * @param mtu          TCP Maximum Segment Size (MSS) on the route
 * @param win          TCP window size for connections over this route
 * @param irtt         initial round trip time (irtt) for TCP connections
 *                     over this route (in milliseconds)
 * @param rt_hndl      Route handle (OUT)
 *
 * @note For more information about the meaning of parameters see
 *       "man route".
 *
 * @return Status code
 *
 * @retval 0  - on success
 */
extern int tapi_cfg_add_route(const char *ta, int addr_family,
                              const void *dst_addr, int prefix,
                              const void *gw_addr, const char *dev,
                              const void *src_addr, uint32_t flags,
                              int metric, int tos, int mtu, int win,
                              int irtt,
                              cfg_handle *rt_hndl);

/**
 * Same as tapi_cfg_add_route() but allows to specify a route type
 * NOTE: currently only "blackhole" and "unicast" types are supported.
 */
extern int tapi_cfg_add_typed_route(const char *ta, int addr_family,
                                    const void *dst_addr, int prefix,
                                    const void *gw_addr, const char *dev, 
                                    const void *src_addr, const char *type,
                                    uint32_t flags, int metric, 
                                    int tos, int mtu,
                                    int win, int irtt, 
                                    cfg_handle *cfg_hndl);


/**
 * Changes attribute of the existing route. 
 *
 * @param ta           Test agent name
 * @param addr_family  Address family of destination and gateway addresses
 * @param dst_addr     Destination address
 * @param prefix       Prefix for destination address
 * @param gw_addr      Gateway address
 * @param dev          Interface name (for direct route)
 * @param flags        Flags to be added for the route
 *                     (see route flags in net/route.h system header)
 * @param metric       Route metric
 * @param tos          Route type of service
 * @param mtu          TCP Maximum Segment Size (MSS) on the route
 * @param win          TCP window size for connections over this route
 * @param irtt         initial round trip time (irtt) for TCP connections
 *                     over this route (in milliseconds)
 * @param rt_hndl      Route handle (OUT)
 *
 * @note For more information about the meaning of parameters see
 *       "man route".
 *
 * @return Status code
 *
 * @retval 0  - on success
 */
extern int tapi_cfg_modify_route(const char *ta, int addr_family,
                                 const void *dst_addr, int prefix,
                                 const void *gw_addr, const char *dev,
                                 const void *src_addr, uint32_t flags,
                                 int metric, int tos, int mtu, int win,
                                 int irtt,
                                 cfg_handle *rt_hndl);

/**
 * Same as tapi_cfg_modify_route() but allows to specify a route type.
 * NOTE: currently "unicast" type is supported, so the function behaves
 * exactly like tapi_cfg_modify_route().
 */
extern int tapi_cfg_modify_typed_route(const char *ta, int addr_family,
                                       const void *dst_addr, int prefix,
                                       const void *gw_addr, const char *dev,
                                       const void *src_addr,
                                       const char *type,
                                       uint32_t flags, int metric, 
                                       int tos, int mtu,
                                       int win, int irtt, 
                                       cfg_handle *cfg_hndl);


/**
 * Delete specified route
 *
 * @param ta           Test agent name
 * @param addr_family  Address family of destination and gateway addresses
 * @param dst_addr     Destination address
 * @param prefix       Prefix for destination address
 * @param gw_addr      Gateway address
 * @param dev          Interface name (for direct route)
 * @param flags        Flags to be added for the route
 *                     (see route flags in net/route.h system header)
 * @param metric       Route metric
 * @param tos          Route type of service
 * @param mtu          TCP Maximum Segment Size (MSS) on the route
 * @param win          TCP window size for connections over this route
 * @param irtt         initial round trip time (irtt) for TCP connections
 *                     over this route (in milliseconds)
 *
 * @note For more information about the meaning of parameters see
 *       "man route".
 *
 * @return Status code
 *
 * @retval 0  - on success
 */
extern int tapi_cfg_del_route_tmp(const char *ta, int addr_family,
                                  const void *dst_addr, int prefix,
                                  const void *gw_addr, const char *dev,
                                  const void *src_addr, uint32_t flags,
                                  int metric, int tos, int mtu,
                                  int win, int irtt);

/**
 * Delete route by handle got with tapi_cfg_add_route() function
 *
 * @param rt_hndl   Route handle
 *
 * @return 0 on success, and TE errno on failure
 */
extern int tapi_cfg_del_route(cfg_handle *rt_hndl);

/**
 * Add a new indirect route (via a gateway) on specified Test agent
 *
 * @param ta           Test agent
 * @param addr_family  Address family of destination and gateway addresses
 * @param dst_addr     Destination address
 * @param prefix       Prefix for destination address
 * @param gw_addr      Gateway address
 *
 * @return 0 - on success, or TE error code
 */
static inline int
tapi_cfg_add_route_via_gw(const char *ta, int addr_family,
                          const void *dst_addr, int prefix,
                          const void *gw_addr)
{
    cfg_handle cfg_hndl;

    return tapi_cfg_add_route(ta, addr_family, dst_addr, prefix,
                              gw_addr, NULL, NULL, 0, 0, 0, 0, 0, 0,
                              &cfg_hndl);
}

/**
 * Deletes route added with 'tapi_cfg_add_route_via_gw' function
 *
 * @param ta           Test agent name
 * @param addr_family  Address family of destination and gateway addresses
 * @param dst_addr     Destination address
 * @param prefix       Prefix for destination address
 * @param gw_addr      Gateway address
 *
 * @return 0 - on success, or TE error code
 */
static inline int
tapi_cfg_del_route_via_gw(const char *ta, int addr_family,
                          const void *dst_addr, int prefix,
                          const void *gw_addr)
{
    return tapi_cfg_del_route_tmp(ta, addr_family, dst_addr, prefix,
                                  gw_addr, NULL, NULL, 0, 0, 0, 0, 0, 0);
}

/**
 * Get neighbour entry.
 *
 * @param ta            Test Agent name
 * @param ifname        Interface name
 * @param net_addr      IP address
 * @param ret_addr      Returned IEEE 802.3 Link layer address
 * @param is_static     NULL or location for a flag: is found entry static?
 * @param state         NULL or location for state of dynamic entry
 *
 * @return Status code
 *
 * @retval 0    on success
 *
 * @note Currently the function supports only (IP -> IEEE 802.3 ethernet) 
 * entries. In the future it might be extended with an additional parameter
 * hw_type to support different classes of link layer addresses.
 */
extern te_errno tapi_cfg_get_neigh_entry(const char *ta,
                                         const char *ifname, 
                                         const struct sockaddr *net_addr,
                                         void *ret_addr,
                                         te_bool *is_static, 
                                         cs_neigh_entry_state *state);

/**
 * Add a new neighbour entry.
 *
 * @param ta            Test Agent name
 * @param ifname        Interface name
 * @param net_addr      IP address
 * @param link_addr     IEEE 802.3 Link layer address
 * @param is_static     Is static (or dynamic) entry should be added
 *
 * @return Status code
 *
 * @retval 0  - on success
 *
 * @note Currently the function supports only (IP -> IEEE 802.3 ethernet) 
 * entries. In the future it might be extended with an additional parameter
 * hw_type to support different classes of link layer addresses.
 */
extern te_errno tapi_cfg_add_neigh_entry(const char *ta,
                                         const char *ifname, 
                                         const struct sockaddr *net_addr,
                                         const void *link_addr,
                                         te_bool is_static);

/**
 * Delete a neighbour entry.
 *
 * @param ta           Test Agent name
 * @param ifname       Interface name
 * @param net_addr     IP address
 *
 * @return Status code
 *
 * @retval 0  - on success
 *
 * @note Currently the function supports only (IP -> IEEE 802.3 ethernet) 
 * entries. In the future it might be extended with an additional parameter
 * address_family to support different classes of link layer addresses.
 */
extern te_errno tapi_cfg_del_neigh_entry(const char *ta,
                                         const char *ifname, 
                                         const struct sockaddr *net_addr);

/**
 * Clenup dynamic neighbour entries.
 *
 * @param ta           Test agent name
 * @param ifname       Interface name or NULL if all interfaces should
 *                     be processed
 *
 * @return Status code
 *
 * @retval 0  - on success
 */
extern te_errno tapi_cfg_del_neigh_dynamic(const char *ta, 
                                           const char *ifname);

/**
 * Returns hardware address of specified interface on a particular 
 * test agent.
 * 
 * @param ta          Test agent name
 * @param ifname      Interface name whose hardware address is obtained
 * @param hwaddr      Hardware address - link-layer address (OUT)
 * @param hwaddr_len  Length of 'hwaddr' buffer (IN/OUT)
 *
 * @return Status of the oprtation
 * @retval 0            on success
 * @retval TE_EMSGSIZE  Buffer is too short to fit the hardware address
 */
extern int tapi_cfg_get_hwaddr(const char *ta,
                               const char *ifname,
                               void *hwaddr, size_t *hwaddr_len);


/**
 * Sets broadcast hardware address of specified interface on
 * a particular test agent.
 *
 * @param ta          Test agent name
 * @param ifname      Interface name whose hardware address should
 *                    be set
 * @param hwaddr      Hardware address - link-layer address
 * @param hwaddr_len  Length of 'hwaddr'
 *
 * @return Status of the oprtation
 * @retval 0            on success
 * @retval TE_EMSGSIZE  Buffer is too short to fit the hardware address
 */
extern te_errno tapi_cfg_set_bcast_hwaddr(const char *ta,
                                          const char *ifname,
                                          const void *hwaddr,
                                          unsigned int hwaddr_len);

/**
 * Returns broadcast hardware address of specified interface on
 * a particular test agent.
 * 
 * @param ta          Test agent name
 * @param ifname      Interface name whose hardware address is obtained
 * @param hwaddr      Hardware address - link-layer address (OUT)
 * @param hwaddr_len  Length of 'hwaddr' buffer (IN/OUT)
 *
 * @return Status of the oprtation
 * @retval 0            on success
 * @retval TE_EMSGSIZE  Buffer is too short to fit the hardware address
 */
extern int tapi_cfg_get_bcast_hwaddr(const char *ta,
                                     const char *ifname,
                                     void *hwaddr, size_t *hwaddr_len);

/**
 * Sets hardware address of specified interface on a particular
 * test agent.
 *
 * @param ta          Test agent name
 * @param ifname      Interface name whose hardware address should be set
 * @param hwaddr      Hardware address - link-layer address
 * @param hwaddr_len  Length of 'hwaddr'
 *
 * @return Status of the oprtation
 * @retval 0            on success
 * @retval TE_EMSGSIZE  Buffer is too short to fit the hardware address
 */
extern te_errno tapi_cfg_set_hwaddr(const char *ta, const char *ifname,
                                    const void *hwaddr,
                                    unsigned int hwaddr_len);

/**
 * Get Configurator handle of free child. The function assumes that
 * all children of the parent has integer value. Zero value is considered
 * as the child is free. The child is marked as non-free by set of 1
 * as the child value.
 * 
 * @param parent_oid    Parent instance OID
 * @param entry         Location for entry handle
 * 
 * @return Status code.
 *
 * @sa tapi_cfg_free_entry
 */
extern int tapi_cfg_alloc_entry(const char *parent_oid, cfg_handle *entry);

/**
 * Free earlier allocated child.
 * Entry handle is set to CFG_HANDLE_INVALID on success.
 *
 * @param entry         Location of entry handle
 *
 * @return Status code.
 *
 * @sa tapi_cfg_alloc_entry
 */
extern int tapi_cfg_free_entry(cfg_handle *entry);

/**
 * Allocate entry in IPv4 subnets pool.
 *
 * @param entry         Location for Cfgr handle
 *
 * @return Status code.
 *
 * @note Use #tapi_cfg_free_entry function to free allocated entry.
 *
 * @sa tapi_cfg_free_entry, tapi_cfg_alloc_net_addr
 */
static inline int
tapi_cfg_alloc_ip4_net(cfg_handle *entry)
{
    return tapi_cfg_alloc_entry("/net_pool:ip4", entry);
}

/**
 * Allocate entry in IPv6 subnets pool.
 *
 * @param entry         Location for Cfgr handle
 *
 * @return Status code.
 *
 * @note Use #tapi_cfg_free_entry function to free allocated entry.
 *
 * @sa tapi_cfg_free_entry, tapi_cfg_alloc_net_addr
 */
static inline int
tapi_cfg_alloc_ip6_net(cfg_handle *entry)
{
    return tapi_cfg_alloc_entry("/net_pool:ip6", entry);
}


/**
 * Add entry to IPv4 or IPv6 subnets pool.
 *
 * @param net_pool      Name of the pool
 * @param net_addr      Network address (or any network node address)
 * @param prefix        Network address prefix
 * @param state         Pool entry initial state (0 or 1)
 * @param entry         Location for Cfgr handle of new entry
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_add_net(const char *net_pool,
                                 const struct sockaddr *net_addr,
                                 unsigned int prefix, int state,
                                 cfg_handle *entry);

/**
 * Add entry to IPv4 subnets pool.
 *
 * @param ip4_net_addr  Network address (or any network node address)
 * @param prefix        Network address prefix
 * @param state         Pool entry initial state (0 or 1)
 * @param entry         Location for Cfgr handle of new entry
 *
 * @return Status code.
 */
static inline te_errno
tapi_cfg_add_ip4_net(const struct sockaddr_in *ip4_net_addr,
                     unsigned int prefix, int state, cfg_handle *entry)
{
    return tapi_cfg_add_net("/net_pool:ip4", CONST_SA(ip4_net_addr),
                            prefix, state, entry);
}

/**
 * Add entry to IPv6 subnets pool.
 *
 * @param ip6_net_addr  Network address (or any network node address)
 * @param prefix        Network address prefix
 * @param state         Pool entry initial state (0 or 1)
 * @param entry         Location for Cfgr handle of new entry
 *
 * @return Status code.
 */
static inline te_errno
tapi_cfg_add_ip6_net(const struct sockaddr_in6 *ip6_net_addr,
                     unsigned int prefix, int state, cfg_handle *entry)
{
    return tapi_cfg_add_net("/net_pool:ip6", CONST_SA(ip6_net_addr),
                            prefix, state, entry);
}


/**
 * Allocate IPv4 address from IPv4 subnet got from IPv4 subnets pool.
 *
 * @param net_pool_entry    Subnet handle
 * @param p_entry           Location for Cfgr handle of new entry
 * @param addr              Location for allocated address
 *
 * @return Status code.
 *
 * @note Use #tapi_cfg_free_entry function to free allocated entry.
 * 
 * @sa tapi_cfg_free_entry, tapi_cfg_alloc_ip4_net
 */
extern int tapi_cfg_alloc_net_addr(cfg_handle        net_pool_entry,
                                   cfg_handle       *p_entry,
                                   struct sockaddr **addr);

/**
 * Add IPv4 address to IPv4 subnet from IPv4 subnets pool.
 *
 * @param net_pool_entry    Subnet handle
 * @param add_addr          Address to add in pool
 * @param p_entry           Location for Cfgr handle of new entry
 *
 * @return Status code.
 */
extern int tapi_cfg_add_net_addr(cfg_handle       net_pool_entry,
                                 struct sockaddr *add_addr,
                                 cfg_handle      *p_entry);

/**
 * Set Environment variables specified in /local/env on corresponding
 * Test Agents. If such variable already exists, set new value. If
 * it does not exist, add it with specified value.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_env_local_to_agent(void);

/**
 * Add RPC server from /local:<ta-name> tree to /agent:<ta-name> tree.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rpcs_local_to_agent(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_H_ */
