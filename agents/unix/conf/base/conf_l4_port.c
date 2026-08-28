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
#include "logger_api.h"
#include "te_str.h"
#include "rcf_pch_tree.h"
#include "agentlib.h"
#include "te_vector.h"

static int socket_family = 0;
static int socket_type = 0;
static te_vec allocated_ports = TE_VEC_INIT(uint16_t);
static bool allocate_on_get = true;
static int32_t last_allocated_port = -1;
static bool allocate_property_changed = false;

/**
 * Update an allocation property (socket family or socket type), tracking
 * whether the effective value actually changed.
 *
 * @param property      Pointer to the property to update
 * @param value         New value, already parsed with base 0
 *
 * @return              Status code
 */
static te_errno
l4_port_alloc_property_set(int *property, int32_t value)
{
    int property_value;

    if (value < 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    property_value = (int)value;

    if (*property != property_value)
        allocate_property_changed = true;

    *property = property_value;

    return 0;
}

static te_errno
l4_port_alloc_family_get(const ta_conf_ctx *ctx, int32_t *val)
{
    UNUSED(ctx);
    *val = socket_family;
    return 0;
}

static te_errno
l4_port_alloc_family_set(const ta_conf_ctx *ctx, int32_t val)
{
    UNUSED(ctx);
    return l4_port_alloc_property_set(&socket_family, val);
}

static te_errno
l4_port_alloc_type_get(const ta_conf_ctx *ctx, int32_t *val)
{
    UNUSED(ctx);
    *val = socket_type;
    return 0;
}

static te_errno
l4_port_alloc_type_set(const ta_conf_ctx *ctx, int32_t val)
{
    UNUSED(ctx);
    return l4_port_alloc_property_set(&socket_type, val);
}

static te_errno
l4_port_alloc_next_get(const ta_conf_ctx *ctx, int32_t *val)
{
    bool realloc_last_port;
    uint16_t port;
    te_errno rc;

    UNUSED(ctx);

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

    allocate_property_changed = false;
    allocate_on_get = false;
    *val = last_allocated_port;

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
l4_port_allocated_add(const ta_conf_ctx *ctx)
{
    const char *port_str = ta_conf_ctx_inst(ctx, "allocated");
    unsigned int port_val;
    uint16_t port;
    te_errno rc;

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
        allocate_on_get = true;

    return rc;
}

static te_errno
l4_port_allocated_del(const ta_conf_ctx *ctx)
{
    const char *port_str = ta_conf_ctx_inst(ctx, "allocated");
    unsigned int port;
    te_errno rc;
    int index;

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
l4_port_allocated_list(const ta_conf_ctx *ctx, te_vec *names)
{
    uint16_t *p;

    UNUSED(ctx);

    TE_VEC_FOREACH(&allocated_ports, p)
    {
        char *name = te_string_fmt("%u", *p);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static const ta_conf_node *const node_port =
    TA_CONF_NA("l4_port",
        TA_CONF_NA("alloc",
            TA_CONF_RO_INT32("next", l4_port_alloc_next_get,
                TA_CONF_RW_INT32("family", l4_port_alloc_family_get,
                                 l4_port_alloc_family_set),
                TA_CONF_RW_INT32("type", l4_port_alloc_type_get,
                                 l4_port_alloc_type_set)),
            TA_CONF_COLL("allocated", l4_port_allocated_add,
                         l4_port_allocated_del,
                         l4_port_allocated_list)));


te_errno
ta_unix_conf_l4_port_init()
{
    return ta_conf_register("/agent", node_port);
}
