/** @file
 * @brief Network devices management in netconf library
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
 * Callback of network devices dump.
 *
 * @param h             Message header
 * @param list          List of info to store
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
static int
link_list_cb(struct nlmsghdr *h, netconf_list *list)
{
    struct ifinfomsg   *ifla = NLMSG_DATA(h);
    struct rtattr      *rta;
    int                 len;
    netconf_link       *link;

    if (netconf_list_extend(list, NETCONF_NODE_LINK) != 0)
        return -1;

    link = &(list->tail->data.link);

    link->type = ifla->ifi_type;
    link->ifindex = ifla->ifi_index;
    link->flags = ifla->ifi_flags;

    rta = (struct rtattr *)((char *)h +
                            NLMSG_SPACE(sizeof(struct ifinfomsg)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(struct ifinfomsg));

    while (RTA_OK(rta, len))
    {
        switch (rta->rta_type)
        {
            case IFLA_ADDRESS:
                link->address = netconf_dup_rta(rta);
                link->addrlen = RTA_PAYLOAD(rta);
                break;

            case IFLA_BROADCAST:
                link->broadcast = netconf_dup_rta(rta);
                break;

            case IFLA_IFNAME:
                link->ifname = netconf_dup_rta(rta);
                break;

            case IFLA_MTU:
                link->mtu = *((uint32_t *)RTA_DATA(rta));
                break;
        }

        rta = RTA_NEXT(rta, len);
    }

    return 0;
}

netconf_list *
netconf_link_dump(netconf_handle nh)
{
    return netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                link_list_cb);
}

void
netconf_link_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    if (node->data.link.address != NULL)
        free(node->data.link.address);
    if (node->data.link.broadcast != NULL)
        free(node->data.link.broadcast);
    if (node->data.link.ifname != NULL)
        free(node->data.link.ifname);

    free(node);
}
