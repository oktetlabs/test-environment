/** @file 
 * @brief Common library for test agents which is very useful
 * in supporting read-create instances implementing "commit" operation.
 *
 * Test Agent configuration library
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
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
#include "logger_api.h"
#include "rcf_pch_ta_cfg.h"


#define TA_OBJS_NUM 10
static ta_cfg_obj_t ta_objs[TA_OBJS_NUM];

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

    obj->in_use = FALSE;
}

/* See the description in rcf_pch_ta_cfg.h */
ta_cfg_obj_t *
ta_obj_find(const char *type, const char *name)
{
    int i;

    assert(type != NULL && name != NULL);

    for (i = 0; i < TA_OBJS_NUM; i++)
    {
        if (ta_objs[i].in_use &&
            strcmp(ta_objs[i].type, type) == 0 &&
            strcmp(ta_objs[i].name, name) == 0)
        {
            return &ta_objs[i];
        }
    }

    return NULL;
}

/* See the description in rcf_pch_ta_cfg.h */
int
ta_obj_add(const char *type, const char *name, const char *value,
           void *user_data, ta_cfg_obj_t **new_obj)
{
    int           i;
    ta_cfg_obj_t *obj = ta_obj_find(type, name);
    
    if (obj != NULL)
        return TE_EEXIST;
    
    /* Find a free slot */
    for (i = 0; i < TA_OBJS_NUM; i++)
    {
        if (!ta_objs[i].in_use)
            break;
    }

    if (i == TA_OBJS_NUM)
        return TE_ENOMEM;

    ta_objs[i].in_use = TRUE;
    ta_objs[i].type = strdup(type);
    ta_objs[i].name = strdup(name);
    ta_objs[i].value = ((value != NULL) ? strdup(value) : NULL);
    ta_objs[i].user_data = user_data;
    ta_objs[i].action = TA_CFG_OBJ_CREATE;
    ta_objs[i].attrs = NULL;

    if (ta_objs[i].type == NULL || ta_objs[i].name == NULL ||
        (value != NULL && ta_objs[i].value == NULL))
    {
        ta_obj_free(&ta_objs[i]);
        return TE_ENOMEM;
    }

    if (new_obj != NULL)
        *new_obj = &ta_objs[i];

    return 0;
}

/* See the description in rcf_pch_ta_cfg.h */
int
ta_obj_value_set(const char *type, const char *name, const char *value)
{
    ta_cfg_obj_t *obj = ta_obj_find(type, name);
    int           rc;
    
    if (obj == NULL)
    {
        /* Add object first, but reset add flag */
        if ((rc = ta_obj_add(type, name, NULL, NULL, &obj)) != 0)
            return rc;

        obj->action = TA_CFG_OBJ_SET;
    }

    obj->value = ((value != NULL) ? strdup(value) : NULL);

    return 0;
}

/* See the description in rcf_pch_ta_cfg.h */
int
ta_obj_set(const char *type, const char *name,
           const char *attr_name, const char *attr_value,
           ta_obj_set_cb cb_func)
{
    ta_cfg_obj_t *obj = ta_obj_find(type, name);
    te_bool       locally_created = FALSE;
    int           rc;

    if (obj == NULL)
    {
        /* Add object first, but reset add flag */
        if ((rc = ta_obj_add(type, name, NULL, NULL, &obj)) != 0)
            return rc;

        obj->action = TA_CFG_OBJ_SET;
        locally_created = TRUE;
        
        if (cb_func != NULL && (rc = cb_func(obj)) != 0)
        {
            ta_obj_free(obj);
            return rc;
        }
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
ta_obj_del(const char *type, const char *name, void *user_data)
{
    ta_cfg_obj_t *obj = ta_obj_find(type, name);
    int           rc;

    if (obj != NULL)
    {
        ERROR("Delete operation on locally added route instance");
        return TE_EFAULT;
    }

    /* Add object, but reset add flag */
    if ((rc = ta_obj_add(type, name, NULL, user_data, &obj)) != 0)
        return rc;

    obj->action = TA_CFG_OBJ_DELETE;

    return rc;
}

/* Route-specific functions */

/* See the description in rcf_pch_ta_cfg.h */
int
ta_rt_parse_inst_name(const char *name, ta_rt_info_t *rt_info)
{
    char        *tmp, *tmp1;
    int          prefix;
    char        *ptr;
    char        *end_ptr;
    char        *term_byte; /* Pointer to the trailing zero byte
                               in instance name */
    static char  inst_copy[RCF_MAX_VAL];
    int          family;

    memset(rt_info, 0, sizeof(*rt_info));

    strncpy(inst_copy, name, sizeof(inst_copy));
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

    term_byte = (char *)(tmp + strlen(tmp));

    if ((ptr = strstr(tmp, "metric=")) != NULL)
    {
        end_ptr = ptr += strlen("metric=");
        rt_info->metric = atoi(ptr);
        rt_info->flags |= TA_RT_INFO_FLG_METRIC;
    }
    
    if ((ptr = strstr(tmp, "tos=")) != NULL)
    {
        end_ptr = ptr += strlen("tos=");
        rt_info->tos = atoi(ptr);
        rt_info->flags |= TA_RT_INFO_FLG_TOS;
    }
    
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
            if (*(attr->value) == '\0' || *(attr->value) == '-' ||
                (int_val = strtol(attr->value, &end_ptr, 10),
                 *end_ptr != '\0'))
            {
                ERROR("Incorrect 'route mtu' value in route");
                return TE_EINVAL;
            }
    
            /* Don't be confused the structure does not have mtu field */
            rt_info->mtu = int_val;
            rt_info->flags |= TA_RT_INFO_FLG_MTU;
        }
        else if (strcmp(attr->name, "win") == 0)
        {
            if (*(attr->value) == '\0' || *(attr->value) == '-' ||
                (int_val = strtol(attr->value, &end_ptr, 10),
                 *end_ptr != '\0'))
            {
                ERROR("Incorrect 'route window' value in route");
                return TE_EINVAL;
            }
            rt_info->win = int_val;
            rt_info->flags |= TA_RT_INFO_FLG_WIN;
        }
        else if (strcmp(attr->name, "irtt") == 0)
        {
            if (*(attr->value) == '\0' || *(attr->value) == '-' ||
                (int_val = strtol(attr->value, &end_ptr, 10),
                 *end_ptr != '\0'))
            {
                ERROR("Incorrect 'route irtt' value in route");
                return TE_EINVAL;
            }
            rt_info->irtt = int_val;
            rt_info->flags |= TA_RT_INFO_FLG_IRTT;
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
        else
        {
            ERROR("Unknown attribute '%s' found in route object",
                  attr->name);
            return TE_EINVAL;
        }
    }
    return 0;
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
