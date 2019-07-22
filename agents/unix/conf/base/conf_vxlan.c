/** @file
 * @brief Virtual eXtensible Local Area Network (vxlan)
 * interface configuration support
 *
 * Implementation of configuration nodes VXLAN interfaces.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Ian Dolzhansky <Ian.Dolzhansky@oktetlabs.ru>
 */

#define TE_LGR_USER     "Unix Conf VXLAN"

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
        if (strcmp(ifname, p->vxlan->ifname) == 0)
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
            rc = netconf_vxlan_del(nh, vxlan_e->vxlan->ifname);
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
        rc = netconf_vxlan_del(nh, vxlan_e->vxlan->ifname);
        if (rc == 0)
            vxlan_e->added = FALSE;
    }

    if (vxlan_e->to_be_deleted)
    {
        SLIST_REMOVE(&vxlans, vxlan_e, vxlan_entry, links);

        free(vxlan_e->vxlan->ifname);
        free(vxlan_e->vxlan);
        free(vxlan_e);

        return 0;
    }

    VERB("%s: ifname=%s enabled=%u added=%u rc=%r", __func__,
         vxlan_e->vxlan->ifname, vxlan_e->enabled, vxlan_e->added, rc);
    return rc;
}

/**
 * Add a new vxlan interface.
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

    vxlan_e->vxlan->ifname = strdup(ifname);
    if (vxlan_e->vxlan->ifname == NULL)
        goto fail_strdup_ifname;

    vxlan_e->vxlan->remote_len = 0;
    vxlan_e->vxlan->local_len = 0;

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
 * Delete a vxlan interface.
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
 * vxlan interfaces.
 *
 * @param ifname    The interface name.
 * @param data      Unused.
 *
 * @return @c TRUE if the interface is grabbed, @c FALSE otherwise.
 */
static te_bool
vxlan_list_include_cb(const char *ifname, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname);
}

/**
 * Get vxlan interfaces list.
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
    te_errno    rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    ENTRY();

    rc = netconf_vxlan_list(nh, vxlan_list_include_cb, NULL, list);

    if (rc == 0)
    {
        struct vxlan_entry *p;
        te_string           str;

        memset(&str, 0, sizeof(str));
        str.ptr = *list;
        str.len = (*list == NULL) ? 0 : strlen(*list);
        str.size = str.len;
        SLIST_FOREACH(p, &vxlans, links)
        {
            if (!p->added)
                te_string_append(&str, "%s ", p->vxlan->ifname);
        }
        *list = str.ptr;
    }

    VERB("%s: rc=%r list=%s", __func__, rc, rc == 0 ? *list : "");
    return rc;
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

    return vxlan_set_addr(value, vxlan_e->vxlan->remote, &(vxlan_e->vxlan->remote_len));
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

    return vxlan_set_addr(value, vxlan_e->vxlan->local, &(vxlan_e->vxlan->local_len));
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

RCF_PCH_CFG_NODE_RW_COLLECTION(node_vxlan, "vxlan", &node_vxlan_local, NULL, vxlan_get, vxlan_set,
                               vxlan_add, vxlan_del, vxlan_list, vxlan_commit);

RCF_PCH_CFG_NODE_NA(node_tunnel, "tunnel", &node_vxlan, NULL);

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_vxlan_init(void)
{
    return rcf_pch_add_node("/agent", &node_tunnel);
}

#else /* USE_LIBNETCONF */
te_errno
ta_unix_conf_vxlan_init(void)
{
    INFO("VXLAN interface configuration is not supported");
    return 0;
}
#endif /* !USE_LIBNETCONF */

