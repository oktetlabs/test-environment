/** @file
 * @brief Test API to configure ethernet interface.
 *
 * Definition of API to configure ethernet interface.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Andrey A. Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Ethernet configuration TAPI"

#include "te_config.h"
#include "conf_api.h"
#include "tapi_cfg_eth.h"
#include "logger_api.h"
#include "te_ethernet.h"
#include "tapi_host_ns.h"

#define TE_CFG_TA_ETH_FMT "/agent:%s/interface:%s"

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_feature_get(const char *ta,
                     const char *ifname,
                     const char *feature_name,
                     int        *feature_value_out)
{
    cfg_val_type type = CVT_INTEGER;

    if ((ta == NULL) || (ifname == NULL) ||
        (feature_name == NULL) || (feature_value_out == NULL))
        return TE_EINVAL;

    return cfg_get_instance_sync_fmt(&type, feature_value_out,
                                     TE_CFG_TA_ETH_FMT "/feature:%s",
                                     ta, ifname, feature_name);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_feature_set(const char *ta,
                     const char *ifname,
                     const char *feature_name,
                     int         feature_value)
{
    if ((ta == NULL) || (ifname == NULL) ||
        (feature_name == NULL))
        return TE_EINVAL;

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, feature_value),
                                TE_CFG_TA_ETH_FMT "/feature:%s",
                                ta, ifname, feature_name);
}

/* Context to set a feature value for interface and its parents. */
typedef struct eth_feature_set_ctx {
    const char *name;
    int         value;
    te_bool     success;
} eth_feature_set_ctx;

/**
 * Callback function to set a feature value for interface and its parents.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param opaque    The context (@c eth_feature_set_ctx)
 *
 * @return Status code.
 */
static te_errno
eth_feature_set_cb(const char *ta, const char *ifname, void *opaque)
{
    eth_feature_set_ctx *ctx = (eth_feature_set_ctx *)opaque;
    te_errno rc;

    rc = tapi_eth_feature_set(ta, ifname, ctx->name, ctx->value);
    if (rc == 0)
        ctx->success = TRUE;
    else if (TE_RC_GET_ERROR(rc) != TE_EOPNOTSUPP)
        return rc;

    return tapi_host_ns_if_parent_iter(ta, ifname, &eth_feature_set_cb,
                                       opaque);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_feature_set_all_parents(const char *ta, const char *ifname,
                                 const char *feature_name, int feature_value)
{
    eth_feature_set_ctx ctx = {.name = feature_name, .value = feature_value};
    te_errno            rc;

    rc = eth_feature_set_cb(ta, ifname, &ctx);

    /* Setting of the feature failed with EOPNOTSUPP for all interfaces. */
    if (rc == 0 && ctx.success == FALSE)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return rc;
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_deviceinfo_drivername_get(const char *ta, const char *ifname,
                                   char **drivername)
{
    return cfg_get_instance_fmt(NULL, drivername, TE_CFG_TA_ETH_FMT
                                "/deviceinfo:/drivername:", ta, ifname);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_deviceinfo_driverversion_get(const char *ta, const char *ifname,
                                      char **driverversion)
{
    return cfg_get_instance_fmt(NULL, driverversion, TE_CFG_TA_ETH_FMT
                                "/deviceinfo:/driverversion:", ta, ifname);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_deviceinfo_firmwareversion_get(const char *ta, const char *ifname,
                                        char **firmwareversion)
{
    return cfg_get_instance_fmt(NULL, firmwareversion, TE_CFG_TA_ETH_FMT
                                "/deviceinfo:/firmwareversion:", ta, ifname);
}


/**
 * Get integer value of an interface field
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param field     Field name
 * @param val       The value location
 *
 * @return Status code
 */
static te_errno
tapi_eth_common_get(const char *ta, const char *ifname, const char *field,
                    int *val)
{
    cfg_val_type type = CVT_INTEGER;
    int          rc;

    if ((rc = cfg_get_instance_fmt(&type, val, TE_CFG_TA_ETH_FMT "/%s:",
                                   ta, ifname, field)) != 0)
        ERROR("Failed to get %s value: %r", field, rc);

    return rc;
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_gro_get(const char *ta, const char *ifname, int *gro)
{
    return tapi_eth_common_get(ta, ifname, "gro", gro);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_gso_get(const char *ta, const char *ifname, int *gso)
{
    return tapi_eth_common_get(ta, ifname, "gso", gso);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_tso_get(const char *ta, const char *ifname, int *tso)
{
    return tapi_eth_common_get(ta, ifname, "tso", tso);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_flags_get(const char *ta, const char *ifname, int *flags)
{
    return tapi_eth_common_get(ta, ifname, "flags", flags);
}

/**
 * Set integer value of an interface field
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param field     Field name
 * @param val       The new value
 *
 * @return Status code
 */
static te_errno
tapi_eth_common_set(const char *ta, const char *ifname, const char *field,
                    int val)
{
    te_errno rc;

    if ((rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)(long)val,
                                   TE_CFG_TA_ETH_FMT "/%s:", ta,
                                   ifname, field)) != 0)
        ERROR("Failed to set %s value: %r", field, rc);

    return rc;
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_gro_set(const char *ta, const char *ifname, int gro)
{
    return tapi_eth_common_set(ta, ifname, "gro", gro);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_gso_set(const char *ta, const char *ifname, int gso)
{
    return tapi_eth_common_set(ta, ifname, "gso", gso);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_tso_set(const char *ta, const char *ifname, int tso)
{
    return tapi_eth_common_set(ta, ifname, "tso", tso);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_flags_set(const char *ta, const char *ifname, int flags)
{
    return tapi_eth_common_set(ta, ifname, "flags", flags);
}

/* See description in the tapi_cfg_eth.h */
te_errno
tapi_eth_reset(const char *ta, const char *ifname)
{
    return tapi_eth_common_set(ta, ifname, "reset", 1);
}
