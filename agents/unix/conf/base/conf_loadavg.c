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
#include "rcf_pch.h"
#include "rcf_pch_tree.h"
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

static te_errno
loadavg_latest_pid_get(const ta_conf_ctx *ctx, uint64_t *val)
{
    loadavg_stats stats;
    te_errno rc;

    UNUSED(ctx);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    *val = stats.latest_pid;
    return 0;
}

static te_errno
loadavg_total_get(const ta_conf_ctx *ctx, uint64_t *val)
{
    loadavg_stats stats;
    te_errno rc;

    UNUSED(ctx);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    *val = stats.total;
    return 0;
}

static te_errno
loadavg_runnable_get(const ta_conf_ctx *ctx, uint64_t *val)
{
    loadavg_stats stats;
    te_errno rc;

    UNUSED(ctx);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    *val = stats.runnable;
    return 0;
}

static te_errno
loadavg_min15_get(const ta_conf_ctx *ctx, te_string *val)
{
    loadavg_stats stats;
    te_errno rc;

    UNUSED(ctx);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    te_string_append(val, "%g", stats.min15);
    return 0;
}

static te_errno
loadavg_min5_get(const ta_conf_ctx *ctx, te_string *val)
{
    loadavg_stats stats;
    te_errno rc;

    UNUSED(ctx);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    te_string_append(val, "%g", stats.min5);
    return 0;
}

static te_errno
loadavg_min1_get(const ta_conf_ctx *ctx, te_string *val)
{
    loadavg_stats stats;
    te_errno rc;

    UNUSED(ctx);

    rc = get_loadavg_stats(&stats);
    if (rc != 0)
        return rc;

    te_string_append(val, "%g", stats.min1);
    return 0;
}

static const ta_conf_node *const node_loadavg =
    TA_CONF_NA("loadavg",
        TA_CONF_RO_STR("min1", loadavg_min1_get),
        TA_CONF_RO_STR("min5", loadavg_min5_get),
        TA_CONF_RO_STR("min15", loadavg_min15_get),
        TA_CONF_RO_UINT64("runnable", loadavg_runnable_get),
        TA_CONF_RO_UINT64("total", loadavg_total_get),
        TA_CONF_RO_UINT64("latest_pid", loadavg_latest_pid_get));

te_errno
ta_unix_conf_loadavg_init(void)
{
    te_errno rc;

    rc = ta_conf_register("/agent", node_loadavg);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/loadavg",
                             rcf_pch_rsrc_grab_dummy,
                             rcf_pch_rsrc_release_dummy);
}