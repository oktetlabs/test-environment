/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */

#include "te_alloc.h"
#include "te_config.h"
#include "te_defs.h"
#include "te_enum.h"
#include "te_str.h"

#include "conf_api.h"
#include "logger_api.h"

#include "tapi_cfg_base.h"
#include "tapi_cfg_tunnel.h"
#include "tapi_cfg.h"

#define TE_CFG_TA_TNL           "/agent:%s/tunnel:/%s:%s"

static const te_enum_map tapi_cfg_tunnel_types[] = {
    { .name = "vxlan", .value = TAPI_CFG_TUNNEL_TYPE_VXLAN },
    TE_ENUM_MAP_END
};

static const char *
tapi_cfg_tunnel_type2str(tapi_cfg_tunnel_type type)
{
    return te_enum_map_from_value(tapi_cfg_tunnel_types, type);
}

static te_errno
cfg_tunnel_if_add(const char *tunnel_oid)
{
    return cfg_add_instance_fmt(NULL, CFG_VAL(INT32, 0), "%s", tunnel_oid);
}

static te_errno
cfg_tunnel_if_del(const char *tunnel_oid)
{
    return cfg_del_instance_fmt(false, "%s", tunnel_oid);
}

static te_errno
cfg_tunnel_status_set(const char *tunnel_oid, bool status)
{
    const int32_t s = status ? 1 : 0;

    return cfg_set_instance_fmt(CFG_VAL(INT32, s), "%s", tunnel_oid);
}

static te_errno
cfg_tunnel_status_get(const char *tunnel_oid, bool *status)
{
    int32_t s;
    te_errno rc;

    rc = cfg_get_int32(&s, "%s", tunnel_oid);
    if (rc != 0)
        return rc;

    *status = !!s;

    return 0;
}

static te_errno
cfg_tunnel_vni_set(const char *tunnel_oid, int32_t vni)
{
    return cfg_set_instance_fmt(CFG_VAL(INT32, vni), "%s/vni:", tunnel_oid);
}

static te_errno
cfg_tunnel_vni_get(const char *tunnel_oid, int32_t *vni)
{
    return cfg_get_int32(vni, "%s/vni:", tunnel_oid);
}

static te_errno
cfg_tunnel_remote_set(const char *tunnel_oid, const struct sockaddr *remote)
{
    if (remote == NULL)
        return 0;

    return cfg_set_instance_fmt(CFG_VAL(ADDRESS, remote),
                                "%s/remote:", tunnel_oid);
}

static te_errno
cfg_tunnel_remote_get(const char *tunnel_oid, struct sockaddr **remote)
{
    return cfg_get_addr(remote, "%s/remote:", tunnel_oid);
}

static te_errno
cfg_tunnel_local_set(const char *tunnel_oid, const struct sockaddr *local)
{
    if (local == NULL)
        return 0;

    return cfg_set_instance_fmt(CFG_VAL(ADDRESS, local),
                                "%s/local:", tunnel_oid);
}

static te_errno
cfg_tunnel_local_get(const char *tunnel_oid, struct sockaddr **local)
{
    return cfg_get_addr(local, "%s/local:", tunnel_oid);
}

static te_errno
cfg_tunnel_port_set(const char *tunnel_oid, uint16_t port)
{
    const int32_t p = port;

    if (port == 0)
        return 0;

    return cfg_set_instance_fmt(CFG_VAL(INT32, p), "%s/port:", tunnel_oid);
}

static te_errno
cfg_tunnel_port_get(const char *tunnel_oid, uint16_t *port)
{
    int32_t p;
    te_errno rc;

    rc = cfg_get_int32(&p, "%s/port:", tunnel_oid);
    if (rc != 0)
        return rc;

    if (p < 0 || p > UINT16_MAX)
        return TE_RC(TE_TAPI, TE_EINVAL);

    *port = (uint16_t)p;

    return 0;
}

static te_errno
cfg_tunnel_dev_set(const char *tunnel_oid, const char *ta, const char *if_name)
{
    char if_oid[CFG_OID_MAX];

    if (if_name == NULL)
        return 0;

    TE_SPRINTF(if_oid, "/agent:%s/interface:%s", ta, if_name);

    return cfg_set_instance_fmt(CFG_VAL(STRING, if_oid), "%s/dev:", tunnel_oid);
}

static te_errno
cfg_tunnel_dev_get(const char *tunnel_oid, char **if_name)
{
    char *oid;
    te_errno rc;

    rc = cfg_get_string(&oid, "%s/dev:", tunnel_oid);
    if (rc != 0)
        return rc;

    rc = cfg_get_ith_inst_name(oid, 2, if_name);

    free(oid);

    return rc;
}

static te_errno
tapi_cfg_tunnel_set_status(const char *ta, const tapi_cfg_tunnel *conf,
                           bool status)
{
    char tunnel_oid[CFG_OID_MAX];

    if (ta == NULL || conf == NULL || conf->tunnel_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    TE_SPRINTF(tunnel_oid, TE_CFG_TA_TNL,
               ta, tapi_cfg_tunnel_type2str(conf->type), conf->tunnel_name);

    return cfg_tunnel_status_set(tunnel_oid, status);
}

static bool
cfg_tunnel_exist(const char *ta, const char *name, const char *type)
{
    cfg_handle hdl;

    return (cfg_find_fmt(&hdl, TE_CFG_TA_TNL, ta, type, name) == 0);
}

static te_errno
cfg_tunnel_type_get(const char *ta, const char *tunnel_name,
                    tapi_cfg_tunnel_type *tunnel_type)
{
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(tapi_cfg_tunnel_types); i++)
    {
        const te_enum_map *type = &tapi_cfg_tunnel_types[i];

        if (cfg_tunnel_exist(ta, tunnel_name, type->name))
        {
            *tunnel_type = type->value;
            return 0;
        }
    }

    return TE_RC(TE_TAPI, TE_EINVAL);
}

static te_errno
tapi_cfg_tunnel_vxlan_setup(const char *ta, const char *vxlan_oid,
                            const tapi_cfg_tunnel_vxlan *conf)
{
    te_errno rc;

    rc = cfg_tunnel_vni_set(vxlan_oid, conf->vni);
    if (rc != 0)
    {
        ERROR("Failed to set VxLAN VNI: %r", rc);
        return rc;
    }

    rc = cfg_tunnel_local_set(vxlan_oid, conf->local);
    if (rc != 0)
    {
        ERROR("Failed to set VxLAN local: %r", rc);
        return rc;
    }

    rc = cfg_tunnel_port_set(vxlan_oid, conf->port);
    if (rc != 0)
    {
        ERROR("Failed to set VxLAN port: %r", rc);
        return rc;
    }

    rc = cfg_tunnel_remote_set(vxlan_oid, conf->remote);
    if (rc != 0)
    {
        ERROR("Failed to set VxLAN remote: %r", rc);
        return rc;
    }

    rc = cfg_tunnel_dev_set(vxlan_oid, ta, conf->if_name);
    if (rc != 0)
    {
        ERROR("Failed to set VxLAN device: %r", rc);
        return rc;
    }

    return 0;
}

static te_errno
tapi_cfg_tunnel_vxlan_get(const char *vxlan_oid, tapi_cfg_tunnel_vxlan *conf)
{
    te_errno rc;

    rc = cfg_tunnel_dev_get(vxlan_oid, &conf->if_name);
    if (rc != 0)
    {
        ERROR("Failed to get VxLAN device: %r", rc);
        return rc;
    }

    rc = cfg_tunnel_vni_get(vxlan_oid, &conf->vni);
    if (rc != 0)
    {
        ERROR("Failed to get VxLAN VNI: %r", rc);
        return rc;
    }

    rc = cfg_tunnel_remote_get(vxlan_oid, &conf->remote);
    if (rc != 0)
    {
        ERROR("Failed to get VxLAN remote: %r", rc);
        return rc;
    }

    rc = cfg_tunnel_local_get(vxlan_oid, &conf->local);
    if (rc != 0)
    {
        ERROR("Failed to get VxLAN local: %r", rc);
        return rc;
    }

    rc = cfg_tunnel_port_get(vxlan_oid, &conf->port);
    if (rc != 0)
    {
        ERROR("Failed to get VxLAN port: %r", rc);
        return rc;
    }

    return 0;
}

/* See description in tapi_cfg_tunnel.h */
te_errno
tapi_cfg_tunnel_add(const char *ta, const tapi_cfg_tunnel *conf)
{
    char oid[CFG_OID_MAX];
    te_errno rc;

    if (ta == NULL || conf == NULL || conf->tunnel_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    TE_SPRINTF(oid, TE_CFG_TA_TNL,
               ta, tapi_cfg_tunnel_type2str(conf->type), conf->tunnel_name);

    rc = tapi_cfg_base_if_add_rsrc(ta, conf->tunnel_name);
    if (rc != 0)
    {
        ERROR("Failed to add TA resources: %r", rc);
        return rc;
    }

    rc = cfg_tunnel_if_add(oid);
    if (rc != 0)
    {
        ERROR("Failed to add tunnel: %r", rc);
        goto err_if_add;
    }

    switch (conf->type)
    {
        case TAPI_CFG_TUNNEL_TYPE_VXLAN:
            rc = tapi_cfg_tunnel_vxlan_setup(ta, oid, &conf->vxlan);
            break;

        default:
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            break;
    }

    if (rc != 0)
    {
        ERROR("Failed to setup new tunnel: %r", rc);
        goto err_setup;
    }

    if (conf->status)
    {
        rc = tapi_cfg_tunnel_enable(ta, conf);
        if (rc != 0)
        {
            ERROR("Failed to up the tunnel: %r", rc);
            goto err_up;
        }
    }

    return 0;

err_up:
err_setup:
    cfg_tunnel_if_del(oid);
err_if_add:
    tapi_cfg_base_if_del_rsrc(ta, conf->tunnel_name);

    return rc;
}

/* See description in tapi_cfg_tunnel.h */
te_errno
tapi_cfg_tunnel_enable(const char *ta, const tapi_cfg_tunnel *conf)
{
    return tapi_cfg_tunnel_set_status(ta, conf, true);
}

/* See description in tapi_cfg_tunnel.h */
te_errno
tapi_cfg_tunnel_disable(const char *ta, const tapi_cfg_tunnel *conf)
{
    return tapi_cfg_tunnel_set_status(ta, conf, false);
}

/* See description in tapi_cfg_tunnel.h */
te_errno
tapi_cfg_tunnel_del(const char *ta, const char *tunnel_name)
{
    char oid[CFG_OID_MAX];
    tapi_cfg_tunnel_type tunnel_type;
    te_errno rc;

    if (ta == NULL || tunnel_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_tunnel_type_get(ta, tunnel_name, &tunnel_type);
    if (rc != 0)
    {
        ERROR("Failed to get tunnel '%s' type: %r", tunnel_name, rc);
        return rc;
    }

    TE_SPRINTF(oid, TE_CFG_TA_TNL,
               ta, tapi_cfg_tunnel_type2str(tunnel_type), tunnel_name);

    rc = cfg_tunnel_if_del(oid);
    if (rc != 0)
    {
        ERROR("Failed to remove tunnel: %r", rc);
        return rc;
    }

    rc = tapi_cfg_base_if_del_rsrc(ta, tunnel_name);
    if (rc != 0)
    {
        ERROR("Failed to remove TA resources: %r", rc);
        return rc;
    }

    return 0;
}

/* See description in tapi_cfg_tunnel.h */
te_errno
tapi_cfg_tunnel_get(const char *ta, const char *tunnel_name,
                    tapi_cfg_tunnel *conf)
{
    char oid[CFG_OID_MAX];
    te_errno rc;

    if (ta == NULL || conf == NULL || tunnel_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_tunnel_type_get(ta, tunnel_name, &conf->type);
    if (rc != 0)
    {
        ERROR("Failed to get tunnel '%s' type: %r", tunnel_name, rc);
        return rc;
    }

    TE_SPRINTF(oid, TE_CFG_TA_TNL,
               ta, tapi_cfg_tunnel_type2str(conf->type), tunnel_name);

    rc = cfg_tunnel_status_get(oid, &conf->status);
    if (rc != 0)
    {
        ERROR("Failed to get tunnel '%s' status: %r", tunnel_name, rc);
        return rc;
    }

    switch (conf->type)
    {
        case TAPI_CFG_TUNNEL_TYPE_VXLAN:
            rc = tapi_cfg_tunnel_vxlan_get(oid, &conf->vxlan);
            break;

        default:
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            break;
    }

    if (rc == 0)
        conf->tunnel_name = TE_STRDUP(tunnel_name);

    return rc;
}
