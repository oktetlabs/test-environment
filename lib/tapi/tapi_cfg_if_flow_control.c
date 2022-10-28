/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for Network Interface flow control
 * configuration (doc/cm/cm_base.xml).
 */

#define TE_LGR_USER "TAPI CFG IF FC"

#include "te_config.h"
#include "te_defs.h"
#include "conf_api.h"
#include "tapi_cfg_if_flow_control.h"

#define COMMON_FMT "/agent:%s/interface:%s/flow_control:"

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_autoneg_get(const char *ta, const char *ifname,
                           int *autoneg)
{
    cfg_val_type type = CVT_INTEGER;

    return cfg_get_instance_fmt(&type, autoneg,
                                COMMON_FMT "/autoneg:",
                                ta, ifname);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_autoneg_set(const char *ta, const char *ifname,
                           int autoneg)
{
    return cfg_set_instance_fmt(CFG_VAL(INT32, autoneg),
                                COMMON_FMT "/autoneg:",
                                ta, ifname);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_autoneg_set_local(const char *ta,
                                 const char *ifname,
                                 int autoneg)
{
    return cfg_set_instance_local_fmt(CFG_VAL(INT32, autoneg),
                                      COMMON_FMT "/autoneg:",
                                      ta, ifname);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_rx_get(const char *ta, const char *ifname, int *rx)
{
    cfg_val_type type = CVT_INTEGER;

    return cfg_get_instance_fmt(&type, rx,
                                COMMON_FMT "/rx:",
                                ta, ifname);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_rx_set(const char *ta, const char *ifname, int rx)
{
    return cfg_set_instance_fmt(CFG_VAL(INT32, rx),
                                COMMON_FMT "/rx:",
                                ta, ifname);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_rx_set_local(const char *ta, const char *ifname,
                            int rx)
{
    return cfg_set_instance_local_fmt(CFG_VAL(INT32, rx),
                                      COMMON_FMT "/rx:",
                                      ta, ifname);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_tx_get(const char *ta, const char *ifname, int *tx)
{
    cfg_val_type type = CVT_INTEGER;

    return cfg_get_instance_fmt(&type, tx,
                                COMMON_FMT "/tx:",
                                ta, ifname);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_tx_set(const char *ta, const char *ifname, int tx)
{
    return cfg_set_instance_fmt(CFG_VAL(INT32, tx),
                                COMMON_FMT "/tx:",
                                ta, ifname);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_tx_set_local(const char *ta, const char *ifname,
                            int tx)
{
    return cfg_set_instance_local_fmt(CFG_VAL(INT32, tx),
                                      COMMON_FMT "/tx:",
                                      ta, ifname);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_commit(const char *ta, const char *if_name)
{
    return cfg_commit_fmt(COMMON_FMT, ta, if_name);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_get(const char *ta, const char *ifname,
                   tapi_cfg_if_fc *params)
{
    te_errno rc;

    rc = tapi_cfg_if_fc_autoneg_get(ta, ifname,
                                    &params->autoneg);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_if_fc_rx_get(ta, ifname, &params->rx);
    if (rc != 0)
        return rc;

    return tapi_cfg_if_fc_tx_get(ta, ifname, &params->tx);
}

/* See description in the tapi_cfg_if_flow_control.h */
te_errno
tapi_cfg_if_fc_set(const char *ta, const char *ifname,
                   tapi_cfg_if_fc *params)
{
    te_errno rc;

    if (params->autoneg >= 0)
    {
        rc = tapi_cfg_if_fc_autoneg_set_local(ta, ifname,
                                              params->autoneg);
        if (rc != 0)
            return rc;
    }

    if (params->rx >= 0)
    {
        rc = tapi_cfg_if_fc_rx_set_local(ta, ifname, params->rx);
        if (rc != 0)
            return rc;
    }

    if (params->tx >= 0)
    {
        rc = tapi_cfg_if_fc_tx_set_local(ta, ifname, params->tx);
        if (rc != 0)
            return rc;
    }

    return tapi_cfg_if_fc_commit(ta, ifname);
}
