/** @file
 * @brief Unix Test Agent
 *
 * Unix TA routing configuring support using Linux netlink interface.
 * The following code is based on iproute2-050816 package.
 * There is no clear specification of netlink interface.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Ivan Soloducha <Ivan.Soloducha@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id: conf.c 24823 2006-03-05 07:24:43Z arybchik $
 */

#ifdef USE_NETLINK

#if !defined(__linux__)
#error netlink can be used on Linux only
#endif

#define TE_LGR_USER     "Unix Conf Route NetLink"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif


#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "cs_common.h"
#include "logger_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "unix_internal.h"
#include "conf_route.h"

#ifdef USE_NETLINK
#include <sys/select.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <fnmatch.h>
#include <linux/sockios.h>
#include <iproute/libnetlink.h>
#include <iproute/rt_names.h>
#include <iproute/utils.h>
#include <iproute/ll_map.h>
#include <iproute/ip_common.h>
#endif


static char buf[4096];


struct nl_request {
    struct nlmsghdr n;
    struct rtmsg    r;
    char            buf[1024];
};

/** 
 * Convert system-independent route info data structure to
 * netlink-specific data structure.
 *
 * @param rt_info TA portable route info
 * @Param req     netlink-specific data structure (OUT)
 */
static int
rt_info2nl_req(const ta_rt_info_t *rt_info, struct nl_request *req)
{
    char                 mxbuf[256];
    struct rtattr       *mxrta = (void*)mxbuf;
    unsigned char        family;  

    assert(rt_info != NULL && req != NULL);

    mxrta->rta_type = RTA_METRICS;
    mxrta->rta_len = RTA_LENGTH(0);

    req->r.rtm_dst_len = rt_info->prefix;
    family = req->r.rtm_family = rt_info->dst.ss_family;
    
    if ((family == AF_INET && 
         addattr_l(&req->n, sizeof(*req), RTA_DST,
                   &SIN(&rt_info->dst)->sin_addr,
                   sizeof(struct in_addr)) != 0) ||
        (family == AF_INET6 &&
         addattr_l(&req->n, sizeof(*req), RTA_DST,
                   &SIN6(&rt_info->dst)->sin6_addr,
                   sizeof(struct in6_addr)) != 0))
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_GW) != 0)
    {
        if (addattr_l(&req->n, sizeof(*req), RTA_GATEWAY,
                      &(SIN(&(rt_info->gw))->sin_addr),
                      sizeof(SIN(&(rt_info->gw))->sin_addr)) != 0)
        {
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }
    if ((rt_info->flags & TA_RT_INFO_FLG_IF) != 0)
    {
        int idx;

        if ((idx = if_nametoindex(rt_info->ifname)) == 0)
        {
            ERROR("Cannot find interface %s", rt_info->ifname);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        addattr32(&req->n, sizeof(*req), RTA_OIF, idx);
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_METRIC) != 0)
        addattr32(&req->n, sizeof(*req), RTA_PRIORITY, rt_info->metric);

    if ((rt_info->flags & TA_RT_INFO_FLG_MTU) != 0)
        rta_addattr32(mxrta, sizeof(mxbuf), RTAX_MTU, rt_info->mtu);

    if ((rt_info->flags & TA_RT_INFO_FLG_WIN) != 0)
        rta_addattr32(mxrta, sizeof(mxbuf), RTAX_WINDOW, rt_info->win);

    if ((rt_info->flags & TA_RT_INFO_FLG_IRTT) != 0)
        rta_addattr32(mxrta, sizeof(mxbuf), RTAX_RTT, rt_info->irtt);

    if ((rt_info->flags & TA_RT_INFO_FLG_TOS) != 0)
    {
        req->r.rtm_tos = rt_info->tos;
    }
    
    if (mxrta->rta_len > RTA_LENGTH(0))
    {
        addattr_l(&req->n, sizeof(*req), RTA_METRICS, RTA_DATA(mxrta),
                  RTA_PAYLOAD(mxrta));
    }

    return 0;
}


/**
 * Flush routing table cache.
 *
 * @param family    Address family
 *
 * @return Status code.
 */
static te_errno
route_flush(sa_family_t family)
{
    const char *fn = (family == AF_INET) ?
                     "/proc/sys/net/ipv4/route/flush" :
                     "/proc/sys/net/ipv6/route/flush";
    const char *cmd = "1\n";
    int         fd;
    te_errno    rc = 0;

    fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if ((fd < 0) || (write(fd, cmd, strlen(cmd)) < 0))
    {
        rc = te_rc_os2te(errno);
        WARN("Failed to flush routing table cache - %s %s: %r",
             (fd < 0) ? "cannot open file" : "cannot write to file",
             fn, rc);
    }

    if ((fd >= 0) && (close(fd) != 0))
    {
        rc = te_rc_os2te(errno);
    }

    return rc;
}

/*
 * See the description in conf_route.h.
 *
 * The code of this function is based on iproute2 GPL package.
 */
te_errno
ta_unix_conf_route_change(ta_cfg_obj_action_e  action,
                          ta_rt_info_t        *rt_info)
{
    static unsigned ta_rt_type2rtm_type[TA_RT_TYPE_MAX_VALUE] = 
        {
            RTN_UNSPEC,
            RTN_UNICAST,
            RTN_LOCAL,
            RTN_BROADCAST,
            RTN_ANYCAST,
            RTN_MULTICAST,
            RTN_BLACKHOLE,
            RTN_UNREACHABLE,
            RTN_PROHIBIT,
            RTN_THROW,
            RTN_NAT
        };
    int                 nlm_action;
    int                 nlm_flags;
    struct nl_request  req;
    struct rtnl_handle rth;
    te_errno           rc;

    switch (action)
    {
#define TA_CFG_OBJ_ACTION_CASE(case_label_, action_, flg_) \
        case TA_CFG_OBJ_ ## case_label_:               \
            nlm_action = (action_);                    \
            nlm_flags = (flg_);                        \
            break
        
        TA_CFG_OBJ_ACTION_CASE(CREATE, RTM_NEWROUTE,
                               NLM_F_CREATE | NLM_F_EXCL);
        TA_CFG_OBJ_ACTION_CASE(DELETE, RTM_DELROUTE, 0);
        TA_CFG_OBJ_ACTION_CASE(SET, RTM_NEWROUTE,
                               NLM_F_REPLACE);

#undef TA_CFG_OBJ_ACTION_CASE

        default:
            ERROR("Unknown object action specified %d", action);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (rt_info == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | nlm_flags;
    req.n.nlmsg_type = nlm_action;

    req.r.rtm_family = SA(&rt_info->dst)->sa_family;
    req.r.rtm_table = RT_TABLE_MAIN;
    req.r.rtm_scope = RT_SCOPE_NOWHERE;

    if (nlm_action != RTM_DELROUTE)
    {
        req.r.rtm_protocol = RTPROT_BOOT;
        req.r.rtm_scope = RT_SCOPE_UNIVERSE;
        req.r.rtm_type  = ta_rt_type2rtm_type[rt_info->type];
    }

    /* Sending the netlink message */
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open the netlink socket");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_IF) != 0)
    {
        /*
         * We need to call this function as rt_info2nl_req() 
         * will need to convert interface name to index.
         */
        ll_init_map(&rth);
    }
    
    if ((rc = rt_info2nl_req(rt_info, &req)) != 0)
    {
        rtnl_close(&rth);
        return rc;
    }
    
    switch (req.r.rtm_type)
    {
        case RTN_LOCAL:
        case RTN_NAT:
            req.r.rtm_scope = RT_SCOPE_HOST;
            break;
        case RTN_UNICAST:
        case RTN_UNSPEC:
        {
            if (nlm_action == RTM_DELROUTE)
                req.r.rtm_scope = RT_SCOPE_NOWHERE;
            else if ((rt_info->flags & TA_RT_INFO_FLG_GW) == 0)
                req.r.rtm_scope = RT_SCOPE_LINK;
            break;
        }
        case RTN_BLACKHOLE:
        case RTN_UNREACHABLE:
        case RTN_PROHIBIT:
            req.r.rtm_scope = RT_SCOPE_NOWHERE;
            break;
        default:
            req.r.rtm_scope = RT_SCOPE_LINK;
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
    {
        ERROR("Failed to send the netlink message");
        rtnl_close(&rth);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rtnl_close(&rth);

    /* Flush routing cache on success, but ignore errors */
    (void)route_flush(rt_info->dst.ss_family);

    return 0;
}


/** Structure used for RTNL user callback */
typedef struct rtnl_cb_user_data {
    ta_rt_info_t *rt_info; /**< Routing entry information (IN/OUT)
                                On input it keeps route key,
                                On output it is augmented with route
                                attributes: mtu, win etc. */
    int if_index;          /**< Interface index in case of direct route.
                                This field has sense  only if 
                                TA_RT_INFO_FLG_IF flag is set */
    te_errno rc;                /**< Return code */
    te_bool filled;        /**< Whether this structure is filled or not */
} rtnl_cb_user_data_t;

/**
 * Callback for rtnl_dump_filter() function.
 */
static int
rtnl_get_route_cb(const struct sockaddr_nl *who,
                  const struct nlmsghdr *n, void *arg)
{
    static ta_route_type rtm_type2ta_rt_type[TA_RT_TYPE_MAX_VALUE] = 
        {
            TA_RT_TYPE_UNSPECIFIED,
            TA_RT_TYPE_UNICAST,
            TA_RT_TYPE_LOCAL,
            TA_RT_TYPE_BROADCAST,
            TA_RT_TYPE_ANYCAST,
            TA_RT_TYPE_MULTICAST,
            TA_RT_TYPE_BLACKHOLE,
            TA_RT_TYPE_UNREACHABLE,
            TA_RT_TYPE_PROHIBIT,
            TA_RT_TYPE_THROW,
            TA_RT_TYPE_NAT
        };
    struct rtmsg        *r = NLMSG_DATA(n);
    int                  len = n->nlmsg_len;
    rtnl_cb_user_data_t *user_data = (rtnl_cb_user_data_t *)arg;
    struct rtattr       *tb[RTA_MAX + 1] = {NULL,};
    int                  family;
    struct in6_addr      addr_any = IN6ADDR_ANY_INIT;

    UNUSED(who);

    if (user_data->filled)
        return 0;

    if (n->nlmsg_type != RTM_NEWROUTE && n->nlmsg_type != RTM_DELROUTE)
        return 0;
    
    if (r->rtm_family != AF_INET && r->rtm_family != AF_INET6)
    {
       return 0;        
    }

    user_data->rt_info->type = rtm_type2ta_rt_type[r->rtm_type];

    len -= NLMSG_LENGTH(sizeof(*r));

    parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);

    family = r->rtm_family;
    /* Check the case of INADDR_ANY address */
    if ((tb[RTA_DST] == NULL &&
         ((family == AF_INET &&
           SIN(&(user_data->rt_info->dst))->sin_addr.s_addr == INADDR_ANY)
       || (family == AF_INET6 &&
           memcmp(&SIN6(&(user_data->rt_info->dst))->sin6_addr.in6_u,
                  &addr_any, sizeof(struct in6_addr)) == 0))) ||
    /* Check that destination address is equal to requested one */
        (tb[RTA_DST] != NULL &&
         ((family == AF_INET &&
           memcmp(RTA_DATA(tb[RTA_DST]),
                  &(SIN(&(user_data->rt_info->dst))->sin_addr),
                  sizeof(struct in_addr)) == 0) ||
          (family == AF_INET6 &&
          memcmp(RTA_DATA(tb[RTA_DST]),
                  &(SIN6(&(user_data->rt_info->dst))->sin6_addr),
                  sizeof(struct in6_addr)) == 0)) &&
          /* Check that prefix is correct */
          user_data->rt_info->prefix == r->rtm_dst_len &&
          ((user_data->rt_info->flags & TA_RT_INFO_FLG_METRIC) == 0 ||
           user_data->rt_info->metric ==
               *(uint32_t *)RTA_DATA(tb[RTA_PRIORITY])) &&
          (user_data->rt_info->tos == r->rtm_tos)))
    {
        if (tb[RTA_OIF] != NULL)
        {            
            user_data->rt_info->flags |= TA_RT_INFO_FLG_IF;
            user_data->if_index = *(int *)RTA_DATA(tb[RTA_OIF]);
            memcpy(user_data->rt_info->ifname,
                   ll_index_to_name(user_data->if_index),
                   IFNAMSIZ);
        }
        
        if (tb[RTA_GATEWAY] != NULL)
        {
            user_data->rt_info->flags |= TA_RT_INFO_FLG_GW;
            user_data->rt_info->gw.ss_family = family;
            if (family == AF_INET)
            {
                memcpy(&SIN(&user_data->rt_info->gw)->sin_addr,
                       RTA_DATA(tb[RTA_GATEWAY]), sizeof(struct in_addr));
            }
            else if (family == AF_INET6)
            {
                memcpy(&SIN6(&user_data->rt_info->gw)->sin6_addr,
                       RTA_DATA(tb[RTA_GATEWAY]), sizeof(struct in6_addr));
            }
        }
 
        if (tb[RTA_PRIORITY] != NULL)
        {
            user_data->rt_info->flags |= TA_RT_INFO_FLG_METRIC;
            user_data->rt_info->metric = *(uint32_t *)
                                         RTA_DATA(tb[RTA_PRIORITY]);
        }
        
        if (tb[RTA_METRICS] != NULL)
        {
            struct rtattr *mxrta[RTAX_MAX + 1] = {NULL,};

            parse_rtattr(mxrta, RTAX_MAX, RTA_DATA(tb[RTA_METRICS]),
                         RTA_PAYLOAD(tb[RTA_METRICS]));
            
            if (mxrta[RTAX_MTU] != NULL)
            {
                user_data->rt_info->flags |= TA_RT_INFO_FLG_MTU;
                user_data->rt_info->mtu = 
                    *(unsigned *)RTA_DATA(mxrta[RTAX_MTU]);
            }
            if (mxrta[RTAX_WINDOW] != NULL)
            {
                user_data->rt_info->flags |= TA_RT_INFO_FLG_WIN;
                user_data->rt_info->win = 
                    *(unsigned *)RTA_DATA(mxrta[RTAX_WINDOW]);
            }
            if (mxrta[RTAX_RTT] != NULL)
            {
                user_data->rt_info->flags |= TA_RT_INFO_FLG_IRTT;
                user_data->rt_info->irtt = 
                    *(unsigned *)RTA_DATA(mxrta[RTAX_RTT]);
            }
        }        

        user_data->filled = TRUE;
        return 0;
    }
    return 0;
}

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_find(ta_rt_info_t *rt_info)
{
    te_errno            rc;
    struct rtnl_handle  rth;
    rtnl_cb_user_data_t user_data;

    memset(&user_data, 0, sizeof(user_data));

    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Failed to open a netlink socket: %r", rc);
        return rc;
    }
    ll_init_map(&rth);

    if (rtnl_wilddump_request(&rth, rt_info->dst.ss_family,
                              RTM_GETROUTE) < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        rtnl_close(&rth);
        ERROR("Cannot send dump request to netlink: %r");
        return rc;
    }

    /* Fill in user_data, which will be passed to callback function */
    user_data.rt_info = rt_info;
    user_data.filled = FALSE;

    if (rtnl_dump_filter(&rth, rtnl_get_route_cb,
                         &user_data, NULL, NULL) < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        rtnl_close(&rth);
        ERROR("Dump terminated: %r");
        return rc;
    }
    rtnl_close(&rth);

    if (!user_data.filled)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return 0;
}


typedef struct {
    int           family;        /**< Route address family */
    char         *buf;           /**< Where to print the route */
} rtnl_print_route_cb_user_data_t;

static int
rtnl_print_route_cb(const struct sockaddr_nl *who,
                    const struct nlmsghdr *n, void *arg)
{
    struct rtmsg        *r = NLMSG_DATA(n);
    int                  len = n->nlmsg_len;
    char                *p;
    const char          *ifname;
    
    rtnl_print_route_cb_user_data_t *user_data = 
                         (rtnl_print_route_cb_user_data_t *)arg;
    
    struct rtattr       *tb[RTA_MAX + 1] = {NULL,};
    int                  family;

    UNUSED(who);

      
    family = r->rtm_family;
    
    if (family != user_data->family)
    {
        return 0;
    }

    if (family != AF_INET && family != AF_INET6)
    {
        return 0;
    }
    
    if (family == AF_INET6)
    {
#if 1
        /* Route destination is unreachable */
        if (tb[RTA_PRIORITY] && *(int*)RTA_DATA(tb[RTA_PRIORITY]) == -1)
            return 0;

        if ((r->rtm_flags & RTM_F_CLONED) != 0)
            return 0;

        if (r->rtm_type != RTN_UNICAST)
            return 0;

        if (n->nlmsg_type != RTM_NEWROUTE)
            return 0;
#endif        
    }
    else
    {
        if (r->rtm_table != RT_TABLE_MAIN)
            return 0;
    }

    len -= NLMSG_LENGTH(sizeof(*r));

    parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);

    if (tb[RTA_OIF] == NULL)
        return 0;
    
    ifname = ll_index_to_name(*(int *)RTA_DATA(tb[RTA_OIF]));
    if (!INTERFACE_IS_MINE(ifname))
        return 0;

    p = user_data->buf;
    
    if (tb[RTA_DST] == NULL)
    {
        if (r->rtm_dst_len != 0)
        {
            ERROR("NULL destination with non-zero prefix");
            return 0;
        }
        else
        {
            if (family == AF_INET)
            {
                p += sprintf(p, "0.0.0.0|0");
            }
            else
            {    
                p += sprintf(p, "::|0");
            }
        }
    }
    else
    {
        if (tb[RTA_GATEWAY] != NULL &&
            memcmp(RTA_DATA(tb[RTA_DST]), RTA_DATA(tb[RTA_GATEWAY]),
            r->rtm_dst_len) == 0)
        {
            /* Gateway = destination, skip route */
            return 0;
        }
            
        inet_ntop(family, RTA_DATA(tb[RTA_DST]), p, INET6_ADDRSTRLEN);
        p += strlen(p);
        p += sprintf(p, "|%d", r->rtm_dst_len);        
    }

    if (tb[RTA_PRIORITY] != NULL &&
        (*(uint32_t *)RTA_DATA(tb[RTA_PRIORITY]) != 0))
    {
        p += sprintf(p, ",metric=%d", 
                     *(uint32_t *)RTA_DATA(tb[RTA_PRIORITY]));
    }

    if (r->rtm_tos != 0)
    {
        p += sprintf(p, ",tos=%d", r->rtm_tos);
    }

    p += sprintf(p, " ");
    user_data->buf = p;

    return 0;
}

/**
 * Get instance list for object "agent/route".
 *
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return              Status code
 * @retval 0                       success
 * @retval TE_ENOENT               no such instance
 * @retval TE_ENOMEM               cannot allocate memory
 */
te_errno
ta_unix_conf_route_list(char **list)
{
    struct rtnl_handle               rth;
    rtnl_print_route_cb_user_data_t  user_data;    

    ENTRY();

    buf[0] = '\0';

    memset(&user_data, 0, sizeof(user_data));

    user_data.buf = buf;

    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open a netlink socket");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    ll_init_map(&rth);
    
#define GET_ALL_ROUTES_OF_FAMILY(__family)                      \
do {                                                            \
    if (rtnl_wilddump_request(&rth, __family,                   \
                              RTM_GETROUTE) < 0)                \
    {                                                           \
        rtnl_close(&rth);                                       \
        ERROR("Cannot send dump request to netlink");           \
        return TE_OS_RC(TE_TA_UNIX, errno);                     \
    }                                                           \
                                                                \
    /* Fill in user_data, which will be passed to callback function */  \
    user_data.family = __family;                                \
    if (rtnl_dump_filter(&rth, rtnl_print_route_cb,             \
                         &user_data, NULL, NULL) < 0)           \
    {                                                           \
        rtnl_close(&rth);                                       \
        ERROR("Dump terminated");                               \
        return TE_OS_RC(TE_TA_UNIX, errno);                     \
    }                                                           \
} while (0)

    GET_ALL_ROUTES_OF_FAMILY(AF_INET);
#if 0
    GET_ALL_ROUTES_OF_FAMILY(AF_INET6);
#endif

#undef GET_ALL_ROUTES_OF_FAMILY    
    
    rtnl_close(&rth);

    INFO("%s: Routes: %s", __FUNCTION__, buf);
    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}


/**
 * Callback to implement listing of 'blackhole' routes.
 */
static int
rtnl_print_blackhole_cb(const struct sockaddr_nl *who,
                        const struct nlmsghdr *n, void *arg)
{
    struct rtmsg        *r = NLMSG_DATA(n);
    int                  len = n->nlmsg_len;
    char                *p;
    
    rtnl_print_route_cb_user_data_t *user_data = 
                         (rtnl_print_route_cb_user_data_t *)arg;
    
    struct rtattr       *tb[RTA_MAX + 1] = {NULL,};
    int                  family;

    UNUSED(who);

      
    family = r->rtm_family;
    
    if (family != user_data->family)
    {
        return 0;
    }

    if (family != AF_INET && family != AF_INET6)
    {
        return 0;
    }
    
    if (r->rtm_table != RT_TABLE_MAIN || r->rtm_type != RTN_BLACKHOLE)
        return 0;


    len -= NLMSG_LENGTH(sizeof(*r));

    parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);

    p = user_data->buf;
    
    if (tb[RTA_DST] == NULL)
    {
        if (r->rtm_dst_len != 0)
        {
            ERROR("NULL destination with non-zero prefix");
            return 0;
        }
        else
        {
            if (family == AF_INET)
            {
                p += sprintf(p, "0.0.0.0|0");
            }
            else
            {    
                p += sprintf(p, "::|0");
            }
        }
    }
    else
    {
        inet_ntop(family, RTA_DATA(tb[RTA_DST]), p, INET6_ADDRSTRLEN);
        p += strlen(p);
        p += sprintf(p, "|%d", r->rtm_dst_len);        
    }

    p += sprintf(p, " ");
    user_data->buf = p;

    return 0;
}

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_blackhole_list(char **list)
{
    ENTRY();

    buf[0] = '\0';

    struct rtnl_handle               rth;
    rtnl_print_route_cb_user_data_t  user_data;    

    memset(&user_data, 0, sizeof(user_data));

    user_data.buf = buf;

    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open a netlink socket");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    
#define GET_ALL_ROUTES_OF_FAMILY(__family)                      \
do {                                                            \
    if (rtnl_wilddump_request(&rth, __family,                   \
                              RTM_GETROUTE) < 0)                \
    {                                                           \
        rtnl_close(&rth);                                       \
        ERROR("Cannot send dump request to netlink");           \
        return TE_OS_RC(TE_TA_UNIX, errno);                     \
    }                                                           \
                                                                \
    /* Fill in user_data, which will be passed to callback function */  \
    user_data.family = __family;                                \
    if (rtnl_dump_filter(&rth, rtnl_print_blackhole_cb,         \
                         &user_data, NULL, NULL) < 0)           \
    {                                                           \
        rtnl_close(&rth);                                       \
        ERROR("Dump terminated");                               \
        return TE_OS_RC(TE_TA_UNIX, errno);                     \
    }                                                           \
} while (0)

    GET_ALL_ROUTES_OF_FAMILY(AF_INET);
#if 0
    GET_ALL_ROUTES_OF_FAMILY(AF_INET6);
#endif

#undef GET_ALL_ROUTES_OF_FAMILY    
    
    rtnl_close(&rth);

    INFO("%s: Blackholes: %s", __FUNCTION__, buf);
    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_blackhole_add(ta_rt_info_t *rt_info)
{
    struct nl_request  req;
    struct rtnl_handle rth;
    te_errno           rc;

    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;
    req.n.nlmsg_type = RTM_NEWROUTE;

    req.r.rtm_family = SA(&rt_info->dst)->sa_family;
    req.r.rtm_table = RT_TABLE_MAIN;
    req.r.rtm_scope = RT_SCOPE_NOWHERE;
    req.r.rtm_protocol = RTPROT_BOOT;
    req.r.rtm_type = RTN_BLACKHOLE;

    /* Sending the netlink message */
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open the netlink socket");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    if ((rc = rt_info2nl_req(rt_info, &req)) != 0)
    {
        rtnl_close(&rth);
        return rc;
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
    {
        ERROR("Failed to send the netlink message");
        rtnl_close(&rth);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rtnl_close(&rth);
    return 0;
}

/* See the description in conf_route.h */
te_errno 
ta_unix_conf_route_blackhole_del(ta_rt_info_t *rt_info)
{
    struct nl_request  req;
    struct rtnl_handle rth;
    te_errno           rc;
    
    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;
    req.n.nlmsg_type = RTM_DELROUTE;

    req.r.rtm_family = SA(&rt_info->dst)->sa_family;
    req.r.rtm_table = RT_TABLE_MAIN;
    req.r.rtm_scope = RT_SCOPE_NOWHERE;
    req.r.rtm_type = RTN_BLACKHOLE;

    /* Sending the netlink message */
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open the netlink socket");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    if ((rc = rt_info2nl_req(rt_info, &req)) != 0)
    {
        rtnl_close(&rth);
        return rc;
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
    {
        ERROR("Failed to send the netlink message");
        rtnl_close(&rth);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rtnl_close(&rth);
    return 0;
}

#endif /* USE_NETLINK */
