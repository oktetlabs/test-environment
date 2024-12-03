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
#include "tapi_cfg_bridge.h"
#include "tapi_cfg.h"

#define TE_CFG_TA_BRIDGE        "/agent:%s/bridge:%s"
#define TE_CFG_TA_BRIDGE_PORT   TE_CFG_TA_BRIDGE "/port:%s"

static const te_enum_map tapi_cfg_bridge_providers[] = {
    { .name = "", .value = TAPI_CFG_BRIDGE_PROVIDER_DEFAULT },
    TE_ENUM_MAP_END
};

static const char *
tapi_cfg_bridge_provider2str(tapi_cfg_bridge_provider in)
{
    return te_enum_map_from_value(tapi_cfg_bridge_providers, in);
}

static te_errno
tapi_cfg_bridge_str2provider(const char *in, tapi_cfg_bridge_provider *out)
{
    tapi_cfg_bridge_provider label;

    label = te_enum_map_from_str(tapi_cfg_bridge_providers, in, INT_MAX);
    if (label == INT_MAX)
        return TE_RC(TE_TAPI, TE_ERANGE);

    *out = label;

    return 0;
}

static te_errno
cfg_bridge_provider_get(const char *bridge_oid,
                        tapi_cfg_bridge_provider *provider)
{
    char *oid = NULL;
    char *provider_str = NULL;
    te_errno rc;

    rc = cfg_get_string(&oid, "%s", bridge_oid);
    if (rc != 0)
        goto out;

    rc = cfg_get_ith_inst_name(oid, 1, &provider_str);
    if (rc != 0)
        goto out;

    rc = tapi_cfg_bridge_str2provider(provider_str, provider);
    if (rc != 0)
        goto out;

out:
    free(oid);
    free(provider_str);

    return rc;
}

/* See description in tapi_cfg_bridge.h */
te_errno
tapi_cfg_bridge_add(const char *ta, const tapi_cfg_bridge *conf)
{
    const char *provider;
    te_errno rc;

    if (ta == NULL || conf == NULL || conf->bridge_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = tapi_cfg_base_if_add_rsrc(ta, conf->bridge_name);
    if (rc != 0)
    {
        ERROR("Failed to add TA resources: %r", rc);
        return rc;
    }

    provider = tapi_cfg_bridge_provider2str(conf->provider);
    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, provider),
                              TE_CFG_TA_BRIDGE, ta, conf->bridge_name);
    if (rc != 0)
    {
        ERROR("Failed to add bridge: %r", rc);
        goto err_if_add;
    }

    return 0;

err_if_add:
    tapi_cfg_base_if_del_rsrc(ta, conf->bridge_name);

    return rc;
}

/* See description in tapi_cfg_bridge.h */
te_errno
tapi_cfg_bridge_del(const char *ta, const char *bridge_name)
{
    te_errno rc;

    if (ta == NULL || bridge_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_del_instance_fmt(true, TE_CFG_TA_BRIDGE, ta, bridge_name);
    if (rc != 0)
    {
        ERROR("Failed to remove bridge: %r", rc);
        return rc;
    }

    rc = tapi_cfg_base_if_del_rsrc(ta, bridge_name);
    if (rc != 0)
    {
        ERROR("Failed to remove TA resources: %r", rc);
        return rc;
    }

    return 0;
}

/* See description in tapi_cfg_bridge.h */
te_errno
tapi_cfg_bridge_get(const char *ta, const char *bridge_name,
                    tapi_cfg_bridge *conf)
{
    char bridge_oid[CFG_OID_MAX];
    te_errno rc;

    if (ta == NULL || bridge_name == NULL || conf == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    TE_SPRINTF(bridge_oid, TE_CFG_TA_BRIDGE, ta, bridge_name);
    rc = cfg_bridge_provider_get(bridge_oid, &conf->provider);
    if (rc != 0)
    {
        ERROR("Failed to get bridge provider: %r", rc);
        return rc;
    }

    conf->bridge_name = TE_STRDUP(bridge_name);

    return 0;
}

/* See description in tapi_cfg_bridge.h */
te_errno
tapi_cfg_bridge_port_add(const char *ta, const char *bridge_name,
                         const char *if_name)
{
    char if_oid[CFG_OID_MAX];
    te_errno rc;

    if (ta == NULL || bridge_name == NULL || if_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_get_instance_string_fmt(NULL, "/agent:%s/rsrc:%s",
                                     ta, bridge_name);
    if (rc != 0)
        return rc;

    TE_SPRINTF(if_oid, "/agent:%s/interface:%s", ta, if_name);
    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, if_oid),
                              TE_CFG_TA_BRIDGE_PORT, ta, bridge_name, if_name);
    if (rc != 0)
    {
        ERROR("Failed to add bridge interface: %r", rc);
        return rc;
    }

    return 0;
}

/* See description in tapi_cfg_bridge.h */
te_errno
tapi_cfg_bridge_port_del(const char *ta, const char *bridge_name,
                         const char *if_name)
{
    te_errno rc;

    if (ta == NULL || bridge_name == NULL || if_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_get_instance_string_fmt(NULL, "/agent:%s/rsrc:%s",
                                     ta, bridge_name);
    if (rc != 0)
        return rc;

    rc = cfg_del_instance_fmt(true, TE_CFG_TA_BRIDGE_PORT,
                              ta, bridge_name, if_name);
    if (rc != 0)
    {
        ERROR("Failed to remove bridge interface: %r", rc);
        return rc;
    }

    return 0;
}
