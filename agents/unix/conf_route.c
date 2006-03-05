/** @file
 * @brief Unix Test Agent
 *
 * Unix TA routing configuring support
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
 * $Id$
 */

#define TE_LGR_USER     "Unix Conf Route"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
//#include <ctype.h>
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
//#if HAVE_UNISTD_H
//#include <unistd.h>
//#endif
//#if HAVE_FCNTL_H
//#include <fcntl.h>
//#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
//#if HAVE_SYS_IOCTL_H
//#define BSD_COMP
//#include <sys/ioctl.h>
//#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
//#if HAVE_NET_IF_DL_H
//#include <net/if_dl.h>
//#endif
//#if HAVE_NET_ROUTE_H
//#include <net/route.h>
//#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "cs_common.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "unix_internal.h"
#include "conf_route.h"


#ifndef IF_NAMESIZE
#define IF_NAMESIZE IFNAMSIZ
#endif


/*
 * Forward declaration of routing configuration tree accessors.
 */

static te_errno ip4_rt_default_if_get(unsigned int, const char *, char *);


static te_errno route_mtu_get(unsigned int, const char *, char *,
                              const char *);
static te_errno route_mtu_set(unsigned int, const char *, const char *,
                              const char *);

static te_errno route_win_get(unsigned int, const char *, char *,
                              const char *);
static te_errno route_win_set(unsigned int, const char *, const char *,
                              const char *);

static te_errno route_irtt_get(unsigned int, const char *, char *,
                               const char *);
static te_errno route_irtt_set(unsigned int, const char *, const char *,
                               const char *);

static te_errno route_dev_get(unsigned int, const char *, char *,
                              const char *);
static te_errno route_dev_set(unsigned int, const char *, const char *,
                              const char *);

/* FIXME: Route types are disabled until Configurator is redesigned - A.A */
#if 0
static te_errno route_type_get(unsigned int, const char *, char *,
                              const char *);
static te_errno route_type_set(unsigned int, const char *, const char *,
                              const char *);
#endif

static te_errno route_get(unsigned int, const char *, char *, const char *);
static te_errno route_set(unsigned int, const char *, const char *,
                          const char *);

static te_errno route_add(unsigned int, const char *, const char *,
                          const char *);
static te_errno route_del(unsigned int, const char *,
                          const char *);

static te_errno route_list(unsigned int, const char *, char **);

static te_errno route_commit(unsigned int gid, const cfg_oid *p_oid);

static te_errno blackhole_list(unsigned int, const char *, char **);
static te_errno blackhole_add(unsigned int, const char *, const char *,
                              const char *);
static te_errno blackhole_del(unsigned int, const char *, const char *);


/*
 * Unix Test Agent routing configuration tree.
 */

RCF_PCH_CFG_NODE_RO(node_rt_default_if, "ip4_rt_default_if",
                    NULL, NULL, ip4_rt_default_if_get);

RCF_PCH_CFG_NODE_COLLECTION(node_blackhole, "blackhole",
                            NULL, &node_rt_default_if,
                            blackhole_add, blackhole_del,
                            blackhole_list, NULL);

static rcf_pch_cfg_object node_route;

/* FIXME: Route types are disabled until Configurator is redesigned - A.A */
#if 0
RCF_PCH_CFG_NODE_RWC(node_route_type, "type", NULL, NULL,
                     route_type_get, route_type_set, &node_route);
#endif

RCF_PCH_CFG_NODE_RWC(node_route_irtt, "irtt", NULL, NULL,
                     route_irtt_get, route_irtt_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_win, "win", NULL, &node_route_irtt,
                     route_win_get, route_win_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_mtu, "mtu", NULL, &node_route_win,
                     route_mtu_get, route_mtu_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_dev, "dev", NULL, &node_route_mtu,
                     route_dev_get, route_dev_set, &node_route);

static rcf_pch_cfg_object node_route =
    {"route", 0, &node_route_dev, &node_blackhole,
     (rcf_ch_cfg_get)route_get, (rcf_ch_cfg_set)route_set,
     (rcf_ch_cfg_add)route_add, (rcf_ch_cfg_del)route_del,
     (rcf_ch_cfg_list)route_list, (rcf_ch_cfg_commit)route_commit, NULL};


/*
 * Forward declaration of helper functions.
 */
static te_errno route_find(const char *, ta_rt_info_t *);


/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_init(void)
{
    return rcf_pch_add_node("/agent", &node_route);
}


/**
 * Obtain ifname from IPv4 default route record.
 *
 * @param gid          group identifier (unused)
 * @param oid          full object instence identifier (unused)
 * @param ifname       default route ifname
 *
 * @return             Status code
 */
static te_errno
ip4_rt_default_if_get(unsigned int gid, const char *oid, char *ifname)
{

    ta_rt_info_t  attr;
    te_errno      rc;
    char         *route_name = "0.0.0.0|0";
    char          value[INET_ADDRSTRLEN];
    const char   *ret_val = NULL;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = route_find(route_name, &attr)) != 0)
    {
        ERROR("Route %s cannot be found", route_name);
        return rc;
    }

    switch (attr.dst.ss_family)
    {
        case AF_INET:
        {
            ret_val = inet_ntop(AF_INET, &SIN(&attr.gw)->sin_addr,
                                value, INET_ADDRSTRLEN);
            if (ret_val == NULL)
            {
                ERROR("Convertaion failure of the address of family: %d",
                      attr.dst.ss_family);
                return TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT);
            }

            strncpy(ifname, attr.ifname, IF_NAMESIZE);
            break;
        }

        case AF_INET6:
        {
            ret_val = inet_ntop(AF_INET6, &SIN6(&attr.gw)->sin6_addr,
                                value, INET6_ADDRSTRLEN);
            if (ret_val == NULL)
            {
                ERROR("Convertaion failure of the address of family: %d",
                      attr.dst.ss_family);
                return TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT);
            }

            RING("Default route for AF_INET6 is found");
            return TE_OS_RC(TE_TA_UNIX, TE_ENOENT);
            break;
        }

        default:
        {
            ERROR("Unexpected destination address family: %d",
                  attr.dst.ss_family);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }

    return 0;
}


/**
 * Find route and return its attributes.
 *
 * @param route    route instance name: see doc/cm_cm_base.xml
 *                 for the format
 * @param rt_info  route related information (OUT)
 *
 * @return         Status code
 */
static te_errno
route_find(const char *route, ta_rt_info_t *rt_info)
{
    te_errno    rc;

    ENTRY("%s", route);

    if ((rc = ta_rt_parse_inst_name(route, rt_info)) != 0)
    {
        ERROR("Error parsing instance name: %s", route);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    return ta_unix_conf_route_find(rt_info);
}


/**
 * Get route value.
 *
 * @param  gid          Group identifier (unused)
 * @param  oid          Object identifier (unused)
 * @param  value        Place for route value (gateway address
 *                      or zero if it is a direct route)
 * @param route_name    Name of the route
 *
 * @return              Status code
 */
static te_errno
route_get(unsigned int gid, const char *oid,
          char *value, const char *route_name)
{
    ta_rt_info_t  attr;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = route_find(route_name, &attr)) != 0)
    {
        ERROR("Route %s cannot be found", route_name);
        return rc;
    }

    switch (attr.dst.ss_family)
    {
        case AF_INET:
        {
            inet_ntop(AF_INET, &SIN(&attr.gw)->sin_addr,
                      value, INET_ADDRSTRLEN);
            break;
        }

        case AF_INET6:
        {
            inet_ntop(AF_INET6, &SIN6(&attr.gw)->sin6_addr,
                      value, INET6_ADDRSTRLEN);
            break;
        }

        default:
        {
            ERROR("Unexpected destination address family: %d",
                  attr.dst.ss_family);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }

    return 0;
}

/**
 * Set route value.
 *
 * @param       gid        Group identifier (unused)
 * @param       oid        Object identifier
 * @param       value      New value for the route
 * @param       route_name Name of the route
 *
 * @return      Status code.
 */
static te_errno
route_set(unsigned int gid, const char *oid, const char *value,
          const char *route_name)
{   
    UNUSED(gid);
    UNUSED(oid);

    return ta_obj_value_set(TA_OBJ_TYPE_ROUTE, route_name, value);
}

/**
 * Load all route-specific attributes into route object.
 *
 * @param obj  Object to be uploaded
 *
 * @return 0 in case of success, or error code on failure.
 */
static te_errno
route_load_attrs(ta_cfg_obj_t *obj)
{
    ta_rt_info_t rt_info;
    te_errno     rc;
    char         val[128];

    if ((rc = route_find(obj->name, &rt_info)) != 0)
        return rc;

#define ROUTE_LOAD_ATTR(field_flg_, field_) \
    do {                                                      \
        snprintf(val, sizeof(val), "%d", rt_info.field_);     \
        if (rt_info.flags & TA_RT_INFO_FLG_ ## field_flg_ &&  \
            (rc = ta_obj_set(TA_OBJ_TYPE_ROUTE,               \
                             obj->name, #field_,              \
                             val, NULL)) != 0)                \
        {                                                     \
            return rc;                                        \
        }                                                     \
    } while (0)

    ROUTE_LOAD_ATTR(MTU, mtu);
    ROUTE_LOAD_ATTR(WIN, win);
    ROUTE_LOAD_ATTR(IRTT, irtt);

    snprintf(val, sizeof(val), "%s", rt_info.ifname);
    if (rt_info.flags & TA_RT_INFO_FLG_IF &&
        (rc = ta_obj_set(TA_OBJ_TYPE_ROUTE,
                        obj->name, "dev",
                         val, NULL)) != 0)
    {
        ERROR("Invalid interface");
        return rc;
    }

    if ((rc = ta_obj_set(TA_OBJ_TYPE_ROUTE,
                         obj->name, "type",
                         ta_rt_type2name(rt_info.type), 
                         NULL)) != 0)
    {
        ERROR("Invalid route type");
        return rc;
    }

    if (rt_info.gw.ss_family == AF_INET)
    {
        inet_ntop(AF_INET, &(SIN(&rt_info.gw)->sin_addr),
                  val, INET_ADDRSTRLEN);
    }
    else if (rt_info.gw.ss_family == AF_INET6)
    {
        inet_ntop(AF_INET6, &(SIN6(&rt_info.gw)->sin6_addr),
                  val, INET6_ADDRSTRLEN);
    }    

#undef ROUTE_LOAD_ATTR

    return 0;
}

#define DEF_ROUTE_GET_FUNC(field_) \
static te_errno                                             \
route_ ## field_ ## _get(unsigned int gid, const char *oid, \
                         char *value, const char *route)    \
{                                                           \
    te_errno     rc;                                        \
    ta_rt_info_t rt_info;                                   \
                                                            \
    UNUSED(gid);                                            \
    UNUSED(oid);                                            \
                                                            \
    if ((rc = route_find(route, &rt_info)) != 0)             \
        return rc;                                          \
                                                            \
    sprintf(value, "%d", rt_info.field_);                   \
    return 0;                                               \
}

#define DEF_ROUTE_SET_FUNC(field_) \
static te_errno                                                \
route_ ## field_ ## _set(unsigned int gid, const char *oid,    \
                         const char *value, const char *route) \
{                                                              \
    UNUSED(gid);                                               \
    UNUSED(oid);                                               \
                                                               \
    return ta_obj_set(TA_OBJ_TYPE_ROUTE, route, #field_,       \
                      value, route_load_attrs);                \
}

DEF_ROUTE_GET_FUNC(mtu);
DEF_ROUTE_SET_FUNC(mtu);
DEF_ROUTE_GET_FUNC(win);
DEF_ROUTE_SET_FUNC(win);
DEF_ROUTE_GET_FUNC(irtt);
DEF_ROUTE_SET_FUNC(irtt);
DEF_ROUTE_SET_FUNC(dev);
/* FIXME: Route types are disabled until Configurator is redesigned - A.A */
#if 0
DEF_ROUTE_SET_FUNC(type);
#endif

static te_errno
route_dev_get(unsigned int gid, const char *oid,
              char *value, const char *route) 
{
    te_errno     rc;
    ta_rt_info_t rt_info;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = route_find(route, &rt_info)) != 0)
        return rc;

    sprintf(value, "%s", rt_info.ifname);
    return 0;
}

/* FIXME: Route types are disabled until Configurator is redesigned - A.A */
#if 0
static te_errno
route_type_get(unsigned int gid, const char *oid,
              char *value, const char *route) 
{
    te_errno     rc;
    ta_rt_info_t rt_info;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = route_find(route, &rt_info)) != 0)
        return rc;

    if (rt_info.type >= TA_RT_TYPE_MAX_VALUE)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(value, "%s", ta_rt_type2name(rt_info.type));
    return 0;
}
#endif

#undef DEF_ROUTE_GET_FUNC
#undef DEF_ROUTE_SET_FUNC


/**
 * Add a new route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         value string
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return              Status code
 */
static te_errno
route_add(unsigned int gid, const char *oid, const char *value,
          const char *route)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    return ta_obj_add(TA_OBJ_TYPE_ROUTE, route, value, NULL, NULL);
}

/**
 * Delete a route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return              Status code
 */
static te_errno
route_del(unsigned int gid, const char *oid, const char *route)
{
    UNUSED(gid);
    UNUSED(oid);

    return ta_obj_del(TA_OBJ_TYPE_ROUTE, route, NULL);
}


static te_errno
route_list(unsigned int gid, const char *oid, char **list)
{
    UNUSED(gid);
    UNUSED(oid);

    return ta_unix_conf_route_list(list);
}


static te_errno
route_commit(unsigned int gid, const cfg_oid *p_oid)
{
    const char             *route;
    ta_cfg_obj_t           *obj;
    ta_rt_info_t            rt_info;
    te_errno                rc;
    ta_cfg_obj_action_e     obj_action;
    

    UNUSED(gid);
    ENTRY("%s", route);

    route = ((cfg_inst_subid *)(p_oid->ids))[p_oid->len - 1].name;
    
    if ((obj = ta_obj_find(TA_OBJ_TYPE_ROUTE, route)) == NULL)
    {
        WARN("Commit for %s route which has not been updated", route);
        return 0;
    }

    memset(&rt_info, 0, sizeof(rt_info));

    if ((rc = ta_rt_parse_obj(obj, &rt_info)) != 0)
    {
        ta_obj_free(obj);
        return rc;
    }

    obj_action = obj->action;

    ta_obj_free(obj);

    return ta_unix_conf_route_change(obj_action, &rt_info);
}


static te_errno
blackhole_list(unsigned int gid, const char *oid, char **list)
{
    UNUSED(gid);
    UNUSED(oid);

    return ta_unix_conf_route_blackhole_list(list);
}

static te_errno
blackhole_add(unsigned int gid, const char *oid, const char *value,
              const char *route)
{
    te_errno        rc;
    ta_rt_info_t    rt_info;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    
    rc = ta_rt_parse_inst_name(route, &rt_info);
    if (rc != 0)
        return rc;

    return ta_unix_conf_route_blackhole_add(&rt_info);
}

static te_errno 
blackhole_del(unsigned int gid, const char *oid, const char *route)
{
    te_errno        rc;
    ta_rt_info_t    rt_info;

    UNUSED(gid);
    UNUSED(oid);
    
    rc = ta_rt_parse_inst_name(route, &rt_info);
    if (rc != 0)
        return rc;

    return ta_unix_conf_route_blackhole_add(&rt_info);
}
