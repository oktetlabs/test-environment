/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Unix Test Agent
 *
 * Flow control parameters for a network interface
 */

#define TE_LGR_USER     "Conf Intf FlowCtrl"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "rcf_pch_tree.h"
#include "conf_ethtool.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H

/* Enumeration of pause parameters from ethtool_pauseparam */
typedef enum {
    PAUSE_AUTONEG,
    PAUSE_RX,
    PAUSE_TX
} te_if_pause_param;

/* Get pointer to a specific field in ethtool_pauseparam structure */
static uint32_t *
get_field(struct ethtool_pauseparam *val, te_if_pause_param field)
{
    switch (field)
    {
        case PAUSE_AUTONEG:
            return &val->autoneg;

        case PAUSE_RX:
            return &val->rx_pause;

        case PAUSE_TX:
            return &val->tx_pause;
    }

    ERROR("Unknown field ID %d", field);
    return NULL;
}

/* Common function for getting pause parameter value */
static te_errno
process_get_command(const ta_conf_ctx *ctx, te_if_pause_param field, bool *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_pauseparam *eptr;
    uint32_t *param_val;
    te_errno rc;

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx), TA_ETHTOOL_PAUSEPARAM,
                           (void **)&eptr);
    if (rc != 0)
    {
        if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
        {
            /*
             * This will cause Configurator to ignore absence of
             * value silently; it simply will not show not supported
             * node in the tree.
             */
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        return rc;
    }

    param_val = get_field(eptr, field);
    if (param_val == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *val = (*param_val != 0);
    return 0;
}

/* Get pause autonegotiation state */
static te_errno
autoneg_get(const ta_conf_ctx *ctx, bool *val)
{
    return process_get_command(ctx, PAUSE_AUTONEG, val);
}

/* Get Rx pause state */
static te_errno
rx_get(const ta_conf_ctx *ctx, bool *val)
{
    return process_get_command(ctx, PAUSE_RX, val);
}

/* Get Tx pause state */
static te_errno
tx_get(const ta_conf_ctx *ctx, bool *val)
{
    return process_get_command(ctx, PAUSE_TX, val);
}

/* Common function for setting pause parameter value */
static te_errno
process_set_command(const ta_conf_ctx *ctx, te_if_pause_param field, bool val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_pauseparam *eptr;
    uint32_t *param_val;
    te_errno rc;

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx), TA_ETHTOOL_PAUSEPARAM,
                           (void **)&eptr);
    if (rc != 0)
        return rc;

    param_val = get_field(eptr, field);
    if (param_val == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *param_val = val ? 1 : 0;
    return 0;
}

/* Set pause autonegotiation state */
static te_errno
autoneg_set(const ta_conf_ctx *ctx, bool val)
{
    return process_set_command(ctx, PAUSE_AUTONEG, val);
}

/* Set Rx pause state */
static te_errno
rx_set(const ta_conf_ctx *ctx, bool val)
{
    return process_set_command(ctx, PAUSE_RX, val);
}

/* Set Tx pause state */
static te_errno
tx_set(const ta_conf_ctx *ctx, bool val)
{
    return process_set_command(ctx, PAUSE_TX, val);
}

/* Commit changes to flow control parameters */
static te_errno
flow_ctrl_commit(const ta_conf_ctx *ctx)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");

    return commit_ethtool_value(if_name, ta_conf_ctx_gid(ctx),
                                TA_ETHTOOL_PAUSEPARAM);
}

static const ta_conf_node *const node_flow_control =
    TA_CONF_NA_COMMIT("flow_control", flow_ctrl_commit,
        TA_CONF_RW_BOOL("tx", tx_get, tx_set),
        TA_CONF_RW_BOOL("rx", rx_get, rx_set),
        TA_CONF_RW_BOOL("autoneg", autoneg_get, autoneg_set));

/**
 * Add a child node for flow control parameters to the interface object.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_flow_ctrl_init(void)
{
    return ta_conf_register("/agent/interface", node_flow_control);
}

#else

/* See description above */
extern te_errno
ta_unix_conf_if_flow_ctrl_init(void)
{
    WARN("Interface flow control parameters are not supported");

    return 0;
}
#endif
