/** @file
 * @brief UDP Tunnel (Virtual eXtensible Local Area Network (VXLAN)) interface
 * configuration support
 *
 * Implementation of configuration nodes of VXLAN interfaces.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Ian Dolzhansky <Ian.Dolzhansky@oktetlabs.ru>
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
    UDP_TUNNEL_ENTRY_VXLAN,
} udp_tunnel_entry_type;

typedef struct vxlan_entry {
    SLIST_ENTRY(vxlan_entry)    links;
    te_bool                     enabled;
    te_bool                     added;
    te_bool                     to_be_deleted;
    netconf_vxlan              *vxlan;
} vxlan_entry;

static SLIST_HEAD(, vxlan_entry) vxlans = SLIST_HEAD_INITIALIZER(vxlans);

static struct vxlan_entry *
vxlan_find(const char *ifname)
{
    struct vxlan_entry *p;

    SLIST_FOREACH(p, &vxlans, links)
    {
        if (strcmp(ifname, p->vxlan->generic.ifname) == 0)
            return p;
    }

    return NULL;
}

static te_bool
vxlan_entry_valid(const struct vxlan_entry *vxlan_e)
{
    return (vxlan_e != NULL) && !vxlan_e->to_be_deleted;
}

static te_errno
vxlan_commit(unsigned int gid, const cfg_oid *p_oid)
{
    struct vxlan_entry *vxlan_e;
    const char         *ifname;
    te_errno            rc = 0;

    UNUSED(gid);

    ifname = ((cfg_inst_subid *)(p_oid->ids))[p_oid->len - 1].name;
    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (vxlan_e == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vxlan_e->enabled)
    {
        if (vxlan_e->added)
        {
            rc = netconf_vxlan_del(nh, vxlan_e->vxlan->generic.ifname);
            if (rc == 0)
            {
                rc = netconf_vxlan_add(nh, vxlan_e->vxlan);
                if (rc != 0)
                    vxlan_e->added = FALSE;
            }
        }
        else
        {
            rc = netconf_vxlan_add(nh, vxlan_e->vxlan);
            if (rc == 0)
                vxlan_e->added = TRUE;
        }
    }
    else if (vxlan_e->added)
    {
        rc = netconf_vxlan_del(nh, vxlan_e->vxlan->generic.ifname);
        if (rc == 0)
            vxlan_e->added = FALSE;
    }

    if (vxlan_e->to_be_deleted)
    {
        SLIST_REMOVE(&vxlans, vxlan_e, vxlan_entry, links);

        free(vxlan_e->vxlan->generic.ifname);
        free(vxlan_e->vxlan->dev);
        free(vxlan_e->vxlan);
        free(vxlan_e);

        return 0;
    }

    VERB("%s: ifname=%s enabled=%u added=%u rc=%r", __func__,
         vxlan_e->vxlan->generic.ifname, vxlan_e->enabled, vxlan_e->added, rc);
    return rc;
}

/**
 * Add a new VXLAN interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param ifname    The inteface name
 *
 * @return      Status code
 */
static te_errno
vxlan_add(unsigned int gid, const char *oid, const char *value,
          const char *tunnelname, const char *ifname)
{
    struct vxlan_entry *vxlan_e;
    unsigned int        uint_value;
    uint16_t            default_port = 4789;
    te_errno            rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnelname);

    ENTRY("%s", ifname);

    if (vxlan_find(ifname) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    rc = te_strtoui(value, 0, &uint_value);
    if (rc != 0)
        return rc;
    if (uint_value > 1)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vxlan_e = TE_ALLOC(sizeof(vxlan_entry));
    if (vxlan_e == NULL)
        goto fail_alloc_vxlan_entry;

    vxlan_e->vxlan = TE_ALLOC(sizeof(netconf_vxlan));
    if (vxlan_e->vxlan == NULL)
        goto fail_alloc_vxlan;

    vxlan_e->vxlan->generic.ifname = strdup(ifname);
    if (vxlan_e->vxlan->generic.ifname == NULL)
        goto fail_strdup_ifname;

    vxlan_e->vxlan->remote_len = 0;
    vxlan_e->vxlan->local_len = 0;
    vxlan_e->vxlan->port = default_port;
    vxlan_e->vxlan->dev = NULL;

    vxlan_e->enabled = (uint_value == 1);
    vxlan_e->added = FALSE;
    vxlan_e->to_be_deleted = FALSE;
    SLIST_INSERT_HEAD(&vxlans, vxlan_e, links);

    return 0;

fail_strdup_ifname:
    free(vxlan_e->vxlan);

fail_alloc_vxlan:
    free(vxlan_e);

fail_alloc_vxlan_entry:
    return TE_RC(TE_TA_UNIX, TE_ENOMEM);
}

/**
 * Delete a VXLAN interface.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param ifname    The interface name
 *
 * @return      Status code
 */
static te_errno
vxlan_del(unsigned int gid, const char *oid, const char *tunnelname,
          const char *ifname)
{
    struct vxlan_entry *vxlan_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnelname);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (vxlan_e == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vxlan_e->enabled = FALSE;
    vxlan_e->to_be_deleted = TRUE;

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
        case UDP_TUNNEL_ENTRY_VXLAN:
            rc = netconf_vxlan_list(nh, udp_tunnel_list_include_cb, NULL, list);
            break;

        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (rc == 0)
    {
        struct vxlan_entry *p;
        te_string           str;

        memset(&str, 0, sizeof(str));
        str.ptr = *list;
        str.len = (*list == NULL) ? 0 : strlen(*list);
        str.size = str.len + 1;
        SLIST_FOREACH(p, &vxlans, links)
        {
            if (!p->added)
                te_string_append(&str, "%s ", p->vxlan->generic.ifname);
        }
        *list = str.ptr;
    }

    VERB("%s: rc=%r list=%s", __func__, rc, rc == 0 ? *list : "");
    return rc;
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
vxlan_vni_get(unsigned int gid, const char *oid, char *value,
              const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%u", vxlan_e->vxlan->vni);

    return 0;
}

static te_errno
vxlan_vni_set(unsigned int gid, const char *oid, char *value,
              const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;
    unsigned int        vni;
    te_errno            rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoui(value, 0, &vni);
    if (rc != 0)
        return rc;
    if (vni >= (1U << 24))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vxlan_e->vxlan->vni = vni;

    return 0;
}

static te_errno
vxlan_get_addr(char *value, const uint8_t *addr, size_t addr_len)
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
vxlan_set_addr(const char *value, uint8_t *addr, size_t *addr_size)
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
vxlan_remote_get(unsigned int gid, const char *oid, char *value,
                 const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return vxlan_get_addr(value, vxlan_e->vxlan->remote,
                          vxlan_e->vxlan->remote_len);
}

static te_errno
vxlan_remote_set(unsigned int gid, const char *oid, const char *value,
                 const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return vxlan_set_addr(value, vxlan_e->vxlan->remote,
                          &(vxlan_e->vxlan->remote_len));
}

static te_errno
vxlan_local_get(unsigned int gid, const char *oid, char *value,
                const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return vxlan_get_addr(value, vxlan_e->vxlan->local,
                          vxlan_e->vxlan->local_len);
}

static te_errno
vxlan_local_set(unsigned int gid, const char *oid, const char *value,
                const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return vxlan_set_addr(value, vxlan_e->vxlan->local,
                          &(vxlan_e->vxlan->local_len));
}

static te_errno
vxlan_port_get(unsigned int gid, const char *oid, char *value,
               const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%u", vxlan_e->vxlan->port);

    return 0;
}

static te_errno
vxlan_port_set(unsigned int gid, const char *oid, const char *value,
               const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;
    unsigned int        port;
    te_errno            rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoui(value, 0, &port);
    if (rc != 0)
        return rc;
    if (port >= (1U << 16))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vxlan_e->vxlan->port = port;

    return 0;
}

static te_errno
vxlan_dev_get(unsigned int gid, const char *oid, char *value,
              const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "/agent:%s/interface:%s", ta_name,
             vxlan_e->vxlan->dev);

    return 0;
}

static te_errno
vxlan_dev_set(unsigned int gid, const char *oid, const char *value,
              const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;
    cfg_oid            *tmp_oid;
    char               *tmp_dev;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (!rcf_pch_rsrc_accessible(value))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    tmp_oid = cfg_convert_oid_str(value);
    if (tmp_oid == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (!tmp_oid->inst || tmp_oid->len != 2 ||
        strcmp(CFG_OID_GET_INST_NAME(tmp_oid, 0), ta_name) != 0)
    {
        cfg_free_oid(tmp_oid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    tmp_dev = strdup(CFG_OID_GET_INST_NAME(tmp_oid, 1));
    cfg_free_oid(tmp_oid);
    if (tmp_dev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    free(vxlan_e->vxlan->dev);
    vxlan_e->vxlan->dev = tmp_dev;

    return 0;
}

static te_errno
vxlan_get(unsigned int gid, const char *oid, char *value,
          const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%u", vxlan_e->enabled);

    return 0;
}

static te_errno
vxlan_set(unsigned int gid, const char *oid, char *value,
          const char *tunnel, const char *ifname)
{
    struct vxlan_entry *vxlan_e;
    unsigned int        enabled;
    te_errno            rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tunnel);

    ENTRY("%s", ifname);

    vxlan_e = vxlan_find(ifname);
    if (!vxlan_entry_valid(vxlan_e))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoui(value, 0, &enabled);
    if (rc != 0)
        return rc;
    if (enabled > 1)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vxlan_e->enabled = enabled;

    return 0;
}

RCF_PCH_CFG_NODE_RW(node_vxlan_vni, "vni", NULL, NULL,
                    vxlan_vni_get, vxlan_vni_set);

RCF_PCH_CFG_NODE_RW(node_vxlan_remote, "remote", NULL, &node_vxlan_vni,
                    vxlan_remote_get, vxlan_remote_set);

RCF_PCH_CFG_NODE_RW(node_vxlan_local, "local", NULL, &node_vxlan_remote,
                    vxlan_local_get, vxlan_local_set);

RCF_PCH_CFG_NODE_RW(node_vxlan_port, "port", NULL, &node_vxlan_local,
                    vxlan_port_get, vxlan_port_set);

RCF_PCH_CFG_NODE_RW(node_vxlan_dev, "dev", NULL, &node_vxlan_port,
                    vxlan_dev_get, vxlan_dev_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_vxlan, "vxlan", &node_vxlan_dev,
                               NULL, vxlan_get, vxlan_set, vxlan_add,
                               vxlan_del, vxlan_list, vxlan_commit);

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

