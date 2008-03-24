/** @file
 * @brief Routing tables management in netconf library
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
 * Callback of routes dump.
 *
 * @param h             Message header
 * @param list          List of info to store
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
static int
route_list_cb(struct nlmsghdr *h, netconf_list *list)
{
    struct rtmsg       *rtm = NLMSG_DATA(h);
    struct rtattr      *rta;
    int                 len;
    netconf_route      *route;

    if (netconf_list_extend(list, NETCONF_NODE_ROUTE) != 0)
        return -1;

    route = &(list->tail->data.route);

    route->family = rtm->rtm_family;
    route->dstlen = rtm->rtm_dst_len;
    route->srclen = rtm->rtm_src_len;
    route->tos = rtm->rtm_tos;
    route->table = rtm->rtm_table;
    route->protocol = rtm->rtm_protocol;
    route->scope = rtm->rtm_scope;
    route->type = rtm->rtm_type;
    route->flags = rtm->rtm_flags;

    rta = (struct rtattr *)((char *)h +
                            NLMSG_SPACE(sizeof(struct rtmsg)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(struct rtmsg));

    while (RTA_OK(rta, len))
    {
        switch (rta->rta_type)
        {
            case RTA_DST:
                route->dst = netconf_dup_rta(rta);
                break;

            case RTA_GATEWAY:
                route->gateway = netconf_dup_rta(rta);
                break;

            case RTA_PREFSRC:
                route->src = netconf_dup_rta(rta);
                break;

            case RTA_OIF:
                route->oifindex = *((uint32_t *)RTA_DATA(rta));
                break;

            case RTA_PRIORITY:
                route->metric = *((uint32_t *)RTA_DATA(rta));
                break;

            case RTA_METRICS:
                {
                    int            mxlen = RTA_PAYLOAD(rta);
                    struct rtattr *mxrta = RTA_DATA(rta);

                    while (RTA_OK(mxrta, mxlen))
                    {
                        switch (mxrta->rta_type)
                        {
                            case RTAX_MTU:
                                route->mtu =
                                    *((uint32_t *)RTA_DATA(mxrta));
                                break;

                            case RTAX_WINDOW:
                                route->win =
                                    *((uint32_t *)RTA_DATA(mxrta));
                                break;

                            case RTAX_RTT:
                                route->irtt =
                                    *((uint32_t *)RTA_DATA(mxrta));
                                break;
                        }

                        mxrta = RTA_NEXT(mxrta, mxlen);
                    }

                    break;
                }
        }

        rta = RTA_NEXT(rta, len);
    }

    return 0;
}

netconf_list *
netconf_route_dump(netconf_handle nh, unsigned char family)
{
    return netconf_dump_request(nh, RTM_GETROUTE, family,
                                route_list_cb);
}

void
netconf_route_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    if (node->data.route.dst != NULL)
        free(node->data.route.dst);
    if (node->data.route.src != NULL)
        free(node->data.route.src);
    if (node->data.route.gateway != NULL)
        free(node->data.route.gateway);

    free(node);
}

void
netconf_route_init(netconf_route *route)
{
    memset(route, 0 , sizeof(netconf_route));
    route->family = AF_INET;
    route->dstlen = NETCONF_PREFIX_UNSPEC;
    route->srclen = NETCONF_PREFIX_UNSPEC;
    route->table = NETCONF_RT_TABLE_UNSPEC;
    route->protocol = NETCONF_RTPROT_UNSPEC;
    route->scope = NETCONF_RT_SCOPE_UNSPEC;
    route->type = NETCONF_RTN_UNSPEC;
}

int
netconf_route_modify(netconf_handle nh, netconf_cmd cmd,
                     const netconf_route *route)
{
    char                req[NETCONF_MAX_REQ_LEN];
    char                mxbuf[NETCONF_MAX_MXBUF_LEN];
    struct nlmsghdr    *h;
    struct rtmsg       *rtm;
    unsigned int        mxbuflen;
    unsigned int        addrlen;

    /* Check required fields */
    if ((route == NULL) ||
        ((route->family != AF_INET) && (route->family != AF_INET6)))
    {
        errno = EINVAL;
        return -1;
    }

    /* Generate request */
    memset(&req, 0, sizeof(req));

    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    h->nlmsg_type = (cmd == NETCONF_CMD_DEL) ?
                    RTM_DELROUTE : RTM_NEWROUTE;
    h->nlmsg_flags = netconf_cmd_to_flags(cmd);
    if (h->nlmsg_flags == 0)
    {
        errno = EINVAL;
        return -1;
    }
    h->nlmsg_seq = ++nh->seq;

    /* Set parameters or some defaults if unspecified */

    rtm = NLMSG_DATA(h);
    rtm->rtm_family = route->family;

    if (route->dstlen == NETCONF_PREFIX_UNSPEC)
        rtm->rtm_dst_len = (route->dst == NULL) ? 0 : 32;
    else
        rtm->rtm_dst_len = route->dstlen;

    if (route->srclen == NETCONF_PREFIX_UNSPEC)
        rtm->rtm_src_len = (route->src == NULL) ? 0 : 32;
    else
        rtm->rtm_src_len = route->srclen;

    rtm->rtm_tos = route->tos;

    rtm->rtm_table = (route->table == NETCONF_RT_TABLE_UNSPEC) ?
                     RT_TABLE_MAIN : route->table;

    if ((route->protocol == NETCONF_RTPROT_UNSPEC) &&
        (cmd != NETCONF_CMD_DEL))
    {
        rtm->rtm_protocol = RTPROT_BOOT;
    }
    else
    {
        rtm->rtm_protocol = route->protocol;
    }

    if ((route->type == NETCONF_RTN_UNSPEC) && (cmd != NETCONF_CMD_DEL))
        rtm->rtm_type = RTN_UNICAST;
    else
        rtm->rtm_type = route->type;

    if (route->scope == NETCONF_RT_SCOPE_UNSPEC)
    {
        if (cmd != NETCONF_CMD_DEL)
            rtm->rtm_scope = RT_SCOPE_UNIVERSE;
        else
            rtm->rtm_scope = RT_SCOPE_NOWHERE;

        switch (rtm->rtm_type)
        {
            case RTN_LOCAL:
            case RTN_NAT:
                rtm->rtm_scope = RT_SCOPE_HOST;
                break;

            case RTN_BROADCAST:
            case RTN_MULTICAST:
            case RTN_ANYCAST:
                rtm->rtm_scope = RT_SCOPE_LINK;
                break;

            case RTN_UNICAST:
            case RTN_UNSPEC:
                if (cmd == NETCONF_CMD_DEL)
                    rtm->rtm_scope = RT_SCOPE_NOWHERE;
                else if (route->gateway == NULL)
                    rtm->rtm_scope = RT_SCOPE_LINK;

                break;
        }
    }
    else
    {
        rtm->rtm_scope = route->scope;
    }

    rtm->rtm_flags = route->flags;

    NETCONF_ASSERT((route->family == AF_INET) ||
                   (route->family == AF_INET6));
    addrlen = (route->family == AF_INET) ?
              sizeof(struct in_addr) : sizeof(struct in6_addr);

    if (route->dst != NULL)
        netconf_append_rta(h, route->dst, addrlen, RTA_DST);

    if (route->oifindex != 0)
        netconf_append_rta(h, &(route->oifindex), sizeof(int), RTA_OIF);

    if (route->src != NULL)
        netconf_append_rta(h, route->src, addrlen, RTA_PREFSRC);

    if (route->gateway != NULL)
        netconf_append_rta(h, route->gateway, addrlen, RTA_GATEWAY);

    if (route->metric != 0)
    {
        netconf_append_rta(h, &(route->metric), sizeof(route->metric),
                           RTA_PRIORITY);
    }

    memset(mxbuf, sizeof(mxbuf), 0);
    mxbuflen = 0;

    if (route->mtu != 0)
    {
        struct rtattr  *subrta = (struct rtattr *)(mxbuf + mxbuflen);
        unsigned int    len = sizeof(route->mtu);

        subrta->rta_type = RTAX_MTU;
        subrta->rta_len = RTA_LENGTH(len);

        memcpy(RTA_DATA(subrta), &(route->mtu), len);
        mxbuflen += RTA_SPACE(len);
    }

    if (route->win != 0)
    {
        struct rtattr  *subrta = (struct rtattr *)(mxbuf + mxbuflen);
        unsigned int    len = sizeof(route->win);

        subrta->rta_type = RTAX_WINDOW;
        subrta->rta_len = RTA_LENGTH(len);

        memcpy(RTA_DATA(subrta), &(route->win), len);
        mxbuflen += RTA_SPACE(len);
    }

    if (route->irtt != 0)
    {
        struct rtattr  *subrta = (struct rtattr *)(mxbuf + mxbuflen);
        unsigned int    len = sizeof(route->irtt);

        subrta->rta_type = RTAX_RTT;
        subrta->rta_len = RTA_LENGTH(len);

        memcpy(RTA_DATA(subrta), &(route->irtt), len);
        mxbuflen += RTA_SPACE(len);
    }

    if (mxbuflen > RTA_LENGTH(0))
        netconf_append_rta(h, mxbuf, mxbuflen, RTA_METRICS);

    /* Send request and receive ACK */
    return netconf_talk(nh, &req, sizeof(req), NULL, NULL);
}
