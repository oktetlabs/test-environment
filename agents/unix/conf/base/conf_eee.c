/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Energy Efficient Ethernet
 *
 * Unix TA Network Interface Energy Efficient Ethernet settings
 */

#define TE_LGR_USER     "Conf EEE"

#include "te_config.h"
#include "config.h"

#include <limits.h>

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "rcf_pch_tree.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

#include "conf_ethtool.h"

#ifdef ETHTOOL_GEEE

/** Fields in ethtool_eee structure */
typedef enum eee_field {
    /** eee_active field */
    FIELD_EEE_ACTIVE,
    /** eee_enabled field */
    FIELD_EEE_ENABLED,
    /** tx_lpi_enabled field */
    FIELD_TX_LPI_ENABLED,
    /** tx_lpi_timer field */
    FIELD_TX_LPI_TIMER,
} eee_field;

/* Get pointer to field of ethtool_eee structure. */
 static uint32_t *
get_field_ptr(struct ethtool_eee *eptr, eee_field field)
{
    switch (field)
    {
        case FIELD_EEE_ACTIVE:
            return &eptr->eee_active;
        case FIELD_EEE_ENABLED:
            return &eptr->eee_enabled;
        case FIELD_TX_LPI_ENABLED:
            return &eptr->tx_lpi_enabled;
        case FIELD_TX_LPI_TIMER:
            return &eptr->tx_lpi_timer;
    }

    return NULL;
}

/* Common code for getting field value */
static te_errno
common_field_get(const ta_conf_ctx *ctx, uintmax_t *val, eee_field field)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_eee *eptr;
    uint32_t *field_ptr;
    te_errno rc;

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx), TA_ETHTOOL_EEE,
                           (void **)&eptr);
    if (rc != 0)
    {
        if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
        {
            /*
             * Avoid Configurator errors if EEE is not supported.
             */
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        return rc;
    }

    field_ptr = get_field_ptr(eptr, field);
    if (field_ptr == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = *field_ptr;
    return 0;
}

/* Common code for setting field value */
static te_errno
common_param_set(const ta_conf_ctx *ctx, uintmax_t val, eee_field field)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_eee *eptr;
    uint32_t *field_ptr;
    te_errno rc;

    if (val > UINT_MAX)
    {
        ERROR("%s(): too big value '%ju'", __FUNCTION__, val);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx), TA_ETHTOOL_EEE,
                           (void **)&eptr);
    if (rc != 0)
        return rc;

    field_ptr = get_field_ptr(eptr, field);
    if (field_ptr == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *field_ptr = val;
    return 0;
}

/* Get eee_active field */
static te_errno
eee_active_get(const ta_conf_ctx *ctx, bool *val)
{
    uintmax_t raw;
    te_errno rc = common_field_get(ctx, &raw, FIELD_EEE_ACTIVE);

    if (rc != 0)
        return rc;
    if (raw > 1)
        return TE_RC(TE_TA_UNIX, TE_ERANGE);
    *val = raw != 0;
    return 0;
}

/* Get eee_enabled field */
static te_errno
eee_enabled_get(const ta_conf_ctx *ctx, bool *val)
{
    uintmax_t raw;
    te_errno rc = common_field_get(ctx, &raw, FIELD_EEE_ENABLED);

    if (rc != 0)
        return rc;
    if (raw > 1)
        return TE_RC(TE_TA_UNIX, TE_ERANGE);
    *val = raw != 0;
    return 0;
}

/* Get tx_lpi_enabled field */
static te_errno
tx_lpi_enabled_get(const ta_conf_ctx *ctx, uint64_t *val)
{
    return common_field_get(ctx, val, FIELD_TX_LPI_ENABLED);
}

/* Get tx_lpi_timer field */
static te_errno
tx_lpi_timer_get(const ta_conf_ctx *ctx, uint32_t *val)
{
    uintmax_t raw;
    te_errno rc = common_field_get(ctx, &raw, FIELD_TX_LPI_TIMER);

    if (rc != 0)
        return rc;
    if (raw > UINT32_MAX)
        return TE_RC(TE_TA_UNIX, TE_ERANGE);
    *val = raw;
    return 0;
}

/* Set eee_enabled field */
static te_errno
eee_enabled_set(const ta_conf_ctx *ctx, bool val)
{
    return common_param_set(ctx, val, FIELD_EEE_ENABLED);
}

/* Set tx_lpi_enabled field */
static te_errno
tx_lpi_enabled_set(const ta_conf_ctx *ctx, uint64_t val)
{
    return common_param_set(ctx, val, FIELD_TX_LPI_ENABLED);
}

/* Set tx_lpi_timer field */
static te_errno
tx_lpi_timer_set(const ta_conf_ctx *ctx, uint32_t val)
{
    return common_param_set(ctx, val, FIELD_TX_LPI_TIMER);
}

/* Commit changes to EEE configuration */
static te_errno
eee_commit(const ta_conf_ctx *ctx)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");

    return commit_ethtool_value(if_name, ta_conf_ctx_gid(ctx), TA_ETHTOOL_EEE);
}

static const ta_conf_node *const node_eee =
    TA_CONF_NA_COMMIT("eee", eee_commit,
        TA_CONF_RO_BOOL("eee_active", eee_active_get),
        TA_CONF_RW_BOOL("eee_enabled", eee_enabled_get, eee_enabled_set),
        TA_CONF_RW_UINT64("tx_lpi_enabled", tx_lpi_enabled_get,
                        tx_lpi_enabled_set),
        TA_CONF_RW_UINT32("tx_lpi_timer", tx_lpi_timer_get, tx_lpi_timer_set));

/**
 * Add "eee" node to interface in configuration tree.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_eee_init(void)
{
    return ta_conf_register("/agent/interface", node_eee);
}

#else

/* See description above */
extern te_errno
ta_unix_conf_if_eee_init(void)
{
    WARN("Interface Energy Efficient Ethernet settings are not supported");
    return 0;
}
#endif
