/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Port support
 *
 * Port configuration tree support
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Port"

#include "te_config.h"
#include "config.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "conf_oid.h"
#include "logger_api.h"
#include "te_str.h"
#include "rcf_pch.h"
#include "agentlib.h"
#include "te_vector.h"

static int socket_family = 0;
static int socket_type = 0;
static te_vec allocated_ports = TE_VEC_INIT(uint16_t);
static te_bool allocate_on_get = TRUE;
static int32_t last_allocated_port = -1;
static te_bool allocate_property_changed = FALSE;

static int *
l4_port_alloc_property_ptr_by_oid(const char *oid)
{
    cfg_oid *coid = cfg_convert_oid_str(oid);
    int *result = NULL;
    char *prop_subid;

    if (coid == NULL)
        goto exit;

    prop_subid = cfg_oid_inst_subid(coid, 5);
    if (prop_subid == NULL)
        goto exit;

    if (strcmp(prop_subid, "family") == 0)
        result = &socket_family;
    else if (strcmp(prop_subid, "type") == 0)
        result = &socket_type;

exit:
    cfg_free_oid(coid);

    if (result == NULL)
        ERROR("Failed to get property by oid '%s'", oid);

    return result;
}

static te_errno
l4_port_alloc_property_set(unsigned int gid, const char *oid, const char *value)
{
    int *property;
    int property_value;

    UNUSED(gid);

    property = l4_port_alloc_property_ptr_by_oid(oid);
    if (property == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (te_strtoi(value, 0, &property_value) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (*property != property_value)
        allocate_property_changed = TRUE;

    *property = property_value;

    return 0;
}

static te_errno
l4_port_alloc_property_get(unsigned int gid, const char *oid, char *value)
{
    int *property;

    UNUSED(gid);

    property = l4_port_alloc_property_ptr_by_oid(oid);
    if (property == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_snprintf(value, RCF_MAX_VAL, "%d", *property);

    return 0;
}

static te_errno
l4_port_alloc_next_get(unsigned int gid, const char *oid, char *value)
{
    te_bool realloc_last_port;
    uint16_t port;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    realloc_last_port = !allocate_on_get && allocate_property_changed &&
                        !agent_check_l4_port_is_free(socket_family, socket_type,
                                                     last_allocated_port);

    if (realloc_last_port)
        agent_free_l4_port(last_allocated_port);

    if (allocate_on_get || realloc_last_port)
    {
        rc = agent_alloc_l4_port(socket_family, socket_type, &port);
        if (rc != 0)
            return rc;

        last_allocated_port = port;
    }

    allocate_property_changed = FALSE;
    allocate_on_get = FALSE;
    te_snprintf(value, RCF_MAX_VAL, "%d", last_allocated_port);

    return 0;
}

static int
l4_port_allocated_find(uint16_t port)
{
    size_t i;

    for (i = 0; i < te_vec_size(&allocated_ports); i++)
    {
        if (TE_VEC_GET(uint16_t, &allocated_ports, i) == port)
            return i;
    }

    return -1;
}

static te_errno
l4_port_allocated_add(unsigned int gid, const char *oid, const char *value,
                      const char *empty1, const char *empty2,
                      const char *port_str)
{
    unsigned int port_val;
    uint16_t port;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(empty1);
    UNUSED(empty2);

    rc = te_strtoui(port_str, 0, &port_val);
    if (rc != 0)
        return rc;

    if (port_val > UINT16_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    port = port_val;

    if ((int32_t)port != last_allocated_port)
    {
        if (agent_alloc_l4_specified_port(socket_type, socket_family,
                                          port) != 0)
        {
            ERROR("Failed to add a new port");
            return TE_RC(TE_TA_UNIX, TE_EPERM);
        }
    }

    if (l4_port_allocated_find(port) >= 0)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    rc = TE_VEC_APPEND(&allocated_ports, port);
    if (rc == 0)
        allocate_on_get = TRUE;

    return rc;
}

static te_errno
l4_port_allocated_del(unsigned int gid, const char *oid, const char *empty1,
                      const char *empty2, const char *port_str)
{
    unsigned int port;
    te_errno rc;
    int index;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(empty1);
    UNUSED(empty2);

    rc = te_strtoui(port_str, 0, &port);
    if (rc != 0)
        return rc;

    index = l4_port_allocated_find(port);
    if (index < 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_vec_remove_index(&allocated_ports, index);
    agent_free_l4_port(port);

    return 0;
}

static te_errno
l4_port_allocated_list(unsigned int gid, const char *oid, const char *empty1,
                       char **list)
{
    te_string result = TE_STRING_INIT;
    te_errno rc;
    uint16_t *p;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(empty1);

    TE_VEC_FOREACH(&allocated_ports, p)
    {
        rc = te_string_append(&result, "%u ", *p);
        if (rc != 0)
        {
            te_string_free(&result);
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }
    }

    *list = result.ptr;

    return 0;
}

RCF_PCH_CFG_NODE_COLLECTION(node_port_allocated, "allocated",
                    NULL, NULL,
                    l4_port_allocated_add, l4_port_allocated_del,
                    l4_port_allocated_list, NULL);
RCF_PCH_CFG_NODE_RW(node_port_alloc_type, "type",
                    NULL, NULL,
                    l4_port_alloc_property_get, l4_port_alloc_property_set);
RCF_PCH_CFG_NODE_RW(node_port_alloc_family, "family",
                    NULL, &node_port_alloc_type,
                    l4_port_alloc_property_get, l4_port_alloc_property_set);
RCF_PCH_CFG_NODE_RW(node_port_alloc_next, "next",
                    &node_port_alloc_family, &node_port_allocated,
                    l4_port_alloc_next_get, NULL);

RCF_PCH_CFG_NODE_NA(node_port_alloc, "alloc", &node_port_alloc_next, NULL);
RCF_PCH_CFG_NODE_NA(node_port, "l4_port", &node_port_alloc, NULL);


te_errno
ta_unix_conf_l4_port_init()
{
    return rcf_pch_add_node("/agent", &node_port);
}
