/** @file
 * @brief Unix Test Agent
 *
 * Unix TA routing configuring support
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
#include "te_string.h"
#include "te_str.h"
#include "cs_common.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "unix_internal.h"
#include "conf_route.h"

#if defined(USE_LIBNETCONF) || defined(USE_ROUTE_SOCKET)

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
    if (rt_cache_name != NULL)
        ta_rt_info_clean(&rt_cache_info);
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
 * Obtain interface name associated with a given route.
 *
 * @param gid          Group identifier
 * @param route_name   Route node name
 * @param ifname       Where to save interface name
 * @param len          Number of bytes in ifname
 *
 * @return Status code
 */
static te_errno
rt_if_get(unsigned int gid, const char *route_name, char *ifname,
          size_t len)
{
    ta_rt_info_t *attr;
    te_errno      rc;

    if (len < IF_NAMESIZE)
    {
        ERROR("%s(): too short buffer for interface name",
              __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ENOBUFS);
    }

    rc = route_find(gid, route_name, &attr);
    if (rc == 0)
        te_strlcpy(ifname, attr->ifname, len);

    return rc;
}
 
/**
 * Obtain interface name from IPv4 default route record.
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier (unused)
 * @param ifname       Where to save interface name
 *
 * @return             Status code
 */
static te_errno
ip4_rt_default_if_get(unsigned int gid, const char *oid, char *ifname)
{
    UNUSED(oid);
 
    return rt_if_get(gid, "0.0.0.0|0", ifname, RCF_MAX_VAL);
}
 
/**
 * Obtain interface name from IPv6 default route record.
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier (unused)
 * @param ifname       Where to save interface name
 *
 * @return             Status code
 */
static te_errno
ip6_rt_default_if_get(unsigned int gid, const char *oid, char *ifname)
{
    UNUSED(oid);

    return rt_if_get(gid, "::|0", ifname, RCF_MAX_VAL);
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
    ROUTE_LOAD_ATTR(HOPLIMIT, hoplimit);

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

        rc = ta_obj_value_set(TA_OBJ_TYPE_ROUTE, obj->name, val,
                              obj->gid, NULL);
        if (rc != 0)
        {
            ERROR("Failed to set route object value: %r", rc);
            return rc;
        }
    }

    if (rt_info->flags & TA_RT_INFO_FLG_MULTIPATH)
    {
        ta_rt_nexthops_t  *hops;

        if (obj->user_data != NULL)
        {
            ERROR("Trying to fill nexthops in a temporary route object "
                  "the second time");
            return TE_EFAIL;
        }

        hops = calloc(1, sizeof(*hops));
        if (hops == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);

        TAILQ_INIT(hops);
        TAILQ_CONCAT(hops, &rt_info->nexthops, links);
        obj->user_data = hops;
    }

#undef ROUTE_LOAD_ATTR

    return 0;
}

/**
 * Set route value.
 *
 * @param       gid        Group identifier
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
    UNUSED(oid);

    return ta_obj_value_set(TA_OBJ_TYPE_ROUTE, route_name, value, gid,
                            route_load_attrs);
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
DEF_ROUTE_GET_FUNC(hoplimit);
DEF_ROUTE_SET_FUNC(hoplimit);

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
DEF_ROUTE_SET_FUNC(type);

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

    return ta_obj_add(TA_OBJ_TYPE_ROUTE, route, value, gid,
                      NULL, NULL, NULL);
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

    return ta_obj_del(TA_OBJ_TYPE_ROUTE, route, NULL, NULL, gid,
                      route_load_attrs);
}


static te_errno
route_list(unsigned int gid, const char *oid, const char *sub_id,
           char **list)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

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

    if ((obj = ta_obj_find(TA_OBJ_TYPE_ROUTE, route, gid)) == NULL)
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

    if (obj->user_data != NULL)
    {
        TAILQ_INIT(&rt_info.nexthops);
        TAILQ_CONCAT(&rt_info.nexthops,
                     (ta_rt_nexthops_t *)(obj->user_data),
                     links);
        free(obj->user_data);
        obj->user_data = NULL;
        rt_info.flags |= TA_RT_INFO_FLG_MULTIPATH;
    }

    obj_action = obj->action;

    ta_obj_free(obj);

    rc = ta_unix_conf_route_change(obj_action, &rt_info);
    ta_rt_info_clean(&rt_info);
    return rc;
}


static te_errno
blackhole_list(unsigned int gid, const char *oid, const char *sub_id,
               char **list)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

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

/**
 * Convert nexthop ID from string representation to numeric value.
 *
 * @param id_str_     ID in string representation.
 * @param id_num_     Where to save numeric value.
 */
#define CONVERT_NH_ID(id_str_, id_num_) \
    do {                                                              \
        te_errno          rc_;                                        \
        long unsigned int res_;                                       \
                                                                      \
        rc_ = te_strtoul((id_str_), 10, &res_);                       \
        if (rc_ != 0)                                                 \
        {                                                             \
            ERROR("%s(): failed to convert '%s' to nexthop number",   \
                  __FUNCTION__, (id_str_));                           \
            return TE_RC(TE_TA_UNIX, rc_);                            \
        }                                                             \
                                                                      \
        (id_num_) = res_;                                             \
    } while (0)

/**
 * Find multipath route nexthop by its ID in a queue of nexthops.
 *
 * @param hops        Head of the queue.
 * @param id          Nexthop ID.
 * @param nh          Where to save pointer to found nexthop.
 *
 * @return Status code.
 */
static te_errno
find_nexthop_by_id(ta_rt_nexthops_t *hops, unsigned int id,
                   ta_rt_nexthop_t **nh)
{
    ta_rt_nexthop_t     *rt_nh = NULL;

    TAILQ_FOREACH(rt_nh, hops, links)
    {
        if (rt_nh->id == id)
        {
            *nh = rt_nh;
            return 0;
        }
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/**
 * Find nexthop of a multipath route for the purpose of changing it.
 *
 * @param gid       Group ID.
 * @param route     Route node name.
 * @param hop_id    Nexthop index.
 * @param nhq       Where to save pointer to a queue head of
 *                  route nexthops (may be @c NULL).
 * @param nh        Where to save pointer to nexthop structure.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_set_find(unsigned int gid,
                       const char *route,
                       const char *hop_id,
                       ta_rt_nexthops_t **nhq,
                       ta_rt_nexthop_t **nh)
{
    ta_rt_nexthops_t  *hops = NULL;
    ta_cfg_obj_t      *route_obj = NULL;
    te_errno           rc;
    unsigned int       id;

    CONVERT_NH_ID(hop_id, id);

    rc = ta_obj_find_create(TA_OBJ_TYPE_ROUTE, route, gid,
                            route_load_attrs, &route_obj, NULL);
    if (rc != 0)
        return rc;

    hops = (ta_rt_nexthops_t *)route_obj->user_data;
    if (hops == NULL)
    {
        ERROR("%s(): no nexthops in route '%s'", __FUNCTION__, route);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    rc = find_nexthop_by_id(hops, id, nh);
    if (rc != 0)
    {
        ERROR("%s(): failed to find nexthop number '%s'",
              __FUNCTION__, hop_id);
        return rc;
    }

    if (nhq != NULL)
        *nhq = hops;

    return 0;
}

/**
 * Find nexthop of a multipath route for reading its properties.
 *
 * @param gid       Group ID.
 * @param route     Route node name.
 * @param hop_id    Nexthop index.
 * @param rt        Where to save pointer to route structure
 *                  (may be @c NULL).
 * @param nh        Where to save pointer to nexthop structure.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_get_find(unsigned int gid, const char *route,
                       const char *hop_id,
                       ta_rt_info_t **rt, ta_rt_nexthop_t **nh)
{
    te_errno             rc;
    ta_rt_info_t        *rt_info;
    unsigned int         id;

    CONVERT_NH_ID(hop_id, id);

    if ((rc = route_find(gid, route, &rt_info)) != 0)
        return rc;

    rc = find_nexthop_by_id(&rt_info->nexthops, id, nh);
    if (rc != 0)
    {
        ERROR("%s(): failed to find nexthop number '%s'",
              __FUNCTION__, hop_id);
        return rc;
    }

    if (rt != NULL)
        *rt = rt_info;

    return 0;
}

/**
 * Add nexthop to a multipath route.
 *
 * @param gid     Group ID (unused).
 * @param oid     OID (unused).
 * @param value   Node value (unused).
 * @param route   Route node name.
 * @param hop_id  Nexthop index (unused, assigned automatically).
 *
 * @return Status code.
 */
static te_errno
route_nexthop_add(unsigned int gid, const char *oid,
                  const char *value, const char *route, const char *hop_id)
{
    ta_cfg_obj_t      *route_obj;
    ta_rt_nexthop_t   *nh;
    ta_rt_nexthop_t   *nh_aux;
    ta_rt_nexthops_t  *hops;
    unsigned int       id;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    CONVERT_NH_ID(hop_id, id);

    route_obj = ta_obj_find(TA_OBJ_TYPE_ROUTE, route, gid);
    if (route_obj == NULL)
    {
        ERROR("%s(): failed to find a route '%s'", __FUNCTION__, route);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (route_obj->user_data != NULL)
    {
        hops = (ta_rt_nexthops_t *)route_obj->user_data;
    }
    else
    {
        hops = calloc(1, sizeof(*hops));
        if (hops == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        TAILQ_INIT(hops);

        route_obj->user_data = hops;
    }

    TAILQ_FOREACH(nh_aux, hops, links)
    {
        if (nh_aux->id == id)
        {
            ERROR("%s(): nexthop %u exists already", __FUNCTION__, id);
            return TE_RC(TE_TA_UNIX, TE_EEXIST);
        }

        if (nh_aux->id > id)
            break;
    }

    nh = calloc(1, sizeof(*nh));
    if (nh == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    nh->weight = 1;
    nh->id = id;

    if (nh_aux == NULL)
        TAILQ_INSERT_TAIL(hops, nh, links);
    else
        TAILQ_INSERT_BEFORE(nh_aux, nh, links);

    return 0;
}

/**
 * Remove nexthop of a multipath route.
 *
 * @param gid       Group ID.
 * @param oid       OID (unused).
 * @param route     Route node name.
 * @param hop_id    Nexthop index.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_del(unsigned int gid, const char *oid,
                  const char *route, const char *hop_id)
{
    ta_rt_nexthops_t    *hops = NULL;
    ta_rt_nexthop_t     *rt_nh = NULL;
    te_errno             rc;

    UNUSED(oid);

    rc = route_nexthop_set_find(gid, route, hop_id, &hops, &rt_nh);
    if (rc != 0)
    {
        /*
         * FIXME: This is done to allow Configurator to remove multipath
         * route automatically in cleanup. Configurator starts
         * by removing nexthop:0, however after that nexthop:1 becomes
         * nexthop:0 or even disappears (routes with the single nexthop
         * are no longer reported as multipath by netlink). So trying
         * to remove the final nexthop may fail.
         * Unfortunately I did not find a way to tell Configurator
         * that after removing a nexthop configurator tree for route
         * should be synchronized automatically.
         */
        if (rc == TE_RC(TE_TA_UNIX, TE_ENOENT))
            return 0;

        return rc;
    }

    TAILQ_REMOVE(hops, rt_nh, links);
    free(rt_nh);

    return 0;
}

/**
 * List nexthops (paths) of a multipath route.
 *
 * @param gid     Group ID.
 * @param oid     OID (unused).
 * @param sub_id  Unused.
 * @param list    Where to save pointer to a string with list of names.
 * @param route   Route node name.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_list(unsigned int gid, const char *oid, const char *sub_id,
                   char **list, const char *route)
{
    ta_rt_info_t *rt_info = NULL;
    te_errno      rc;
    te_string     str = TE_STRING_INIT;

    ta_rt_nexthop_t *rt_nh = NULL;
    unsigned int     i = 0;

    UNUSED(oid);
    UNUSED(sub_id);

    rc = route_find(gid, route, &rt_info);
    if (rc != 0)
        return rc;

    if (rt_info->flags & TA_RT_INFO_FLG_MULTIPATH)
    {
        TAILQ_FOREACH(rt_nh, &rt_info->nexthops, links)
        {
            rc = te_string_append(&str, "%u ", i);
            if (rc != 0)
            {
                te_string_free(&str);
                return rc;
            }
            i++;
        }
    }

    *list = str.ptr;
    return 0;
}

/**
 * Get gateway of a nexthop of a multipath route.
 *
 * @param gid       Group ID.
 * @param oid       OID (unused).
 * @param value     Where to save obtained value.
 * @param route     Route node name.
 * @param hop_id    Nexthop index.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_gw_get(unsigned int gid, const char *oid,
                     char *value, const char *route, const char *hop_id)
{
    te_errno         rc;
    ta_rt_nexthop_t *rt_nh = NULL;
    ta_rt_info_t    *rt_info = NULL;

    UNUSED(oid);

    rc = route_nexthop_get_find(gid, route, hop_id, &rt_info, &rt_nh);
    if (rc != 0)
        return rc;

    if (rt_nh->flags & TA_RT_NEXTHOP_FLG_GW)
    {
        rc = te_sockaddr_h2str_buf(SA(&rt_nh->gw), value, RCF_MAX_VAL);
        if (rc != 0)
        {
            ERROR("%s(): failed to convert address to string, errno=%r",
                  __FUNCTION__, rc);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }
    else
    {
        switch (rt_info->dst.ss_family)
        {
            case AF_INET:
                TE_STRLCPY(value, "0.0.0.0", RCF_MAX_VAL);
                break;

            case AF_INET6:
                TE_STRLCPY(value, "::", RCF_MAX_VAL);
                break;

            default:
                return TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT);
        }
    }

    return 0;
}

/**
 * Set gateway of a nexthop of a multipath route.
 *
 * @param gid       Group ID.
 * @param oid       OID (unused).
 * @param value     Value to set.
 * @param route     Route node name.
 * @param hop_id    Nexthop index.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_gw_set(unsigned int gid, const char *oid,
                     const char *value, const char *route, const char *hop_id)
{
    te_errno             rc;
    ta_rt_nexthop_t     *rt_nh = NULL;

    UNUSED(oid);

    rc = route_nexthop_set_find(gid, route, hop_id, NULL, &rt_nh);
    if (rc != 0)
        return rc;

    rc = te_sockaddr_netaddr_from_string(value, SA(&rt_nh->gw));
    if (rc != 0)
    {
        ERROR("%s(): failed to parse address '%s'", __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, rc);
    }

    rt_nh->flags |= TA_RT_NEXTHOP_FLG_GW;

    return 0;
}

/**
 * Get interface name of a nexthop of a multipath route.
 *
 * @param gid       Group ID.
 * @param oid       OID (unused).
 * @param value     Where to save obtained value.
 * @param route     Route node name.
 * @param hop_id    Nexthop index.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_dev_get(unsigned int gid, const char *oid,
                      char *value, const char *route, const char *hop_id)
{
    te_errno         rc;
    ta_rt_nexthop_t *rt_nh = NULL;

    UNUSED(oid);

    rc = route_nexthop_get_find(gid, route, hop_id, NULL, &rt_nh);
    if (rc != 0)
        return rc;

    if (rt_nh->flags & TA_RT_NEXTHOP_FLG_OIF)
        TE_STRLCPY(value, rt_nh->ifname, RCF_MAX_VAL);

    return 0;
}

/**
 * Set interface of a nexthop of a multipath route.
 *
 * @param gid       Group ID.
 * @param oid       OID (unused).
 * @param value     Value to set.
 * @param route     Route node name.
 * @param hop_id    Nexthop index.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_dev_set(unsigned int gid, const char *oid,
                      const char *value, const char *route, const char *hop_id)
{
    te_errno             rc;
    ta_rt_nexthop_t     *rt_nh = NULL;
    size_t               val_len;

    UNUSED(oid);

    rc = route_nexthop_set_find(gid, route, hop_id, NULL, &rt_nh);
    if (rc != 0)
        return rc;

    val_len = strnlen(value, RCF_MAX_VAL);
    if (val_len > sizeof(rt_nh->ifname) - 1)
        ERROR("%s(): interface name is too long", __FUNCTION__);

    strcpy(rt_nh->ifname, value);

    rt_nh->flags |= TA_RT_NEXTHOP_FLG_OIF;

    return 0;
}

/**
 * Get weight of a nexthop of a multipath route.
 *
 * @param gid       Group ID.
 * @param oid       OID (unused).
 * @param value     Where to save obtained value.
 * @param route     Route node name.
 * @param hop_id    Nexthop index.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_weight_get(unsigned int gid, const char *oid,
                         char *value, const char *route, const char *hop_id)
{
    te_errno         rc;
    ta_rt_nexthop_t *rt_nh = NULL;

    UNUSED(oid);

    rc = route_nexthop_get_find(gid, route, hop_id, NULL, &rt_nh);
    if (rc != 0)
        return rc;

    snprintf(value, RCF_MAX_VAL, "%u", rt_nh->weight);
    return 0;
}

/**
 * Set weight of a nexthop of a multipath route.
 *
 * @param gid       Group ID.
 * @param oid       OID (unused).
 * @param value     Value to set.
 * @param route     Route node name.
 * @param hop_id    Nexthop index.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_weight_set(unsigned int gid, const char *oid,
                         const char *value, const char *route, const char *hop_id)
{
    te_errno             rc;
    ta_rt_nexthop_t     *rt_nh = NULL;
    long unsigned int    weight;

    UNUSED(oid);

    rc = te_strtoul(value, 10, &weight);
    if (rc != 0 || weight > UINT_MAX || weight < 1)
    {
        ERROR("%s(): failed to convert '%s' to correct nexthop weight",
              __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, rc);
    }

    rc = route_nexthop_set_find(gid, route, hop_id, NULL, &rt_nh);
    if (rc != 0)
        return rc;

    rt_nh->weight = weight;

    return 0;
}

/*
 * Unix Test Agent routing configuration tree.
 */

RCF_PCH_CFG_NODE_RO(node_ip4_rt_default_if, "ip4_rt_default_if",
                    NULL, NULL, ip4_rt_default_if_get);

RCF_PCH_CFG_NODE_RO(node_ip6_rt_default_if, "ip6_rt_default_if",
                    NULL, &node_ip4_rt_default_if, ip6_rt_default_if_get);

RCF_PCH_CFG_NODE_COLLECTION(node_blackhole, "blackhole",
                            NULL, &node_ip6_rt_default_if,
                            blackhole_add, blackhole_del,
                            blackhole_list, NULL);

static rcf_pch_cfg_object node_route;

RCF_PCH_CFG_NODE_RWC(node_route_type, "type", NULL, NULL,
                     route_type_get, route_type_set, &node_route);

/*
 * This attribute influences both IPv4 Time To Live and IPv6 Hop Limit
 * header fields.
 */
RCF_PCH_CFG_NODE_RWC(node_route_hoplimit, "hoplimit", NULL,
                     &node_route_type,
                     route_hoplimit_get, route_hoplimit_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_irtt, "irtt", NULL, &node_route_hoplimit,
                     route_irtt_get, route_irtt_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_win, "win", NULL, &node_route_irtt,
                     route_win_get, route_win_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_mtu, "mtu", NULL, &node_route_win,
                     route_mtu_get, route_mtu_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_dev, "dev", NULL, &node_route_mtu,
                     route_dev_get, route_dev_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_src, "src", NULL, &node_route_dev,
                     route_src_get, route_src_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_nexthop_weight, "weight", NULL, NULL,
                     route_nexthop_weight_get,
                     route_nexthop_weight_set, &node_route);
RCF_PCH_CFG_NODE_RWC(node_route_nexthop_dev, "dev", NULL,
                     &node_route_nexthop_weight,
                     route_nexthop_dev_get,
                     route_nexthop_dev_set, &node_route);
RCF_PCH_CFG_NODE_RWC(node_route_nexthop_gw, "gw", NULL,
                     &node_route_nexthop_dev,
                     route_nexthop_gw_get,
                     route_nexthop_gw_set, &node_route);

static rcf_pch_cfg_object node_route_nexthop =
    { "nexthop", 0, &node_route_nexthop_gw, &node_route_src,
      (rcf_ch_cfg_get)NULL, (rcf_ch_cfg_set)NULL,
      (rcf_ch_cfg_add)route_nexthop_add, (rcf_ch_cfg_del)route_nexthop_del,
      (rcf_ch_cfg_list)route_nexthop_list, (rcf_ch_cfg_commit)NULL,
      &node_route, NULL};

static rcf_pch_cfg_object node_route =
    {"route", 0, &node_route_nexthop, &node_blackhole,
     (rcf_ch_cfg_get)route_get, (rcf_ch_cfg_set)route_set,
     (rcf_ch_cfg_add)route_add, (rcf_ch_cfg_del)route_del,
     (rcf_ch_cfg_list)route_list, (rcf_ch_cfg_commit)route_commit, NULL, NULL};

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

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_init(void)
{
    return rcf_pch_add_node("/agent", &node_route);
}
#else
/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_init(void)
{
    INFO("Network route configurations are not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF && !USE_ROUTE_SOCKET */

