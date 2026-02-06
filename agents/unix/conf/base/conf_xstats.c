/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Unix Test Agent
 *
 * Extra Ethernet interface statistics
 */

#define TE_LGR_USER     "Extra eth xstats Conf"

#include "te_config.h"
#include "config.h"

#include "logger_api.h"
#include "unix_internal.h"

#ifdef HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#include "te_printf.h"
#include "conf_ethtool.h"

#ifdef HAVE_LINUX_ETHTOOL_H
te_errno
xstat_get(unsigned int gid, const char *oid, char *value, const char *if_name,
          const char *xstats_name, const char *xstat_name)
{
    const ta_ethtool_strings *xstat_names = NULL;
    const uint64_t *xstat_values = NULL;
    int i;
    te_errno rc;

    UNUSED(xstats_name);

    rc = ta_ethtool_get_strings_stats(gid, if_name, &xstat_names,
                                      &xstat_values);
    if (rc != 0)
        return rc;

    for (i = 0; i < xstat_names->num; i++)
    {
        if (strcmp(xstat_names->strings[i], xstat_name) == 0)
        {
            rc = te_snprintf(value, RCF_MAX_VAL, "%" TE_PRINTF_64 "u",
                             xstat_values[i]);
            if (rc != 0)
            {
                ERROR("%s(): te_snprintf() failed", __FUNCTION__);
                return TE_RC(TE_TA_UNIX, rc);
            }
            return 0;
        }
    }
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

te_errno
xstat_list(unsigned int gid, const char *oid, const char *sub_id,
           char **list, const char *if_name)
{
    te_errno rc;

    UNUSED(sub_id);
    UNUSED(oid);

    rc = ta_ethtool_get_strings_list(gid, if_name, ETH_SS_STATS, list);
    return rc;
}

static rcf_pch_cfg_object node_xstat = {
    .sub_id = "xstat",
    .get = (rcf_ch_cfg_get)xstat_get,
    .list = (rcf_ch_cfg_list)xstat_list,
};

static rcf_pch_cfg_object node_xstats = {
    .sub_id = "xstats",
    .son = &node_xstat,
};

/**
 * Add a child nodes for ethtool statistics to the statistic interface object.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_eth_xstats_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_xstats);
}
#else
te_errno
ta_unix_conf_eth_xstats_init(void)
{
    INFO("Extra ethernet interface statistics are not supported");
    return 0;
}
#endif /* !HAVE_LINUX_ETHTOOL_H */
