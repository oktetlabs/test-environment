/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Network devices management in netconf library
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "netconf.h"
#include "netconf_internal.h"

/**
 * Callback of network devices dump.
 *
 * @param h             Message header
 * @param list          List of info to store
 * @param cookie        Extra parameters (unused)
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
static netconf_recv_cb_t link_list_cb;
static int
link_list_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    struct ifinfomsg   *ifla = NLMSG_DATA(h);
    struct rtattr      *rta;
    int                 len;
    netconf_link       *link;

    UNUSED(cookie);

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

            case IFLA_LINKINFO:
            {
                struct rtattr *linkinfo[IFLA_INFO_MAX + 1];

                netconf_parse_rtattr_nested(rta,
                                            linkinfo,
                                            IFLA_INFO_MAX);

                if (linkinfo[IFLA_INFO_KIND] != NULL)
                    link->info_kind =
                             netconf_dup_rta(linkinfo[IFLA_INFO_KIND]);

                break;
            }

            case IFLA_MTU:
                link->mtu = *((uint32_t *)RTA_DATA(rta));
                break;

            case IFLA_LINK:
                link->link = *((int32_t *)RTA_DATA(rta));
        }

        rta = RTA_NEXT(rta, len);
    }

    return 0;
}

netconf_list *
netconf_link_dump(netconf_handle nh)
{
    return netconf_dump_request(nh, RTM_GETLINK, AF_UNSPEC,
                                link_list_cb, NULL);
}

void
netconf_link_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    free(node->data.link.address);
    free(node->data.link.broadcast);
    free(node->data.link.ifname);
    free(node->data.link.info_kind);

    free(node);
}

te_errno
netconf_link_set_ns(netconf_handle nh, const char *ifname,
                    int32_t fd, pid_t pid)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;

    memset(&req, 0, sizeof(req));

    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    h->nlmsg_type = RTM_NEWLINK;
    h->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    h->nlmsg_seq = ++nh->seq;

    netconf_append_rta(h, ifname, strlen(ifname), IFLA_IFNAME);

    if (fd >= 0)
        netconf_append_rta(h, &fd, sizeof(fd), IFLA_NET_NS_FD);
    else
        netconf_append_rta(h, &pid, sizeof(pid), IFLA_NET_NS_PID);

    if (netconf_talk(nh, &req, sizeof(req), NULL, NULL, NULL) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}
