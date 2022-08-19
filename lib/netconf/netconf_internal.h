/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Support functions for netconf library
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __NETCONF_INTERNAL_H__
#define __NETCONF_INTERNAL_H__

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "netconf.h"

/** Maximum size of request to kernel */
#define NETCONF_MAX_REQ_LEN 512

/** Maximum size of mx buffer */
#define NETCONF_MAX_MXBUF_LEN 256

/** Size of receive buffer */
#define NETCONF_RCV_BUF_LEN 16384

/** Maximum socket send buffer in bytes */
#define NETCONF_SOCK_SNDBUF 32768

/** Maximum socket receive buffer in bytes */
#define NETCONF_SOCK_RCVBUF 32768

/** Invalid prefix length */
#define NETCONF_PREFIX_UNSPEC 255

#ifdef NETCONF_ASSERT_ENABLE
/** Simple assertion */
#define NETCONF_ASSERT(expr) assert(expr)
#else
/** Nothing */
#define NETCONF_ASSERT(expr) (void)(expr)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Get pointer to the tail of netlink message data. */
#define NETCONF_NLMSG_TAIL(_msg) \
    ((struct rtattr *) (((void *) (_msg)) + NLMSG_ALIGN((_msg)->nlmsg_len)))

/** Netconf handle */
struct netconf_handle_s {
    int                 socket;         /**< Session socket */
    struct sockaddr_nl  local_addr;     /**< Socket address */
    uint32_t            seq;            /**< Current sequence number */
};

/** Callback in dump requests */
typedef int (netconf_recv_cb_t)(struct nlmsghdr *h, netconf_list *list,
                                void *cookie);

/**
 * Callback function to decode Geneve link data.
 */
extern netconf_recv_cb_t geneve_list_cb;

/**
 * Callback function to decode VXLAN link data.
 */
extern netconf_recv_cb_t vxlan_list_cb;


/**
 * Get nlmsghdr flags to set depending on command.
 *
 * @param cmd           Action to do
 *
 * @return Set of flags.
 */
uint16_t netconf_cmd_to_flags(netconf_cmd cmd);

/**
 * Handle dump request.
 *
 * @param nh            Netconf handle
 * @param type          Type of dump as in nlmsg_type field
 * @param family        Address family to dump
 * @param recv_cb       Handler of one responce entity
 * @param cookie        Parameters for recv_cb
 *
 * @return List of information nodes, or NULL in case of error.
 */
netconf_list *netconf_dump_request(netconf_handle nh, uint16_t type,
                                   unsigned char family,
                                   netconf_recv_cb_t *recv_cb,
                                   void *cookie);

/**
 * Extend list by one node. The node data is filled with zeroes.
 *
 * @param list          Netconf list
 * @param type          Type of new node
 *
 * @return 0 on success, -1 on error.
 */
int netconf_list_extend(netconf_list *list, netconf_node_type type);

/**
 * Append generic attribute structure (struct nlattr) to the end
 * of netlink message, update length in its header.
 *
 * @param req         Netlink message
 * @param max_len     Maximum length available for the message
 * @param attr_type   Attribute type
 * @param data        Data stored in attribute
 * @param len         Length of the data
 *
 * @return Status code.
 */
extern te_errno netconf_append_attr(char *req, size_t max_len,
                                    uint16_t attr_type, const void *data,
                                    size_t len);

/**
 * Get string value from netlink attribute.
 *
 * @param na      Pointer to the attribute
 * @param value   Where to save the value (dynamically allocated
 *                null-terminated string which the caller should
 *                release)
 *
 * @return Status code.
 */
extern te_errno netconf_get_str_attr(struct nlattr *na, char **value);

/**
 * Get uint8_t value from netlink attribute.
 *
 * @param na      Pointer to the attribute
 * @param value   Where to save obtained value
 *
 * @return Status code.
 */
extern te_errno netconf_get_uint8_attr(struct nlattr *na, uint8_t *value);

/**
 * Get uint16_t value from netlink attribute.
 *
 * @param na      Pointer to the attribute
 * @param value   Where to save obtained value
 *
 * @return Status code.
 */
extern te_errno netconf_get_uint16_attr(struct nlattr *na, uint16_t *value);

/**
 * Get uint32_t value from netlink attribute.
 *
 * @param na      Pointer to the attribute
 * @param value   Where to save obtained value
 *
 * @return Status code.
 */
extern te_errno netconf_get_uint32_attr(struct nlattr *na, uint32_t *value);

/**
 * Get uint64_t value from netlink attribute.
 *
 * @param na      Pointer to the attribute
 * @param value   Where to save obtained value
 *
 * @return Status code.
 */
extern te_errno netconf_get_uint64_attr(struct nlattr *na, uint64_t *value);

/**
 * Append routing attribute to existing nlmsg. There should be enough
 * memory after the message.
 *
 * @param h             Header of message
 * @param data          Data of rtattr
 * @param len           Length of data
 * @param rta_type      Type of rtattr
 */
void netconf_append_rta(struct nlmsghdr *h, const void *data, int len,
                        unsigned short rta_type);

/**
 * Append a nested attribute.
 *
 * @param h         Header of message
 * @param rta_type  Type of rtattr
 * @param rta       Pointer to @b rtattr pointer
 */
extern void netconf_append_rta_nested(struct nlmsghdr *h,
                                      unsigned short rta_type,
                                      struct rtattr **rta);

/**
 * Finalise appending of a nested attribute.
 *
 * @param h     Header of message
 * @param rta   Pointer to the nested attribute
 */
extern void netconf_append_rta_nested_end(struct nlmsghdr *h,
                                          struct rtattr *rta);

/**
 * Parse attributes message.
 *
 * @param rta       Pointer to the first attribute in the message
 * @param len       The message length
 * @param rta_arr   Sorted attributes pointers array
 * @param max       Maximum index of the array
 */
extern void netconf_parse_rtattr(struct rtattr *rta, int len,
                                 struct rtattr **rta_arr, int max);

/**
 * Parse attributes message.
 *
 * @param rta       Pointer to the first attribute in the message
 * @param len       The message length
 * @param rta_arr   Sorted attributes pointers array
 * @param max       Maximum index of the array
 */
extern void netconf_parse_rtattr_nested(struct rtattr *rta,
                                        struct rtattr **rta_arr,
                                        int max);

/**
 * Get u32 value from data of attribute @p rta.
 */
extern uint32_t netconf_get_rta_u32(struct rtattr *rta);

/**
 * Duplicate a data of rtattr. The memory is allocated using malloc().
 *
 * @param rta           Routing atribute
 *
 * @return Address of duplicated data, or NULL in case of error.
 */
void *netconf_dup_rta(const struct rtattr *rta);


/** Generic callback for attribute processing */
typedef te_errno (*netconf_attr_cb)(struct nlattr *na, void *cb_data);

/**
 * Process netlink attributes.
 *
 * @param first_attr      Pointer to the first attribute
 * @param attrs_size      Total number of bytes occupied by attributes
 * @param cb              Callback to which pointer to every attribute
 *                        is passed
 * @param cb_data         User data passed to the callback
 *
 * @return Status code.
 */
extern te_errno netconf_process_attrs(void *first_attr, size_t attrs_size,
                                      netconf_attr_cb cb, void *cb_data);

/**
 * Wrapper over netconf_process_attrs() which assumes that attributes
 * are placed immediately after some header in netlink message.
 *
 * @param h         Pointer to the main header of the netlink message
 * @param hdr_len   Length of the additional header (not the main header)
 * @param cb        Callback to which pointer to every attribute is passed
 * @param cb_data   User data passed to the callback
 *
 * @return Status code.
 */
extern te_errno netconf_process_hdr_attrs(struct nlmsghdr *h,
                                          size_t hdr_len,
                                          netconf_attr_cb cb,
                                          void *cb_data);

/**
 * Wrapper over netconf_process_attrs() which assumes that parsed
 * attributes are nested inside another attribute.
 *
 * @param na_parent   Parent attribute
 * @param cb          Callback to which pointer to every attribute is passed
 * @param cb_data     User data passed to the callback
 *
 * @return Status code.
 */
extern te_errno netconf_process_nested_attrs(struct nlattr *na_parent,
                                             netconf_attr_cb cb,
                                             void *cb_data);

/**
 * Free memory used by node of network interface type.
 *
 * @param node          Node to free
 */
void netconf_link_node_free(netconf_node *node);

/**
 * Free memory used by node of network address type.
 *
 * @param node          Node to free
 */
void netconf_net_addr_node_free(netconf_node *node);

/**
 * Free memory used by node of route type.
 *
 * @param node          Node to free
 */
void netconf_route_node_free(netconf_node *node);

/**
 * Free memory used by node of neighbour type.
 *
 * @param node          Node to free
 */
void netconf_neigh_node_free(netconf_node *node);

/**
 * Free memory used by node of rule type.
 *
 * @param node          Node to free
 */
void netconf_rule_node_free(netconf_node *node);

/**
 * Free memory used by node of MAC VLAN type.
 *
 * @param node  Node to free
 */
extern void netconf_macvlan_node_free(netconf_node *node);

/**
 * Free memory used by node of IP VLAN type.
 *
 * @param node  Node to free
 */
extern void netconf_ipvlan_node_free(netconf_node *node);

/**
 * Free memory used by node of VLAN type.
 *
 * @param node  Node to free
 */
extern void netconf_vlan_node_free(netconf_node *node);

/**
 * Free memory used by a veth node.
 *
 * @param node  Node to free
 */
extern void netconf_veth_node_free(netconf_node *node);

/**
 * Free memory used by a generic UDP Tunnel struct.
 *
 * @param node  Struct to free
 */
extern void netconf_udp_tunnel_free(netconf_udp_tunnel *udp_tunnel);

/**
 * Free memory used by a Geneve node.
 *
 * @param node  Node to free
 */
extern void netconf_geneve_node_free(netconf_node *node);

/**
 * Free memory used by a VXLAN node.
 *
 * @param node  Node to free
 */
extern void netconf_vxlan_node_free(netconf_node *node);

/**
 * Free memory used by a bridge node.
 *
 * @param node  Node to free
 */
extern void netconf_bridge_node_free(netconf_node *node);

/**
 * Free memory used by a bridge port node.
 *
 * @param node  Node to free
 */
extern void netconf_port_node_free(netconf_node *node);

/**
 * Free memory used by a devlink device information node.
 *
 * @param node  Node to free
 */
extern void netconf_devlink_info_node_free(netconf_node *node);

/**
 * Free memory used by a devlink device parameter node.
 *
 * @param node  Node to free
 */
extern void netconf_devlink_param_node_free(netconf_node *node);

/**
 * Send request to kernel and receive response.
 *
 * @param nh            Neconf handle
 * @param req           Request data
 * @param len           Request length
 * @param recv_cb       Handler of one responce entity
 * @param cookie        Parameters for recv_cb
 * @param list          Argument of callback
 */
int netconf_talk(netconf_handle nh, void *req, int len,
                 netconf_recv_cb_t *recv_cb, void *cookie, netconf_list *list);

/**
 * Initialize @p hdr.
 *
 * @param req           Request buffer
 * @param nh            Netconf session handle
 * @param nlmsg_type    Netlink message type
 * @param nlmsg_flags   Netlink message flags
 * @param hdr           Pointer to the netlink message header
 */
extern void netconf_init_nlmsghdr(char *req, netconf_handle nh,
                                  uint16_t nlmsg_type, uint16_t nlmsg_flags,
                                  struct nlmsghdr **hdr);

/**
 * Parse the general link attribute.
 *
 * @param nh        Netconf session handle
 * @param rta_arr   Sorted attributes pointers array
 * @param max       Maximum index of the array
 */
extern void netconf_parse_link(struct nlmsghdr *h, struct rtattr **rta_arr,
                               int max);

/**
 * Get interface index by its name. Return error if it cannot be done.
 *
 * @param _name     Name of the interface.
 * @param _index    Where to save interface index.
 */
#define IFNAME_TO_INDEX(_name, _index) \
    do {                                                            \
        if ((_index = if_nametoindex(_name)) == 0)                  \
        {                                                           \
            ERROR("Failed to get index of interface %s", _name);    \
            return TE_RC(TE_TA_UNIX, TE_EINVAL);                    \
        }                                                           \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* __NETCONF_INTERNAL_H__ */
