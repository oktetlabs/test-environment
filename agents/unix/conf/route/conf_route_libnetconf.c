/** @file
 * @brief Unix Test Agent
 *
 * Unix TA routing configuring support using libnetconf. That library
 * uses netlink interface to communicate with kernel, so it is
 * available only for linux.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Maxim Alyutov <Maxim.Alyutov@oktetlabs.ru>
 *
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
#include "te_str.h"
#include "cs_common.h"
#include "logger_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "unix_internal.h"
#include "conf_route.h"
#include "ta_common.h"
#include "conf_netconf.h"

#include "netconf.h"


/**
 * The Netlink can create or remove the ip route with table id more than 255,
 * but can't get such route.
 *
 * The table id during reading will be written down in a field @b rtm_table
 * in structure @b rtmsg. This field has type @b unsigned @b char. It can be
 * seen in the file (line @c 186):
 * https://github.com/torvalds/linux/blob/v4.0/include/uapi/linux/rtnetlink.h
 *
 * If the table id exceeds @c 255, then the constant @c RT_TABLE_COMPAT will be
 * returned instead of the id. It can be seen in the file:
 * https://github.com/torvalds/linux/blob/v4.0/net/ipv4/fib_semantics.c#L1007
 */
#define NETLINK_LIMIT_TABLE_ID 0x100

/** Max length of temporary buffer for list functions */
#define BUF_MAXLENGTH (4096)

/** Temporary buffer for list functions */
static char buf[BUF_MAXLENGTH];

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
    te_errno             result = 0;

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

        if ((rt_info->prefix != route->dstlen) ||
            (((rt_info->flags & TA_RT_INFO_FLG_METRIC) != 0) &&
             (rt_info->metric != (uint32_t)route->metric)) ||
            (rt_info->tos != route->tos)||
            (rt_info->table != route->table))
        {
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
            if ((route->family == AF_INET) &&
                (memcmp(route->dst, &(SIN(&(rt_info->dst))->sin_addr),
                        sizeof(struct in_addr)) == 0))
            {
                found = TRUE;
            }
            else if ((route->family == AF_INET6) &&
                     (memcmp(route->dst, &(SIN6(&(rt_info->dst))->sin6_addr),
                             sizeof(struct in6_addr)) == 0))
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

        if (route->hoplimit != 0)
        {
            rt_info->flags |= TA_RT_INFO_FLG_HOPLIMIT;
            rt_info->hoplimit = route->hoplimit;
        }

        if (route->table != NETCONF_RT_TABLE_MAIN)
        {
            rt_info->flags |= TA_RT_INFO_FLG_TABLE;
            rt_info->table = route->table;
        }

        if (!LIST_EMPTY(&route->hops))
        {
            netconf_route_nexthop *nc_nh = NULL;
            ta_rt_nexthop_t       *ta_nh = NULL;
            unsigned int           nh_id = 0;

            TAILQ_INIT(&rt_info->nexthops);
            rt_info->flags |= TA_RT_INFO_FLG_MULTIPATH;

            LIST_FOREACH(nc_nh, &route->hops, links)
            {
                ta_nh = calloc(1, sizeof(*ta_nh));
                if (ta_nh == NULL)
                {
                    ERROR("%s(): out of memory", __FUNCTION__);
                    result = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                    goto cleanup;
                }

                ta_nh->id = nh_id;
                nh_id++;

                ta_nh->weight = nc_nh->weight;

                if (nc_nh->gateway != NULL)
                {
                    ta_nh->gw.ss_family = route->family;
                    if (route->family == AF_INET)
                    {
                        memcpy(&(SIN(&ta_nh->gw)->sin_addr),
                               nc_nh->gateway,
                               sizeof(struct in_addr));
                    }
                    else
                    {
                        memcpy(&(SIN6(&ta_nh->gw)->sin6_addr),
                               nc_nh->gateway,
                               sizeof(struct in6_addr));
                    }

                    ta_nh->flags |= TA_RT_NEXTHOP_FLG_GW;
                }

                if (nc_nh->oifindex != 0)
                {
                    char tmp[IF_NAMESIZE];

                    if (if_indextoname(nc_nh->oifindex, tmp) != NULL)
                    {
                        TE_STRLCPY(ta_nh->ifname, tmp, IF_NAMESIZE);
                        ta_nh->flags |= TA_RT_NEXTHOP_FLG_OIF;
                    }
                    else
                    {
                        ERROR("%s(): cannot convert interface %d index "
                              "to interface name", __FUNCTION__,
                              nc_nh->oifindex);
                        free(ta_nh);
                        result = TE_OS_RC(TE_TA_UNIX, errno);
                        goto cleanup;
                    }
                }

                TAILQ_INSERT_TAIL(&rt_info->nexthops, ta_nh, links);
            }
        }

        /* Find the first one */
        break;
    }

cleanup:

    netconf_list_free(list);

    if (!found)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (result != 0)
        ta_rt_info_clean(rt_info);

    return result;
}

te_errno
ta_unix_conf_route_change(ta_cfg_obj_action_e  action,
                          ta_rt_info_t        *rt_info)
{
    netconf_cmd         cmd;
    netconf_route       route;
    unsigned char       family;
    te_errno            rc = 0;

    netconf_route_nexthop *nc_nh = NULL;
    netconf_route_nexthop *nc_nh_aux = NULL;

    if (rt_info == NULL)
    {
        ERROR("%s(): Invalid value for 'rt_info' argument",
              __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (rt_info->table >= NETLINK_LIMIT_TABLE_ID)
    {
        ERROR("%s(): Invalid value for table id (1 <= %d <= %d)",
              __FUNCTION__, rt_info->table, NETLINK_LIMIT_TABLE_ID - 1);
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

    if ((rt_info->flags & TA_RT_INFO_FLG_HOPLIMIT) != 0)
        route.hoplimit = rt_info->hoplimit;

    if ((rt_info->flags & TA_RT_INFO_FLG_TABLE) != 0)
        route.table = rt_info->table;

    if (rt_info->flags & TA_RT_INFO_FLG_MULTIPATH)
    {
        ta_rt_nexthop_t       *ta_nh = NULL;

        TAILQ_FOREACH(ta_nh, &rt_info->nexthops, links)
        {
            nc_nh = calloc(1, sizeof(*nc_nh));
            if (nc_nh == NULL)
            {
                ERROR("%s(): out of memory", __FUNCTION__);
                rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                goto cleanup;
            }

            if (nc_nh_aux == NULL)
                LIST_INSERT_HEAD(&route.hops, nc_nh, links);
            else
                LIST_INSERT_AFTER(nc_nh_aux, nc_nh, links);

            nc_nh_aux = nc_nh;

            nc_nh->weight = ta_nh->weight;

            if (ta_nh->flags & TA_RT_NEXTHOP_FLG_OIF)
            {
                int idx;

                if ((idx = if_nametoindex(ta_nh->ifname)) == 0)
                {
                    ERROR("%s(): Cannot find interface %s",
                          __FUNCTION__, ta_nh->ifname);
                    rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
                    goto cleanup;
                }

                nc_nh->oifindex = idx;
            }

            if (ta_nh->flags & TA_RT_NEXTHOP_FLG_GW)
            {
                if (family == AF_INET)
                    nc_nh->gateway = (uint8_t *)&(SIN(&ta_nh->gw)->sin_addr);
                else
                    nc_nh->gateway = (uint8_t *)&(SIN6(&ta_nh->gw)->sin6_addr);
            }
        }
    }

    if (netconf_route_modify(nh, cmd, &route) < 0)
    {
        ERROR("%s(): Cannot change route", __FUNCTION__);
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto cleanup;
    }

    /* Flush routing cache on success, but ignore errors */
    (void)route_flush(rt_info->dst.ss_family);

cleanup:

    route.dst = NULL;
    route.src = NULL;
    route.gateway = NULL;
    netconf_route_clean(&route);

    return rc;
}

static te_errno
append_routes(netconf_list *nlist, te_string *const str)
{
    netconf_node *t;

    for (t = nlist->head; t != NULL; t = t->next)
    {
        const netconf_route *route = &(t->data.route);
        const unsigned char  family = route->family;
        char                 ifname[IF_NAMESIZE];

        if (family != AF_INET && family != AF_INET6)
        {
            assert(0);
            continue;
        }

        if (LIST_EMPTY(&route->hops))
        {
            if (route->oifindex == 0)
                continue;

            if ((if_indextoname(route->oifindex, ifname)) == NULL)
                continue;

            if (!ta_interface_is_mine(ifname))
                continue;
        }
        else
        {
            netconf_route_nexthop *nc_nh = NULL;

            LIST_FOREACH(nc_nh, &route->hops, links)
            {
                if (nc_nh->oifindex == 0)
                    break;

                if ((if_indextoname(nc_nh->oifindex, ifname)) == NULL)
                    break;

                if (!ta_interface_is_mine(ifname))
                    break;
            }
            if (nc_nh != NULL)
                continue;
        }

        /*
         * The local routing table is maintained by the kernel and shouldn't
         * be manipulated by Configurator.
         */
        if (route->table == NETCONF_RT_TABLE_LOCAL)
           continue;

        /*
         * On some configurations (ARM64 with Ubuntu-20.04) IPv6 routes with
         * type=local may be in the main routing table and Configurator should
         * not manipulate them (see Bug 11178).
         */
        if (family == AF_INET6 &&
            route->table == NETCONF_RT_TABLE_MAIN &&
            route->type == NETCONF_RTN_LOCAL)
        {
            continue;
        }

        /*
         * If expire time is defined for the route, then drop it.
         * Configurator doesn't have any good way to restore such routes.
         */
        if (route->expires != 0)
            continue;

        /*
         * FIXME: Filter cloned routes to prevent configurator errors.
         * It's a workaround for old kernels with Routing Cache.
         */
        if (route->flags & NETCONF_RTM_F_CLONED)
           continue;

        if (family == AF_INET6)
        {
            /*
             * IPv6 requires a link-local address on every network interface.
             * There is also a corresponding entry in the main routing table.
             * Don't pass link-local routes to prevent Configurator errors.
             * Netlink returns RT_SCOPE_UNIVERSE for such routes, so check
             * prefix with prefix length instead.
             */
            if (route->dst != NULL && route->dstlen == 64)
            {
                struct in6_addr addr;

                memcpy(addr.s6_addr, route->dst, sizeof(addr.s6_addr));
                if (IN6_IS_ADDR_LINKLOCAL(&addr))
                    continue;
            }
        }

        /* Append this route to the list */

        if (str->len != 0)
            CHECK_NZ_RETURN(te_string_append(str, " "));

        if (route->dst == NULL)
        {
            assert(route->dstlen == 0);
            CHECK_NZ_RETURN(te_string_append(str, family == AF_INET ?
                                             "0.0.0.0|0" : "::|0"));
        }
        else
        {
            char addr_buf[INET6_ADDRSTRLEN];

            if (inet_ntop(route->family, route->dst,
                          addr_buf, sizeof(addr_buf)) != NULL)
                CHECK_NZ_RETURN(te_string_append(str, "%s|%d", addr_buf,
                                                 route->dstlen));
        }

        if (route->metric != 0)
            CHECK_NZ_RETURN(te_string_append(str, ",metric=%d", route->metric));
        if (route->tos != 0)
            CHECK_NZ_RETURN(te_string_append(str, ",tos=%d", route->tos));
        if (route->table != NETCONF_RT_TABLE_MAIN)
            CHECK_NZ_RETURN(te_string_append(str, ",table=%d", route->table));
    }

    return 0;
}


static te_errno
retrieve_route_list(sa_family_t family, te_string *const str)
{
    netconf_list *nlist;
    te_errno      rc;

    if ((nlist = netconf_route_dump(nh, family)) == NULL)
    {
        ERROR("%s(): Cannot get list of routes", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rc = append_routes(nlist, str);
    netconf_list_free(nlist);

    return rc;
}

te_errno
ta_unix_conf_route_list(char **list)
{
    te_string str = TE_STRING_INIT;
    te_errno  rc;

    if (list == NULL)
    {
        ERROR("%s(): Invalid value for 'list' argument", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* Get IPv4 routes */
    if ((rc = retrieve_route_list(AF_INET, &str)) != 0)
    {
        te_string_free(&str);
        return rc;
    }

    /* Get IPv6 routes */
    if ((rc = retrieve_route_list(AF_INET6, &str)) != 0)
    {
        te_string_free(&str);
        return rc;
    }

    *list = str.ptr;

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
