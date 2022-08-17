/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to access configuration model
 *
 * Definition of API.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup tapi_conf_iface
 * @{
 */

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
/**@} <!-- END tapi_conf_iface --> */

/**
 * @defgroup tapi_conf_switch Network Switch configuration
 * @ingroup tapi_conf
 * @{
 */

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
/**@} <!-- END tapi_conf_switch --> */

/**
 * @defgroup tapi_conf_route Routing Table configuration
 * @ingroup tapi_conf
 * @{
 */

/** Maximum length of the route type name */
#define TYPE_NAMESIZE 32

/** Routing entry data structure */
typedef struct tapi_rt_entry {
    struct sockaddr_storage dst; /**< Route for this destination address */
    unsigned int            prefix; /**< Destination address prefix */
    struct sockaddr_storage gw; /**< Gateway address
                                     (in case RTF_GATEWAY flag is set) */
    struct sockaddr_storage src; /**< Default source address */
    uint16_t flags; /**< Route flags */
    uint32_t metric; /**< Route metric */
    char     dev[IF_NAMESIZE]; /**< Output interface name */
    uint32_t mtu; /**< Route MTU value (for TCP) */
    uint32_t win; /**< Route Window value (for TCP) */
    uint32_t irtt; /**< Route IRTT value (for TCP) */
    uint32_t hoplimit; /**< Route Hop Limit value (influences IPv6
                            Hop Limit and IPv4 Time To Live) */
    char     type[TYPE_NAMESIZE]; /**< Route Type value (for TCP) */

    uint32_t table; /**< Route Table ID value (for TCP) */

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
/** Default source address is specified for the route */
#define TAPI_RT_SRC     0x0010
/** Table ID for the route */
#define TAPI_RT_TABLE   0x0020
/*@}*/

/** Default table for normal rules */
#define TAPI_RT_TABLE_MAIN 254

/** Local table maintained by kernel */
#define TAPI_RT_TABLE_LOCAL 255

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

/** Nexthop of a multipath route. */
typedef struct tapi_cfg_rt_nexthop {
    struct sockaddr_storage gw;                   /**< Gateway address
                                                       (considered empty if
                                                        family is
                                                        @c AF_UNSPEC) */
    char                    ifname[IF_NAMESIZE];  /**< Interface name */
    unsigned int            weight;               /**< Weight */
} tapi_cfg_rt_nexthop;

/** Structure storing parameters of tapi_cfg_add_route2(). */
typedef struct tapi_cfg_rt_params {
    const struct sockaddr     *dst_addr;    /**< Destination address. */
    int                        prefix;      /**< Route prefix length. */
    const struct sockaddr     *gw_addr;     /**< Gateway address. */
    const struct sockaddr     *src_addr;    /**< Default source address. */
    const char                *dev;         /**< Route interface. */
    const char                *type;        /**< Route type. */
    uint32_t                   flags;       /**< Route flags. */
    int                        metric;      /**< Route metric. */
    int                        tos;         /**< Type of service. */
    int                        mtu;         /**< TCP maximum segment size
                                                 (MSS) on the route. */
    int                        win;         /**< TCP window size. */
    int                        irtt;        /**< Initial round trip time
                                                 for TCP connections
                                                 (in milliseconds). */
    int                        hoplimit;    /**< Hop limit. */
    int                        table;       /**< Route table. */

    tapi_cfg_rt_nexthop       *hops;        /**< Nexthops of a multipath
                                                 route. */
    unsigned int               hops_num;    /**< Number of nexthops. */

    /*
     * These are internal fields, do not fill them for
     * tapi_cfg_add_route2().
     */
    int                        addr_family; /**< Address family. */
    const void                *dst;         /**< Pointer to network address
                                                 (struct in_addr,
                                                 struct in6_addr, etc). */
    const void                *gw;          /**< Pointer to gateway
                                                 address. */
    const void                *src;         /**< Pointer to default source
                                                 address. */
} tapi_cfg_rt_params;

/**
 * Initialize tapi_cfg_rt_params fields.
 *
 * @param params      Pointer to tapi_cfg_rt_params structure.
 */
extern void tapi_cfg_rt_params_init(tapi_cfg_rt_params *params);

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
 * @param src_addr     Default source address
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
 * Same as tapi_cfg_add_route() but allows to specify a route type and
 * route table id
 * NOTE: currently only "blackhole" and "unicast" types are supported.
 */
extern int tapi_cfg_add_full_route(const char *ta, int addr_family,
                                   const void *dst_addr, int prefix,
                                   const void *gw_addr, const char *dev,
                                   const void *src_addr, const char *type,
                                   uint32_t flags, int metric,
                                   int tos, int mtu,
                                   int win, int irtt, int table,
                                   cfg_handle *cfg_hndl);

/**
 * Same as tapi_cfg_add_route() but allows to specify a route type
 * NOTE: currently only "blackhole" and "unicast" types are supported.
 */
static inline int
tapi_cfg_add_typed_route(const char *ta, int addr_family,
                         const void *dst_addr, int prefix,
                         const void *gw_addr, const char *dev,
                         const void *src_addr, const char *type,
                         uint32_t flags, int metric, int tos, int mtu,
                         int win, int irtt, cfg_handle *cfg_hndl)
{
    return tapi_cfg_add_full_route(ta, addr_family, dst_addr, prefix,
                                   gw_addr, dev, src_addr, type, flags,
                                   metric, tos, mtu, win, irtt,
                                   TAPI_RT_TABLE_MAIN, cfg_hndl);
}


/**
 * Changes attribute of the existing route.
 *
 * @param ta           Test agent name
 * @param addr_family  Address family of destination and gateway addresses
 * @param dst_addr     Destination address
 * @param prefix       Prefix for destination address
 * @param gw_addr      Gateway address
 * @param dev          Interface name (for direct route)
 * @param src_addr     Default source address
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
 * Modify existing route.
 *
 * @param ta        Test Agent name.
 * @param params    Route parameters.
 * @param rt_hndl   Where to save configurator handle of a route
 *                  (may be @c NULL).
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_modify_route2(const char *ta,
                                       tapi_cfg_rt_params *params,
                                       cfg_handle *rt_hndl);

/**
 * Same as tapi_cfg_modify_route() but allows to specify a route type and
 * route table id
 * NOTE: currently only "blackhole" and "unicast" types are supported.
 */
extern int tapi_cfg_modify_full_route(const char *ta, int addr_family,
                                      const void *dst_addr, int prefix,
                                      const void *gw_addr, const char *dev,
                                      const void *src_addr,
                                      const char *type,
                                      uint32_t flags, int metric,
                                      int tos, int mtu,
                                      int win, int irtt, int table,
                                      cfg_handle *cfg_hndl);

/**
 * Same as tapi_cfg_modify_route() but allows to specify a route type.
 * NOTE: currently "unicast" type is supported, so the function behaves
 * exactly like tapi_cfg_modify_route().
 */
static inline int
tapi_cfg_modify_typed_route(const char *ta, int addr_family,
                            const void *dst_addr, int prefix,
                            const void *gw_addr, const char *dev,
                            const void *src_addr, const char *type,
                            uint32_t flags, int metric, int tos, int mtu,
                            int win, int irtt, cfg_handle *cfg_hndl)
{
    return tapi_cfg_modify_full_route(ta, addr_family, dst_addr, prefix,
                                      gw_addr, dev, src_addr, type, flags,
                                      metric, tos, mtu, win, irtt,
                                      TAPI_RT_TABLE_MAIN, cfg_hndl);
}


/**
 * Delete specified route
 *
 * @param ta           Test agent name
 * @param addr_family  Address family of destination and gateway addresses
 * @param dst_addr     Destination address
 * @param prefix       Prefix for destination address
 * @param gw_addr      Gateway address
 * @param dev          Interface name (for direct route)
 * @param src_addr     Default source address
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
 * Add a new simple route to destination network or host address
 *
 * @note Added route can be removed with tapi_cfg_del_route_simple()
 *
 * @param ta           Test agent name
 * @param target       Destination network or host address
 * @param prefixlen    Prefix length of destination address
 * @param gw           Gateway address to route packets via a gateway,
 *                     may be @c NULL for direct route
 * @param dev          Interface name, force the route to be associated with
 *                     the specified device. May be @c NULL if @p gw is defined
 *
 * @return Status code
 *
 * @sa tapi_cfg_add_route_via_gw, tapi_cfg_add_route
 */
extern te_errno tapi_cfg_add_route_simple(const char *ta,
                                          const struct sockaddr *target,
                                          int prefixlen,
                                          const struct sockaddr *gw,
                                          const char *dev);

/**
 * Delete a route added with tapi_cfg_add_route_simple(), use the same argument
 * values
 */
extern te_errno tapi_cfg_del_route_simple(const char *ta,
                                          const struct sockaddr *target,
                                          int prefixlen,
                                          const struct sockaddr *gw,
                                          const char *dev);

/**
 * Add a new route.
 *
 * @param ta        Test Agent name.
 * @param params    Route parameters.
 * @param rt_hndl   Where to save configurator handle of a new route
 *                  (may be @c NULL).
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_add_route2(const char *ta,
                                    tapi_cfg_rt_params *params,
                                    cfg_handle *rt_hndl);

/**@} <!-- END tapi_conf_route --> */

/**
 * @defgroup tapi_conf_neigh Neighbour table configuration
 * @ingroup tapi_conf
 * @{
 */

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
 * Set a new value for neighbour entry.
 *
 * @param ta            Test Agent name
 * @param ifname        Interface name
 * @param net_addr      IP address
 * @param link_addr     IEEE 802.3 Link layer address
 * @param is_static     Whether static or dynamic entry should be set
 *
 * @return Status code
 *
 * @retval 0  - on success
 *
 * @note Currently the function supports only (IP -> IEEE 802.3 ethernet)
 * entries. In the future it might be extended with an additional parameter
 * hw_type to support different classes of link layer addresses.
 */
extern te_errno tapi_cfg_set_neigh_entry(const char *ta,
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
 * Add neighbor proxy entry for a given interface.
 *
 * @param ta        Test Agent name.
 * @param ifname    Interface name.
 * @param net_addr  Network address to proxy.
 * @param p_handle  Where to save pointer to configuration handle
 *                  (may be @c NULL).
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_add_neigh_proxy(const char *ta, const char *ifname,
                                         const struct sockaddr *net_addr,
                                         cfg_handle *p_handle);

/**@} <!-- END tapi_conf_neigh --> */

/**
 * @defgroup tapi_conf_iface Network Interface configuration
 * @ingroup tapi_conf
 * @{
 */

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
 * Network interface kinds.
 * This list is mostly Linux-specific, but other OSes may add other kinds or
 * re-use existing Linux kinds.
 *
 * Note: this list enumerates not all possible types of interfaces, but
 * all possible values of interface:/kind: in configuration tree, which
 * gets its value from @c IFLA_INFO_KIND netlink attribute. It seems netlink
 * reports only types of interfaces here which it can manipulate
 * (create, destroy), so for physical interfaces and loopback empty string
 * (@c TE_INTERFACE_KIND_NONE) is returned.
 */
typedef enum {
    TE_INTERFACE_KIND_NONE,     /**< Kind is not specified - a usual network
                                     interface. */
    TE_INTERFACE_KIND_VLAN,     /**< VLAN */
    TE_INTERFACE_KIND_MACVLAN,  /**< MAC VLAN */
    TE_INTERFACE_KIND_IPVLAN,   /**< IP VLAN */
    TE_INTERFACE_KIND_VETH,     /**< VETH */
    TE_INTERFACE_KIND_BOND,     /**< Bonding interface */
    TE_INTERFACE_KIND_TEAM,     /**< BOND and TEAM are different linux
                                     implementation of the same Link
                                     aggregation concept. */
    TE_INTERFACE_KIND_BRIDGE,   /**< Bridge */
    TE_INTERFACE_KIND_TUN,      /**< TAP/TUN: in both cases ethtool says
                                     it is tun */
    TE_INTERFACE_KIND_END       /**< Not a real interface kind, but ending
                                     enum element. */
} te_interface_kind;

/** Mapping list for TEST_GET_ENUM_PARAM */
#define TE_INTERFACE_KIND_MAPPING_LIST \
    { "none",       TE_INTERFACE_KIND_NONE },       \
    { "vlan",       TE_INTERFACE_KIND_VLAN },       \
    { "macvlan",    TE_INTERFACE_KIND_MACVLAN },    \
    { "ipvlan",     TE_INTERFACE_KIND_IPVLAN },     \
    { "veth",       TE_INTERFACE_KIND_VETH },       \
    { "bond",       TE_INTERFACE_KIND_BOND },       \
    { "team",       TE_INTERFACE_KIND_TEAM },       \
    { "bridge",     TE_INTERFACE_KIND_BRIDGE },     \
    { "tun",        TE_INTERFACE_KIND_TUN }

/**
 * Get value of a test parameter of type @ref te_interface_kind.
 * Should be used like
 *    te_interface_kind var;
 *    var = TEST_TE_INTERFACE_KIND_PARAM(if_kind)
 *
 * @param _name     Parameter name
 */
#define TEST_TE_INTERFACE_KIND_PARAM(_name) \
    TEST_ENUM_PARAM(_name, TE_INTERFACE_KIND_MAPPING_LIST)

/**
 * Get value of a test parameter of type @ref te_interface_kind.
 * Should be used like
 *    te_interface_kind if_kind;
 *    TEST_GET_TE_INTERFACE_KIND_PARAM(if_kind);
 *
 * @param _var_name     Parameter name (should be the same as
 *                      name of the variable in which obtained
 *                      value is stored).
 */
#define TEST_GET_TE_INTERFACE_KIND_PARAM(_var_name) \
    TEST_GET_ENUM_PARAM(_var_name, TE_INTERFACE_KIND_MAPPING_LIST)

/**
 * Get kind of an interface.
 *
 * @param ta        Test agent name.
 * @param ifname    Interface name.
 * @param kind      Where to save the interface type.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_get_if_kind(const char *ta,
                                     const char *ifname,
                                     te_interface_kind *kind);

/**
 * Get name of an interface on which the given interface is based.
 *
 * @param ta              Test agent name.
 * @param ifname          Interface name.
 * @param parent_ifname   Where to save the name of the "parent"
 *                        interface.
 * @param len             Available space in @p parent_ifname.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_get_if_parent(const char *ta,
                                       const char *ifname,
                                       char *parent_ifname,
                                       size_t len);

/**
 * Get name of the last known ancestor of a given interface
 * (for example, if interface is eth3.macvlan.100, this function
 * should return "eth3").
 *
 * @note If the interface has no ancestors, this function returns
 *       its name.
 *
 * @param ta                Test agent name.
 * @param ifname            Interface name.
 * @param ancestor_ifname   Where to save the name of the "ancestor"
 *                          interface.
 * @param len               Available space in @p ancestor_ifname.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_get_if_last_ancestor(const char *ta,
                                              const char *ifname,
                                              char *ancestor_ifname,
                                              size_t len);

/**@} <!-- END tapi_conf_iface --> */

/**
 * @defgroup tapi_conf_net_pool Manipulation of network address pools
 * @ingroup tapi_conf
 * @{
 */

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
 * Allocate entry in IPv4 or IPv6 subnets pool.
 *
 * @param af            @c AF_INET or @c AF_INET6
 * @param entry         Location for Configurator handle
 *
 * @return Status code.
 *
 * @note Use tapi_cfg_free_entry() function to free allocated entry.
 */
extern te_errno tapi_cfg_alloc_net(int af, cfg_handle *entry);

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
/** @} <!-- END tapi_conf_net_pool --> */

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

extern char *te_sprintf(const char *fmt, ...);

extern int te_asprintf(char **strp, const char *fmt, ...);

extern int te_vasprintf(char **strp, const char *fmt, va_list ap);

#define TAPI_CFG_LINK_PREFIX "cfg:"

/**
 * Check if string is configurator link.
 *
 * @param s        String to check, whether it has configurator link prefix
 *
 * @return True if string has configurator link prefix, False otherwise.
 */
static inline te_bool
tapi_is_cfg_link(char *s)
{
    return (strncmp(s, TAPI_CFG_LINK_PREFIX,
            strlen(TAPI_CFG_LINK_PREFIX)) == 0);
}

/**
 * Set the console log level
 *
 * @param agent    Test agent name
 * @param level    Console log level (See printk(9))
 *
 * @return Status code
 * @retval 0            Success
 */
extern te_errno tapi_cfg_set_loglevel(const char *agent, int level);

/**
 * Resolve configurator link into its value.
 *
 * @param link     Link to string to dereference
 *
 * @return Dereferenced link value from the configuration tree.
 */
static inline char * tapi_get_cfg_link(char *link)
{
    char     *cur = link + strlen(TAPI_CFG_LINK_PREFIX);
    char     *c;
    int       rc = 0;

    c = strchr(cur + 1, '/');

    do {
        char          *s = NULL;
        cfg_handle     handle;
        cfg_obj_descr  descr;

        *c = '\0';

        if ((rc = cfg_find_fmt(&handle, "%s", cur)) != 0)
        {
            ERROR("Failed to get configurator object '%s': rc=%r",
                  cur, rc);
            return NULL;
        }

        if ((rc = cfg_get_object_descr(handle, &descr)) != 0)
        {
            ERROR("Failed to get configurator object description: rc=%r",
                  rc);
            return NULL;
        }

        if (descr.type != CVT_NONE)
        {
            if ((rc = cfg_get_instance_str(NULL, &s, cur)) != 0)
            {
                ERROR("Failed to get configurator object '%s': rc=%r",
                      cur, rc);
                return NULL;
            }
        }
        *c = '/';

        if (s != NULL && tapi_is_cfg_link(s))
        {
            te_asprintf(&cur, "%s/%s", s + strlen(TAPI_CFG_LINK_PREFIX),
                    c + 1);
            RING("%s: %s", __FUNCTION__, cur);
            c = cur;
        }

        c = strchr(c + 1, '/');
        if (c == NULL)
        {
            if ((rc = cfg_get_instance_str(NULL, &s, cur)) != 0)
            {
                ERROR("Failed to get configurator object '%s': rc=%r",
                      cur, rc);
                return NULL;
            }
            return s;
        }
    } while (1);
    return NULL;
}

/**
 * Set kernel log level to a new value optionally saving the old one.
 *
 * @param ta        Agent name
 * @param new_val   Value to be set (ignored if less than zero)
 * @param old_val   Where to save previous value (ignored if NULL)
 *
 * @return 0 on success or status code
 */
extern te_errno tapi_cfg_set_loglevel_save(const char *ta, int new_val,
                                           int *old_val);

/**
 * Get integer value from configuration tree.
 *
 * @param value     Where to save value.
 * @param oid       OID string.
 *
 * @return 0 on success or error code
 */
extern te_errno tapi_cfg_get_int_str(int *value, const char *oid);

/**
 * Get integer value from configuration tree.
 *
 * @param value     Where to save value.
 * @param format    Format string for configuration path.
 * @param ...       Configuration path parameters.
 *
 * @return 0 on success or error code
 */
extern te_errno tapi_cfg_get_int_fmt(int *value, const char *format, ...);

/**
 * Set integer value in configuration tree.
 *
 * @param value       Value to set.
 * @param old_value   If not NULL, here will be saved previous value.
 * @param oid         OID string.
 *
 * @return 0 on success or error code
 */
extern te_errno tapi_cfg_set_int_str(int value, int *old_value,
                                     const char *oid);

/**
 * Set integer value in configuration tree.
 *
 * @param value       Value to set.
 * @param old_value   If not NULL, here will be saved previous value.
 * @param format      Format string for configuration path.
 * @param ...         Configuration path parameters.
 *
 * @return 0 on success or error code
 */
extern te_errno tapi_cfg_set_int_fmt(int value, int *old_value,
                                     const char *format, ...);

/**
 * Get uint64_t value from configuration tree.
 *
 * @param value     Where to save value.
 * @param oid       OID string.
 *
 * @return 0 on success or error code
 */
extern te_errno tapi_cfg_get_uint64_str(uint64_t *value, const char *oid);

/**
 * Get uint64_t value from configuration tree.
 *
 * @param value     Where to save value.
 * @param format    Format string for configuration path.
 * @param ...       Configuration path parameters.
 *
 * @return 0 on success or error code
 */
extern te_errno tapi_cfg_get_uint64_fmt(uint64_t *value,
                                        const char *format, ...);

/**
 * Set uint64_t value in configuration tree.
 *
 * @param value       Value to set.
 * @param old_value   If not NULL, here will be saved previous value.
 * @param oid         OID string.
 *
 * @return 0 on success or error code
 */
extern te_errno tapi_cfg_set_uint64_str(uint64_t value, uint64_t *old_value,
                                        const char *oid);

/**
 * Set uint64_t value in configuration tree.
 *
 * @param value       Value to set.
 * @param old_value   If not NULL, here will be saved previous value.
 * @param format      Format string for configuration path.
 * @param ...         Configuration path parameters.
 *
 * @return 0 on success or error code
 */
extern te_errno tapi_cfg_set_uint64_fmt(uint64_t value, uint64_t *old_value,
                                        const char *format, ...);

/**
 * Allocate two IP addresses from a free net_pool.
 *
 * @param addr1     The first IP address pointer location
 * @param addr2     The second IP address pointer location
 * @param prefix    The prefix length
 *
 * @return Status code
 */
extern te_errno tapi_cfg_alloc_net_addr_pair(struct sockaddr **addr1,
                                             struct sockaddr **addr2,
                                             int *prefix);

/**
 * Add a new user on TA.
 *
 * @note User name will be TE_USER_PREFIX + uid, TA requires this format.
 *       User will be created in a new group named after the user and having
 *       gid = uid.
 *
 * @param agent       Agent on which to create a new user.
 * @param uid         User ID.
 */
extern te_errno tapi_cfg_add_new_user(const char *agent, int uid);

/**
 * Remove a user previously added by tapi_cfg_add_new_user().
 *
 * @param agent       Agent on which to remove a user.
 * @param uid         User ID.
 */
extern te_errno tapi_cfg_del_user(const char *agent, int uid);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_H_ */
