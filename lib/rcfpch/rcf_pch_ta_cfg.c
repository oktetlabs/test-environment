/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Common library for test agents which is very useful
 * in supporting read-create instances implementing "commit" operation.
 *
 * Test Agent configuration library
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "RCF PCH TA CFG"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef __CYGWIN__

#include <winsock2.h>
#include <w32api/ws2tcpip.h>

extern int inet_pton(int af, const char *src, void *dst);

#define IFNAMSIZ        16

#undef ERROR

#else

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#endif

#include "te_errno.h"
#include "te_defs.h"
#include "te_str.h"
#include "logger_api.h"
#include "rcf_pch_ta_cfg.h"

#ifdef WITH_NETCONF
#include "netconf.h"
#else

/**
 * Default routing table id
 */
#define LOCAL_RT_TABLE_MAIN 254

#endif

#define TA_OBJS_NUM 1000
static ta_cfg_obj_t ta_objs[TA_OBJS_NUM];

/* See the description in rcf_pch_ta_cfg.h */
void
ta_obj_cleanup(void)
{
    unsigned int i;

    for (i = 0; i < TA_OBJS_NUM; i++)
    {
        if (ta_objs[i].in_use)
            ta_obj_free(&ta_objs[i]);
    }
}

/* See the description in rcf_pch_ta_cfg.h */
int
ta_obj_attr_set(ta_cfg_obj_t *obj, const char *name, const char *value)
{
    ta_cfg_obj_attr_t *attr;

    for (attr = obj->attrs; attr != NULL; attr = attr->next)
    {
        if (strcmp(attr->name, name) == 0)
            break;
    }

    if (attr == NULL)
    {
        /* Add a new attribute */
        attr = (ta_cfg_obj_attr_t *)malloc(sizeof(ta_cfg_obj_attr_t));
        if (attr == NULL)
            return TE_ENOMEM;

        snprintf(attr->name, sizeof(attr->name), "%s", name);
        attr->next = obj->attrs;
        obj->attrs = attr;
    }

    /* Set a new value */
    snprintf(attr->value, sizeof(attr->value), "%s", value);

    return 0;
}

/* See the description in rcf_pch_ta_cfg.h */
ta_cfg_obj_attr_t *
ta_obj_attr_find(ta_cfg_obj_t *obj, const char *name)
{
    ta_cfg_obj_attr_t *attr;

    for (attr = obj->attrs; attr != NULL; attr = attr->next)
    {
        if (strcmp(attr->name, name) == 0)
            return attr;
    }

    return NULL;
}

/* See the description in rcf_pch_ta_cfg.h */
void
ta_obj_free(ta_cfg_obj_t *obj)
{
    ta_cfg_obj_attr_t *attr;
    ta_cfg_obj_attr_t *cur_attr;

    assert(obj != NULL);

    free(obj->type);
    free(obj->name);
    free(obj->value);

    attr = obj->attrs;
    while (attr != NULL)
    {
        cur_attr = attr;
        attr = attr->next;
        free(cur_attr);
    }

    if (obj->user_free != NULL)
    {
        obj->user_free(obj->user_data);
        obj->user_data = NULL;
    }

    obj->in_use = FALSE;
}

/* See the description in rcf_pch_ta_cfg.h */
ta_cfg_obj_t *
ta_obj_find(const char *type, const char *name,
            unsigned int gid)
{
    int i;

    assert(type != NULL && name != NULL);

    for (i = 0; i < TA_OBJS_NUM; i++)
    {
        if (ta_objs[i].in_use &&
            strcmp(ta_objs[i].type, type) == 0 &&
            strcmp(ta_objs[i].name, name) == 0 &&
            ta_objs[i].gid == gid)
        {
            return &ta_objs[i];
        }
    }

    return NULL;
}

/* See the description in rcf_pch_ta_cfg.h */
te_errno
ta_obj_find_create(const char *type, const char *name,
                   unsigned int gid, ta_obj_cb cb_func,
                   ta_cfg_obj_t **obj, te_bool *created)
{
    ta_cfg_obj_t *tmp = NULL;
    int           rc;

    tmp = ta_obj_find(type, name, gid);
    if (tmp != NULL)
    {
        if (created != NULL)
            *created = FALSE;

        *obj = tmp;

        return 0;
    }

    rc = ta_obj_add(type, name, NULL, gid, NULL, NULL, &tmp);
    if (rc != 0)
        return rc;

    if (cb_func != NULL && (rc = cb_func(tmp)) != 0)
    {
        ta_obj_free(tmp);
        return rc;
    }

    if (created != NULL)
        *created = TRUE;

    tmp->action = TA_CFG_OBJ_SET;
    tmp->gid = gid;
    *obj = tmp;

    return 0;
}

/* See the description in rcf_pch_ta_cfg.h */
int
ta_obj_add(const char *type, const char *name, const char *value,
           unsigned int gid, void *user_data,
           ta_cfg_obj_data_free *user_free, ta_cfg_obj_t **new_obj)
{
    int           i;
    ta_cfg_obj_t *obj = ta_obj_find(type, name, gid);

    if (obj != NULL)
        return TE_EEXIST;

    /* Find a free slot */
    for (i = 0; i < TA_OBJS_NUM; i++)
    {
        if (!ta_objs[i].in_use)
            break;

        if (ta_objs[i].gid != gid)
        {
            /*
             * Assume that objects with not matching group ID are
             * outdated and can be released. gid is incremented
             * with every new request that is not within a group
             * in rcf_pch_configure().
             */
            ta_obj_free(&ta_objs[i]);
            break;
        }
    }

    if (i == TA_OBJS_NUM)
        return TE_ENOMEM;

    ta_objs[i].in_use = TRUE;
    ta_objs[i].type = strdup(type);
    ta_objs[i].name = strdup(name);
    ta_objs[i].value = ((value != NULL) ? strdup(value) : NULL);
    ta_objs[i].gid = gid;
    ta_objs[i].action = TA_CFG_OBJ_CREATE;
    ta_objs[i].attrs = NULL;

    if (ta_objs[i].type == NULL || ta_objs[i].name == NULL ||
        (value != NULL && ta_objs[i].value == NULL))
    {
        ta_obj_free(&ta_objs[i]);
        return TE_ENOMEM;
    }

    ta_objs[i].user_data = user_data;
    ta_objs[i].user_free = user_free;

    if (new_obj != NULL)
        *new_obj = &ta_objs[i];

    return 0;
}

/* See the description in rcf_pch_ta_cfg.h */
te_errno
ta_obj_value_set(const char *type, const char *name, const char *value,
                 unsigned int gid, ta_obj_cb cb_func)
{
    ta_cfg_obj_t *obj = NULL;
    te_bool       locally_created = FALSE;
    te_errno      rc;

    rc = ta_obj_find_create(type, name, gid, cb_func,
                            &obj, &locally_created);
    if (rc != 0)
        return rc;

    obj->value = ((value != NULL) ? strdup(value) : NULL);
    if (obj->value == NULL && value != NULL)
    {
        if (locally_created)
            ta_obj_free(obj);

        ERROR("%s(): out of memory", __FUNCTION__);
        return TE_ENOMEM;
    }

    return 0;
}

/* See the description in rcf_pch_ta_cfg.h */
int
ta_obj_set(const char *type, const char *name,
           const char *attr_name, const char *attr_value,
           unsigned int gid, ta_obj_cb cb_func)
{
    ta_cfg_obj_t *obj = NULL;
    te_bool       locally_created = FALSE;
    int           rc;

    rc = ta_obj_find_create(type, name, gid, cb_func,
                            &obj, &locally_created);
    if (rc != 0)
        return rc;

    if (gid != obj->gid)
    {
        ERROR("%s(): Request GID=%u does not match object GID=%u",
              __FUNCTION__, gid, obj->gid);
        return TE_RC(TE_RCF_PCH, TE_ESYNCFAILED);
    }

    /* Now set specified attribute */
    if ((rc = ta_obj_attr_set(obj, attr_name, attr_value)) != 0 &&
        locally_created)
    {
        /* Delete obj from the container */
        ta_obj_free(obj);
    }

    return rc;
}

/* See the description in rcf_pch_ta_cfg.h */
int
ta_obj_del(const char *type, const char *name, void *user_data,
           ta_cfg_obj_data_free *user_free,
           unsigned int gid, ta_obj_cb cb_func)
{
    ta_cfg_obj_t *obj = ta_obj_find(type, name, gid);
    int           rc;

    if (obj != NULL)
    {
        ERROR("Delete operation on locally added route instance");
        return TE_EFAULT;
    }

    /* Add object, but reset add flag */
    if ((rc = ta_obj_add(type, name, NULL, gid,
                         user_data, user_free, &obj)) != 0)
        return rc;

    obj->action = TA_CFG_OBJ_DELETE;
    obj->gid = gid;

    if (cb_func != NULL && (rc = cb_func(obj)) != 0)
    {
        ta_obj_free(obj);
        return rc;
    }

    return 0;
}

/* See the description in rcf_pch_ta_cfg.h */
void
ta_cfg_obj_log(const ta_cfg_obj_t *obj)
{
    ta_cfg_obj_attr_t  *attr;

    if (!obj->in_use)
    {
        ERROR("%s(): called for an object not in use");
        return;
    }

    RING("TA configuration object: type=%s name=%s value=%s user_data=%p "
         "gid=%u action=%u", obj->type, obj->name, obj->value,
         obj->user_data, obj->gid, obj->action);

    for (attr = obj->attrs; attr != NULL; attr = attr->next)
        RING("TA configuration object: type=%s name=%s %s=%s",
             obj->type, obj->name, attr->name, attr->value);
}


/* Route-specific functions */

/* See the description in rcf_pch_ta_cfg.h */
int
ta_rt_parse_inst_name(const char *name, ta_rt_info_t *rt_info)
{
    char        *tmp, *tmp1;
    int          prefix;
    char        *ptr;
    static char  inst_copy[RCF_MAX_VAL];
    int          family;

    memset(rt_info, 0, sizeof(*rt_info));

    te_strlcpy(inst_copy, name, sizeof(inst_copy));
    inst_copy[sizeof(inst_copy) - 1] = '\0';

    if ((tmp = strchr(inst_copy, '|')) == NULL)
        return TE_EINVAL;

    *tmp = '\0';

    family = (strchr(inst_copy, ':') == NULL) ? AF_INET : AF_INET6;

    ((struct sockaddr *)&(rt_info->dst))->sa_family = family;

    if ((family == AF_INET &&
         inet_pton(family, inst_copy, &(SIN(&(rt_info->dst))->sin_addr))
         == 0) ||
        (family == AF_INET6 &&
         inet_pton(family, inst_copy, &(SIN6(&(rt_info->dst))->sin6_addr))
         == 0))
    {
        ERROR("Incorrect 'destination address' value in route %s", name);
        return TE_EINVAL;
    }
    tmp++;
    if (*tmp == '-' ||
        (prefix = strtol(tmp, &tmp1, 10), tmp == tmp1 ||
         (family == AF_INET && prefix > 32) ||
         (family == AF_INET6 && prefix > 128)))
    {
        ERROR("Incorrect 'prefix length' value in route %s", name);
        return TE_EINVAL;
    }
    tmp = tmp1;
    rt_info->prefix = prefix;

    if ((ptr = strstr(tmp, "metric=")) != NULL)
    {
        ptr += strlen("metric=");
        rt_info->metric = atoi(ptr);
        rt_info->flags |= TA_RT_INFO_FLG_METRIC;
    }

    if ((ptr = strstr(tmp, "tos=")) != NULL)
    {
        ptr += strlen("tos=");
        rt_info->tos = atoi(ptr);
        rt_info->flags |= TA_RT_INFO_FLG_TOS;
    }

    if ((ptr = strstr(tmp, "table=")) != NULL)
    {
        ptr += strlen("table=");
        rt_info->table = atoi(ptr);
        rt_info->flags |= TA_RT_INFO_FLG_TABLE;
    }
    else
    {
#ifdef WITH_NETCONF
        rt_info->table = NETCONF_RT_TABLE_MAIN;
#else
        rt_info->table = LOCAL_RT_TABLE_MAIN;
#endif
    }

    /*
     * Set the route type to unicast by default.
     * We anyway support only unicast and blackhole
     * route types.
     */
    rt_info->type = TA_RT_TYPE_UNICAST;

    return 0;
}

static char *rt_type_names[TA_RT_TYPE_MAX_VALUE] =
{
    "",
    "unicast",
    "local",
    "broadcast",
    "anycast",
    "multicast",
    "blackhole",
    "unreachable",
    "prohibit",
    "throw",
    "nat"
};

const char *
ta_rt_type2name(ta_route_type type)
{
    if (type >= TA_RT_TYPE_MAX_VALUE)
    {
        ERROR("Invalid route type number: %d", type);
        return "";
    }
    return rt_type_names[type];
}

static ta_route_type
ta_rt_name2type(const char *name)
{
    char **iter;

    for (iter = rt_type_names + 1;
         iter < rt_type_names + TE_ARRAY_LEN(rt_type_names);
         iter++)
    {
        if (strcmp(name, *iter) == 0)
            return iter - rt_type_names;
    }
    return TA_RT_TYPE_UNSPECIFIED;
}

/* See the description in rcf_pch_ta_cfg.h */
int
ta_rt_parse_inst_value(const char *value, ta_rt_info_t *rt_info)
{
    int family = AF_INET;
    int rc = 0;

    if (value != NULL)
    {
        if (index(value, ':') != NULL)
            family = AF_INET6;

        rt_info->gw.ss_family = family;
        if (family == AF_INET)
        {
            rc = inet_pton(family, value,
                           &SIN(&rt_info->gw)->sin_addr);
            if (SIN(&rt_info->gw)->sin_addr.s_addr != INADDR_ANY)
            {
                rt_info->flags |= TA_RT_INFO_FLG_GW;
            }
        }
        else if (family == AF_INET6)
        {
            rc = inet_pton(family, value,
                           &SIN6(&rt_info->gw)->sin6_addr);
            if (!IN6_IS_ADDR_UNSPECIFIED(&SIN6(&rt_info->gw)->sin6_addr))
            {
                rt_info->flags |= TA_RT_INFO_FLG_GW;
            }
        }

        if (rc <= 0)
        {
            /* Clear gateway flag */
            rt_info->flags &= (~TA_RT_INFO_FLG_GW);
            ERROR("Invalid value of route: '%s'", value);
            return TE_EINVAL;
        }
    }
    else
    {
        rt_info->flags &= (~TA_RT_INFO_FLG_GW);
    }

    return 0;
}

/* See the description in rcf_pch_ta_cfg.h */
int
ta_rt_parse_attrs(ta_cfg_obj_attr_t *attrs, ta_rt_info_t *rt_info)
{
#define PARSE_UINT_ATTR \
    if (*(attr->value) == '\0' || *(attr->value) == '-' ||    \
        (int_val = strtol(attr->value, &end_ptr, 10),         \
         *end_ptr != '\0'))                                   \
    {                                                         \
        ERROR("Incorrect '%s' attribute value in route",      \
              attr->name);                                    \
        return TE_EINVAL;                                     \
    }

    ta_cfg_obj_attr_t *attr;
    char              *end_ptr;
    int                int_val;

    rt_info->type = TA_RT_TYPE_UNICAST;
    for (attr = attrs; attr != NULL; attr = attr->next)
    {
        if (strcmp(attr->name, "dev") == 0)
        {
            if (attr->value[0] != '\0')
            {
                if (strlen(attr->value) > IFNAMSIZ)
                {
                    ERROR("Interface name is too long");
                    return TE_EINVAL;
                }
                rt_info->flags |= TA_RT_INFO_FLG_IF;
                strcpy(rt_info->ifname, attr->value);
            }
        }
        else if (strcmp(attr->name, "mtu") == 0)
        {
            PARSE_UINT_ATTR;

            /* Don't be confused the structure does not have mtu field */
            rt_info->mtu = int_val;
            rt_info->flags |= TA_RT_INFO_FLG_MTU;
        }
        else if (strcmp(attr->name, "win") == 0)
        {
            PARSE_UINT_ATTR;
            rt_info->win = int_val;
            rt_info->flags |= TA_RT_INFO_FLG_WIN;
        }
        else if (strcmp(attr->name, "irtt") == 0)
        {
            PARSE_UINT_ATTR;
            rt_info->irtt = int_val;
            rt_info->flags |= TA_RT_INFO_FLG_IRTT;
        }
        else if (strcmp(attr->name, "hoplimit") == 0)
        {
            PARSE_UINT_ATTR;
            rt_info->hoplimit = int_val;
            rt_info->flags |= TA_RT_INFO_FLG_HOPLIMIT;
        }
        else if (strcmp(attr->name, "type") == 0)
        {
            ta_route_type type = ta_rt_name2type(attr->value);

            if (type == TA_RT_TYPE_UNSPECIFIED)
            {
                ERROR("Invalid route type: %s", attr->value);
                return TE_EINVAL;
            }
            rt_info->type = type;
        }
        else if (strcmp(attr->name, "src") == 0)
        {
            rt_info->src.ss_family = (strchr(attr->value, ':') != NULL) ?
                                     AF_INET6 : AF_INET;
            if (inet_pton(rt_info->src.ss_family, attr->value,
                          (rt_info->src.ss_family == AF_INET)?
                          (void *)(&SIN(&rt_info->src)->sin_addr) :
                          (void *)(&SIN6(&rt_info->src)->sin6_addr)) <= 0)
            {
                ERROR("Incorrect source address: %s", attr->value);
                return TE_EINVAL;
            }
            rt_info->flags |= TA_RT_INFO_FLG_SRC;
        }
        else
        {
            ERROR("Unknown attribute '%s' found in route object",
                  attr->name);
            return TE_EINVAL;
        }
    }
    return 0;
#undef PARSE_UINT_ATTR
}

/* See the description in rcf_pch_ta_cfg.h */
int
ta_rt_parse_obj(ta_cfg_obj_t *obj, ta_rt_info_t *rt_info)
{
    int rc;

    if ((rc = ta_rt_parse_inst_name(obj->name, rt_info)) != 0)
        return rc;

    if ((rc = ta_rt_parse_inst_value(obj->value, rt_info)) != 0)
        return rc;

    return ta_rt_parse_attrs(obj->attrs, rt_info);
}

/* See the description in rcf_pch_ta_cfg.h */
void
ta_rt_nexthops_clean(ta_rt_nexthops_t *hops)
{
    ta_rt_nexthop_t *nh;
    ta_rt_nexthop_t *t_nh;

    TAILQ_FOREACH_SAFE(nh, hops, links, t_nh)
    {
        TAILQ_REMOVE(hops, nh, links);
        free(nh);
    }
}

/* See the description in rcf_pch_ta_cfg.h */
void
ta_rt_info_clean(ta_rt_info_t *rt_info)
{
    if (rt_info->flags == TA_RT_INFO_FLG_MULTIPATH)
        ta_rt_nexthops_clean(&rt_info->nexthops);

    memset(rt_info, 0, sizeof(*rt_info));
}
