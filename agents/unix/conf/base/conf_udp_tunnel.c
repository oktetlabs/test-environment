/** @file
 * @brief UDP Tunnel (Virtual eXtensible Local Area Network (VXLAN) and
 * GEneric NEtwork Virtualization Encapsulation (Geneve)) interface
 * configuration support
 *
 * Implementation of configuration nodes of VXLAN and Geneve interfaces.
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_LGR_USER     "Unix Conf UDP Tunnel"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(USE_LIBNETCONF)

#include "conf_netconf.h"
#include "rcf_pch.h"
#include "te_defs.h"
#include "te_alloc.h"
#include "te_str.h"
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
    te_bool                         enabled;
    te_bool                         added;
    te_bool                         to_be_deleted;
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

static udp_tunnel_entry_type
udp_tunnel_discover_type(const char *oid)
{
    const cfg_oid  *p_oid = cfg_convert_oid_str(oid);
    const size_t    index = 3;
    const char     *subid;

    if (p_oid != NULL && p_oid->len >= index + 1)
    {
        subid = ((cfg_inst_subid *)(p_oid->ids))[index].subid;
        if (strcmp(subid, "geneve") == 0)
            return UDP_TUNNEL_ENTRY_GENEVE;
        if (strcmp(subid, "vxlan") == 0)
            return UDP_TUNNEL_ENTRY_VXLAN;
    }

    ERROR("Failed to discover UDP Tunnel type of oid %s", oid);
    return UDP_TUNNEL_ENTRY_NONE;
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

static te_bool
udp_tunnel_entry_valid(const udp_tunnel_entry *udp_tunnel_e)
{
    return (udp_tunnel_e != NULL) && !udp_tunnel_e->to_be_deleted;
}

static te_errno
udp_tunnel_commit(unsigned int gid, const cfg_oid *p_oid)
{
    udp_tunnel_entry       *udp_tunnel_e;
    const char             *ifname;
    char                   *oid;
    udp_tunnel_entry_type   type;
    const char             *tunnel;
    te_errno                rc = 0;
    void                   *target_data;

    UNUSED(gid);

    ifname = ((cfg_inst_subid *)(p_oid->ids))[p_oid->len - 1].name;

    oid = cfg_convert_oid(p_oid);
    type = udp_tunnel_discover_type(oid);
    free(oid);

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
                    udp_tunnel_e->added = FALSE;
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
                udp_tunnel_e->added = TRUE;
        }
    }
    else if (udp_tunnel_e->added)
    {
        rc = netconf_udp_tunnel_del(nh, ifname);
        if (rc == 0)
            udp_tunnel_e->added = FALSE;
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
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param ifname    The inteface name
 *
 * @return      Status code
 */
static te_errno
udp_tunnel_add(unsigned int gid, const char *oid, const char *value,
               const char *tunnelname, const char *ifname)
{
    udp_tunnel_entry       *udp_tunnel_e;
    udp_tunnel_entry_type   type = udp_tunnel_discover_type(oid);
    unsigned int            uint_value;
    uint16_t                default_port;
    void                   *target_data;
    te_errno                rc = 0;

    UNUSED(gid);
    UNUSED(tunnelname);

    ENTRY("%s", ifname);

    if (udp_tunnel_find(ifname, type) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    rc = te_strtoui(value, 0, &uint_value);
    if (rc != 0)
        return rc;
    if (uint_value > 1)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    udp_tunnel_e = TE_ALLOC(sizeof(udp_tunnel_entry));
    if (udp_tunnel_e == NULL)
        goto fail_alloc_udp_tunnel_entry;

    switch (type)
    {
        case UDP_TUNNEL_ENTRY_GENEVE:
            default_port = 6081;
            target_data = udp_tunnel_e->data.geneve;
            udp_tunnel_e->data.geneve = TE_ALLOC(sizeof(netconf_geneve));
            if (udp_tunnel_e->data.geneve == NULL)
                goto fail_alloc_data;

            if (udp_tunnel_generic_init(&(udp_tunnel_e->data.geneve->generic),
                                        ifname, default_port) != 0)
                goto fail_strdup_ifname;

            break;

        case UDP_TUNNEL_ENTRY_VXLAN:
            default_port = 4789;
            target_data = udp_tunnel_e->data.vxlan;
            udp_tunnel_e->data.vxlan = TE_ALLOC(sizeof(netconf_vxlan));
            if (udp_tunnel_e->data.vxlan == NULL)
                goto fail_alloc_data;

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
    udp_tunnel_e->enabled = (uint_value == 1);
    udp_tunnel_e->added = FALSE;
    udp_tunnel_e->to_be_deleted = FALSE;
    SLIST_INSERT_HEAD(&udp_tunnels, udp_tunnel_e, links);

    return 0;

fail_strdup_ifname:
    free(target_data);

fail_alloc_data:
    free(udp_tunnel_e);

fail_alloc_udp_tunnel_entry:
    return TE_RC(TE_TA_UNIX, TE_ENOMEM);
}

/**
 * Delete a UDP Tunnel interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param ifname    The interface name
 *
 * @return      Status code
 */
static te_errno
udp_tunnel_del(unsigned int gid, const char *oid, const char *tunnelname,
               const char *ifname)
{
    udp_tunnel_entry       *udp_tunnel_e;
    udp_tunnel_entry_type   type = udp_tunnel_discover_type(oid);

    UNUSED(gid);
    UNUSED(tunnelname);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (udp_tunnel_e == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    udp_tunnel_e->enabled = FALSE;
    udp_tunnel_e->to_be_deleted = TRUE;

    return 0;
}

/**
 * Check whether a given interface is grabbed by TA when creating a list of
 * UDP Tunnel interfaces.
 *
 * @param ifname    The interface name.
 * @param data      Unused.
 *
 * @return @c TRUE if the interface is grabbed, @c FALSE otherwise.
 */
static te_bool
udp_tunnel_list_include_cb(const char *ifname, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname);
}

static te_errno
udp_tunnel_list(char **list, udp_tunnel_entry_type type)
{
    te_errno        rc;
    ENTRY();

    switch (type)
    {
        case UDP_TUNNEL_ENTRY_GENEVE:
            rc = netconf_geneve_list(nh, udp_tunnel_list_include_cb, NULL, list);
            break;

        case UDP_TUNNEL_ENTRY_VXLAN:
            rc = netconf_vxlan_list(nh, udp_tunnel_list_include_cb, NULL, list);
            break;

        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (rc == 0)
    {
        struct udp_tunnel_entry    *p;
        te_string                   str;

        memset(&str, 0, sizeof(str));
        str.ptr = *list;
        str.len = (*list == NULL) ? 0 : strlen(*list);
        str.size = str.len + 1;
        SLIST_FOREACH(p, &udp_tunnels, links)
        {
            if (p->type == type && !p->added)
            {
                rc = te_string_append(&str, "%s ",
                                      udp_tunnel_get_generic(p)->ifname);
                if (rc != 0)
                {
                    te_string_free(&str);
                    return rc;
                }
            }
        }
        *list = str.ptr;
    }

    VERB("%s: rc=%r list=%s", __func__, rc, rc == 0 ? *list : "");
    return rc;
}

/**
 * Get Geneve interfaces list.
 *
 * @param gid     Group identifier (unused)
 * @param oid     Full identifier of the father instance (unused)
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    Location for the list pointer
 *
 * @return      Status code
 */
static te_errno
geneve_list(unsigned int gid, const char *oid,
            const char *sub_id, char **list)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    return udp_tunnel_list(list, UDP_TUNNEL_ENTRY_GENEVE);
}

/**
 * Get VXLAN interfaces list.
 *
 * @param gid     Group identifier (unused)
 * @param oid     Full identifier of the father instance (unused)
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    Location for the list pointer
 *
 * @return      Status code
 */
static te_errno
vxlan_list(unsigned int gid, const char *oid,
           const char *sub_id, char **list)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    return udp_tunnel_list(list, UDP_TUNNEL_ENTRY_VXLAN);
}

static te_errno
udp_tunnel_vni_get(unsigned int gid, const char *oid, char *value,
                   const char *tunnel, const char *ifname)
{
    udp_tunnel_entry       *udp_tunnel_e;
    udp_tunnel_entry_type   type = udp_tunnel_discover_type(oid);
    uint32_t                vni;

    UNUSED(gid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vni = udp_tunnel_get_generic(udp_tunnel_e)->vni;

    sprintf(value, "%u", vni);
    return 0;
}

static te_errno
udp_tunnel_vni_set(unsigned int gid, const char *oid, char *value,
                   const char *tunnel, const char *ifname)
{
    udp_tunnel_entry       *udp_tunnel_e;
    udp_tunnel_entry_type   type = udp_tunnel_discover_type(oid);
    uint32_t                vni;
    uint32_t               *target;
    te_errno                rc = 0;

    UNUSED(gid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    target = &(udp_tunnel_get_generic(udp_tunnel_e)->vni);

    rc = te_strtoui(value, 0, &vni);
    if (rc != 0)
        return rc;
    if (vni >= (1U << 24))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *target = vni;
    return 0;
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
udp_tunnel_remote_get(unsigned int gid, const char *oid, char *value,
                      const char *tunnel, const char *ifname)
{
    udp_tunnel_entry       *udp_tunnel_e;
    udp_tunnel_entry_type   type = udp_tunnel_discover_type(oid);
    uint8_t                *target;
    size_t                  target_len;

    UNUSED(gid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    target = udp_tunnel_get_generic(udp_tunnel_e)->remote;
    target_len = udp_tunnel_get_generic(udp_tunnel_e)->remote_len;

    return udp_tunnel_get_addr(value, target, target_len);
}

static te_errno
udp_tunnel_remote_set(unsigned int gid, const char *oid, const char *value,
                      const char *tunnel, const char *ifname)
{
    udp_tunnel_entry       *udp_tunnel_e;
    udp_tunnel_entry_type   type = udp_tunnel_discover_type(oid);
    uint8_t                *target;
    size_t                 *target_len;

    UNUSED(gid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    target = udp_tunnel_get_generic(udp_tunnel_e)->remote;
    target_len = &(udp_tunnel_get_generic(udp_tunnel_e)->remote_len);

    return udp_tunnel_set_addr(value, target, target_len);
}

static te_errno
vxlan_local_get(unsigned int gid, const char *oid, char *value,
                const char *tunnel, const char *ifname)
{
    struct udp_tunnel_entry    *udp_tunnel_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, UDP_TUNNEL_ENTRY_VXLAN);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return udp_tunnel_get_addr(value, udp_tunnel_e->data.vxlan->local,
                               udp_tunnel_e->data.vxlan->local_len);
}

static te_errno
vxlan_local_set(unsigned int gid, const char *oid, const char *value,
                const char *tunnel, const char *ifname)
{
    struct udp_tunnel_entry    *udp_tunnel_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, UDP_TUNNEL_ENTRY_VXLAN);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return udp_tunnel_set_addr(value, udp_tunnel_e->data.vxlan->local,
                               &(udp_tunnel_e->data.vxlan->local_len));
}

static te_errno
udp_tunnel_port_get(unsigned int gid, const char *oid, char *value,
                    const char *tunnel, const char *ifname)
{
    udp_tunnel_entry       *udp_tunnel_e;
    udp_tunnel_entry_type   type = udp_tunnel_discover_type(oid);
    uint16_t                port;

    UNUSED(gid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    port = udp_tunnel_get_generic(udp_tunnel_e)->port;

    sprintf(value, "%u", port);
    return 0;
}

static te_errno
udp_tunnel_port_set(unsigned int gid, const char *oid, const char *value,
                    const char *tunnel, const char *ifname)
{
    udp_tunnel_entry       *udp_tunnel_e;
    udp_tunnel_entry_type   type = udp_tunnel_discover_type(oid);
    unsigned int            port;
    uint16_t               *target;
    te_errno                rc = 0;

    UNUSED(gid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, type);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    target = &(udp_tunnel_get_generic(udp_tunnel_e)->port);

    rc = te_strtoui(value, 0, &port);
    if (rc != 0)
        return rc;
    if (port >= UINT16_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *target = port;
    return 0;
}

static te_errno
vxlan_dev_get(unsigned int gid, const char *oid, char *value,
              const char *tunnel, const char *ifname)
{
    struct udp_tunnel_entry    *udp_tunnel_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, UDP_TUNNEL_ENTRY_VXLAN);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (udp_tunnel_e->data.vxlan->dev != NULL)
    {
        snprintf(value, RCF_MAX_VAL, "/agent:%s/interface:%s", ta_name,
                 udp_tunnel_e->data.vxlan->dev);
    }
    else
    {
        value[0] = '\0';
    }
    return 0;
}

static te_errno
vxlan_dev_set(unsigned int gid, const char *oid, const char *value,
              const char *tunnel, const char *ifname)
{
    struct udp_tunnel_entry    *udp_tunnel_e;
    cfg_oid                    *tmp_oid;
    char                       *tmp_dev;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, UDP_TUNNEL_ENTRY_VXLAN);
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (value[0] != '\0')
    {
        if (!rcf_pch_rsrc_accessible(value))
            return TE_RC(TE_TA_UNIX, TE_EINVAL);

        tmp_oid = cfg_convert_oid_str(value);
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
udp_tunnel_get(unsigned int gid, const char *oid, char *value,
               const char *tunnel, const char *ifname)
{
    udp_tunnel_entry   *udp_tunnel_e;

    UNUSED(gid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, udp_tunnel_discover_type(oid));
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%u", udp_tunnel_e->enabled);
    return 0;
}

static te_errno
udp_tunnel_set(unsigned int gid, const char *oid, const char *value,
               const char *tunnel, const char *ifname)
{
    udp_tunnel_entry   *udp_tunnel_e;
    uint32_t            enabled;
    te_errno            rc = 0;

    UNUSED(gid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    udp_tunnel_e = udp_tunnel_find(ifname, udp_tunnel_discover_type(oid));
    if (!udp_tunnel_entry_valid(udp_tunnel_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoui(value, 0, &enabled);
    if (rc != 0)
        return rc;
    if (enabled > 1)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    udp_tunnel_e->enabled = enabled;
    return 0;
}

RCF_PCH_CFG_NODE_RW(node_geneve_vni, "vni", NULL, NULL,
                    udp_tunnel_vni_get, udp_tunnel_vni_set);

RCF_PCH_CFG_NODE_RW(node_geneve_remote, "remote", NULL, &node_geneve_vni,
                    udp_tunnel_remote_get, udp_tunnel_remote_set);

RCF_PCH_CFG_NODE_RW(node_geneve_port, "port", NULL, &node_geneve_remote,
                    udp_tunnel_port_get, udp_tunnel_port_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_geneve, "geneve", &node_geneve_port, NULL,
                               udp_tunnel_get, udp_tunnel_set, udp_tunnel_add,
                               udp_tunnel_del, geneve_list, udp_tunnel_commit);

RCF_PCH_CFG_NODE_RW(node_vxlan_vni, "vni", NULL, NULL,
                    udp_tunnel_vni_get, udp_tunnel_vni_set);

RCF_PCH_CFG_NODE_RW(node_vxlan_remote, "remote", NULL, &node_vxlan_vni,
                    udp_tunnel_remote_get, udp_tunnel_remote_set);

RCF_PCH_CFG_NODE_RW(node_vxlan_local, "local", NULL, &node_vxlan_remote,
                    vxlan_local_get, vxlan_local_set);

RCF_PCH_CFG_NODE_RW(node_vxlan_port, "port", NULL, &node_vxlan_local,
                    udp_tunnel_port_get, udp_tunnel_port_set);

RCF_PCH_CFG_NODE_RW(node_vxlan_dev, "dev", NULL, &node_vxlan_port,
                    vxlan_dev_get, vxlan_dev_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_vxlan, "vxlan", &node_vxlan_dev,
                               &node_geneve, udp_tunnel_get, udp_tunnel_set,
                               udp_tunnel_add, udp_tunnel_del, vxlan_list,
                               udp_tunnel_commit);

RCF_PCH_CFG_NODE_NA(node_tunnel, "tunnel", &node_vxlan, NULL);

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_udp_tunnel_init(void)
{
    return rcf_pch_add_node("/agent", &node_tunnel);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_udp_tunnel_init(void)
{
    INFO("UDP Tunnel interfaces configuration is not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF */

