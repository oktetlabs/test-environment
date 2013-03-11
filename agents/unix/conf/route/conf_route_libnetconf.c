/** @file
 * @brief Unix Test Agent
 *
 * Unix TA routing configuring support using libnetconf. That library
 * uses netlink interface to communicate with kernel, so it is
 * available only for linux.
 *
 * Copyright (C) 2008 Test Environment authors (see file AUTHORS
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
 * @author Maxim Alyutov <Maxim.Alyutov@oktetlabs.ru>
 *
 * $Id: $
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(USE_LIBNETCONF)

#if !defined(__linux__)
/* libnetconf is only available for linux now */
#error Netlink can be used on Linux only
#endif

#define TE_LGR_USER     "Unix Conf Route NetLink"

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
#include "ta_common.h"

#include <netconf.h>

extern netconf_handle nh;

/** Temponary buffer for list functions */
static char buf[4096];

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

te_errno
ta_unix_conf_route_find(ta_rt_info_t *rt_info)
{
    netconf_list        *list;
    netconf_node        *t;
    te_errno             found;

    if (rt_info == NULL)
    {
        ERROR("%s(): Invalid value for 'rt_info' argument", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((list = netconf_route_dump(nh, rt_info->dst.ss_family)) == NULL)
    {
        ERROR("%s(): Cannot get list of addresses", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    found = FALSE;
    for (t = list->head; t != NULL; t = t->next)
    {
        const netconf_route    *route = &(t->data.route);
        const struct in6_addr   addr_any = IN6ADDR_ANY_INIT;

        if ((route->family != AF_INET) && (route->family != AF_INET6))
        {
            assert(0);
            continue;
        }

        /*
         * Check the case of INADDR_ANY address or that destination
         * address is equal to requested one.
         */

        if (route->dst == NULL)
        {
            if ((route->family == AF_INET) &&
                (SIN(&(rt_info->dst))->sin_addr.s_addr == INADDR_ANY))
            {
                found = TRUE;
            }
            else if ((route->family == AF_INET6) &&
                     (memcmp(&SIN6(&(rt_info->dst))->sin6_addr,
                             &addr_any, sizeof(struct in6_addr)) == 0))
            {
                found = TRUE;
            }
        }
        else
        {
            if ((rt_info->prefix != route->dstlen) ||
                (((rt_info->metric & TA_RT_INFO_FLG_METRIC) != 0) &&
                 (rt_info->metric != (uint32_t)route->metric)) ||
                (rt_info->tos != route->tos))
            {
                continue;
            }

            if ((route->family == AF_INET) &&
                (memcmp(route->dst, &(SIN(&(rt_info->dst))->sin_addr),
                        sizeof(struct in_addr)) == 0))
            {
                found = TRUE;
            }
            else if ((route->family == AF_INET6) &&
                     (memcmp(&SIN6(&(rt_info->dst))->sin6_addr,
                             &addr_any, sizeof(struct in6_addr)) == 0))
            {
                found = TRUE;
            }
        }

        if (!found)
            continue;

        /* Route is found, filling fields */

        rt_info->type = route->type;

        if (route->oifindex != 0)
        {
            char tmp[IF_NAMESIZE];

            if (if_indextoname(route->oifindex, tmp) != NULL)
            {
                rt_info->flags |= TA_RT_INFO_FLG_IF;
                strcpy(rt_info->ifname, tmp);
            }
        }

        if (route->src != NULL)
        {
            rt_info->flags |= TA_RT_INFO_FLG_SRC;
            rt_info->src.ss_family = route->family;

            if (route->family == AF_INET)
            {
                memcpy(&(SIN(&rt_info->src)->sin_addr), route->src,
                       sizeof(struct in_addr));
            }
            else
            {
                memcpy(&(SIN6(&rt_info->src)->sin6_addr), route->src,
                       sizeof(struct in6_addr));
            }
        }

        if (route->gateway != NULL)
        {
            rt_info->flags |= TA_RT_INFO_FLG_GW;
            rt_info->gw.ss_family = route->family;

            if (route->family == AF_INET)
            {
                memcpy(&(SIN(&rt_info->gw)->sin_addr), route->gateway,
                       sizeof(struct in_addr));
            }
            else
            {
                memcpy(&(SIN6(&rt_info->gw)->sin6_addr), route->gateway,
                       sizeof(struct in6_addr));
            }
        }

        if (route->metric != 0)
        {
            rt_info->flags |= TA_RT_INFO_FLG_METRIC;
            rt_info->metric = route->metric;
        }

        if (route->mtu != 0)
        {
            rt_info->flags |= TA_RT_INFO_FLG_MTU;
            rt_info->mtu = route->mtu;
        }

        if (route->win != 0)
        {
            rt_info->flags |= TA_RT_INFO_FLG_WIN;
            rt_info->win = route->win;
        }

        if (route->irtt != 0)
        {
            rt_info->flags |= TA_RT_INFO_FLG_IRTT;
            rt_info->irtt = route->irtt;
        }

        /* Find the first one */
        break;
    }

    netconf_list_free(list);

    if (!found)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return 0;
}

te_errno
ta_unix_conf_route_change(ta_cfg_obj_action_e  action,
                          ta_rt_info_t        *rt_info)
{
    netconf_cmd         cmd;
    netconf_route       route;
    unsigned char       family;

    if (rt_info == NULL)
    {
        ERROR("%s(): Invalid value for 'rt_info' argument",
              __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    switch (action)
    {
        case TA_CFG_OBJ_CREATE:
            cmd = NETCONF_CMD_ADD;
            break;

        case TA_CFG_OBJ_DELETE:
            cmd = NETCONF_CMD_DEL;
            break;

        case TA_CFG_OBJ_SET:
            cmd = NETCONF_CMD_CHANGE;
            break;

        default:
            ERROR("%s(): Unknown object action specified %d",
                  __FUNCTION__, action);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    family = SA(&rt_info->dst)->sa_family;
    if ((family != AF_INET) && (family != AF_INET6))
    {
        ERROR("%s(): Unsupported destinaton address specified",
              __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT);
    }

    netconf_route_init(&route);

    route.family = family;
    route.dstlen = rt_info->prefix;

    if ((rt_info->flags & TA_RT_INFO_FLG_TOS) != 0)
        route.tos = rt_info->tos;

    if (cmd != NETCONF_CMD_DEL)
        route.type = rt_info->type;

#if 0
    /* This is difference from old version */
    if ((route.type == NETCONF_RTN_BLACKHOLE) ||
        (route.type == NETCONF_RTN_UNREACHABLE) ||
        (route.type == NETCONF_RTN_PROHIBIT))
    {
        route.scope = NETCONF_RT_SCOPE_NOWHERE;
    }
    else if (route.type == NETCONF_RTN_THROW)
    {
        route.scope = NETCONF_RT_SCOPE_LINK;
    }
#endif

    if ((rt_info->flags & TA_RT_INFO_FLG_IF) != 0)
    {
        int idx;

        if ((idx = if_nametoindex(rt_info->ifname)) == 0)
        {
            ERROR("%s(): Cannot find interface %s",
                  __FUNCTION__, rt_info->ifname);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        route.oifindex = idx;
    }

    if (family == AF_INET)
        route.dst = (uint8_t *)&(SIN(&rt_info->dst)->sin_addr);
    else
        route.dst = (uint8_t *)&(SIN6(&rt_info->dst)->sin6_addr);

    if ((rt_info->flags & TA_RT_INFO_FLG_SRC) != 0)
    {
        route.srclen = 0;
        if (family == AF_INET)
            route.src = (uint8_t *)&(SIN(&rt_info->src)->sin_addr);
        else
            route.src = (uint8_t *)&(SIN6(&rt_info->src)->sin6_addr);
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_GW) != 0)
    {
        if (family == AF_INET)
            route.gateway = (uint8_t *)&(SIN(&rt_info->gw)->sin_addr);
        else
            route.gateway = (uint8_t *)&(SIN6(&rt_info->gw)->sin6_addr);
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_METRIC) != 0)
        route.metric = rt_info->metric;

    if ((rt_info->flags & TA_RT_INFO_FLG_MTU) != 0)
        route.mtu = rt_info->mtu;

    if ((rt_info->flags & TA_RT_INFO_FLG_WIN) != 0)
        route.win = rt_info->win;

    if ((rt_info->flags & TA_RT_INFO_FLG_IRTT) != 0)
        route.irtt = rt_info->irtt;

    if (netconf_route_modify(nh, cmd, &route) < 0)
    {
        ERROR("%s(): Cannot change route", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    /* Flush routing cache on success, but ignore errors */
    (void)route_flush(rt_info->dst.ss_family);

    return 0;
}

te_errno
ta_unix_conf_route_list(char **list)
{
    netconf_list       *nlist;
    netconf_node       *t;
    char               *cur_ptr;

    if (list == NULL)
    {
        ERROR("%s(): Invalid value for 'list' argument", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* Get IPv4 routes */
    if ((nlist = netconf_route_dump(nh, AF_INET)) == NULL)
    {
        ERROR("%s(): Cannot get list of routes", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    buf[0] = '\0';
    cur_ptr = buf;
    for (t = nlist->head; t != NULL; t = t->next)
    {
        const netconf_route *route = &(t->data.route);
        char                 ifname[IF_NAMESIZE];

        if (route->family != AF_INET)
        {
            assert(0);
            continue;
        }

        if (route->table != NETCONF_RT_TABLE_MAIN)
            continue;

        if (route->oifindex == 0)
            continue;

        if ((if_indextoname(route->oifindex, ifname)) == NULL)
            continue;

        if (!ta_interface_is_mine(ifname))
            continue;

        /* Append this route to the list */

        if (cur_ptr != buf)
            cur_ptr += sprintf(cur_ptr, " ");

        if (route->dst == NULL)
        {
            assert(route->dstlen == 0);
            cur_ptr += sprintf(cur_ptr, "0.0.0.0|0");
        }
        else
        {
            if (inet_ntop(route->family, route->dst,
                          cur_ptr, INET_ADDRSTRLEN) != NULL)
            {
                cur_ptr += strlen(cur_ptr);
                cur_ptr += sprintf(cur_ptr, "|%d", route->dstlen);
            }
        }

        if (route->metric != 0)
            cur_ptr += sprintf(cur_ptr, ",metric=%d", route->metric);

        if (route->tos != 0)
            cur_ptr += sprintf(cur_ptr, ",tos=%d", route->tos);
    }

    netconf_list_free(nlist);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

te_errno
ta_unix_conf_route_blackhole_list(char **list)
{
    netconf_list       *nlist;
    netconf_node       *t;
    char               *cur_ptr;

    if (list == NULL)
    {
        ERROR("%s(): Invalid value for 'list' argument", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* Get IPv4 routes */
    if ((nlist = netconf_route_dump(nh, AF_INET)) == NULL)
    {
        ERROR("%s(): Cannot get list of routes", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    buf[0] = '\0';
    cur_ptr = buf;
    for (t = nlist->head; t != NULL; t = t->next)
    {
        const netconf_route *route = &(t->data.route);

        if (route->family != AF_INET)
        {
            assert(0);
            continue;
        }

        if (route->table != NETCONF_RT_TABLE_MAIN)
            continue;

        if (route->type != NETCONF_RTN_BLACKHOLE)
            continue;

        /* Append this route to the list */

        if (cur_ptr != buf)
            cur_ptr += sprintf(cur_ptr, " ");

        if (route->dst == NULL)
        {
            assert(route->dstlen == 0);
            cur_ptr += sprintf(cur_ptr, "0.0.0.0|0");
        }
        else
        {
            if (inet_ntop(route->family, route->dst,
                          cur_ptr, INET_ADDRSTRLEN) != NULL)
            {
                cur_ptr += strlen(cur_ptr);
                cur_ptr += sprintf(cur_ptr, "|%d", route->dstlen);
            }
        }
    }

    netconf_list_free(nlist);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

te_errno
ta_unix_conf_route_blackhole_add(ta_rt_info_t *rt_info)
{
    rt_info->type = TA_RT_TYPE_BLACKHOLE;
    return ta_unix_conf_route_change(TA_CFG_OBJ_CREATE, rt_info);
}

te_errno
ta_unix_conf_route_blackhole_del(ta_rt_info_t *rt_info)
{
    rt_info->type = TA_RT_TYPE_BLACKHOLE;
    return ta_unix_conf_route_change(TA_CFG_OBJ_DELETE, rt_info);
}

#endif /* USE_LIBNETCONF */
