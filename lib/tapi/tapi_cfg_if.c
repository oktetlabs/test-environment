/** @file
 * @brief Test API to configure network interface.
 *
 * Definition of API to configure network interface.
 *
 * Copyright (C) 2004-2021 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrey A. Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#define TE_LGR_USER     "Interface configuration TAPI"

#include "te_config.h"
#include "conf_api.h"
#include "tapi_cfg_if.h"
#include "logger_api.h"
#include "te_ethernet.h"
#include "tapi_host_ns.h"

#define TE_CFG_TA_IF_FMT "/agent:%s/interface:%s"

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_feature_is_readonly(const char *ta,
                                const char *ifname,
                                const char *feature_name,
                                te_bool *readonly)
{
    int val;
    te_errno rc;

    if (ta == NULL || ifname == NULL ||
        feature_name == NULL || readonly == NULL)
        return TE_EINVAL;

    rc = cfg_get_instance_int_sync_fmt(&val,
                                       TE_CFG_TA_IF_FMT "/feature:%s/readonly:",
                                       ta, ifname, feature_name);
    if (rc != 0)
        return rc;

    *readonly = (val == 0 ? FALSE : TRUE);
    return 0;
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_feature_is_present(const char *ta,
                               const char *ifname,
                               const char *feature_name,
                               te_bool *present)
{
    te_errno rc;
    unsigned int p_num = 0;
    cfg_handle *p_set = NULL;

    rc = cfg_find_pattern_fmt(&p_num, &p_set,
                              TE_CFG_TA_IF_FMT
                              "/feature:%s",
                              ta, ifname, feature_name);
    if (rc != 0)
        return rc;

    free(p_set);

    if (p_num > 0)
        *present = TRUE;
    else
        *present = FALSE;

    return 0;
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_feature_get(const char *ta,
                        const char *ifname,
                        const char *feature_name,
                        int        *feature_value_out)
{
    if ((ta == NULL) || (ifname == NULL) ||
        (feature_name == NULL) || (feature_value_out == NULL))
        return TE_EINVAL;

    return cfg_get_instance_int_sync_fmt(feature_value_out,
                                         TE_CFG_TA_IF_FMT "/feature:%s",
                                         ta, ifname, feature_name);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_feature_set(const char *ta,
                        const char *ifname,
                        const char *feature_name,
                        int         feature_value)
{
    if ((ta == NULL) || (ifname == NULL) ||
        (feature_name == NULL))
        return TE_EINVAL;

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, feature_value),
                                TE_CFG_TA_IF_FMT "/feature:%s",
                                ta, ifname, feature_name);
}

/* Context to set a feature value for interface and its parents. */
typedef struct if_feature_set_ctx {
    const char *name;
    int         value;
    te_bool     success;
} if_feature_set_ctx;

/**
 * Callback function to set a feature value for interface and its parents.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param opaque    The context (@c if_feature_set_ctx)
 *
 * @return Status code.
 */
static te_errno
if_feature_set_cb(const char *ta, const char *ifname, void *opaque)
{
    if_feature_set_ctx *ctx = (if_feature_set_ctx *)opaque;
    te_errno rc;
    te_bool readonly;
    int value;

    rc = tapi_cfg_if_feature_is_readonly(ta, ifname, ctx->name,
                                         &readonly);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) != TE_EOPNOTSUPP)
            return rc;
    }
    else if (readonly)
    {
        rc = tapi_cfg_if_feature_get(ta, ifname, ctx->name,
                                     &value);
        if (rc != 0)
            return rc;

        if (value == ctx->value)
            ctx->success = TRUE;
    }
    else
    {
        rc = tapi_cfg_if_feature_set(ta, ifname, ctx->name, ctx->value);
        if (rc == 0)
            ctx->success = TRUE;
        else if (TE_RC_GET_ERROR(rc) != TE_EOPNOTSUPP)
            return rc;
    }

    return tapi_host_ns_if_parent_iter(ta, ifname, &if_feature_set_cb,
                                       opaque);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_feature_set_all_parents(const char *ta, const char *ifname,
                                    const char *feature_name,
                                    int feature_value)
{
    if_feature_set_ctx ctx = {.name = feature_name, .value = feature_value};
    te_errno            rc;

    rc = if_feature_set_cb(ta, ifname, &ctx);

    /*
     * Setting of the feature failed with EOPNOTSUPP or or it was
     * read-only for all interfaces.
     */
    if (rc == 0 && ctx.success == FALSE)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return rc;
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_deviceinfo_drivername_get(const char *ta, const char *ifname,
                                      char **drivername)
{
    return cfg_get_instance_fmt(NULL, drivername, TE_CFG_TA_IF_FMT
                                "/deviceinfo:/drivername:", ta, ifname);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_deviceinfo_driverversion_get(const char *ta, const char *ifname,
                                         char **driverversion)
{
    return cfg_get_instance_fmt(NULL, driverversion, TE_CFG_TA_IF_FMT
                                "/deviceinfo:/driverversion:", ta, ifname);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_deviceinfo_firmwareversion_get(const char *ta, const char *ifname,
                                           char **firmwareversion)
{
    return cfg_get_instance_fmt(NULL, firmwareversion, TE_CFG_TA_IF_FMT
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
tapi_cfg_if_common_get(const char *ta, const char *ifname, const char *field,
                       int *val)
{
    int rc;

    if ((rc = cfg_get_instance_int_fmt(val, TE_CFG_TA_IF_FMT "/%s:",
                                       ta, ifname, field)) != 0)
        ERROR("Failed to get %s value: %r", field, rc);

    return rc;
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_gro_get(const char *ta, const char *ifname, int *gro)
{
    return tapi_cfg_if_common_get(ta, ifname, "gro", gro);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_gso_get(const char *ta, const char *ifname, int *gso)
{
    return tapi_cfg_if_common_get(ta, ifname, "gso", gso);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_tso_get(const char *ta, const char *ifname, int *tso)
{
    return tapi_cfg_if_common_get(ta, ifname, "tso", tso);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_flags_get(const char *ta, const char *ifname, int *flags)
{
    return tapi_cfg_if_common_get(ta, ifname, "flags", flags);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_get_ring_size(const char *ta, const char *ifname,
                          bool is_rx, int *ring_size)
{
    const char *path;

    path = is_rx ? "ring:/rx:/current" : "ring:/tx:/current";

    return tapi_cfg_if_common_get(ta, ifname, path, ring_size);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_get_max_ring_size(const char *ta, const char *ifname,
                              bool is_rx, int *max_ring_size)
{
    const char *path;

    path = is_rx ? "ring:/rx:/max" : "ring:/tx:/max";

    return tapi_cfg_if_common_get(ta, ifname, path, max_ring_size);
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
tapi_cfg_if_common_set(const char *ta, const char *ifname,
                       const char *field, int val)
{
    te_errno rc;

    if ((rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)(long)val,
                                   TE_CFG_TA_IF_FMT "/%s:", ta,
                                   ifname, field)) != 0)
        ERROR("Failed to set %s value: %r", field, rc);

    return rc;
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_gro_set(const char *ta, const char *ifname, int gro)
{
    return tapi_cfg_if_common_set(ta, ifname, "gro", gro);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_gso_set(const char *ta, const char *ifname, int gso)
{
    return tapi_cfg_if_common_set(ta, ifname, "gso", gso);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_tso_set(const char *ta, const char *ifname, int tso)
{
    return tapi_cfg_if_common_set(ta, ifname, "tso", tso);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_flags_set(const char *ta, const char *ifname, int flags)
{
    return tapi_cfg_if_common_set(ta, ifname, "flags", flags);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_set_ring_size(const char *ta, const char *ifname,
                          bool is_rx, int ring_size)
{
    const char *path;

    path = is_rx ? "ring:/rx:/current" : "ring:/tx:/current";

    return tapi_cfg_if_common_set(ta, ifname, path, ring_size);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_reset(const char *ta, const char *ifname)
{
    return tapi_cfg_if_common_set(ta, ifname, "reset", 1);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_msglvl_get(const char *ta, const char *ifname, uint64_t *msglvl)
{
    return cfg_get_instance_uint64_fmt(msglvl, "/agent:%s/interface:%s/msglvl:",
                                       ta, ifname);
}

/* See description in the tapi_cfg_if.h */
te_errno
tapi_cfg_if_msglvl_set(const char *ta, const char *ifname, uint64_t msglvl)
{
    return cfg_set_instance_fmt(CFG_VAL(UINT64, msglvl),
                                "/agent:%s/interface:%s/msglvl:",
                                ta, ifname);
}
