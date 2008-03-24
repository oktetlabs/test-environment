/** @file
 * @brief Network addresses management in netconf library
 *
 * Copyright (C) 2008 OKTET Labs, St.-Petersburg, Russia
 *
 * @author Maxim Alyutov <Maxim.Alyutov@oktetlabs.ru>
 *
 * $Id: $
 */

#include "netconf.h"
#include "netconf_internal.h"

/**
 * Callback of network addresses dump.
 *
 * @param h             Message header
 * @param list          List of info to store
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
static int
net_addr_list_cb(struct nlmsghdr *h, netconf_list *list)
{
    struct ifaddrmsg   *ifa = NLMSG_DATA(h);
    struct rtattr      *rta;
    int                 len;
    netconf_net_addr   *net_addr;

    if (netconf_list_extend(list, NETCONF_NODE_NET_ADDR) != 0)
        return -1;

    net_addr = &(list->tail->data.net_addr);

    net_addr->family = ifa->ifa_family;
    net_addr->prefix = ifa->ifa_prefixlen;
    net_addr->flags = ifa->ifa_flags;
    net_addr->ifindex = ifa->ifa_index;

    rta = (struct rtattr *)((char *)h +
                            NLMSG_SPACE(sizeof(struct ifaddrmsg)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(struct ifaddrmsg));

    while (RTA_OK(rta, len))
    {
        switch (rta->rta_type)
        {
            case IFA_ADDRESS:
                net_addr->address = netconf_dup_rta(rta);
                break;

            case IFA_BROADCAST:
                net_addr->broadcast = netconf_dup_rta(rta);
                break;
        }

        rta = RTA_NEXT(rta, len);
    }

    return 0;
}

netconf_list *
netconf_net_addr_dump(netconf_handle nh, unsigned char family)
{
    return netconf_dump_request(nh, RTM_GETADDR, family,
                                net_addr_list_cb);
}

/**
 * Filter with interface index.
 *
 * @param node          Node of list
 * @param user_data     Address of interface index
 *
 * @return true, if index in node is the same, false otherwise.
 */
static bool
netconf_net_addr_filter_iface(netconf_node *node, void *user_data)
{
    int ifindex;

    NETCONF_ASSERT(user_data != NULL);
    ifindex = *((int *)user_data);

    return (node->data.net_addr.ifindex == ifindex);
}

netconf_list *
netconf_net_addr_dump_iface(netconf_handle nh,
                            unsigned char family,
                            int ifindex)
{
    netconf_list *list;

    list = netconf_net_addr_dump(nh, family);
    netconf_list_filter(list, netconf_net_addr_filter_iface, &ifindex);

    return list;
}

/**
 * Filter with primary/secondary address.
 *
 * @param node          Node of list
 * @param user_data     true to get primary, false - secondary
 *
 * @return true, if flags is ok, false otherwise.
 */
static bool
netconf_net_addr_filter_primary(netconf_node *node, void *user_data)
{
    bool primary;

    NETCONF_ASSERT(user_data != NULL);
    primary = *((bool *)user_data);

    if (primary)
        return !(node->data.net_addr.flags & IFA_F_SECONDARY);
    else
        return (node->data.net_addr.flags & IFA_F_SECONDARY);
}

netconf_list *
netconf_net_addr_dump_primary(netconf_handle nh,
                              unsigned char family,
                              bool primary)
{
    netconf_list *list;

    list = netconf_net_addr_dump(nh, family);
    netconf_list_filter(list, netconf_net_addr_filter_primary, &primary);

    return list;
}

void
netconf_net_addr_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    if (node->data.net_addr.address != NULL)
        free(node->data.net_addr.address);
    if (node->data.net_addr.broadcast != NULL)
        free(node->data.net_addr.broadcast);

    free(node);
}

void
netconf_net_addr_init(netconf_net_addr *net_addr)
{
    memset(net_addr, 0 , sizeof(netconf_net_addr));
    net_addr->family = AF_INET;
    net_addr->prefix = NETCONF_PREFIX_UNSPEC;
}

int
netconf_net_addr_modify(netconf_handle nh, netconf_cmd cmd,
                        const netconf_net_addr *net_addr)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    struct ifaddrmsg   *ifa;
    unsigned int        addrlen;

    /* Check required fields */
    if ((net_addr == NULL) ||
        ((net_addr->family != AF_INET) &&
         (net_addr->family != AF_INET6)) ||
        (net_addr->address == NULL) ||
        (net_addr->ifindex == 0))
    {
        errno = EINVAL;
        return -1;
    }

    /* Generate request */
    memset(&req, 0, sizeof(req));

    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    h->nlmsg_type = (cmd == NETCONF_CMD_DEL) ? RTM_DELADDR : RTM_NEWADDR;
    h->nlmsg_flags = netconf_cmd_to_flags(cmd);
    if (h->nlmsg_flags == 0)
    {
        errno = EINVAL;
        return -1;
    }
    h->nlmsg_seq = ++nh->seq;

    /* Set parameters or some defaults if unspecified */

    ifa = NLMSG_DATA(h);
    ifa->ifa_family = net_addr->family;

    if (net_addr->prefix == NETCONF_PREFIX_UNSPEC)
    {
        if (cmd == NETCONF_CMD_ADD)
            ifa->ifa_prefixlen = 32;
        else
            ifa->ifa_prefixlen = 0;
    }
    else
    {
        ifa->ifa_prefixlen = net_addr->prefix;
    }

    ifa->ifa_flags = net_addr->flags;
    ifa->ifa_index = net_addr->ifindex;

    NETCONF_ASSERT((net_addr->family == AF_INET) ||
                   (net_addr->family == AF_INET6));
    addrlen = (net_addr->family == AF_INET) ?
              sizeof(struct in_addr) : sizeof(struct in6_addr);

    NETCONF_ASSERT(net_addr->address != NULL);
    netconf_append_rta(h, net_addr->address, addrlen, IFA_ADDRESS);
    netconf_append_rta(h, net_addr->address, addrlen, IFA_LOCAL);

    if (net_addr->broadcast != NULL)
    {
        netconf_append_rta(h, net_addr->broadcast, addrlen,
                           IFA_BROADCAST);
    }

    /* Send request and receive ACK */
    return netconf_talk(nh, &req, sizeof(req), NULL, NULL);
}
