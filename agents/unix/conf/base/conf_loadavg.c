/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Loadavg stats support
 *
 * Loadavg stats configuration tree support
 *
 * Copyright (C) 2020-2023 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Loadavg"

#include "te_config.h"
#include "config.h"

#include "logger_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "conf_common.h"
#include "te_str.h"

typedef struct loadavg_stats {
    double min1;
    double min5;
    double min15;
    uint64_t runnable;
    uint64_t total;
    uint64_t latest_pid;
} loadavg_stats;

static te_errno
get_loadavg_stats(loadavg_stats *stats)
{
    te_errno rc;
    char buf[RCF_MAX_VAL];

    rc = read_sys_value(buf, sizeof(buf), false, "/proc/loadavg");
    if (rc != 0)
        return rc;

    rc = sscanf(buf, "%lg %lg %lg %" SCNu64 "/%" SCNu64 "%" SCNu64,
                &stats->min1, &stats->min5, &stats->min15, &stats->runnable,
                &stats->total, &stats->latest_pid);

    if (rc != 6)
    {
        ERROR("Could not read loadavg values from /proc/loadavg");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return 0;
}

te_errno
loadavg_latest_pid_get(unsigned int gid, const char *oid, char *value)
{
    loadavg_stats stats;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    return te_snprintf(value, RCF_MAX_VAL, "%" PRIu64, stats.latest_pid);
}

te_errno
loadavg_total_get(unsigned int gid, const char *oid, char *value)
{
    loadavg_stats stats;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    return te_snprintf(value, RCF_MAX_VAL, "%" PRIu64, stats.total);
}

te_errno
loadavg_runnable_get(unsigned int gid, const char *oid, char *value)
{
    loadavg_stats stats;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    return te_snprintf(value, RCF_MAX_VAL, "%" PRIu64, stats.runnable);
}

te_errno
loadavg_min15_get(unsigned int gid, const char *oid, char *value)
{
    loadavg_stats stats;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    return te_snprintf(value, RCF_MAX_VAL, "%g", stats.min15);
}

te_errno
loadavg_min5_get(unsigned int gid, const char *oid, char *value)
{
    loadavg_stats stats;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    return te_snprintf(value, RCF_MAX_VAL, "%g", stats.min5);
}

te_errno
loadavg_min1_get(unsigned int gid, const char *oid, char *value)
{
    loadavg_stats stats;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    return te_snprintf(value, RCF_MAX_VAL, "%g", stats.min1);
}

RCF_PCH_CFG_NODE_RO(node_loadavg_latest_pid, "latest_pid", NULL, NULL,
                    loadavg_latest_pid_get);

RCF_PCH_CFG_NODE_RO(node_loadavg_total, "total", NULL, &node_loadavg_latest_pid,
                    loadavg_total_get);

RCF_PCH_CFG_NODE_RO(node_loadavg_runnable, "runnable", NULL,
                    &node_loadavg_total, loadavg_runnable_get);

RCF_PCH_CFG_NODE_RO(node_loadavg_min15, "min15", NULL, &node_loadavg_runnable,
                    loadavg_min15_get);

RCF_PCH_CFG_NODE_RO(node_loadavg_min5, "min5", NULL, &node_loadavg_min15,
                    loadavg_min5_get);

RCF_PCH_CFG_NODE_RO(node_loadavg_min1, "min1", NULL, &node_loadavg_min5,
                    loadavg_min1_get);

RCF_PCH_CFG_NODE_NA(node_loadavg, "loadavg", &node_loadavg_min1, NULL);

te_errno
ta_unix_conf_loadavg_init(void)
{
    te_errno rc;

    rc = rcf_pch_add_node("/agent", &node_loadavg);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/loadavg",
                             rcf_pch_rsrc_grab_dummy,
                             rcf_pch_rsrc_release_dummy);
}