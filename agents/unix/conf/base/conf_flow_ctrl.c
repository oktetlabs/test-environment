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
#include "te_str.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
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
process_get_command(unsigned int gid, const char *if_name,
                    te_if_pause_param field, char *value)
{
    struct ethtool_pauseparam *eptr;
    uint32_t *param_val;
    te_errno rc;

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_PAUSEPARAM,
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

    rc = te_snprintf(value, RCF_MAX_VAL, "%u", *param_val);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

/* Get pause autonegotiation state */
static te_errno
autoneg_get(unsigned int gid, const char *oid,
            char *value, const char *if_name)
{
    UNUSED(oid);

    return process_get_command(gid, if_name, PAUSE_AUTONEG, value);
}

/* Get Rx pause state */
static te_errno
rx_get(unsigned int gid, const char *oid,
       char *value, const char *if_name)
{
    UNUSED(oid);

    return process_get_command(gid, if_name, PAUSE_RX, value);
}

/* Get Tx pause state */
static te_errno
tx_get(unsigned int gid, const char *oid,
       char *value, const char *if_name)
{
    UNUSED(oid);

    return process_get_command(gid, if_name, PAUSE_TX, value);
}

/* Common function for setting pause parameter value */
static te_errno
process_set_command(unsigned int gid, const char *if_name,
                    te_if_pause_param field, const char *value)
{
    struct ethtool_pauseparam *eptr;
    unsigned long int parsed_val;
    uint32_t *param_val;
    te_errno rc;

    rc = te_strtoul(value, 10, &parsed_val);
    if (rc != 0 || (parsed_val != 0 && parsed_val != 1))
    {
        ERROR("%s(): invalid value '%s'", __FUNCTION__, value);
        return (rc != 0 ? rc : TE_RC(TE_TA_UNIX, TE_EINVAL));
    }

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_PAUSEPARAM,
                           (void **)&eptr);
    if (rc != 0)
        return rc;

    param_val = get_field(eptr, field);
    if (param_val == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *param_val = parsed_val;
    return 0;
}

/* Set pause autonegotiation state */
static te_errno
autoneg_set(unsigned int gid, const char *oid,
            const char *value, const char *if_name)
{
    UNUSED(oid);

    return process_set_command(gid, if_name, PAUSE_AUTONEG, value);
}

/* Set Rx pause state */
static te_errno
rx_set(unsigned int gid, const char *oid,
       const char *value, const char *if_name)
{
    UNUSED(oid);

    return process_set_command(gid, if_name, PAUSE_RX, value);
}

/* Set Tx pause state */
static te_errno
tx_set(unsigned int gid, const char *oid,
       const char *value, const char *if_name)
{
    UNUSED(oid);

    return process_set_command(gid, if_name, PAUSE_TX, value);
}

/* Commit changes to flow control parameters */
static te_errno
flow_ctrl_commit(unsigned int gid, const cfg_oid *p_oid)
{
    char *if_name;

    UNUSED(gid);
    if_name = CFG_OID_GET_INST_NAME(p_oid, 2);

    return commit_ethtool_value(if_name, gid, TA_ETHTOOL_PAUSEPARAM);
}


static rcf_pch_cfg_object node_flow_control;

RCF_PCH_CFG_NODE_RWC(node_autoneg, "autoneg", NULL,
                     NULL, autoneg_get, autoneg_set, &node_flow_control);

RCF_PCH_CFG_NODE_RWC(node_rx, "rx", NULL,
                     &node_autoneg, rx_get, rx_set, &node_flow_control);

RCF_PCH_CFG_NODE_RWC(node_tx, "tx", NULL,
                     &node_rx, tx_get, tx_set, &node_flow_control);

RCF_PCH_CFG_NODE_NA_COMMIT(node_flow_control, "flow_control", &node_tx,
                           NULL, &flow_ctrl_commit);

/**
 * Add a child node for flow control parameters to the interface object.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_flow_ctrl_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_flow_control);
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
