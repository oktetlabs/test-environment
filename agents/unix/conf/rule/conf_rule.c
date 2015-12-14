/** @file
 * @brief Unix Test Agent
 *
 * Unix TA rules configuration support
 *
 *
 * Copyright (C) 2015 Test Environment authors (see file AUTHORS
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
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Unix Conf Rule"


#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

#include "conf_ip_rule.h"
#include "conf_netconf.h"
#include "conf_rule.h"
#include "logger_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"

#if defined(USE_LIBNETCONF) || defined(USE_ROUTE_SOCKET)

#include "netconf.h"

/**
 * Find a rule and return its attributes.
 *
 * @param gid       Operation group ID
 * @param rule      Rule instance name: see doc/cm_cm_base.xml
 *                  for the format
 * @param rule_info Rule related information (OUT)
 *
 * @return Status code.
 *
 * @note The function uses static variables for caching, so it is
 *       not multithread safe.
 */
static te_errno
rule_find(unsigned int gid, const char *rule, te_conf_ip_rule **rule_info)
{
    static unsigned int     rule_cache_gid = (unsigned int)-1;
    static char            *rule_cache_name = NULL;
    static te_conf_ip_rule  rule_cache_info;

    te_errno    rc;
    uint32_t    required;

    ENTRY("GID=%u rule=%s", gid, rule);

    if ((gid == rule_cache_gid) &&
        (rule_cache_name != NULL) &&
        (strcmp(rule, rule_cache_name) == 0))
    {
        *rule_info = &rule_cache_info;
        return 0;
    }

    /* Make cache invalid */
    free(rule_cache_name);
    rule_cache_name = NULL;

    rc = te_conf_ip_rule_from_str(rule, &required, &rule_cache_info);
    if (rc != 0)
    {
        ERROR("Cannot parse instance name: %s", rule);
        return TE_RC(TE_TA_UNIX, rc);
    }

    if ((rc = ta_unix_conf_rule_find(required, &rule_cache_info)) != 0)
    {
        return TE_RC(TE_TA_UNIX, rc);
    }

    rule_cache_gid = gid;
    rule_cache_name = strdup(rule);
    if (rule_cache_name == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    *rule_info = &rule_cache_info;

    return 0;
}

/**
 * Add a new rule.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full object instance identifier (unused)
 * @param value         Value string
 * @param rule          Rule instance name: see doc/cm/cm_base.xml
 *                      for the format
 *
 * @return              Status code
 */
static te_errno
rule_add(unsigned int gid, const char *oid, const char *value,
         const char *rule)
{
    te_errno            rc;
    uint32_t            required;
    te_conf_ip_rule     ip_rule;
    te_conf_ip_rule     found_ip_rule;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    rc = te_conf_ip_rule_from_str(rule, &required, &ip_rule);
    memcpy(&found_ip_rule, &ip_rule, sizeof(ip_rule));

    rc = ta_unix_conf_rule_find(required, &found_ip_rule);
    if (rc == 0 && memcmp(&found_ip_rule, &ip_rule, sizeof(ip_rule)) == 0)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    return netconf_rule_modify(nh, NETCONF_CMD_ADD, &ip_rule);
}

/**
 * Delete a rule.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full object instance identifier (unused)
 * @param rule          Rule instance name: see doc/cm/cm_base.xml
 *                      for the format
 *
 * @return              Status code
 */
static te_errno
rule_del(unsigned int gid, const char *oid, const char *rule)
{
    int         rc;
    te_conf_ip_rule *ip_rule;

    UNUSED(oid);

    if ((rc = rule_find(gid, rule, &ip_rule)) != 0)
    {
        ERROR("Rule %s cannot be found", rule);
        return rc;
    }

    return netconf_rule_modify(nh, NETCONF_CMD_DEL, ip_rule);
}

/**
 * Get rules list.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full identifier of the father instance (unused)
 * @param list          Location for the list pointer
 *
 * @return              Status code
 */
static te_errno
rule_list(unsigned int gid, const char *oid, char **list)
{
    UNUSED(gid);
    UNUSED(oid);

    return ta_unix_conf_rule_list(list);
}

/**
 * Commit a rule.
 *
 * @param gid           Group identifier (unused)
 * @param p_oid         Parsed object instance identifier (unused)
 *
 * @return              Status code
 */
static te_errno
rule_commit(unsigned int gid, const cfg_oid *p_oid)
{
    UNUSED(gid);
    UNUSED(p_oid);

    return 0;
}

/*
 * Unix Test Agent rules configuration tree.
 */

static rcf_pch_cfg_object node_rule;

static rcf_pch_cfg_object node_rule =
    {"rule", 0, NULL, NULL,
     (rcf_ch_cfg_get)NULL, (rcf_ch_cfg_set)NULL,
     (rcf_ch_cfg_add)rule_add, (rcf_ch_cfg_del)rule_del,
     (rcf_ch_cfg_list)rule_list, (rcf_ch_cfg_commit)rule_commit, NULL};

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_rule_init(void)
{
    return rcf_pch_add_node("/agent", &node_rule);
}

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_rule_list(char **list)
{
    te_errno            rc = 0;
    te_string           str = TE_STRING_INIT;
    char               *data;
    netconf_list       *nlist;
    netconf_node       *current;

    if (list == NULL)
    {
        ERROR("%s(): Invalid value for 'list' argument", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* Get IPv4 rules */
    if ((nlist = netconf_rule_dump(nh, AF_INET)) == NULL)
    {
        ERROR("%s(): Cannot get list of rules", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (current = nlist->head; current != NULL; current = current->next)
    {
        const netconf_rule *rule = &(current->data.rule);

        if (rule->family != AF_INET)
        {
            assert(0);
            continue;
        }

        /* Append this rule to the list */

        if (str.ptr && (rc = te_string_append(&str, " ")) != 0)
        {
            ERROR("%s(): Cannot add space to string", __FUNCTION__);
            break;
        }

        rc = te_conf_ip_rule_to_str(rule, &data);
        if (rc != 0)
        {
            ERROR("%s(): Cannot transform struct netconf_rule to char *",
                  __FUNCTION__);
            break;
        }
        rc = te_string_append(&str, "%s", data);
        free(data);

        if (rc != 0)
        {
            ERROR("%s(): Cannot add rule to string", __FUNCTION__);
            break;
        }
    }

    netconf_list_free(nlist);

    if (rc)
    {
        te_string_free(&str);
        return rc;
    }

    *list = strdup(str.ptr);
    te_string_free(&str);

    if (*list == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_rule_find(uint32_t required, te_conf_ip_rule *ip_rule)
{
    te_errno                    rc;
    te_conf_obj_compare_result  cmp;
    netconf_list               *list;
    netconf_node               *current;

    if (ip_rule == NULL)
    {
        ERROR("%s(): Invalid value for 'ip_rule' argument", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((list = netconf_rule_dump(nh, ip_rule->family)) == NULL)
    {
        ERROR("%s(): Cannot get list of rules", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
    for (current = list->head; current != NULL; current = current->next)
    {
        const netconf_rule *rule = &(current->data.rule);

        cmp = te_conf_ip_rule_compare(required, ip_rule, rule);
        if (cmp == TE_CONF_OBJ_COMPARE_EQUAL)
        {
            rc = 0;
            memcpy(ip_rule, rule, sizeof(*ip_rule));
            break;
        }
    }

    netconf_list_free(list);

    return rc;
}

#else
/* See the description in conf_rule.h */
te_errno
ta_unix_conf_rule_init(void)
{
    INFO("Network rule configurations are not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF && !USE_ROUTE_SOCKET */
