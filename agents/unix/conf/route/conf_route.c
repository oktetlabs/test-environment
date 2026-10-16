/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix TA routing configuring support
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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

#include "te_alloc.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_sockaddr.h"
#include "te_string.h"
#include "te_str.h"
#include "te_vector.h"
#include "cs_common.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "rcf_pch_tree.h"
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
 * Append space-separated instance names to a list vector.
 *
 * @param list      Space-separated names (may be @c NULL), freed here
 * @param names     Vector of heap-allocated names to append to
 */
static void
route_names_split(char *list, te_vec *names)
{
    char *saveptr = NULL;
    char *tok;

    if (list == NULL)
        return;

    for (tok = strtok_r(list, " ", &saveptr); tok != NULL;
         tok = strtok_r(NULL, " ", &saveptr))
    {
        char *name = TE_STRDUP(tok);

        TE_VEC_APPEND(names, name);
    }
    free(list);
}

/**
 * Obtain interface name associated with a given route.
 *
 * @param gid          Group identifier
 * @param route_name   Route node name
 * @param ifname       Where to save interface name
 *
 * @return Status code
 */
static te_errno
rt_if_get(unsigned int gid, const char *route_name, te_string *ifname)
{
    ta_rt_info_t *attr;
    te_errno      rc;

    rc = route_find(gid, route_name, &attr);
    if (rc == 0)
        te_string_append(ifname, "%s", attr->ifname);

    return rc;
}

/**
 * Obtain interface name from IPv4 default route record.
 *
 * @param ctx          Request context
 * @param ifname       Where to save interface name
 *
 * @return             Status code
 */
static te_errno
ip4_rt_default_if_get(const ta_conf_ctx *ctx, te_string *ifname)
{
    return rt_if_get(ta_conf_ctx_gid(ctx), "0.0.0.0|0", ifname);
}

/**
 * Obtain interface name from IPv6 default route record.
 *
 * @param ctx          Request context
 * @param ifname       Where to save interface name
 *
 * @return             Status code
 */
static te_errno
ip6_rt_default_if_get(const ta_conf_ctx *ctx, te_string *ifname)
{
    return rt_if_get(ta_conf_ctx_gid(ctx), "::|0", ifname);
}

/**
 * Copy the IP address of @p addr to @p value, in the address family
 * of the route destination: an address that is not set (no gateway,
 * no source) reads as the unspecified address of that family.
 *
 * @param rt_info       Route
 * @param addr          Gateway or source address of the route
 * @param value         Where to put the address
 *
 * @return              Status code
 */
static te_errno
route_addr_get(const ta_rt_info_t *rt_info,
               const struct sockaddr_storage *addr,
               struct sockaddr_storage *value)
{
    memset(value, 0, sizeof(*value));

    switch (rt_info->dst.ss_family)
    {
        case AF_INET:
            SIN(value)->sin_family = AF_INET;
            SIN(value)->sin_addr = CONST_SIN(addr)->sin_addr;
            return 0;

        case AF_INET6:
            SIN6(value)->sin6_family = AF_INET6;
            SIN6(value)->sin6_addr = CONST_SIN6(addr)->sin6_addr;
            return 0;

        default:
            ERROR("Unexpected destination address family: %d",
                  rt_info->dst.ss_family);
            return TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT);
    }
}

/**
 * Format an address for a route object, which keeps addresses in
 * their textual form until the commit parses them.
 *
 * @param addr          Address
 * @param buf           Buffer of RCF_MAX_VAL bytes for the text
 *
 * @return              Status code
 */
static te_errno
route_addr2str(const struct sockaddr *addr, char *buf)
{
    te_string str = TE_STRING_EXT_BUF_INIT(buf, RCF_MAX_VAL);

    return te_netaddr2te_str(addr, &str);
}

/**
 * Get route value.
 *
 * @param ctx           Request context
 * @param value         Place for route value (gateway address
 *                      or zero if it is a direct route)
 *
 * @return              Status code
 */
static te_errno
route_get(const ta_conf_ctx *ctx, struct sockaddr_storage *value)
{
    const char   *route_name = ta_conf_ctx_inst(ctx, "route");
    ta_rt_info_t *attr;
    te_errno      rc;

    if ((rc = route_find(ta_conf_ctx_gid(ctx), route_name, &attr)) != 0)
    {
        ERROR("Route %s cannot be found", route_name);
        return rc;
    }

    return route_addr_get(attr, &attr->gw, value);
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
            (rc = ta_obj_attr_set(obj, #field_, val)) != 0)   \
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
            (rc = ta_obj_attr_set(obj, "src", val)) != 0)
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

        hops = TE_ALLOC(sizeof(*hops));

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
 * @param       ctx        Request context
 * @param       value      New value for the route
 *
 * @return      Status code.
 */
static te_errno
route_set(const ta_conf_ctx *ctx, const struct sockaddr *value)
{
    char     buf[RCF_MAX_VAL];
    te_errno rc;

    rc = route_addr2str(value, buf);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return ta_obj_value_set(TA_OBJ_TYPE_ROUTE,
                            ta_conf_ctx_inst(ctx, "route"), buf,
                            ta_conf_ctx_gid(ctx), route_load_attrs);
}

#define DEF_ROUTE_GET_FUNC(field_) \
static te_errno                                                     \
route_ ## field_ ## _get(const ta_conf_ctx *ctx, int32_t *value)    \
{                                                                   \
    te_errno        rc;                                             \
    ta_rt_info_t   *rt_info;                                        \
                                                                    \
    rc = route_find(ta_conf_ctx_gid(ctx),                           \
                    ta_conf_ctx_inst(ctx, "route"), &rt_info);      \
    if (rc != 0)                                                    \
        return rc;                                                  \
                                                                    \
    *value = (int32_t)rt_info->field_;                              \
    return 0;                                                       \
}

#define DEF_ROUTE_SET_FUNC(field_) \
static te_errno                                                     \
route_ ## field_ ## _set(const ta_conf_ctx *ctx, const char *value) \
{                                                                   \
    return ta_obj_set(TA_OBJ_TYPE_ROUTE,                            \
                      ta_conf_ctx_inst(ctx, "route"), #field_,      \
                      value, ta_conf_ctx_gid(ctx),                  \
                      route_load_attrs);                            \
}

#define DEF_ROUTE_SET_INT_FUNC(field_) \
static te_errno                                                     \
route_ ## field_ ## _set(const ta_conf_ctx *ctx, int32_t value)     \
{                                                                   \
    char buf[16];                                                   \
                                                                    \
    snprintf(buf, sizeof(buf), "%" PRId32, value);                  \
    return ta_obj_set(TA_OBJ_TYPE_ROUTE,                            \
                      ta_conf_ctx_inst(ctx, "route"), #field_,      \
                      buf, ta_conf_ctx_gid(ctx),                    \
                      route_load_attrs);                            \
}

DEF_ROUTE_GET_FUNC(mtu);
DEF_ROUTE_SET_INT_FUNC(mtu);
DEF_ROUTE_GET_FUNC(win);
DEF_ROUTE_SET_INT_FUNC(win);
DEF_ROUTE_GET_FUNC(irtt);
DEF_ROUTE_SET_INT_FUNC(irtt);
DEF_ROUTE_GET_FUNC(hoplimit);
DEF_ROUTE_SET_INT_FUNC(hoplimit);

static te_errno
route_src_get(const ta_conf_ctx *ctx, struct sockaddr_storage *value)
{
    te_errno        rc;
    ta_rt_info_t   *rt_info;

    rc = route_find(ta_conf_ctx_gid(ctx), ta_conf_ctx_inst(ctx, "route"),
                    &rt_info);
    if (rc != 0)
        return rc;

    return route_addr_get(rt_info, &rt_info->src, value);
}

static te_errno
route_src_set(const ta_conf_ctx *ctx, const struct sockaddr *value)
{
    char     buf[RCF_MAX_VAL];
    te_errno rc;

    rc = route_addr2str(value, buf);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return ta_obj_set(TA_OBJ_TYPE_ROUTE, ta_conf_ctx_inst(ctx, "route"),
                      "src", buf, ta_conf_ctx_gid(ctx), route_load_attrs);
}

DEF_ROUTE_SET_FUNC(dev);
DEF_ROUTE_SET_FUNC(type);

static te_errno
route_dev_get(const ta_conf_ctx *ctx, te_string *value)
{
    te_errno        rc;
    ta_rt_info_t   *rt_info;

    rc = route_find(ta_conf_ctx_gid(ctx), ta_conf_ctx_inst(ctx, "route"),
                    &rt_info);
    if (rc != 0)
        return rc;

    te_string_append(value, "%s", rt_info->ifname);
    return 0;
}

static te_errno
route_type_get(const ta_conf_ctx *ctx, te_string *value)
{
    te_errno        rc;
    ta_rt_info_t   *rt_info;

    rc = route_find(ta_conf_ctx_gid(ctx), ta_conf_ctx_inst(ctx, "route"),
                    &rt_info);
    if (rc != 0)
        return rc;

    if (rt_info->type >= TA_RT_TYPE_MAX_VALUE)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    te_string_append(value, "%s", ta_rt_type2name(rt_info->type));
    return 0;
}

#undef DEF_ROUTE_GET_FUNC
#undef DEF_ROUTE_SET_FUNC
#undef DEF_ROUTE_SET_INT_FUNC


/**
 * Add a new route.
 *
 * @param ctx           Request context (route instance name: see
 *                      doc/cm_cm_base.xml for the format)
 * @param value         value string
 *
 * @return              Status code
 */
static te_errno
route_add(const ta_conf_ctx *ctx, const struct sockaddr *value)
{
    char     buf[RCF_MAX_VAL];
    te_errno rc;

    rc = route_addr2str(value, buf);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return ta_obj_add(TA_OBJ_TYPE_ROUTE, ta_conf_ctx_inst(ctx, "route"),
                      buf, ta_conf_ctx_gid(ctx), NULL, NULL, NULL);
}

/**
 * Delete a route.
 *
 * @param ctx           Request context (route instance name: see
 *                      doc/cm_cm_base.xml for the format)
 *
 * @return              Status code
 */
static te_errno
route_del(const ta_conf_ctx *ctx)
{
    return ta_obj_del(TA_OBJ_TYPE_ROUTE, ta_conf_ctx_inst(ctx, "route"),
                      NULL, NULL, ta_conf_ctx_gid(ctx), route_load_attrs);
}


static te_errno
route_list(const ta_conf_ctx *ctx, te_vec *names)
{
    char     *list = NULL;
    te_errno  rc;

    UNUSED(ctx);

    rc = ta_unix_conf_route_list(&list);
    if (rc != 0)
        return rc;

    route_names_split(list, names);
    return 0;
}


static te_errno
route_commit(const ta_conf_ctx *ctx)
{
    unsigned int            gid = ta_conf_ctx_gid(ctx);
    const char             *route = ta_conf_ctx_inst(ctx, "route");
    ta_cfg_obj_t           *obj;
    ta_rt_info_t            rt_info;
    te_errno                rc;
    ta_cfg_obj_action_e     obj_action;

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
blackhole_list(const ta_conf_ctx *ctx, te_vec *names)
{
    char     *list = NULL;
    te_errno  rc;

    UNUSED(ctx);

    rc = ta_unix_conf_route_blackhole_list(&list);
    if (rc != 0)
        return rc;

    route_names_split(list, names);
    return 0;
}

static te_errno
blackhole_add(const ta_conf_ctx *ctx)
{
    return ta_obj_add(TA_OBJ_TYPE_BLACKHOLE,
                      ta_conf_ctx_inst(ctx, "blackhole"), NULL,
                      ta_conf_ctx_gid(ctx), NULL, NULL, NULL);
}

static te_errno
blackhole_del(const ta_conf_ctx *ctx)
{
    return ta_obj_del(TA_OBJ_TYPE_BLACKHOLE,
                      ta_conf_ctx_inst(ctx, "blackhole"), NULL,
                      NULL, ta_conf_ctx_gid(ctx), route_load_attrs);
}

static te_errno
blackhole_commit(const ta_conf_ctx *ctx)
{
    unsigned int            gid = ta_conf_ctx_gid(ctx);
    const char             *route = ta_conf_ctx_inst(ctx, "blackhole");
    ta_cfg_obj_t           *obj;
    ta_rt_info_t            rt_info;
    te_errno                rc;
    ta_cfg_obj_action_e     obj_action;

    ENTRY("%s", route);

    obj = ta_obj_find(TA_OBJ_TYPE_BLACKHOLE, route, gid);
    if (obj == NULL)
    {
        WARN("Commit for %s route which has not been updated", route);
        return 0;
    }

    memset(&rt_info, 0, sizeof(rt_info));

    rc = ta_rt_parse_inst_name(obj->name, &rt_info);
    if (rc != 0)
    {
        ERROR("%s(): ta_rt_parse_inst_name() failed: %r", __func__, rc);
        ta_obj_free(obj);
        return rc;
    }

    rc = ta_rt_parse_attrs(obj->attrs, &rt_info);
    if (rc != 0)
    {
        ERROR("%s(): ta_rt_parse_attrs() failed: %r", __func__, rc);
        ta_obj_free(obj);
        return rc;
    }

    if (rt_info.type != TA_RT_TYPE_BLACKHOLE &&
        rt_info.type != TA_RT_TYPE_UNREACHABLE &&
        rt_info.type != TA_RT_TYPE_PROHIBIT &&
        rt_info.type != TA_RT_TYPE_THROW)
    {
        ERROR("Blackhole route type '%s' is not supported yet",
              ta_rt_type2name(rt_info.type));
        ta_obj_free(obj);
        ta_rt_info_clean(&rt_info);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    obj_action = obj->action;
    ta_obj_free(obj);

    rc = ta_unix_conf_route_change(obj_action, &rt_info);
    ta_rt_info_clean(&rt_info);
    return rc;
}

static te_errno
blackhole_type_get(const ta_conf_ctx *ctx, te_string *value)
{
    te_errno rc;
    ta_rt_info_t *rt_info;

    rc = route_find(ta_conf_ctx_gid(ctx), ta_conf_ctx_inst(ctx, "blackhole"),
                    &rt_info);
    if (rc != 0)
        return rc;

    if (rt_info->type >= TA_RT_TYPE_MAX_VALUE)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    te_string_append(value, "%s", ta_rt_type2name(rt_info->type));
    return 0;
}

static te_errno
blackhole_type_set(const ta_conf_ctx *ctx, const char *value)
{
    return ta_obj_set(TA_OBJ_TYPE_BLACKHOLE,
                      ta_conf_ctx_inst(ctx, "blackhole"), "type",
                      value, ta_conf_ctx_gid(ctx), route_load_attrs);
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
 * @param ctx     Request context (the nexthop index is assigned
 *                automatically and only checked for uniqueness).
 *
 * @return Status code.
 */
static te_errno
route_nexthop_add(const ta_conf_ctx *ctx)
{
    const char        *route = ta_conf_ctx_inst(ctx, "route");
    const char        *hop_id = ta_conf_ctx_inst(ctx, "nexthop");
    ta_cfg_obj_t      *route_obj;
    ta_rt_nexthop_t   *nh;
    ta_rt_nexthop_t   *nh_aux;
    ta_rt_nexthops_t  *hops;
    unsigned int       id;

    CONVERT_NH_ID(hop_id, id);

    route_obj = ta_obj_find(TA_OBJ_TYPE_ROUTE, route, ta_conf_ctx_gid(ctx));
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
        hops = TE_ALLOC(sizeof(*hops));
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

    nh = TE_ALLOC(sizeof(*nh));

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
 * @param ctx       Request context.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_del(const ta_conf_ctx *ctx)
{
    ta_rt_nexthops_t    *hops = NULL;
    ta_rt_nexthop_t     *rt_nh = NULL;
    te_errno             rc;

    rc = route_nexthop_set_find(ta_conf_ctx_gid(ctx),
                                ta_conf_ctx_inst(ctx, "route"),
                                ta_conf_ctx_inst(ctx, "nexthop"),
                                &hops, &rt_nh);
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
 * @param ctx     Request context.
 * @param names   Where to append the names.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_list(const ta_conf_ctx *ctx, te_vec *names)
{
    ta_rt_info_t *rt_info = NULL;
    te_errno      rc;

    ta_rt_nexthop_t *rt_nh = NULL;
    unsigned int     i = 0;

    rc = route_find(ta_conf_ctx_gid(ctx), ta_conf_ctx_inst(ctx, "route"),
                    &rt_info);
    if (rc != 0)
        return rc;

    if (rt_info->flags & TA_RT_INFO_FLG_MULTIPATH)
    {
        TAILQ_FOREACH(rt_nh, &rt_info->nexthops, links)
        {
            char  buf[16];
            char *name;

            snprintf(buf, sizeof(buf), "%u", i);
            name = TE_STRDUP(buf);
            TE_VEC_APPEND(names, name);
            i++;
        }
    }

    return 0;
}

/**
 * Get gateway of a nexthop of a multipath route.
 *
 * @param ctx       Request context.
 * @param value     Where to save obtained value.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_gw_get(const ta_conf_ctx *ctx, struct sockaddr_storage *value)
{
    te_errno         rc;
    ta_rt_nexthop_t *rt_nh = NULL;
    ta_rt_info_t    *rt_info = NULL;

    rc = route_nexthop_get_find(ta_conf_ctx_gid(ctx),
                                ta_conf_ctx_inst(ctx, "route"),
                                ta_conf_ctx_inst(ctx, "nexthop"),
                                &rt_info, &rt_nh);
    if (rc != 0)
        return rc;

    if (!(rt_nh->flags & TA_RT_NEXTHOP_FLG_GW))
    {
        /* No gateway: the unspecified address of the route family */
        if (rt_info->dst.ss_family != AF_INET &&
            rt_info->dst.ss_family != AF_INET6)
            return TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT);

        memset(value, 0, sizeof(*value));
        value->ss_family = rt_info->dst.ss_family;
        return 0;
    }

    if (rt_nh->gw.ss_family != AF_INET && rt_nh->gw.ss_family != AF_INET6)
    {
        ERROR("%s(): unexpected gateway address family %d", __FUNCTION__,
              rt_nh->gw.ss_family);
        return TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT);
    }

    *value = rt_nh->gw;
    return 0;
}

/**
 * Set gateway of a nexthop of a multipath route.
 *
 * @param ctx       Request context.
 * @param value     Value to set.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_gw_set(const ta_conf_ctx *ctx, const struct sockaddr *value)
{
    te_errno             rc;
    ta_rt_nexthop_t     *rt_nh = NULL;

    rc = route_nexthop_set_find(ta_conf_ctx_gid(ctx),
                                ta_conf_ctx_inst(ctx, "route"),
                                ta_conf_ctx_inst(ctx, "nexthop"),
                                NULL, &rt_nh);
    if (rc != 0)
        return rc;

    if (value->sa_family != AF_INET && value->sa_family != AF_INET6)
    {
        ERROR("%s(): gateway is not an IP address", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rt_nh->gw.ss_family = value->sa_family;
    te_sockaddr_set_netaddr(SA(&rt_nh->gw), te_sockaddr_get_netaddr(value));
    rt_nh->flags |= TA_RT_NEXTHOP_FLG_GW;

    return 0;
}

/**
 * Get interface name of a nexthop of a multipath route.
 *
 * @param ctx       Request context.
 * @param value     Where to save obtained value.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_dev_get(const ta_conf_ctx *ctx, te_string *value)
{
    te_errno         rc;
    ta_rt_nexthop_t *rt_nh = NULL;

    rc = route_nexthop_get_find(ta_conf_ctx_gid(ctx),
                                ta_conf_ctx_inst(ctx, "route"),
                                ta_conf_ctx_inst(ctx, "nexthop"),
                                NULL, &rt_nh);
    if (rc != 0)
        return rc;

    if (rt_nh->flags & TA_RT_NEXTHOP_FLG_OIF)
        te_string_append(value, "%s", rt_nh->ifname);

    return 0;
}

/**
 * Set interface of a nexthop of a multipath route.
 *
 * @param ctx       Request context.
 * @param value     Value to set.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_dev_set(const ta_conf_ctx *ctx, const char *value)
{
    te_errno             rc;
    ta_rt_nexthop_t     *rt_nh = NULL;
    size_t               val_len;

    rc = route_nexthop_set_find(ta_conf_ctx_gid(ctx),
                                ta_conf_ctx_inst(ctx, "route"),
                                ta_conf_ctx_inst(ctx, "nexthop"),
                                NULL, &rt_nh);
    if (rc != 0)
        return rc;

    val_len = strnlen(value, RCF_MAX_VAL);
    if (val_len > sizeof(rt_nh->ifname) - 1)
    {
        ERROR("%s(): interface name is too long", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ENAMETOOLONG);
    }

    strcpy(rt_nh->ifname, value);

    rt_nh->flags |= TA_RT_NEXTHOP_FLG_OIF;

    return 0;
}

/**
 * Get weight of a nexthop of a multipath route.
 *
 * @param ctx       Request context.
 * @param value     Where to save obtained value.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_weight_get(const ta_conf_ctx *ctx, uint32_t *value)
{
    te_errno         rc;
    ta_rt_nexthop_t *rt_nh = NULL;

    rc = route_nexthop_get_find(ta_conf_ctx_gid(ctx),
                                ta_conf_ctx_inst(ctx, "route"),
                                ta_conf_ctx_inst(ctx, "nexthop"),
                                NULL, &rt_nh);
    if (rc != 0)
        return rc;

    *value = rt_nh->weight;
    return 0;
}

/**
 * Set weight of a nexthop of a multipath route.
 *
 * @param ctx       Request context.
 * @param value     Value to set.
 *
 * @return Status code.
 */
static te_errno
route_nexthop_weight_set(const ta_conf_ctx *ctx, uint32_t value)
{
    te_errno             rc;
    ta_rt_nexthop_t     *rt_nh = NULL;

    if (value < 1)
    {
        ERROR("%s(): failed to convert '%" PRIu32 "' to correct nexthop "
              "weight", __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = route_nexthop_set_find(ta_conf_ctx_gid(ctx),
                                ta_conf_ctx_inst(ctx, "route"),
                                ta_conf_ctx_inst(ctx, "nexthop"),
                                NULL, &rt_nh);
    if (rc != 0)
        return rc;

    rt_nh->weight = value;

    return 0;
}

/*
 * Unix Test Agent routing configuration tree.
 *
 * The four roots are registered on "/agent" one by one, see
 * ta_unix_conf_route_init().
 */

static const ta_conf_node *const node_ip4_rt_default_if =
    TA_CONF_RO_STR("ip4_rt_default_if", ip4_rt_default_if_get);

static const ta_conf_node *const node_ip6_rt_default_if =
    TA_CONF_RO_STR("ip6_rt_default_if", ip6_rt_default_if_get);

static const ta_conf_node *const node_blackhole =
    TA_CONF_COLL_COMMIT("blackhole", blackhole_add, blackhole_del,
                        blackhole_list, blackhole_commit,
        TA_CONF_RW_STR("type", blackhole_type_get, blackhole_type_set));

static const ta_conf_node *const node_route =
    TA_CONF_NODE((.name = "route", .type = CVT_ADDRESS,
                  .get = { .as_addr = route_get },
                  .set = { .as_addr = route_set },
                  .add = { .as_addr = route_add },
                  .del = route_del,
                  .list = route_list,
                  .commit = route_commit),
        TA_CONF_COLL("nexthop", route_nexthop_add, route_nexthop_del,
                     route_nexthop_list,
            TA_CONF_RW_ADDRESS("gw", route_nexthop_gw_get,
                               route_nexthop_gw_set),
            TA_CONF_RW_STR("dev", route_nexthop_dev_get,
                           route_nexthop_dev_set),
            TA_CONF_RW_UINT32("weight", route_nexthop_weight_get,
                              route_nexthop_weight_set)),
        TA_CONF_RW_ADDRESS("src", route_src_get, route_src_set),
        TA_CONF_RW_STR("dev", route_dev_get, route_dev_set),
        TA_CONF_RW_INT32("mtu", route_mtu_get, route_mtu_set),
        TA_CONF_RW_INT32("win", route_win_get, route_win_set),
        TA_CONF_RW_INT32("irtt", route_irtt_get, route_irtt_set),
        /*
         * This attribute influences both IPv4 Time To Live and IPv6
         * Hop Limit header fields.
         */
        TA_CONF_RW_INT32("hoplimit", route_hoplimit_get,
                         route_hoplimit_set),
        TA_CONF_RW_STR("type", route_type_get, route_type_set));

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
    te_errno rc;

    /*
     * Each ta_conf_register() call prepends its root to the children
     * of "/agent", so register in reverse to keep the legacy order:
     * route, blackhole, ip6_rt_default_if, ip4_rt_default_if.
     */
    rc = ta_conf_register("/agent", node_ip4_rt_default_if);
    if (rc != 0)
        return rc;

    rc = ta_conf_register("/agent", node_ip6_rt_default_if);
    if (rc != 0)
        return rc;

    rc = ta_conf_register("/agent", node_blackhole);
    if (rc != 0)
        return rc;

    return ta_conf_register("/agent", node_route);
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

