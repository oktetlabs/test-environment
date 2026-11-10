/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of TAPI for network interface channels configuration
 * (doc/cm/cm_base.xml).
 */

#include "te_config.h"
#include "te_defs.h"
#include "conf_api.h"
#include "tapi_cfg_if_chan.h"

/* Get name of channel node in configuration tree */
static const char *
chan_name(tapi_cfg_if_chan chan_type)
{
    switch (chan_type)
    {
        case TAPI_CFG_IF_CHAN_RX:
            return "rx";

        case TAPI_CFG_IF_CHAN_TX:
            return "tx";

        case TAPI_CFG_IF_CHAN_OTHER:
            return "other";

        case TAPI_CFG_IF_CHAN_COMBINED:
            return "combined";

        default:
            return "<UNKNOWN>";
    }
}

/* See description in the tapi_cfg_if_chan.h */
te_errno
tapi_cfg_if_chan_cur_get(const char *ta,
                         const char *if_name,
                         tapi_cfg_if_chan chan_type,
                         int *num)
{
    /*
     * Configurator does not read the value back after a set, and a
     * driver may change the number itself: get what the driver reports.
     */
    return cfg_get_int32_sync(num,
                              "/agent:%s/interface:%s/channels:/%s:/current:",
                              ta, if_name, chan_name(chan_type));
}

/* See description in the tapi_cfg_if_chan.h */
te_errno
tapi_cfg_if_chan_tx_queues_get(const char *ta, const char *if_name,
                               int *num)
{
    int combined = 0;
    int tx = 0;
    te_errno rc;

    rc = tapi_cfg_if_chan_cur_get(ta, if_name, TAPI_CFG_IF_CHAN_COMBINED,
                                  &combined);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_if_chan_cur_get(ta, if_name, TAPI_CFG_IF_CHAN_TX, &tx);
    if (rc != 0)
        return rc;

    *num = combined + tx;

    return 0;
}

/* See description in the tapi_cfg_if_chan.h */
te_errno
tapi_cfg_if_chan_max_get(const char *ta,
                         const char *if_name,
                         tapi_cfg_if_chan chan_type,
                         int *num)
{
    return cfg_get_int32(num,
                         "/agent:%s/interface:%s/channels:/%s:/maximum:",
                         ta, if_name, chan_name(chan_type));
}

/* See description in the tapi_cfg_if_chan.h */
te_errno
tapi_cfg_if_chan_cur_set(const char *ta,
                         const char *if_name,
                         tapi_cfg_if_chan chan_type,
                         int num)
{
    return cfg_set_instance_fmt(
                       CFG_VAL(INT32, num),
                       "/agent:%s/interface:%s/channels:/%s:/current:",
                       ta, if_name, chan_name(chan_type));
}
