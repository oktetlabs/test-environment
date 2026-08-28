/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief UDP Tunnel (Virtual eXtensible Local Area Network (VXLAN) and
 * GEneric NEtwork Virtualization Encapsulation (Geneve)) interface
 * configuration support
 *
 * Implementation of configuration nodes of VXLAN and Geneve interfaces.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Unix Conf UDP Tunnel"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(USE_LIBNETCONF)

#include "conf_netconf.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"
#include "te_defs.h"
#include "te_alloc.h"
#include "te_string.h"
#include "te_vector.h"
#include "unix_internal.h"
#include "netconf.h"

#include "logger_api.h"

typedef enum udp_tunnel_entry_type {
    UDP_TUNNEL_ENTRY_NONE,
    UDP_TUNNEL_ENTRY_GENEVE,
    UDP_TUNNEL_ENTRY_VXLAN,
} udp_tunnel_entry_type;

typedef struct udp_tunnel_entry {
    SLIST_ENTRY(udp_tunnel_entry)   links;
    bool enabled;
    bool added;
    bool to_be_deleted;
    udp_tunnel_entry_type           type;
    union {
        netconf_geneve             *geneve;
        netconf_vxlan              *vxlan;
    } data;
} udp_tunnel_entry;

static SLIST_HEAD(, udp_tunnel_entry) udp_tunnels =
    SLIST_HEAD_INITIALIZER(udp_tunnels);

static netconf_udp_tunnel *
udp_tunnel_get_generic(const udp_tunnel_entry *udp_tunnel_e)
{
    switch (udp_tunnel_e->type)
    {
        case UDP_TUNNEL_ENTRY_GENEVE:
            return &(udp_tunnel_e->data.geneve->generic);

        case UDP_TUNNEL_ENTRY_VXLAN:
            return &(udp_tunnel_e->data.vxlan->generic);

        default:
            return NULL;
    }
}

static struct udp_tunnel_entry *
udp_tunnel_find(const char *ifname, udp_tunnel_entry_type type)
{
    struct udp_tunnel_entry *p;

    SLIST_FOREACH(p, &udp_tunnels, links)
    {
        if (p->type == type &&
            strcmp(ifname, udp_tunnel_get_generic(p)->ifname) == 0)
            return p;
    }

    return NULL;
}

static bool
udp_tunnel_entry_valid(const udp_tunnel_entry *udp_tunnel_e)
{
    return (udp_tunnel_e != NULL) && !udp_tunnel_e->to_be_deleted;
}

static te_errno
udp_tunnel_commit_core(const char *ifname, udp_tunnel_entry_type type)
{
    udp_tunnel_entry       *udp_tunnel_e;
    const char             *tunnel;
    te_errno                rc = 0;
    void                   *target_data;

    ENTRY("%s", ifname);

    switch (type)
    {
        case UDP_TUNNEL_ENTRY_GENEVE:
            tunnel = "geneve";
            break;

        case UDP_TUNNEL_ENTRY_VXLAN:
            tunnel = "vxlan";
            break;

        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (udp_tunnel_e == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (udp_tunnel_e->enabled)
    {
        if (udp_tunnel_e->added)
        {
            rc = netconf_udp_tunnel_del(nh, ifname);
            if (rc == 0)
            {
                switch (udp_tunnel_e->type)
                {
                    case UDP_TUNNEL_ENTRY_GENEVE:
                        rc = netconf_geneve_add(nh, udp_tunnel_e->data.geneve);
                        break;

                    case UDP_TUNNEL_ENTRY_VXLAN:
                        rc = netconf_vxlan_add(nh, udp_tunnel_e->data.vxlan);
                        break;

                    default:
                        return TE_RC(TE_TA_UNIX, TE_EINVAL);
                }
                if (rc != 0)
                    udp_tunnel_e->added = false;
            }
        }
        else
        {
            switch (udp_tunnel_e->type)
            {
                case UDP_TUNNEL_ENTRY_GENEVE:
                    rc = netconf_geneve_add(nh, udp_tunnel_e->data.geneve);
                    break;

                case UDP_TUNNEL_ENTRY_VXLAN:
                    rc = netconf_vxlan_add(nh, udp_tunnel_e->data.vxlan);
                    break;

                default:
                    return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }
            if (rc == 0)
                udp_tunnel_e->added = true;
        }
    }
    else if (udp_tunnel_e->added)
    {
        rc = netconf_udp_tunnel_del(nh, ifname);
        if (rc == 0)
            udp_tunnel_e->added = false;
    }

    if (udp_tunnel_e->to_be_deleted)
    {
        SLIST_REMOVE(&udp_tunnels, udp_tunnel_e, udp_tunnel_entry, links);

        switch (udp_tunnel_e->type)
        {
            case UDP_TUNNEL_ENTRY_GENEVE:
                target_data = udp_tunnel_e->data.geneve;
                break;

            case UDP_TUNNEL_ENTRY_VXLAN:
                free(udp_tunnel_e->data.vxlan->dev);
                target_data = udp_tunnel_e->data.vxlan;
                break;

            default:
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        free(udp_tunnel_get_generic(udp_tunnel_e)->ifname);
        free(target_data);
        free(udp_tunnel_e);
        return 0;
    }

    VERB("%s: tunnel=%s ifname=%s enabled=%u added=%u rc=%r", __func__, tunnel,
         ifname, udp_tunnel_e->enabled, udp_tunnel_e->added, rc);
    return rc;
}

static te_errno
vxlan_commit(const ta_conf_ctx *ctx)
{
    return udp_tunnel_commit_core(ta_conf_ctx_inst(ctx, "vxlan"),
                                  UDP_TUNNEL_ENTRY_VXLAN);
}

static te_errno
geneve_commit(const ta_conf_ctx *ctx)
{
    return udp_tunnel_commit_core(ta_conf_ctx_inst(ctx, "geneve"),
                                  UDP_TUNNEL_ENTRY_GENEVE);
}

static te_errno
udp_tunnel_generic_init(netconf_udp_tunnel *generic, const char *ifname,
                        uint16_t default_port)
{
    generic->ifname = strdup(ifname);
    if (generic->ifname == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    generic->remote_len = 0;
    generic->port = default_port;

    return 0;
}

/**
 * Add a new UDP Tunnel interface.
 *
 * @param ifname    The interface name
 * @param type      Tunnel type
 * @param val       Whether the tunnel should be enabled (0 or 1)
 *
 * @return      Status code
 */
static te_errno
udp_tunnel_add_core(const char *ifname, udp_tunnel_entry_type type,
                    int32_t val)
{
    udp_tunnel_entry       *udp_tunnel_e;
    uint16_t                default_port;
    void                   *target_data;

    ENTRY("%s", ifname);

    if (udp_tunnel_find(ifname, type) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (val < 0 || val > 1)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    udp_tunnel_e = TE_ALLOC(sizeof(udp_tunnel_entry));

    switch (type)
    {
        case UDP_TUNNEL_ENTRY_GENEVE:
            default_port = 6081;
            udp_tunnel_e->data.geneve = TE_ALLOC(sizeof(netconf_geneve));
            target_data = udp_tunnel_e->data.geneve;

            if (udp_tunnel_generic_init(&(udp_tunnel_e->data.geneve->generic),
                                        ifname, default_port) != 0)
                goto fail_strdup_ifname;

            break;

        case UDP_TUNNEL_ENTRY_VXLAN:
            default_port = 4789;
            udp_tunnel_e->data.vxlan = TE_ALLOC(sizeof(netconf_vxlan));
            target_data = udp_tunnel_e->data.vxlan;

            if (udp_tunnel_generic_init(&(udp_tunnel_e->data.vxlan->generic),
                                        ifname, default_port) != 0)
                goto fail_strdup_ifname;

            udp_tunnel_e->data.vxlan->local_len = 0;
            udp_tunnel_e->data.vxlan->dev = NULL;

            break;

        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    udp_tunnel_e->type = type;
    udp_tunnel_e->enabled = (val == 1);
    udp_tunnel_e->added = false;
    udp_tunnel_e->to_be_deleted = false;
    SLIST_INSERT_HEAD(&udp_tunnels, udp_tunnel_e, links);

    return 0;

fail_strdup_ifname:
    free(target_data);
    free(udp_tunnel_e);

    return TE_RC(TE_TA_UNIX, TE_ENOMEM);
}

static te_errno
vxlan_add(const ta_conf_ctx *ctx, int32_t val)
{
    return udp_tunnel_add_core(ta_conf_ctx_inst(ctx, "vxlan"),
                               UDP_TUNNEL_ENTRY_VXLAN, val);
}

static te_errno
geneve_add(const ta_conf_ctx *ctx, int32_t val)
{
    return udp_tunnel_add_core(ta_conf_ctx_inst(ctx, "geneve"),
                               UDP_TUNNEL_ENTRY_GENEVE, val);
}

/**
 * Delete a UDP Tunnel interface.
 *
 * @param ifname    The interface name
 * @param type      Tunnel type
 *
 * @return      Status code
 */
static te_errno
udp_tunnel_del_core(const char *ifname, udp_tunnel_entry_type type)
{
    udp_tunnel_entry       *udp_tunnel_e;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (udp_tunnel_e == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    udp_tunnel_e->enabled = false;
    udp_tunnel_e->to_be_deleted = true;

    return 0;
}

static te_errno
vxlan_del(const ta_conf_ctx *ctx)
{
    return udp_tunnel_del_core(ta_conf_ctx_inst(ctx, "vxlan"),
                               UDP_TUNNEL_ENTRY_VXLAN);
}

static te_errno
geneve_del(const ta_conf_ctx *ctx)
{
    return udp_tunnel_del_core(ta_conf_ctx_inst(ctx, "geneve"),
                               UDP_TUNNEL_ENTRY_GENEVE);
}

/**
 * Check whether a given interface is grabbed by TA when creating a list of
 * UDP Tunnel interfaces.
 *
 * @param ifname    The interface name.
 * @param data      Unused.
 *
 * @return @c true if the interface is grabbed, @c false otherwise.
 */
static bool
udp_tunnel_list_include_cb(const char *ifname, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname);
}

static te_errno
udp_tunnel_list(te_vec *names, udp_tunnel_entry_type type)
{
    te_errno        rc;
    ENTRY();

    switch (type)
    {
        case UDP_TUNNEL_ENTRY_GENEVE:
            rc = netconf_geneve_list(nh, udp_tunnel_list_include_cb, NULL,
                                     names);
            break;

        case UDP_TUNNEL_ENTRY_VXLAN:
            rc = netconf_vxlan_list(nh, udp_tunnel_list_include_cb, NULL,
                                    names);
            break;

        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (rc == 0)
    {
        struct udp_tunnel_entry    *p;

        SLIST_FOREACH(p, &udp_tunnels, links)
        {
            if (p->type == type && !p->added)
            {
                char *name = TE_STRDUP(udp_tunnel_get_generic(p)->ifname);

                TE_VEC_APPEND(names, name);
            }
        }
    }

    VERB("%s: rc=%r", __func__, rc);
    return rc;
}

/**
 * List Geneve interfaces.
 *
 * @param ctx     Request context (parent instance OID)
 * @param names   Vector of heap-allocated names to append to
 *
 * @return      Status code
 */
static te_errno
geneve_list(const ta_conf_ctx *ctx, te_vec *names)
{
    UNUSED(ctx);

    return udp_tunnel_list(names, UDP_TUNNEL_ENTRY_GENEVE);
}

/**
 * List VXLAN interfaces.
 *
 * @param ctx     Request context (parent instance OID)
 * @param names   Vector of heap-allocated names to append to
 *
 * @return      Status code
 */
static te_errno
vxlan_list(const ta_conf_ctx *ctx, te_vec *names)
{
    UNUSED(ctx);

    return udp_tunnel_list(names, UDP_TUNNEL_ENTRY_VXLAN);
}

static te_errno
udp_tunnel_vni_get_core(const char *ifname, udp_tunnel_entry_type type,
                        int32_t *val)
{
    udp_tunnel_entry       *udp_tunnel_e;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = udp_tunnel_get_generic(udp_tunnel_e)->vni;
    return 0;
}

static te_errno
udp_tunnel_vni_set_core(const char *ifname, udp_tunnel_entry_type type,
                        int32_t val)
{
    udp_tunnel_entry       *udp_tunnel_e;
    uint32_t               *target;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    target = &(udp_tunnel_get_generic(udp_tunnel_e)->vni);

    if (val < 0 || val >= (1U << 24))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *target = val;
    return 0;
}

static te_errno
vxlan_vni_get(const ta_conf_ctx *ctx, int32_t *val)
{
    return udp_tunnel_vni_get_core(ta_conf_ctx_inst(ctx, "vxlan"),
                                   UDP_TUNNEL_ENTRY_VXLAN, val);
}

static te_errno
vxlan_vni_set(const ta_conf_ctx *ctx, int32_t val)
{
    return udp_tunnel_vni_set_core(ta_conf_ctx_inst(ctx, "vxlan"),
                                   UDP_TUNNEL_ENTRY_VXLAN, val);
}

static te_errno
geneve_vni_get(const ta_conf_ctx *ctx, int32_t *val)
{
    return udp_tunnel_vni_get_core(ta_conf_ctx_inst(ctx, "geneve"),
                                   UDP_TUNNEL_ENTRY_GENEVE, val);
}

static te_errno
geneve_vni_set(const ta_conf_ctx *ctx, int32_t val)
{
    return udp_tunnel_vni_set_core(ta_conf_ctx_inst(ctx, "geneve"),
                                   UDP_TUNNEL_ENTRY_GENEVE, val);
}

static te_errno
udp_tunnel_get_addr(char *value, const uint8_t *addr, size_t addr_len)
{
    const char *ret_val = NULL;

    switch (addr_len)
    {
        case sizeof(struct in_addr):
            ret_val = inet_ntop(AF_INET, addr, value, INET_ADDRSTRLEN);
            return (ret_val == NULL) ? TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT) : 0;

        case sizeof(struct in6_addr):
            ret_val = inet_ntop(AF_INET6, addr, value, INET6_ADDRSTRLEN);
            return (ret_val == NULL) ? TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT) : 0;

        case 0:
            value[0] = '\0';
            return 0;

        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
}

static te_errno
udp_tunnel_set_addr(const char *value, uint8_t *addr, size_t *addr_size)
{
    te_errno    rc;

    if (strlen(value) == 0)
        *addr_size = 0;
    else
    {
        rc = inet_pton(AF_INET, value, addr);
        if (rc <= 0)
        {
            rc = inet_pton(AF_INET6, value, addr);
            if (rc <= 0)
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            else
                *addr_size = sizeof(struct in6_addr);
        } else {
            *addr_size = sizeof(struct in_addr);
        }
    }

    return 0;
}

static te_errno
udp_tunnel_remote_get_core(const char *ifname, udp_tunnel_entry_type type,
                           te_string *val)
{
    udp_tunnel_entry       *udp_tunnel_e;
    uint8_t                *target;
    size_t                  target_len;
    char                    buf[RCF_MAX_VAL] = {0};
    te_errno                rc;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    target = udp_tunnel_get_generic(udp_tunnel_e)->remote;
    target_len = udp_tunnel_get_generic(udp_tunnel_e)->remote_len;

    rc = udp_tunnel_get_addr(buf, target, target_len);
    if (rc != 0)
        return rc;

    te_string_append(val, "%s", buf);
    return 0;
}

static te_errno
udp_tunnel_remote_set_core(const char *ifname, udp_tunnel_entry_type type,
                           const char *val)
{
    udp_tunnel_entry       *udp_tunnel_e;
    uint8_t                *target;
    size_t                 *target_len;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    target = udp_tunnel_get_generic(udp_tunnel_e)->remote;
    target_len = &(udp_tunnel_get_generic(udp_tunnel_e)->remote_len);

    return udp_tunnel_set_addr(val, target, target_len);
}

static te_errno
vxlan_remote_get(const ta_conf_ctx *ctx, te_string *val)
{
    return udp_tunnel_remote_get_core(ta_conf_ctx_inst(ctx, "vxlan"),
                                      UDP_TUNNEL_ENTRY_VXLAN, val);
}

static te_errno
vxlan_remote_set(const ta_conf_ctx *ctx, const char *val)
{
    return udp_tunnel_remote_set_core(ta_conf_ctx_inst(ctx, "vxlan"),
                                      UDP_TUNNEL_ENTRY_VXLAN, val);
}

static te_errno
geneve_remote_get(const ta_conf_ctx *ctx, te_string *val)
{
    return udp_tunnel_remote_get_core(ta_conf_ctx_inst(ctx, "geneve"),
                                      UDP_TUNNEL_ENTRY_GENEVE, val);
}

static te_errno
geneve_remote_set(const ta_conf_ctx *ctx, const char *val)
{
    return udp_tunnel_remote_set_core(ta_conf_ctx_inst(ctx, "geneve"),
                                      UDP_TUNNEL_ENTRY_GENEVE, val);
}

static te_errno
vxlan_local_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct udp_tunnel_entry    *udp_tunnel_e;
    const char                 *ifname = ta_conf_ctx_inst(ctx, "vxlan");
    char                        buf[RCF_MAX_VAL] = {0};
    te_errno                    rc;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, UDP_TUNNEL_ENTRY_VXLAN);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = udp_tunnel_get_addr(buf, udp_tunnel_e->data.vxlan->local,
                             udp_tunnel_e->data.vxlan->local_len);
    if (rc != 0)
        return rc;

    te_string_append(val, "%s", buf);
    return 0;
}

static te_errno
vxlan_local_set(const ta_conf_ctx *ctx, const char *val)
{
    struct udp_tunnel_entry    *udp_tunnel_e;
    const char                 *ifname = ta_conf_ctx_inst(ctx, "vxlan");

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, UDP_TUNNEL_ENTRY_VXLAN);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return udp_tunnel_set_addr(val, udp_tunnel_e->data.vxlan->local,
                               &(udp_tunnel_e->data.vxlan->local_len));
}

static te_errno
udp_tunnel_port_get_core(const char *ifname, udp_tunnel_entry_type type,
                         int32_t *val)
{
    udp_tunnel_entry       *udp_tunnel_e;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = udp_tunnel_get_generic(udp_tunnel_e)->port;
    return 0;
}

static te_errno
udp_tunnel_port_set_core(const char *ifname, udp_tunnel_entry_type type,
                         int32_t val)
{
    udp_tunnel_entry       *udp_tunnel_e;
    uint16_t               *target;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    target = &(udp_tunnel_get_generic(udp_tunnel_e)->port);

    if (val < 0 || val >= UINT16_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *target = val;
    return 0;
}

static te_errno
vxlan_port_get(const ta_conf_ctx *ctx, int32_t *val)
{
    return udp_tunnel_port_get_core(ta_conf_ctx_inst(ctx, "vxlan"),
                                    UDP_TUNNEL_ENTRY_VXLAN, val);
}

static te_errno
vxlan_port_set(const ta_conf_ctx *ctx, int32_t val)
{
    return udp_tunnel_port_set_core(ta_conf_ctx_inst(ctx, "vxlan"),
                                    UDP_TUNNEL_ENTRY_VXLAN, val);
}

static te_errno
geneve_port_get(const ta_conf_ctx *ctx, int32_t *val)
{
    return udp_tunnel_port_get_core(ta_conf_ctx_inst(ctx, "geneve"),
                                    UDP_TUNNEL_ENTRY_GENEVE, val);
}

static te_errno
geneve_port_set(const ta_conf_ctx *ctx, int32_t val)
{
    return udp_tunnel_port_set_core(ta_conf_ctx_inst(ctx, "geneve"),
                                    UDP_TUNNEL_ENTRY_GENEVE, val);
}

static te_errno
vxlan_dev_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct udp_tunnel_entry    *udp_tunnel_e;
    const char                 *ifname = ta_conf_ctx_inst(ctx, "vxlan");

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, UDP_TUNNEL_ENTRY_VXLAN);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (udp_tunnel_e->data.vxlan->dev != NULL)
    {
        te_string_append(val, "/agent:%s/interface:%s", ta_name,
                         udp_tunnel_e->data.vxlan->dev);
    }
    return 0;
}

static te_errno
vxlan_dev_set(const ta_conf_ctx *ctx, const char *val)
{
    struct udp_tunnel_entry    *udp_tunnel_e;
    const char                 *ifname = ta_conf_ctx_inst(ctx, "vxlan");
    cfg_oid                    *tmp_oid;
    char                       *tmp_dev;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, UDP_TUNNEL_ENTRY_VXLAN);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (val[0] != '\0')
    {
        if (!rcf_pch_rsrc_accessible(val))
            return TE_RC(TE_TA_UNIX, TE_EINVAL);

        tmp_oid = cfg_convert_oid_str(val);
        if (tmp_oid == NULL)
            return TE_RC(TE_TA_UNIX, TE_EINVAL);

        if (!tmp_oid->inst || tmp_oid->len != 3 ||
            strcmp(CFG_OID_GET_INST_NAME(tmp_oid, 1), ta_name) != 0)
        {
            cfg_free_oid(tmp_oid);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        tmp_dev = strdup(CFG_OID_GET_INST_NAME(tmp_oid, 2));
        cfg_free_oid(tmp_oid);
        if (tmp_dev == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    else
    {
        tmp_dev = NULL;
    }

    free(udp_tunnel_e->data.vxlan->dev);
    udp_tunnel_e->data.vxlan->dev = tmp_dev;
    return 0;
}

static te_errno
udp_tunnel_get_core(const char *ifname, udp_tunnel_entry_type type,
                    int32_t *val)
{
    udp_tunnel_entry   *udp_tunnel_e;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = udp_tunnel_e->enabled;
    return 0;
}

static te_errno
udp_tunnel_set_core(const char *ifname, udp_tunnel_entry_type type,
                    int32_t val)
{
    udp_tunnel_entry   *udp_tunnel_e;

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (val < 0 || val > 1)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    udp_tunnel_e->enabled = val;
    return 0;
}

static te_errno
vxlan_get(const ta_conf_ctx *ctx, int32_t *val)
{
    return udp_tunnel_get_core(ta_conf_ctx_inst(ctx, "vxlan"),
                               UDP_TUNNEL_ENTRY_VXLAN, val);
}

static te_errno
vxlan_set(const ta_conf_ctx *ctx, int32_t val)
{
    return udp_tunnel_set_core(ta_conf_ctx_inst(ctx, "vxlan"),
                               UDP_TUNNEL_ENTRY_VXLAN, val);
}

static te_errno
geneve_get(const ta_conf_ctx *ctx, int32_t *val)
{
    return udp_tunnel_get_core(ta_conf_ctx_inst(ctx, "geneve"),
                               UDP_TUNNEL_ENTRY_GENEVE, val);
}

static te_errno
geneve_set(const ta_conf_ctx *ctx, int32_t val)
{
    return udp_tunnel_set_core(ta_conf_ctx_inst(ctx, "geneve"),
                               UDP_TUNNEL_ENTRY_GENEVE, val);
}

static const ta_conf_node *const node_tunnel =
    TA_CONF_NA("tunnel",
        TA_CONF_COLL_INT32_RW_COMMIT("vxlan", vxlan_get, vxlan_set,
                                     vxlan_add, vxlan_del, vxlan_list,
                                     vxlan_commit,
            TA_CONF_RW_STR("dev", vxlan_dev_get, vxlan_dev_set),
            TA_CONF_RW_INT32("port", vxlan_port_get, vxlan_port_set),
            TA_CONF_RW_STR("local", vxlan_local_get, vxlan_local_set),
            TA_CONF_RW_STR("remote", vxlan_remote_get,
                           vxlan_remote_set),
            TA_CONF_RW_INT32("vni", vxlan_vni_get, vxlan_vni_set)),
        TA_CONF_COLL_INT32_RW_COMMIT("geneve", geneve_get, geneve_set,
                                     geneve_add, geneve_del, geneve_list,
                                     geneve_commit,
            TA_CONF_RW_INT32("port", geneve_port_get, geneve_port_set),
            TA_CONF_RW_STR("remote", geneve_remote_get,
                           geneve_remote_set),
            TA_CONF_RW_INT32("vni", geneve_vni_get, geneve_vni_set)));

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_udp_tunnel_init(void)
{
    return ta_conf_register("/agent", node_tunnel);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_udp_tunnel_init(void)
{
    INFO("UDP Tunnel interfaces configuration is not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF */
