/** @file
 * @brief Linux network configuration support library
 *
 * This library provides access to Linux network configuration using
 * netlink protocol. It is similiar to libnetlink from iproute2 package
 * but has higher level interface and appropriate license. Following
 * information is covered: network devices, IP addresses, neighbour
 * tables, routing tables.
 *
 * Copyright (C) 2008 OKTET Labs, St.-Petersburg, Russia
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
    NETCONF_RTPROT_STATIC
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

/** Network device */
typedef struct netconf_link {
    netconf_link_type   type;           /**< Device type */
    int                 ifindex;        /**< Interface index */
    unsigned int        flags;          /**< Device flags */
    unsigned int        addrlen;        /**< Length of address field */
    uint8_t            *address;        /**< Interface hardware address */
    uint8_t            *broadcast;      /**< Broadcast address */
    char               *ifname;         /**< Device name as string */
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
} netconf_route;

/** Neighbour table entry (ARP or NDISC cache) */
typedef struct netconf_neigh {
    unsigned char        family;        /**< Address family of entry */
    int                  ifindex;       /**< Interface index */
    uint16_t             state;         /**< State of neighbour entry */
    uint8_t             *dst;           /**< Protocol address of
                                             the neighbour */
    unsigned int         addrlen;       /**< Length of harware address
                                             field */
    uint8_t             *lladdr;        /**< Hardware address of
                                             the neighbour */
} netconf_neigh;

/** Type of nodes in the list */
typedef enum netconf_node_type {
    NETCONF_NODE_UNSPEC,                /**< Unspecified */
    NETCONF_NODE_LINK,                  /**< Network device */
    NETCONF_NODE_NET_ADDR,              /**< Network address */
    NETCONF_NODE_ROUTE,                 /**< Routing table entry */
    NETCONF_NODE_NEIGH                  /**< Neighbour table entry */
} netconf_node_type;

/** Information node of the list */
typedef struct netconf_node {
    netconf_node_type     type;         /**< Type of node */
    union {
        netconf_link      link;
        netconf_net_addr  net_addr;
        netconf_route     route;
        netconf_neigh     neigh;
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
 * @param nh            Address to store netconf session handle
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
int netconf_open(netconf_handle *nh);

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
 */
void netconf_route_init(netconf_route *route);

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

#ifdef __cplusplus
}
#endif

#endif /* __NETCONF_H__ */
