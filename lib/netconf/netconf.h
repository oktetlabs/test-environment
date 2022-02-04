/** @file
 * @brief Linux network configuration support library
 *
 * This library provides access to Linux network configuration using
 * netlink protocol. It is similiar to libnetlink from iproute2 package
 * but has higher level interface and appropriate license. Following
 * information is covered: network devices, IP addresses, neighbour
 * tables, routing tables.
 *
 * Copyright (C) 2008-2019 OKTET Labs, St.-Petersburg, Russia
 *
 * @author Maxim Alyutov <Maxim.Alyutov@oktetlabs.ru>
 *
 * $Id: $
 */

#ifndef __NETCONF_H__
#define __NETCONF_H__

#ifndef __linux__
#error This library can be used only on linux
#endif

#include <stdbool.h>
#include <stdint.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>

#include "conf_ip_rule.h"
#include "te_queue.h"

/** Invalid netconf session handle */
#define NETCONF_HANDLE_INVALID NULL

#ifdef __cplusplus
extern "C" {
#endif

/** Types of network devices */
typedef enum netconf_link_type {
    NETCONF_LINK_UNSPEC         = -1,
    NETCONF_LINK_ETHER          = 1,
    NETCONF_LINK_ATM            = 19,
    NETCONF_LINK_IEEE1394       = 24,
    NETCONF_LINK_LOOPBACK       = 772,
    NETCONF_LINK_SIT            = 776
} netconf_link_type;

/** Flags of network address */
typedef enum netconf_net_addr_flag {
    NETCONF_IFA_F_SECONDARY     = 0x01,
    NETCONF_IFA_F_PERMANENT     = 0x80
} netconf_net_addr_flag;

/** Types of route entry */
typedef enum netconf_route_type {
    NETCONF_RTN_UNSPEC,
    NETCONF_RTN_UNICAST,
    NETCONF_RTN_LOCAL,
    NETCONF_RTN_BROADCAST,
    NETCONF_RTN_ANYCAST,
    NETCONF_RTN_MULTICAST,
    NETCONF_RTN_BLACKHOLE,
    NETCONF_RTN_UNREACHABLE,
    NETCONF_RTN_PROHIBIT,
    NETCONF_RTN_THROW,
    NETCONF_RTN_NAT
} netconf_route_type;

/** Protocols of route entry */
typedef enum netconf_route_prot {
    NETCONF_RTPROT_UNSPEC,
    NETCONF_RTPROT_REDIRECT,
    NETCONF_RTPROT_KERNEL,
    NETCONF_RTPROT_BOOT,
    NETCONF_RTPROT_STATIC,
    NETCONF_RTPROT_RA = 9
} netconf_route_prot;

/** Scopes of route entry */
typedef enum netconf_route_scope {
    NETCONF_RT_SCOPE_UNSPEC     = -1,
    NETCONF_RT_SCOPE_UNIVERSE   = 0,
    NETCONF_RT_SCOPE_SITE       = 200,
    NETCONF_RT_SCOPE_LINK       = 253,
    NETCONF_RT_SCOPE_HOST       = 254,
    NETCONF_RT_SCOPE_NOWHERE    = 255
} netconf_route_scope;

/** Routing table ids */
typedef enum netconf_route_table {
    NETCONF_RT_TABLE_UNSPEC     = 0,
    NETCONF_RT_TABLE_DEFAULT    = 253,
    NETCONF_RT_TABLE_MAIN       = 254,
    NETCONF_RT_TABLE_LOCAL      = 255
} netconf_route_table;

/** States of neighbour entry */
typedef enum netconf_neigh_state {
    NETCONF_NUD_UNSPEC          = 0x00,
    NETCONF_NUD_INCOMPLETE      = 0x01,
    NETCONF_NUD_REACHABLE       = 0x02,
    NETCONF_NUD_STALE           = 0x04,
    NETCONF_NUD_DELAY           = 0x08,
    NETCONF_NUD_PROBE           = 0x10,
    NETCONF_NUD_FAILED          = 0x20,
    NETCONF_NUD_NOARP           = 0x40,
    NETCONF_NUD_PERMANENT       = 0x80
} netconf_neigh_state;

/** Neighbor entry flags */
typedef enum netconf_neigh_flags {
    NETCONF_NTF_USE           = 0x01,
    NETCONF_NTF_SELF          = 0x02,
    NETCONF_NTF_MASTER        = 0x04,
    NETCONF_NTF_PROXY         = 0x08,
    NETCONF_NTF_EXT_LEARNED   = 0x10,
    NETCONF_NTF_ROUTER        = 0x20,
} netconf_neigh_flags;

#define NETCONF_RTM_F_CLONED RTM_F_CLONED

/** Network device */
typedef struct netconf_link {
    netconf_link_type   type;           /**< Device type */
    int                 ifindex;        /**< Interface index */
    int                 link;           /**< Value of IFLA_LINK
                                             attribute. */
    unsigned int        flags;          /**< Device flags */
    unsigned int        addrlen;        /**< Length of address field */
    uint8_t            *address;        /**< Interface hardware address */
    uint8_t            *broadcast;      /**< Broadcast address */
    char               *ifname;         /**< Device name as string */
    char               *info_kind;      /**< Value of IFLA_INFO_KIND
                                             attribute */
    uint32_t            mtu;            /**< MTU of the device */
} netconf_link;

/** Network address (IPv4 or IPv6) on a device */
typedef struct netconf_net_addr {
    unsigned char       family;         /**< Address family */
    unsigned char       prefix;         /**< Prefix length of address */
    unsigned char       flags;          /**< Address flags */
    int                 ifindex;        /**< Interface index */
    uint8_t            *address;        /**< Address in network
                                             byte order */
    uint8_t            *broadcast;      /**< Broadcast address or NULL
                                             if not available */
} netconf_net_addr;

/** Nexthop of a multipath route. */
typedef struct netconf_route_nexthop {
    uint32_t    weight;       /**< Weight. */
    int         oifindex;     /**< Interface index. */
    uint8_t    *gateway;      /**< Gateway address */

    LIST_ENTRY(netconf_route_nexthop) links;  /**< List links. */
} netconf_route_nexthop;

/** List head type for list of netconf_route_nexthop. */
typedef LIST_HEAD(netconf_route_nexthops,
                  netconf_route_nexthop) netconf_route_nexthops;

/** Routing table entry */
typedef struct netconf_route {
    unsigned char       family;         /**< Address family of route */
    unsigned char       dstlen;         /**< Prefix length of
                                             destination */
    unsigned char       srclen;         /**< Prefix length of
                                             preffered source */
    unsigned char       tos;            /**< Type Of Service key */
    netconf_route_table table;          /**< Routing table id */
    netconf_route_prot  protocol;       /**< Protocol of this route */
    netconf_route_scope scope;          /**< Route scope */
    netconf_route_type  type;           /**< Type of route entry */
    unsigned int        flags;          /**< Route flags */
    int                 oifindex;       /**< Output device index */
    uint8_t            *dst;            /**< Destination of the route */
    uint8_t            *src;            /**< Preffered source */
    uint8_t            *gateway;        /**< Gateway address */
    int                 metric;         /**< Route priority */
    uint32_t            mtu;            /**< Route MTU */
    uint32_t            win;            /**< Route window size */
    uint32_t            irtt;           /**< Route transfer time */
    uint32_t            hoplimit;       /**< Route hoplimit (influences both
                                             IPv4 TTL and IPv6 Hop Limit
                                             header fields) */
    int32_t             expires;        /**< Route expiration time */

    netconf_route_nexthops   hops;      /**< Nexthops of a multipath
                                             route. */
} netconf_route;

/** Neighbour table entry (ARP or NDISC cache) */
typedef struct netconf_neigh {
    unsigned char        family;        /**< Address family of entry */
    int                  ifindex;       /**< Interface index */
    uint16_t             state;         /**< State of neighbour entry */
    uint8_t              flags;         /**< Neighbor entry flags
                                             (see @ref netconf_neigh_flags)
                                             */
    uint8_t             *dst;           /**< Protocol address of
                                             the neighbour */
    unsigned int         addrlen;       /**< Length of harware address
                                             field */
    uint8_t             *lladdr;        /**< Hardware address of
                                             the neighbour */
} netconf_neigh;

/** MAC VLAN network interface */
typedef struct netconf_macvlan {
    int         link;      /**< Parent (link) interface index */
    int         ifindex;   /**< MAC VLAN interface index */
    uint32_t    mode;      /**< MAC VLAN mode */
    char       *ifname;    /**< Interface name */
} netconf_macvlan;

/** IP VLAN network interface */
typedef struct netconf_ipvlan {
    int         link;      /**< Parent (link) interface index */
    int         ifindex;   /**< IP VLAN interface index */
    uint32_t    mode;      /**< IP VLAN mode */
    uint32_t    flag;      /**< IP VLAN flag */
    char       *ifname;    /**< Interface name */
} netconf_ipvlan;

/** VLAN network interface */
typedef struct netconf_vlan {
    int         link;      /**< Parent (link) interface index */
    int         ifindex;   /**< VLAN interface index */
    char       *ifname;    /**< Interface name */
    uint32_t    vid;       /**< VLAN ID */
} netconf_vlan;

/** Virtual Ethernet network interface */
typedef struct netconf_veth {
    char   *ifname;     /**< Interface name */
    char   *peer;       /**< Peer interface name */
} netconf_veth;

/** Generic UDP Tunnel interface */
typedef struct netconf_udp_tunnel {
    char       *ifname;     /**< Interface name */
    uint32_t    vni;        /**< Virtual Network Identifier */
    uint8_t     remote[sizeof(struct in6_addr)];    /*<< Remote address */
    size_t      remote_len; /**< Remote address length */
    uint16_t    port;       /**< Destination port */
} netconf_udp_tunnel;

/** Geneve network interface */
typedef struct netconf_geneve {
    netconf_udp_tunnel  generic;    /**< Generic fields of UDP Tunnel */
} netconf_geneve;

/** VXLAN network interface */
typedef struct netconf_vxlan {
    netconf_udp_tunnel  generic;    /**< Generic fields of UDP Tunnel */
    uint8_t             local[sizeof(struct in6_addr)];     /**< Local
                                                                 address */
    size_t              local_len;  /**< Local address length */
    char               *dev;        /**< Device name */
} netconf_vxlan;

/** Bridge network interface */
typedef struct netconf_bridge {
    char   *ifname;     /**< Interface name */
} netconf_bridge;

/** Bridge port network interface */
typedef struct netconf_bridge_port {
    char   *name;       /**< Interface name */
} netconf_bridge_port;

/** Information about device from devlink */
typedef struct netconf_devlink_info {
    char *bus_name;       /**< Bus name */
    char *dev_name;       /**< Device name (PCI address for PCI device) */
    char *driver_name;    /**< Driver name */
    char *serial_number;  /**< Device serial number */
} netconf_devlink_info;

/**
 * Parameter types from DEVLINK_ATTR_PARAM_TYPE.
 *
 * These must have the same values as NLA_* constants defined
 * in include/net/netlink.h in Linux kernel sources. I have no
 * idea why they are not defined in any header which can be
 * included from here or why netlink_attribute_type enum in
 * include/linux/netlink.h differs from the list defined in
 * kernel sources.
 */
typedef enum netconf_nla_type {
    NETCONF_NLA_UNSPEC,
    NETCONF_NLA_U8,
    NETCONF_NLA_U16,
    NETCONF_NLA_U32,
    NETCONF_NLA_U64,
    NETCONF_NLA_STRING,
    NETCONF_NLA_FLAG,
} netconf_nla_type;

/**
 * Get string name of netlink attribute type.
 *
 * @param nla_type      Attribute type
 *
 * @return Pointer to statically allocated string with attribute name.
 */
extern const char *netconf_nla_type2str(netconf_nla_type nla_type);

/** Device parameter value data */
typedef union netconf_devlink_param_value_data {
    uint8_t u8;   /**< uint8 value */
    uint16_t u16; /**< uint16 value */
    uint32_t u32; /**< uint32 value */
    uint64_t u64; /**< uint64 value */
    char *str;    /**< string value */
    te_bool flag; /**< flag */
} netconf_devlink_param_value_data;

/**
 * Move parameter value data from one structure to another one.
 * This function releases memory allocated for old value if necessary.
 *
 * @param nla_type        Parameter type
 * @param dst             Destination structure
 * @param src             Source structure
 */
extern void netconf_devlink_param_value_data_mv(
                        netconf_nla_type nla_type,
                        netconf_devlink_param_value_data *dst,
                        netconf_devlink_param_value_data *src);

/** Device parameter value */
typedef struct netconf_devlink_param_value {
    te_bool defined; /**< @c TRUE if value is defined */
    netconf_devlink_param_value_data data; /**< Value */
} netconf_devlink_param_value;

/** Configuration mode for device parameter value */
typedef enum netconf_devlink_param_cmode {
    NETCONF_DEVLINK_PARAM_CMODE_RUNTIME, /**< Value is applied at runtime */
    NETCONF_DEVLINK_PARAM_CMODE_DRIVERINIT, /**< Value is applied when
                                                 driver is initialized */
    NETCONF_DEVLINK_PARAM_CMODE_PERMANENT,  /**< Value is stored in
                                                 device non-volatile memory,
                                                 hard reset is required to
                                                 apply new value */

    /* This should be the last in enum */
    NETCONF_DEVLINK_PARAM_CMODE_UNDEF,      /**< Not defined */
} netconf_devlink_param_cmode;

/** Number of supported device parameter configuration modes */
#define NETCONF_DEVLINK_PARAM_CMODES NETCONF_DEVLINK_PARAM_CMODE_UNDEF

/**
 * Get string name of device parameter configuration mode.
 *
 * @param cmode       Configuration mode
 *
 * @return Pointer to statically allocated string.
 */
extern const char *devlink_param_cmode_netconf2str(
                          netconf_devlink_param_cmode cmode);

/**
 * Parse name of device parameter configuration mode.
 *
 * @param cmode       Name to parse
 *
 * @return One of the values from netconf_devlink_param_cmode.
 */
extern netconf_devlink_param_cmode devlink_param_cmode_str2netconf(
                                                      const char *cmode);

/** Information about device parameter obtained from devlink */
typedef struct netconf_devlink_param {
    char *bus_name;         /**< Bus name */
    char *dev_name;         /**< Device name */
    char *name;             /**< Parameter name */
    te_bool generic;        /**< Is parameter generic or driver-specific */
    netconf_nla_type type;  /**< Parameter type */

    /** Parameter values in different configuration modes */
    netconf_devlink_param_value values[NETCONF_DEVLINK_PARAM_CMODES];
} netconf_devlink_param;

/** Type of nodes in the list */
typedef enum netconf_node_type {
    NETCONF_NODE_UNSPEC,                /**< Unspecified */
    NETCONF_NODE_LINK,                  /**< Network device */
    NETCONF_NODE_NET_ADDR,              /**< Network address */
    NETCONF_NODE_ROUTE,                 /**< Routing table entry */
    NETCONF_NODE_NEIGH,                 /**< Neighbour table entry */
    NETCONF_NODE_RULE,                  /**< Rule entry in the routing
                                             policy database */
    NETCONF_NODE_MACVLAN,               /**< MAC VLAN interface */
    NETCONF_NODE_IPVLAN,                /**< IP VLAN interface */
    NETCONF_NODE_VLAN,                  /**< VLAN interface */
    NETCONF_NODE_VETH,                  /**< Virtual Ethernet interface */
    NETCONF_NODE_GENEVE,                /**< Geneve interface */
    NETCONF_NODE_VXLAN,                 /**< VXLAN interface */
    NETCONF_NODE_BRIDGE,                /**< Bridge interface */
    NETCONF_NODE_BRIDGE_PORT,           /**< Bridge port interface */

    NETCONF_NODE_DEVLINK_INFO,          /**< Device information obtained
                                             from devlink */
    NETCONF_NODE_DEVLINK_PARAM,         /**< Device parameters data obtained
                                             from devlink */
} netconf_node_type;

typedef te_conf_ip_rule netconf_rule;

/** Information node of the list */
typedef struct netconf_node {
    netconf_node_type     type;         /**< Type of node */
    union {
        netconf_link             link;
        netconf_net_addr         net_addr;
        netconf_route            route;
        netconf_neigh            neigh;
        netconf_rule             rule;
        netconf_macvlan          macvlan;
        netconf_ipvlan           ipvlan;
        netconf_vlan             vlan;
        netconf_veth             veth;
        netconf_geneve           geneve;
        netconf_vxlan            vxlan;
        netconf_bridge           bridge;
        netconf_bridge_port      bridge_port;

        netconf_devlink_info     devlink_info;
        netconf_devlink_param    devlink_param;
    } data;                             /**< Network data */
    struct netconf_node  *next;         /**< Next node of the list */
    struct netconf_node  *prev;         /**< Previous node of the list */
} netconf_node;

/** List of network information nodes */
typedef struct netconf_list {
    netconf_node       *head;           /**< Head of the list */
    netconf_node       *tail;           /**< Tail of the list */
    unsigned int        length;         /**< Length of the list */
} netconf_list;

/** Command to modifying functions */
typedef enum netconf_cmd {
    NETCONF_CMD_ADD,                    /**< Add some entry */
    NETCONF_CMD_DEL,                    /**< Delete some entry */
    NETCONF_CMD_CHANGE,                 /**< Change an existing entry
                                             in one step */
    NETCONF_CMD_REPLACE                 /**< Change an existing entry or
                                             add new if it is not exist */
} netconf_cmd;

/** Filter callback */
typedef bool (*netconf_node_filter_t)(netconf_node *node,
                                      void *user_data);

/** Netconf session handle */
typedef struct netconf_handle_s *netconf_handle;

/**
 * Open the netconf session and get handle of it on success. This
 * function should be called before any other in this library.
 *
 * @param nh                Address to store netconf session handle
 * @param netlink_family    Netlink family to use
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
int netconf_open(netconf_handle *nh, int netlink_family);

/**
 * Get list of all network devices. Free it with netconf_list_free()
 * function.
 *
 * @param nh            Netconf session handle
 *
 * @return List, or NULL in case of error (check errno for details).
 */
netconf_list *netconf_link_dump(netconf_handle nh);

/**
 * Move interface to a network namespace.
 *
 * @param nh        Netconf handle.
 * @param ifname    Interface name.
 * @param fd        File descriptor belongs to a network namespace or @c -1 to
 *                  choose namespace using @p pid.
 * @param pid       Process identifier in the target namespace.
 *
 * @return Status code
 */
extern te_errno netconf_link_set_ns(netconf_handle nh, const char *ifname,
                                    int32_t fd, pid_t pid);

/**
 * Set default values to fields in network address struct.
 *
 * @param net_addr      Network address struct
 */
void netconf_net_addr_init(netconf_net_addr *net_addr);

/**
 * Modify some network address. You should call netconf_net_addr_init()
 * on struct before. The only required fields to set is address and
 * ifindex.
 *
 * @param nh            Netconf session handle
 * @param cmd           Action to do
 * @param net_addr      Network address to modify
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
int netconf_net_addr_modify(netconf_handle nh, netconf_cmd cmd,
                            const netconf_net_addr *net_addr);

/**
 * Get list of all network addresses. Free it with netconf_list_free()
 * function.
 *
 * @param nh            Netconf session handle
 * @param family        Address type to filter list
 *                      (or AF_UNSPEC to get both)
 *
 * @return List, or NULL in case of error (check errno for details).
 */
netconf_list *netconf_net_addr_dump(netconf_handle nh,
                                    unsigned char family);

/**
 * Set default values to fields in route struct.
 *
 * @param route         Route entry struct
 *
 * @sa netconf_route_clean
 */
extern void netconf_route_init(netconf_route *route);

/**
 * Release memory allocated for netconf_route fields.
 *
 * @param route     Pointer to netconf_route.
 *
 * @sa netconf_route_init
 */
extern void netconf_route_clean(netconf_route *route);

/**
 * Modify some route. You should call netconf_route_init() on struct
 * before.

 *
 * @param nh            Netconf session handle
 * @param cmd           Action to do
 * @param route         Route to modify
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
int netconf_route_modify(netconf_handle nh, netconf_cmd cmd,
                         const netconf_route *route);

/**
 * Get list of all routes. Free it with netconf_list_free() function.
 *
 * @param nh            Netconf session handle
 * @param family        Address type to filter list
 *                      (or AF_UNSPEC to get both)
 *
 * @return List, or NULL in case of error (check errno for details).
 */
netconf_list *netconf_route_dump(netconf_handle nh,
                                 unsigned char family);

/**
 * Get list with routing table entry for specified destination address.
 *
 * @param nh            Netconf session handle.
 * @param dst_addr      Destination address, now only IPv4 is supported.
 *
 * @return List, or NULL in case of error (check errno for details).
 */
netconf_list *netconf_route_get_entry_for_addr(netconf_handle nh,
                                               const struct sockaddr *dst_addr);

/**
 * Get source address and interface for specified destination address.
 *
 * @param nh           Netconf session handle
 * @param dst_addr     Destination address
 * @param src_addr     Source address (OUT)
 * @param ifname       Source interface. Must be at least IF_NAMESIZE. (OUT)
 *
 * @return Status code
 */
int netconf_route_get_src_addr_and_iface(netconf_handle         nh,
                                         const struct sockaddr  *dst_addr,
                                         const struct sockaddr  *src_addr,
                                         char                   *ifname);

/**
 * Set default values to fields in rule struct.
 *
 * @param rule          Rule entry struct
 */
void netconf_rule_init(netconf_rule *rule);

/**
 * Get list of all rules. Free it with @b netconf_list_free() function.
 *
 * @param nh            Netconf session handle
 * @param family        Address type to filter list
 *                      (or AF_UNSPEC to get both)
 *
 * @return List, or @c NULL in case of error (check @b errno for details).
 */
netconf_list *netconf_rule_dump(netconf_handle nh,
                                unsigned char family);

/**
 * Modify some rule. You should call netconf_rule_init() on struct
 * before.
 *
 * @param nh            Netconf session handle
 * @param cmd           Action to do
 * @param rule          Rule to modify
 *
 * @return Status code.
 */
extern te_errno netconf_rule_modify(netconf_handle nh, netconf_cmd cmd,
                                    const netconf_rule *rule);

/**
 * Set default values to fields in neighbour struct.
 *
 * @param neigh         Neighbour entry struct
 */
void netconf_neigh_init(netconf_neigh *neigh);

/**
 * Modify some neighbour table entry. You should call netconf_neigh_init()
 * on struct before. The only required fields to set is dst and ifindex.

 *
 * @param nh            Netconf session handle
 * @param cmd           Action to do
 * @param neigh         Neighbour table entry to modify
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
int netconf_neigh_modify(netconf_handle nh, netconf_cmd cmd,
                         const netconf_neigh *neigh);

/**
 * Get list of all neighbour table entries. Free it with
 * netconf_list_free() function.
 *
 * @param nh            Netconf session handle
 * @param family        Address type to filter list
 *                      (or AF_UNSPEC to get both)
 *
 * @return List, or NULL in case of error (check errno for details).
 */
netconf_list *netconf_neigh_dump(netconf_handle nh,
                                 unsigned char family);

/**
 * Filter list with custom filter function. This function calls callback
 * on every node at the list. If it returns false, delete that node,
 * else save.
 *
 * @param list          List to filter
 * @param filter        Filter function
 * @param user_data     Data passed to callback
 */
void netconf_list_filter(netconf_list *list,
                         netconf_node_filter_t filter,
                         void *user_data);

/**
 * Free resources used by some netconf list. The list handle is invalid
 * after call of this.
 *
 * @param list          List to free
 */
void netconf_list_free(netconf_list *list);

/**
 * Close the session and free any resources used by it.
 *
 * @param nh            Netconf session handle
 */
void netconf_close(netconf_handle nh);


/* These functions get dump of some entity and filter it */

/**
 * Get list of all network addresses on specified interface. Free it
 * with netconf_net_addr_free() function.
 *
 * @param nh            Netconf session handle
 * @param family        Address type to filter list
 *                      (or AF_UNSPEC to get both)
 * @param ifindex       Interface index to filter with
 *
 * @return List, or NULL in case of error (check errno for details).
 */
netconf_list *netconf_net_addr_dump_iface(netconf_handle nh,
                                          unsigned char family,
                                          int ifindex);

/**
 * Get list of all primary/secondary network addresses. Free it
 * with netconf_net_addr_free() function.
 *
 * @param nh            Netconf session handle
 * @param family        Address type to filter list
 *                      (or AF_UNSPEC to get both)
 * @param primary       true to get primary addresses, false - secondary
 *
 * @return List, or NULL in case of error (check errno for details).
 */
netconf_list *netconf_net_addr_dump_primary(netconf_handle nh,
                                            unsigned char family,
                                            bool primary);

/**
 * Add or delete MAC VLAN interface or change mode of existing interface.
 *
 * @param nh        Netconf session handle
 * @param cmd       Action to do
 * @param link      Link (main) interface name
 * @param ifname    MAC VLAN interface name
 * @param mode_str  MAC VLAN mode
 *
 * @return Status code
 */
extern te_errno netconf_macvlan_modify(netconf_handle nh, netconf_cmd cmd,
                                       const char *link,
                                       const char *ifname,
                                       const char *mode_str);

/**
 * Get MAC VLAN interfaces list on @p link.
 *
 * @param nh        Netconf session handle
 * @param link      Link (main) interface name
 * @param list      Space separated list of MAC VLAN interfaces names
 *
 * @return Status code
 */
extern te_errno netconf_macvlan_list(netconf_handle nh, const char *link,
                                     char **list);

/**
 * Get MAC VLAN interface mode.
 *
 * @param nh        Netconf session handle
 * @param ifname    MAC VLAN interface name
 * @param mode_str  Pointer to the MAC VLAN mode string
 *
 * @return Status code
 */
extern te_errno netconf_macvlan_get_mode(netconf_handle nh,
                                         const char *ifname,
                                         const char **mode_str);

/**
 * Add or delete IP VLAN interface or change mode of existing interface.
 *
 * @param nh        Netconf session handle
 * @param cmd       Action to do
 * @param link      Link (main) interface name
 * @param ifname    IP VLAN interface name
 * @param mode      IP VLAN mode
 * @param flag      IP VLAN flag
 *
 * @return Status code
 */
extern te_errno netconf_ipvlan_modify(netconf_handle nh, netconf_cmd cmd,
                                      const char *link, const char *ifname,
                                      uint16_t mode, uint16_t flag);

/**
 * Get IP VLAN interfaces list on @p link.
 *
 * @param nh        Netconf session handle
 * @param link      Link (main) interface name
 * @param list      Space separated list of IP VLAN interfaces names
 *
 * @return Status code
 */
extern te_errno netconf_ipvlan_list(netconf_handle nh, const char *link,
                                    char **list);

/**
 * Get IP VLAN interface mode.
 *
 * @param[in] nh        Netconf session handle
 * @param[in] ifname    IP VLAN interface name
 * @param[out] mode     Pointer to the IP VLAN mode
 * @param[out] flag     Pointer to the IP VLAN flag
 *
 * @return Status code
 */
extern te_errno netconf_ipvlan_get_mode(netconf_handle nh, const char *ifname,
                                        uint32_t *mode, uint32_t *flag);

/**
 * Add or delete VLAN interface.
 *
 * @param nh        Netconf session handle
 * @param cmd       Action to do
 * @param link      Link (main) interface name
 * @param ifname    VLAN interface name (may be NULL or empty)
 * @param vid       VLAN ID
 *
 * @return Status code
 */
extern te_errno netconf_vlan_modify(netconf_handle nh, netconf_cmd cmd,
                                    const char *link,
                                    const char *ifname,
                                    unsigned int vid);

/**
 * Get VLAN interfaces list on @p link.
 *
 * @param nh        Netconf session handle
 * @param link      Link (main) interface name
 * @param list      Space separated list of VLAN interfaces IDs
 *
 * @return Status code
 */
extern te_errno netconf_vlan_list(netconf_handle nh, const char *link,
                                  char **list);

/**
 * Get name of a VLAN interface by its VLAN ID.
 *
 * @param nh        Netconf session handle
 * @param link      Link (main) interface name
 * @param vid       VLAN ID
 * @param ifname    Where to save VLAN interface name
 * @param len       Length of @p ifname buffer.
 *
 * @return Status code
 */
extern te_errno netconf_vlan_get_ifname(netconf_handle nh, const char *link,
                                        unsigned int vid,
                                        char *ifname, size_t len);

/**
 * Add new veth interface.
 *
 * @param nh        Netconf session handle
 * @param ifname    The interface name
 * @param peer      The peer interface name
 *
 * @return Status code.
 */
extern te_errno netconf_veth_add(netconf_handle nh, const char *ifname,
                                 const char *peer);

/**
 * Delete a veth interface.
 *
 * @param nh        Netconf session handle
 * @param ifname    The interface name
 *
 * @return Status code.
 */
extern te_errno netconf_veth_del(netconf_handle nh, const char *ifname);

/**
 * Get a veth peer interface.
 *
 * @param nh        Netconf session handle
 * @param ifname    The veth interface name
 * @param peer      Buffer to save the peer interface name
 * @param peer_len  @p peer buffer length
 *
 * @return Status code.
 */
extern te_errno netconf_veth_get_peer(netconf_handle nh, const char *ifname,
                                      char *peer, size_t peer_len);

/**
 * Type of callback function to pass to netconf_veth_list(). It is used to
 * decide if the interface should be included to the list.
 *
 * @param ifname    The interface name
 * @param data      Opaque data
 *
 * @return @c TRUE to include interface to the list.
 */
typedef te_bool (*netconf_veth_list_filter_func)(const char *ifname,
                                                 void *data);

/**
 * Get veth interfaces list.
 *
 * @param nh            Netconf session handle
 * @param filter_cb     Filtering callback function or @c NULL
 * @param filter_opaque Opaque data to pass to the filtering function
 * @param list          Space-separated interfaces list (allocated from the heap)
 *
 * @return Status code.
 */
extern te_errno netconf_veth_list(netconf_handle nh,
                                  netconf_veth_list_filter_func filter_cb,
                                  void *filter_opaque, char **list);

/**
 * Type of callback function to pass to netconf_udp_tunnel_list(). It is used to
 * decide if the interface should be included to the list.
 *
 * @param ifname    The interface name
 * @param data      Opaque data
 *
 * @return @c TRUE to include interface to the list.
 */
typedef te_bool (*netconf_udp_tunnel_list_filter_func)(const char *ifname,
                                                       void *data);

/**
 * Delete a UDP Tunnel interface.
 *
 * @param nh        Netconf session handle
 * @param ifname    The interface name
 *
 * @return Status code.
 */
extern te_errno netconf_udp_tunnel_del(netconf_handle nh, const char *ifname);

/**
 * Get UDP Tunnel interfaces list.
 *
 * @param nh            Netconf session handle
 * @param filter_cb     Filtering callback function or @c NULL
 * @param filter_opaque Opaque data to pass to the filtering function
 * @param list          Space-separated interfaces list (allocated from the heap)
 * @param link_kind     Link kind name to specify behavior
 *
 * @return Status code.
 */
extern te_errno netconf_udp_tunnel_list(netconf_handle nh,
                                        netconf_udp_tunnel_list_filter_func
                                            filter_cb,
                                        void *filter_opaque, char **list,
                                        char *link_kind);

/**
 * Add new Geneve interface.
 *
 * @param nh        Netconf session handle
 * @param ifname    The interface name
 *
 * @return Status code.
 */
extern te_errno netconf_geneve_add(netconf_handle nh, const netconf_geneve *geneve);

/**
 * Get Geneve interfaces list.
 *
 * @param nh            Netconf session handle
 * @param filter_cb     Filtering callback function or @c NULL
 * @param filter_opaque Opaque data to pass to the filtering function
 * @param list          Space-separated interfaces list (allocated from the heap)
 *
 * @return Status code.
 */
extern te_errno netconf_geneve_list(netconf_handle nh,
                                    netconf_udp_tunnel_list_filter_func
                                        filter_cb,
                                    void *filter_opaque, char **list);

/**
 * Add new VXLAN interface.
 *
 * @param nh        Netconf session handle
 * @param ifname    The interface name
 *
 * @return Status code.
 */
extern te_errno netconf_vxlan_add(netconf_handle nh, const netconf_vxlan *vxlan);

/**
 * Get VXLAN interfaces list.
 *
 * @param nh            Netconf session handle
 * @param filter_cb     Filtering callback function or @c NULL
 * @param filter_opaque Opaque data to pass to the filtering function
 * @param list          Space-separated interfaces list (allocated from the heap)
 *
 * @return Status code.
 */
extern te_errno netconf_vxlan_list(netconf_handle nh,
                                   netconf_udp_tunnel_list_filter_func
                                       filter_cb,
                                   void *filter_opaque, char **list);

/**
 * Add new bridge interface.
 *
 * @param nh        Netconf session handle
 * @param ifname    The interface name
 *
 * @return Status code.
 */
extern te_errno netconf_bridge_add(netconf_handle nh, const char *ifname);

/**
 * Delete a bridge interface.
 *
 * @param nh        Netconf session handle
 * @param ifname    The interface name
 *
 * @return Status code.
 */
extern te_errno netconf_bridge_del(netconf_handle nh, const char *ifname);

/**
 * Type of callback function to pass to netconf_bridge_list(). It is used to
 * decide if the interface should be included to the list.
 *
 * @param ifname    The interface name
 * @param data      Opaque data
 *
 * @return @c TRUE to include interface to the list.
 */
typedef te_bool (*netconf_bridge_list_filter_func)(const char *ifname,
                                                   void *data);

/**
 * Get bridge interfaces list.
 *
 * @param nh            Netconf session handle
 * @param filter_cb     Filtering callback function or @c NULL
 * @param filter_opaque Opaque data to pass to the filtering function
 * @param list          Space-separated interfaces list (allocated from the heap)
 *
 * @return Status code.
 */
extern te_errno netconf_bridge_list(netconf_handle nh,
                                    netconf_bridge_list_filter_func filter_cb,
                                    void *filter_opaque, char **list);

/**
 * Type of callback function to pass to netconf_port_list(). It is used to
 * decide if the interface should be included to the list.
 *
 * @param ifname    The interface name
 * @param data      Opaque data
 *
 * @return @c TRUE to include interface to the list.
 */
typedef te_bool (*netconf_port_list_filter_func)(const char *ifname,
                                                 void *data);

/**
 * Get port interfaces list.
 *
 * @param nh            Netconf session handle
 * @param brname        The bridge name
 * @param filter_cb     Filtering callback function or @c NULL
 * @param filter_opaque Opaque data to pass to the filtering function
 * @param list          Space-separated interfaces list (allocated from the heap)
 *
 * @return Status code.
 */
extern te_errno netconf_port_list(netconf_handle nh, const char *brname,
                                  netconf_port_list_filter_func filter_cb,
                                  void *filter_opaque, char **list);

/**
 * Add new bridge port interface.
 *
 * @param nh        Netconf session handle
 * @param brname    The bridge interface name
 * @param ifname    The interface name
 *
 * @return Status code.
 */
extern te_errno netconf_port_add(netconf_handle nh, const char *brname,
                                   const char *ifname);

/**
 * Delete a bridge port interface.
 *
 * @param nh        Netconf session handle
 * @param ifname    The interface name
 *
 * @return Status code.
 */
extern te_errno netconf_port_del(netconf_handle nh, const char *ifname);

/**
 * Get device infomation from devlink.
 *
 * @note @p bus and @p dev should be either both @c NULL (to retrieve
 *       information about all devices) or not @c NULL (to retrieve
 *       information about specific device).
 *
 * @param nh        Netconf session handle
 * @param bus       Bus name
 * @param dev       Device name (PCI address for PCI device)
 * @param list      Where to save pointer to the list of retrieved
 *                  device data (caller should release it)
 *
 * @return Status code.
 */
extern te_errno netconf_devlink_get_info(netconf_handle nh,
                                         const char *bus,
                                         const char *dev,
                                         netconf_list **list);

/**
 * Get list of supported device parameters from devlink.
 *
 * @param nh        Netconf session handle
 * @param list      Where to save pointer to the list of retrieved
 *                  device parameters (caller should release it)
 *
 * @return Status code.
 */
extern te_errno netconf_devlink_param_dump(netconf_handle nh,
                                           netconf_list **list);

/**
 * Set value for a device parameter in some configuration mode
 * via devlink.
 *
 * @param nh          Netconf session handle
 * @param bus         Bus name
 * @param dev         Device name (PCI address for PCI device)
 * @param param_name  Parameter name
 * @param nla_type    Parameter type
 * @param cmode       Configuration mode in which to set value
 * @param value       Value to set
 *
 * @return Status code.
 */
extern te_errno netconf_devlink_param_set(
                            netconf_handle nh,
                            const char *bus,
                            const char *dev,
                            const char *param_name,
                            netconf_nla_type nla_type,
                            netconf_devlink_param_cmode cmode,
                            const netconf_devlink_param_value_data *value);

#ifdef __cplusplus
}
#endif

#endif /* __NETCONF_H__ */
