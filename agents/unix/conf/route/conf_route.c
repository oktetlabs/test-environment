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
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_sockaddr.h"
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


/**
 * Find route and return its attributes.
 *
 * @param gid       Operation group ID
 * @param route     Route instance name: see doc/cm_cm_base.xml
 *                  for the format
 * @param rt_info   Route related information (OUT)
 *
 * @return Status code.
 *
 * @note The function uses static variables for caching, so it is
 *       not multithread safe.
 */
static te_errno
route_find(unsigned int gid, const char *route, ta_rt_info_t **rt_info)
{
    static unsigned int  rt_cache_gid = (unsigned int)-1;
    static char         *rt_cache_name = NULL;
    static ta_rt_info_t  rt_cache_info;
    
    te_errno    rc;

    ENTRY("GID=%u route=%s", gid, route);

    if ((gid == rt_cache_gid) && (rt_cache_name != NULL) &&
        (strcmp(route, rt_cache_name) == 0))
    {
        *rt_info = &rt_cache_info;
        return 0;
    }

    /* Make cache invalid */
    free(rt_cache_name);
    rt_cache_name = NULL;

    if ((rc = ta_rt_parse_inst_name(route, &rt_cache_info)) != 0)
    {
        ERROR("Error parsing instance name: %s", route);
        return TE_RC(TE_TA_UNIX, rc);
    }
    if ((rc = ta_unix_conf_route_find(&rt_cache_info)) != 0)
    {
        return TE_RC(TE_TA_UNIX, rc);
    }

    rt_cache_gid = gid;
    rt_cache_name = strdup(route);
    if (rt_cache_name == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    *rt_info = &rt_cache_info;

    return 0;
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

    ta_rt_info_t *attr;
    te_errno      rc;
    char         *route_name = "0.0.0.0|0";
    char          value[INET_ADDRSTRLEN];
    const char   *ret_val = NULL;

    UNUSED(oid);

    if ((rc = route_find(gid, route_name, &attr)) != 0)
    {
        ERROR("Route %s cannot be found", route_name);
        return rc;
    }

    switch (attr->dst.ss_family)
    {
        case AF_INET:
        {
            ret_val = inet_ntop(AF_INET, &SIN(&attr->gw)->sin_addr,
                                value, INET_ADDRSTRLEN);
            if (ret_val == NULL)
            {
                ERROR("Convertaion failure of the address of family: %d",
                      attr->dst.ss_family);
                return TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT);
            }

            strncpy(ifname, attr->ifname, IF_NAMESIZE);
            break;
        }

        case AF_INET6:
        {
            ret_val = inet_ntop(AF_INET6, &SIN6(&attr->gw)->sin6_addr,
                                value, INET6_ADDRSTRLEN);
            if (ret_val == NULL)
            {
                ERROR("Convertaion failure of the address of family: %d",
                      attr->dst.ss_family);
                return TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT);
            }

            RING("Default route for AF_INET6 is found");
            return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
            break;
        }

        default:
        {
            ERROR("Unexpected destination address family: %d",
                  attr->dst.ss_family);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }

    return 0;
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
    ta_rt_info_t *attr;
    te_errno      rc;

    UNUSED(oid);
    
    if ((rc = route_find(gid, route_name, &attr)) != 0)
    {
        ERROR("Route %s cannot be found", route_name);
        return rc;
    }

    switch (attr->dst.ss_family)
    {
        case AF_INET:
        {
            inet_ntop(AF_INET, &SIN(&attr->gw)->sin_addr,
                      value, INET_ADDRSTRLEN);
            break;
        }

        case AF_INET6:
        {
            inet_ntop(AF_INET6, &SIN6(&attr->gw)->sin6_addr,
                      value, INET6_ADDRSTRLEN);
            break;
        }

        default:
        {
            ERROR("Unexpected destination address family: %d",
                  attr->dst.ss_family);
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

    return ta_obj_value_set(TA_OBJ_TYPE_ROUTE, route_name, value, gid);
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
    ta_rt_info_t   *rt_info;
    te_errno        rc;
    char            val[128];

    if ((rc = route_find(obj->gid, obj->name, &rt_info)) != 0)
        return rc;

#define ROUTE_LOAD_ATTR(field_flg_, field_) \
    do {                                                      \
        snprintf(val, sizeof(val), "%d", rt_info->field_);    \
        if (rt_info->flags & TA_RT_INFO_FLG_ ## field_flg_ && \
            (rc = ta_obj_attr_set(obj, #field_, val) != 0))   \
        {                                                     \
            return rc;                                        \
        }                                                     \
    } while (0)

    ROUTE_LOAD_ATTR(MTU, mtu);
    ROUTE_LOAD_ATTR(WIN, win);
    ROUTE_LOAD_ATTR(IRTT, irtt);

    switch (rt_info->dst.ss_family)
    {
        case AF_INET:
            inet_ntop(AF_INET, &SIN(&rt_info->src)->sin_addr,
                      val, sizeof(val));
            break;
        case AF_INET6:
            inet_ntop(AF_INET6, &SIN6(&rt_info->src)->sin6_addr,
                      val, sizeof(val));
            break;
        default:
            return TE_EAFNOSUPPORT;
    }
    
    if (rt_info->flags & TA_RT_INFO_FLG_SRC &&
            (rc = ta_obj_attr_set(obj, "src", val) != 0))
    {
        ERROR("Invalid source address");
        return rc;
    }

    snprintf(val, sizeof(val), "%s", rt_info->ifname);
    if (rt_info->flags & TA_RT_INFO_FLG_IF &&
        (rc = ta_obj_attr_set(obj, "dev", val)) != 0)
    {
        ERROR("Invalid interface");
        return rc;
    }

    if ((rc = ta_obj_attr_set(obj, "type",
                              ta_rt_type2name(rt_info->type))) != 0)
    {
        ERROR("Invalid route type");
        return rc;
    }

    if (rt_info->flags & TA_RT_INFO_FLG_GW)
    {
        if (inet_ntop(rt_info->gw.ss_family,
                      te_sockaddr_get_netaddr(SA(&rt_info->gw)),
                      val, sizeof(val)) == NULL)
        {
            ERROR("Invalid gateway address");
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        //~ rc = ta_obj_value_set(TA_OBJ_TYPE_ROUTE, obj->name, val,
                              //~ obj->gid);
        if (rc != 0)
        {
            ERROR("Failed to set route object value: %r", rc);
            return rc;
        }
    }


#undef ROUTE_LOAD_ATTR

    return 0;
}

#define DEF_ROUTE_GET_FUNC(field_) \
static te_errno                                             \
route_ ## field_ ## _get(unsigned int gid, const char *oid, \
                         char *value, const char *route)    \
{                                                           \
    te_errno        rc;                                     \
    ta_rt_info_t   *rt_info;                                \
                                                            \
    UNUSED(oid);                                            \
                                                            \
    if ((rc = route_find(gid, route, &rt_info)) != 0)       \
        return rc;                                          \
                                                            \
    sprintf(value, "%d", rt_info->field_);                  \
    return 0;                                               \
}

#define DEF_ROUTE_SET_FUNC(field_) \
static te_errno                                                \
route_ ## field_ ## _set(unsigned int gid, const char *oid,    \
                         const char *value, const char *route) \
{                                                              \
    UNUSED(oid);                                               \
                                                               \
    return ta_obj_set(TA_OBJ_TYPE_ROUTE, route, #field_,       \
                      value, gid, route_load_attrs);           \
}

DEF_ROUTE_GET_FUNC(mtu);
DEF_ROUTE_SET_FUNC(mtu);
DEF_ROUTE_GET_FUNC(win);
DEF_ROUTE_SET_FUNC(win);
DEF_ROUTE_GET_FUNC(irtt);
DEF_ROUTE_SET_FUNC(irtt);

static te_errno
route_src_get(unsigned int gid, const char *oid,
              char *value, const char *route)
{
    te_errno        rc;
    ta_rt_info_t   *rt_info;
    
    UNUSED(oid);
    
    if ((rc = route_find(gid, route, &rt_info)) != 0)
        return rc;
   
    /*
     * Switch by destination address family in order to process
     * zero (non-specified) source address correctly.
     */
    switch (rt_info->dst.ss_family)
    {
        case AF_INET:
            inet_ntop(AF_INET, &SIN(&rt_info->src)->sin_addr,
                      value, INET_ADDRSTRLEN);
            break;
        case AF_INET6:
            inet_ntop(AF_INET6, &SIN6(&rt_info->src)->sin6_addr,
                      value, INET6_ADDRSTRLEN);
            break;
        default:
            return TE_EAFNOSUPPORT;
    }
    return 0;
}

DEF_ROUTE_SET_FUNC(src);
DEF_ROUTE_SET_FUNC(dev);
/* FIXME: Route types are disabled until Configurator is redesigned - A.A */
#if 0
DEF_ROUTE_SET_FUNC(type);
#endif

static te_errno
route_dev_get(unsigned int gid, const char *oid,
              char *value, const char *route) 
{
    te_errno        rc;
    ta_rt_info_t   *rt_info;

    UNUSED(oid);
    
    if ((rc = route_find(gid, route, &rt_info)) != 0)
        return rc;

    sprintf(value, "%s", rt_info->ifname);
    return 0;
}

/* FIXME: Route types are disabled until Configurator is redesigned - A.A */
#if 0
static te_errno
route_type_get(unsigned int gid, const char *oid,
              char *value, const char *route) 
{
    te_errno        rc;
    ta_rt_info_t   *rt_info;

    UNUSED(oid);
    
    if ((rc = route_find(gid, route, &rt_info)) != 0)
        return rc;

    if (rt_info->type >= TA_RT_TYPE_MAX_VALUE)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(value, "%s", ta_rt_type2name(rt_info->type));
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
    UNUSED(oid);

    return ta_obj_add(TA_OBJ_TYPE_ROUTE, route, value, gid, NULL, NULL);
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
    UNUSED(oid);

    return ta_obj_del(TA_OBJ_TYPE_ROUTE, route, NULL, gid,
                      route_load_attrs);
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

    route = ((cfg_inst_subid *)(p_oid->ids))[p_oid->len - 1].name;
    ENTRY("%s", route);
    
    if ((obj = ta_obj_find(TA_OBJ_TYPE_ROUTE, route)) == NULL)
    {
        WARN("Commit for %s route which has not been updated", route);
        return 0;
    }

    memset(&rt_info, 0, sizeof(rt_info));

    if ((rc = ta_rt_parse_obj(obj, &rt_info)) != 0)
    {
        ERROR("%s(): ta_rt_parse_obj() failed: %r", __FUNCTION__, rc);
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

    return ta_unix_conf_route_blackhole_del(&rt_info);
}

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

RCF_PCH_CFG_NODE_RWC(node_route_src, "src", NULL, &node_route_dev,
                     route_src_get, route_src_set, &node_route);

static rcf_pch_cfg_object node_route =
    {"route", 0, &node_route_src, &node_blackhole,
     (rcf_ch_cfg_get)route_get, (rcf_ch_cfg_set)route_set,
     (rcf_ch_cfg_add)route_add, (rcf_ch_cfg_del)route_del,
     (rcf_ch_cfg_list)route_list, (rcf_ch_cfg_commit)route_commit, NULL};

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_init(void)
{
    return rcf_pch_add_node("/agent", &node_route);
}

/* See the description in conf_route.h */
te_errno
ta_unix_conf_outgoing_if(ta_rt_info_t *rt_info)
{
    te_errno    rc;
    char        buf[INET6_ADDRSTRLEN];

    rc = ta_unix_conf_route_find(rt_info);
    if (rc != 0)
    {
        WARN("Failed to find route to destination %s get outgoing "
             "interface name: %r",
             inet_ntop(rt_info->dst.ss_family,
                       te_sockaddr_get_netaddr(CONST_SA(&rt_info->dst)),
                       buf, sizeof(buf)), rc);
        return rc;
    }
    if (~rt_info->flags & TA_RT_INFO_FLG_IF)
    {
        if (~rt_info->flags & TA_RT_INFO_FLG_GW)
        {
            ERROR("%s(): Invalid result of ta_unix_conf_route_find(), "
                  "route entry contains neither outgoing interface nor "
                  "gateway address", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        
        assert(sizeof(rt_info->dst) == sizeof(rt_info->gw));
        memcpy(&rt_info->dst, &rt_info->gw, sizeof(rt_info->dst));
        
        rc = ta_unix_conf_route_find(rt_info);
        if (rc != 0)
        {
            WARN("Failed to find route to gateway %s get outgoing "
                 "interface name: %r",
                 inet_ntop(rt_info->dst.ss_family,
                     te_sockaddr_get_netaddr(CONST_SA(&rt_info->dst)),
                     buf, sizeof(buf)), rc);
            return rc;
        }
        if (~rt_info->flags & TA_RT_INFO_FLG_IF)
        {
            ERROR("Gateway %s is not directly reachable",
                  inet_ntop(rt_info->dst.ss_family,
                      te_sockaddr_get_netaddr(CONST_SA(&rt_info->dst)),
                      buf, sizeof(buf)));
        }
    }
    return 0;
}
