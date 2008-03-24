/** @file
 * @brief Support functions for netconf library
 *
 * Copyright (C) 2008 OKTET Labs, St.-Petersburg, Russia
 *
 * @author Maxim Alyutov <Maxim.Alyutov@oktetlabs.ru>
 *
 * $Id: $
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

/** Netconf handle */
struct netconf_handle_s {
    int                 socket;         /**< Session socket */
    struct sockaddr_nl  local_addr;     /**< Socket address */
    uint32_t            seq;            /**< Current sequence number */
};

/** Callback in dump requests */
typedef int (*netconf_recv_cb_t)(struct nlmsghdr *h, netconf_list *list);

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
 *
 * @return List of information nodes, or NULL in case of error.
 */
netconf_list *netconf_dump_request(netconf_handle nh, uint16_t type,
                                   unsigned char family,
                                   netconf_recv_cb_t recv_cb);

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
 * Duplicate a data of rtattr. The memory is allocated using malloc().
 *
 * @param rta           Routing atribute
 *
 * @return Address of duplicated data, or NULL in case of error.
 */
void *netconf_dup_rta(const struct rtattr *rta);

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
 * Send request to kernel and receive response.
 *
 * @param nh            Neconf handle
 * @param req           Request data
 * @param len           Request length
 * @param recv_cb       Handler of one responce entity
 * @param list          Argument of callback
 */
int netconf_talk(netconf_handle nh, void *req, int len,
                 netconf_recv_cb_t recv_cb, netconf_list *list);

#ifdef __cplusplus
}
#endif

#endif /* __NETCONF_INTERNAL_H__ */
