/** @file
 * @brief Routing tables management in netconf library
 *
 * Copyright (C) 2004-2022 OKTET Labs, St.-Petersburg, Russia
 *
 * @author Maxim Alyutov <Maxim.Alyutov@oktetlabs.ru>
 *
 */

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"
#include "te_sockaddr.h"
#include <sys/ioctl.h>

/**
 * Callback of routes dump.
 *
 * @param h             Message header
 * @param list          List of info to store
 * @param cookie        Extra parameters (unused)
 *
 * @return 0 on success, -1 on error (check errno for details).
 */
static netconf_recv_cb_t route_list_cb;
static int
route_list_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    struct rtmsg       *rtm = NLMSG_DATA(h);
    struct rtattr      *rta;
    int                 len;
    netconf_route      *route;

    UNUSED(cookie);

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
    LIST_INIT(&route->hops);

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

                            case RTAX_HOPLIMIT:
                                route->hoplimit =
                                    *((uint32_t *)RTA_DATA(mxrta));
                                break;
                        }

                        mxrta = RTA_NEXT(mxrta, mxlen);
                    }

                    break;
                }
            case RTA_CACHEINFO:
                {
                    struct rta_cacheinfo *ci = RTA_DATA(rta);

                    route->expires = ci->rta_expires;
                    break;
                }
            case RTA_MULTIPATH:
                {
                    struct rtnexthop *nh;
                    struct rtattr    *nh_rta;
                    int               len;
                    int               nh_len;

                    netconf_route_nexthop   *nc_nh_prev = NULL;
                    netconf_route_nexthop   *nc_nh;

                    len = RTA_PAYLOAD(rta);
                    nh = RTA_DATA(rta);

                    while (len > 0 &&
                           (unsigned int)len >= sizeof(struct rtnexthop) &&
                           RTNH_OK(nh, len))
                    {
                        nh_rta = RTNH_DATA(nh);
                        nh_len = nh->rtnh_len -
                                      ((uint8_t *)nh_rta - (uint8_t *)nh);

                        nc_nh = calloc(1, sizeof(*nc_nh));
                        if (nc_nh == NULL)
                            return -1;

                        if (nc_nh_prev != NULL)
                            LIST_INSERT_AFTER(nc_nh_prev, nc_nh, links);
                        else
                            LIST_INSERT_HEAD(&route->hops, nc_nh, links);

                        nc_nh_prev = nc_nh;

                        nc_nh->weight = nh->rtnh_hops + 1;
                        nc_nh->oifindex = nh->rtnh_ifindex;

                        while (RTA_OK(nh_rta, nh_len))
                        {
                            switch (nh_rta->rta_type)
                            {
                                case RTA_GATEWAY:
                                    nc_nh->gateway = netconf_dup_rta(nh_rta);
                                    break;
                            }

                            nh_rta = RTA_NEXT(nh_rta, nh_len);
                        }

                        len -= (uint8_t *)RTNH_NEXT(nh) - (uint8_t *)nh;
                        nh = RTNH_NEXT(nh);
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
                                route_list_cb, NULL);
}

/* See description in netconf.h */
void
netconf_route_clean(netconf_route *route)
{
    netconf_route_nexthop *nc_nh;
    netconf_route_nexthop *nc_nh_next;

    NETCONF_ASSERT(route != NULL);

    if (route->dst != NULL)
        free(route->dst);
    if (route->src != NULL)
        free(route->src);
    if (route->gateway != NULL)
        free(route->gateway);

    LIST_FOREACH_SAFE(nc_nh, &route->hops, links, nc_nh_next)
    {
        LIST_REMOVE(nc_nh, links);
        free(nc_nh);
    }
}

void
netconf_route_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);
    netconf_route_clean(&node->data.route);
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
    LIST_INIT(&route->hops);
}

/**
 * Fill nexthops of a multipath route in netlink request.
 *
 * @param h             Pointer to nlmsghdr structure.
 * @param max_size      Maximum number of bytes in netlink request.
 * @param addr_family   Route address family.
 * @param hops          List of route nexthops.
 *
 * @return @c 0 on success, @c -1 on failure. Sets errno in case
 *         of failure.
 */
static int
fill_nexthops(struct nlmsghdr *h,
              size_t max_size,
              int addr_family,
              const netconf_route_nexthops *hops)
{
    netconf_route_nexthop *p;

    struct rtattr    *rta_main;
    struct rtattr    *rta;
    struct rtnexthop *rt_nh;
    size_t            addr_len;
    size_t            attr_len;
    size_t            total_size;
    size_t            new_nl_len;

    rta_main = NETCONF_NLMSG_TAIL(h);

    addr_len = te_netaddr_get_size(addr_family);
    total_size = 0;

    LIST_FOREACH(p, hops, links)
    {
        total_size += RTNH_SPACE(p->gateway != NULL ?
                                 RTA_SPACE(addr_len) :
                                 0);
    }

    new_nl_len =
          NLMSG_LENGTH(((uint8_t *)rta_main + RTA_SPACE(total_size)) -
                       (uint8_t *)NLMSG_DATA(h));

    if (new_nl_len > max_size)
    {
        errno = ENOBUFS;
        return -1;
    }

    rta_main->rta_type = RTA_MULTIPATH;
    rt_nh = RTA_DATA(rta_main);

    LIST_FOREACH(p, hops, links)
    {
        rt_nh->rtnh_hops = p->weight - 1;
        rt_nh->rtnh_ifindex = p->oifindex;
        attr_len = 0;
        if (p->gateway != NULL)
        {
            rta = RTNH_DATA(rt_nh);
            rta->rta_type = RTA_GATEWAY;
            memcpy(RTA_DATA(rta), p->gateway, addr_len);
            rta->rta_len = RTA_LENGTH(addr_len);
            attr_len = RTA_SPACE(addr_len);
        }

        rt_nh->rtnh_len = RTNH_LENGTH(attr_len);

        rt_nh = RTNH_NEXT(rt_nh);
    }

    rta_main->rta_len = RTA_LENGTH(total_size);
    h->nlmsg_len = new_nl_len;

    return 0;
}

/**
 * Append route attribute to buffer storing array of route metrics.
 *
 * @param buf         Buffer to which to append.
 * @param cur_len     Current length of data in the buffer (will be
 *                    updated by this function on success).
 * @param max_len     Maximum length of data in the buffer.
 * @param type        Type of route attribute.
 * @param val         Pointer to value of attribute.
 * @param val_len     Length of the value in bytes.
 *
 * @return @c 0 on success, @c -1 on failure (errno is set in case
 *         of failure).
 */
static int
append_rtax(char *buf, size_t *cur_len, size_t max_len,
            int type, const void *val, size_t val_len)
{
    struct rtattr  *subrta = (struct rtattr *)(buf + *cur_len);

    if (*cur_len + RTA_SPACE(val_len) > max_len)
    {
        ERROR("%s(): not enough space for route metric %d",
              __FUNCTION__, type);
        errno = ENOBUFS;
        return -1;
    }

    subrta->rta_type = type;
    subrta->rta_len = RTA_LENGTH(val_len);

    memcpy(RTA_DATA(subrta), val, val_len);
    *cur_len += RTA_SPACE(val_len);
    return 0;
}

int
netconf_route_modify(netconf_handle nh, netconf_cmd cmd,
                     const netconf_route *route)
{
    char                req[NETCONF_MAX_REQ_LEN];
    char                mxbuf[NETCONF_MAX_MXBUF_LEN];
    struct nlmsghdr    *h;
    struct rtmsg       *rtm;
    size_t              mxbuflen;
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

        /*
         * This is not always necessary, however for unknown reason
         * adding multipath route with scope LINK fails with ENETUNREACH
         * error if gateways are specified in nexthops.
         */
        if (!LIST_EMPTY(&route->hops))
            rtm->rtm_scope = RT_SCOPE_UNIVERSE;
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

    if (route->table != NETCONF_RT_TABLE_MAIN)
    {
        netconf_append_rta(h, &(route->table), sizeof(route->table),
                           RTA_TABLE);
    }

    memset(mxbuf, 0, sizeof(mxbuf));
    mxbuflen = 0;

    if (route->mtu != 0)
    {
        if (append_rtax(mxbuf, &mxbuflen, sizeof(mxbuf), RTAX_MTU,
                        &(route->mtu), sizeof(route->mtu)) < 0)
            return -1;
    }

    if (route->win != 0)
    {
        if (append_rtax(mxbuf, &mxbuflen, sizeof(mxbuf), RTAX_WINDOW,
                        &(route->win), sizeof(route->win)) < 0)
            return -1;
    }

    if (route->irtt != 0)
    {
        if (append_rtax(mxbuf, &mxbuflen, sizeof(mxbuf), RTAX_RTT,
                        &(route->irtt), sizeof(route->irtt)) < 0)
            return -1;
    }

    if (route->hoplimit != 0)
    {
        if (append_rtax(mxbuf, &mxbuflen, sizeof(mxbuf), RTAX_HOPLIMIT,
                        &(route->hoplimit), sizeof(route->hoplimit)) < 0)
            return -1;
    }

    if (mxbuflen > RTA_LENGTH(0))
        netconf_append_rta(h, mxbuf, mxbuflen, RTA_METRICS);

    if (!LIST_EMPTY(&route->hops))
    {
        if (fill_nexthops(h, sizeof(req), route->family,
                          &route->hops) < 0)
            return -1;
    }

    /* Send request and receive ACK */
    return netconf_talk(nh, &req, sizeof(req), NULL, NULL, NULL);
}

netconf_list *
netconf_route_get_entry_for_addr(netconf_handle nh,
                                 const struct sockaddr *dst_addr)
{
    char                req[NETCONF_MAX_REQ_LEN];
    struct nlmsghdr    *h;
    struct rtmsg       *rtm;
    netconf_list       *list;

    if (nh == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    if (dst_addr->sa_family != AF_INET)
    {
        ERROR("%s(): failed, IPv6 is not supported", __FUNCTION__);
        errno = ENOTSUP;
        return NULL;
    }

    memset(&req, 0, sizeof(req));

    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    h->nlmsg_type = RTM_GETROUTE;
    h->nlmsg_flags = NLM_F_REQUEST;
    h->nlmsg_seq = ++nh->seq;

    rtm = NLMSG_DATA(h);
    rtm->rtm_family = AF_INET;
    rtm->rtm_dst_len = sizeof(in_addr_t);
    rtm->rtm_src_len = 0;
    rtm->rtm_tos = 0;
    rtm->rtm_table = RT_TABLE_MAIN;
    rtm->rtm_protocol = RTPROT_UNSPEC;
    rtm->rtm_scope = RT_SCOPE_UNIVERSE;
    rtm->rtm_type = RTN_UNSPEC;
    rtm->rtm_flags = 0;

    netconf_append_rta(h, &(SIN(dst_addr)->sin_addr.s_addr),
                       sizeof(in_addr_t), RTA_DST);

    list = malloc(sizeof(*list));
    if (list == NULL)
        return NULL;

    memset(list, 0, sizeof(netconf_list));

    if (netconf_talk(nh, &req, h->nlmsg_len, route_list_cb, NULL, list) != 0)
    {
        int err = errno;

        netconf_list_free(list);
        errno = err;
        return NULL;
    }

    return list;
}

int
netconf_route_get_src_addr_and_iface(netconf_handle         nh,
                                     const struct sockaddr  *dst_addr,
                                     const struct sockaddr  *src_addr,
                                     char                   *ifname)
{
    netconf_list    *l;
    netconf_route   *route;
    int             rc;

    l = netconf_route_get_entry_for_addr(nh, dst_addr);
    if (l == NULL)
    {
        ERROR("%s(): failed to get entry in routing table for remote "
              "IP address", __FUNCTION__);
        return errno;
    }

    route = &(l->tail->data.route);
    if (route->family != AF_INET)
    {
        ERROR("%s(): failed, IPv6 is not supported", __FUNCTION__);
        rc = ENOTSUP;
        goto out;
    }
    SIN(src_addr)->sin_family = route->family;
    memcpy(&(SIN(src_addr)->sin_addr.s_addr), route->src, sizeof(in_addr_t));

    if (if_indextoname(route->oifindex, ifname) == NULL)
    {
        rc = errno;
        goto out;
    }
    rc = 0;

out:
    netconf_list_free(l);
    return rc;
}
