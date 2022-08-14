/** @file
 * @brief Bridge interfaces management using netconf library
 *
 * Implementation of bridge interfaces configuration commands.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "Netconf bridge"

#include "te_config.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"

/* bridge link kind to pass in IFLA_INFO_KIND. */
#define NETCONF_LINK_KIND_BRIDGE "bridge"

/**
 * Initialize @p hdr.
 *
 * @param req           Request buffer
 * @param nh            Netconf session handle
 * @param nlmsg_type    Netlink message type
 * @param nlmsg_flags   Netlink message flags
 * @param hdr           Pointer to the netlink message header
 */
static void
port_init_nlmsghdr(char *req, netconf_handle nh, uint16_t nlmsg_type,
                   uint16_t nlmsg_flags, struct nlmsghdr **hdr)
{
    struct nlmsghdr *h;

    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    h->nlmsg_type = nlmsg_type;
    h->nlmsg_flags = nlmsg_flags;
    h->nlmsg_seq = ++nh->seq;

    *hdr = h;
}

/**
 * Parse the general link attribute.
 *
 * @param nh        Netconf session handle
 * @param rta_arr   Sorted attributes pointers array
 * @param max       Maximum index of the array
 */
static void
port_parse_link(struct nlmsghdr *h, struct rtattr **rta_arr, int max)
{
    struct rtattr   *rta_link;
    int              len;

    rta_link = (struct rtattr *)((char *)h + NLMSG_SPACE(sizeof(struct ifinfomsg)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(struct ifinfomsg));
    netconf_parse_rtattr(rta_link, len, rta_arr, max);
}

/**
 * Check if link is port.
 *
 * @param linkgen   The general link attributes array
 * @param cookie    Pointer to the bridge interface index
 * @return @c TRUE if link is port.
 */
static te_bool
port_link_is_bridge_port(struct rtattr **linkgen, void *cookie)
{
    if (linkgen[IFLA_MASTER] == NULL ||
        *(uint32_t *)RTA_DATA(linkgen[IFLA_MASTER]) != *(uint32_t *)cookie)
        return FALSE;
    return TRUE;
}

/**
 * Callback function to decode bridge port link data.
 *
 * @param h         The netlink message header
 * @param list      Netconf list to keep the data
 * @param cookie    Pointer to the bridge interface index
 *
 * @return @c 0 on success, @c -1 on error (check @b errno for details).
 */
static netconf_recv_cb_t port_list_cb;
static int
port_list_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    netconf_bridge_port      port;
    struct rtattr           *linkgen[IFLA_MAX + 1];

    memset(&port, 0, sizeof(port));

    port_parse_link(h, linkgen, IFLA_MAX);

    if (linkgen[IFLA_IFNAME] == NULL ||
        port_link_is_bridge_port(linkgen, cookie) == FALSE)
        return 0;

    port.name = netconf_dup_rta(linkgen[IFLA_IFNAME]);
    if (port.name == NULL)
        return -1;

    if (netconf_list_extend(list, NETCONF_NODE_BRIDGE_PORT) != 0)
    {
        free(port.name);
        return -1;
    }

    memcpy(&list->tail->data.bridge_port, &port, sizeof(port));

    return 0;
}

/* See netconf_internal.h */
void
netconf_port_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    free(node->data.bridge_port.name);
    free(node);
}

/* See netconf.h */
te_errno
netconf_port_add(netconf_handle nh, const char *brname, const char *ifname)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    uint32_t            br_ifind;

    memset(req, 0, sizeof(req));
    IFNAME_TO_INDEX(brname, br_ifind);
    port_init_nlmsghdr(req, nh, RTM_SETLINK,
                       NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL,
                       &h);

    netconf_append_rta(h, ifname, strlen(ifname) + 1, IFLA_IFNAME);
    netconf_append_rta(h, &br_ifind, sizeof(uint32_t), IFLA_MASTER);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL, NULL) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* See netconf.h */
te_errno
netconf_port_del(netconf_handle nh, const char *ifname)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    uint32_t            zero_ind = 0;

    memset(req, 0, sizeof(req));
    port_init_nlmsghdr(req, nh, RTM_SETLINK, NLM_F_REQUEST | NLM_F_ACK, &h);

    netconf_append_rta(h, ifname, strlen(ifname) + 1, IFLA_IFNAME);
    netconf_append_rta(h, &zero_ind, sizeof(uint32_t), IFLA_MASTER);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL, NULL) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* See netconf.h */
te_errno
netconf_port_list(netconf_handle nh, const char *brname,
                  netconf_port_list_filter_func filter_cb,
                  void *filter_opaque, char **list)
{
    netconf_list *nlist;
    netconf_node *node;
    te_string     str = TE_STRING_INIT;
    te_errno      rc = 0;
    uint32_t      br_ifind;

    IFNAME_TO_INDEX(brname, br_ifind);
    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                 port_list_cb, &br_ifind);
    if (nlist == NULL)
    {
        ERROR("Failed to get bridge port interfaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (node->data.bridge_port.name != NULL)
        {
            if (filter_cb != NULL &&
                filter_cb(node->data.bridge_port.name, filter_opaque) == TRUE)
                continue;
            rc = te_string_append(&str, "%s ", node->data.bridge_port.name);
            if (rc != 0)
            {
                te_string_free(&str);
                rc = TE_RC(TE_TA_UNIX, rc);
                break;
            }
        }
    }

    netconf_list_free(nlist);

    if (rc == 0)
        *list = str.ptr;

    return rc;
}

/**
 * Initialize @p hdr.
 *
 * @param req           Request buffer
 * @param nh            Netconf session handle
 * @param nlmsg_type    Netlink message type
 * @param nlmsg_flags   Netlink message flags
 * @param hdr           Pointer to the netlink message header
 */
static void
bridge_init_nlmsghdr(char *req, netconf_handle nh, uint16_t nlmsg_type,
                   uint16_t nlmsg_flags, struct nlmsghdr **hdr)
{
    struct nlmsghdr *h;

    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    h->nlmsg_type = nlmsg_type;
    h->nlmsg_flags = nlmsg_flags;
    h->nlmsg_seq = ++nh->seq;

    *hdr = h;
}

/* See netconf_internal.h */
void
netconf_bridge_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    free(node->data.bridge.ifname);
    free(node);
}

/**
 * Parse the general link attribute.
 *
 * @param nh        Netconf session handle
 * @param rta_arr   Sorted attributes pointers array
 * @param max       Maximum index of the array
 */
static void
bridge_parse_link(struct nlmsghdr *h, struct rtattr **rta_arr, int max)
{
    struct rtattr   *rta_link;
    int              len;

    rta_link = (struct rtattr *)((char *)h + NLMSG_SPACE(sizeof(struct ifinfomsg)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(struct ifinfomsg));
    netconf_parse_rtattr(rta_link, len, rta_arr, max);
}

/**
 * Check if link is bridge.
 *
 * @param linkgen   The general link attributes array
 *
 * @return @c TRUE if link is bridge.
 */
static te_bool
bridge_link_is_bridge(struct rtattr **linkgen)
{
    struct rtattr *linkinfo[IFLA_INFO_MAX + 1];

    netconf_parse_rtattr_nested(linkgen[IFLA_LINKINFO], linkinfo,
                                IFLA_INFO_MAX);

    if (linkinfo[IFLA_INFO_KIND] == NULL ||
        strcmp(RTA_DATA(linkinfo[IFLA_INFO_KIND]),
               NETCONF_LINK_KIND_BRIDGE) != 0)
        return FALSE;

    return TRUE;
}

/**
 * Callback function to decode bridge link data.
 *
 * @param h         The netlink message header
 * @param list      Netconf list to keep the data
 * @param cookie    Extra parameters (unused)
 *
 * @return @c 0 on success, @c -1 on error (check @b errno for details).
 */
static netconf_recv_cb_t bridge_list_cb;
static int
bridge_list_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    netconf_bridge      bridge;
    struct rtattr      *linkgen[IFLA_MAX + 1];

    UNUSED(cookie);

    memset(&bridge, 0, sizeof(bridge));

    bridge_parse_link(h, linkgen, IFLA_MAX);

    if (linkgen[IFLA_LINKINFO] == NULL || linkgen[IFLA_IFNAME] == NULL ||
        bridge_link_is_bridge(linkgen) == FALSE)
        return 0;

    bridge.ifname = netconf_dup_rta(linkgen[IFLA_IFNAME]);
    if (bridge.ifname == NULL)
    {
        return -1;
    }

    if (netconf_list_extend(list, NETCONF_NODE_BRIDGE) != 0)
    {
        free(bridge.ifname);
        return -1;
    }

    memcpy(&list->tail->data.bridge, &bridge, sizeof(bridge));

    return 0;
}

te_errno
netconf_bridge_add(netconf_handle nh, const char *ifname)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    struct rtattr      *linkinfo;
    struct rtattr      *data;

    memset(req, 0, sizeof(req));
    bridge_init_nlmsghdr(req, nh, RTM_NEWLINK,
                       NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL,
                       &h);

    netconf_append_rta(h, ifname, strlen(ifname) + 1, IFLA_IFNAME);
    netconf_append_rta_nested(h, IFLA_LINKINFO, &linkinfo);

    netconf_append_rta(h, NETCONF_LINK_KIND_BRIDGE,
                       strlen(NETCONF_LINK_KIND_BRIDGE) + 1, IFLA_INFO_KIND);

    netconf_append_rta_nested(h, IFLA_INFO_DATA, &data);

    netconf_append_rta_nested_end(h, data);
    netconf_append_rta_nested_end(h, linkinfo);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL, NULL) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* See netconf.h */
te_errno
netconf_bridge_del(netconf_handle nh, const char *ifname)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;

    memset(req, 0, sizeof(req));
    bridge_init_nlmsghdr(req, nh, RTM_DELLINK, NLM_F_REQUEST | NLM_F_ACK, &h);

    netconf_append_rta(h, ifname, strlen(ifname) + 1, IFLA_IFNAME);

    if (netconf_talk(nh, req, sizeof(req), NULL, NULL, NULL) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* See netconf.h */
te_errno
netconf_bridge_list(netconf_handle nh, netconf_bridge_list_filter_func filter_cb,
                  void *filter_opaque, char **list)
{
    netconf_list *nlist;
    netconf_node *node;
    te_string     str = TE_STRING_INIT;
    te_errno      rc = 0;

    nlist = netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                 bridge_list_cb, NULL);
    if (nlist == NULL)
    {
        ERROR("Failed to get bridge interfaces list");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (node = nlist->head; node != NULL; node = node->next)
    {
        if (node->data.bridge.ifname != NULL)
        {
            if (filter_cb != NULL &&
                filter_cb(node->data.bridge.ifname, filter_opaque) == FALSE)
                continue;
            rc = te_string_append(&str, "%s ", node->data.bridge.ifname);
            if (rc != 0)
            {
                te_string_free(&str);
                rc = TE_RC(TE_TA_UNIX, rc);
                break;
            }
        }
    }

    netconf_list_free(nlist);

    if (rc == 0)
        *list = str.ptr;

    return rc;
}
