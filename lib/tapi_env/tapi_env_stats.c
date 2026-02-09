/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Environment Library
 *
 * Environment statistics.
 */

/** Logger subsystem user of the library */
#define TE_LGR_USER "Environment stats LIB"

#include "te_config.h"

#include "te_vector.h"
#include "logger_api.h"
#include "tapi_cfg_stats.h"
#include "tapi_env.h"


te_errno
tapi_env_stats_gather(tapi_env *env)
{
    tapi_env_host *host;
    tapi_env_if *iface;
    te_errno rc;

    SLIST_FOREACH(host, &env->hosts, links)
    {
        tapi_cfg_net_stats *stats;

        stats = te_vec_replace(&host->net_stats,
                               te_vec_size(&host->net_stats), NULL);
        rc = tapi_cfg_stats_net_stats_get(host->ta, stats);
        if (rc != 0)
            return rc;
    }

    CIRCLEQ_FOREACH(iface, &env->ifs, links)
    {
        tapi_cfg_if_stats *stats;
        tapi_cfg_if_xstats *xstats;

        if (iface->rsrc_type != NET_NODE_RSRC_TYPE_INTERFACE)
            continue;

        stats = te_vec_replace(&iface->stats, te_vec_size(&iface->stats), NULL);
        xstats = te_vec_replace(&iface->xstats, te_vec_size(&iface->xstats),
                                NULL);

        rc = tapi_cfg_stats_if_stats_get(iface->host->ta,
                                         iface->if_info.if_name, stats);
        if (rc != 0)
            return rc;
        rc = tapi_cfg_stats_if_xstats_get(iface->host->ta,
                                          iface->if_info.if_name, xstats);
        if (rc != 0)
            return rc;
    }

    return 0;
}

te_errno
tapi_env_stats_log_diff(tapi_env *env)
{
    tapi_env_host *host;
    tapi_env_if *iface;
    te_errno rc;

    SLIST_FOREACH(host, &env->hosts, links)
    {
        size_t n_stats = te_vec_size(&host->net_stats);
        const tapi_cfg_net_stats *prev;
        const tapi_cfg_net_stats *last;

        if (n_stats == 0)
            continue;

        last = te_vec_get(&host->net_stats, n_stats - 1);
        prev = n_stats > 1 ? te_vec_get(&host->net_stats, n_stats - 2) : NULL;

        rc = tapi_cfg_stats_net_stats_print_diff(last, prev,
                "Network stats diff on host '%s' (TA %s)",
                host->name, host->ta);
        if (rc != 0)
            return rc;
    }

    CIRCLEQ_FOREACH(iface, &env->ifs, links)
    {
        size_t n_stats = te_vec_size(&iface->stats);
        size_t n_xstats = te_vec_size(&iface->xstats);
        const tapi_cfg_if_stats *prev;
        const tapi_cfg_if_stats *last;
        const tapi_cfg_if_xstats *xprev;
        const tapi_cfg_if_xstats *xlast;

        if (iface->rsrc_type != NET_NODE_RSRC_TYPE_INTERFACE)
            continue;

        if (n_stats != 0)
        {
            last = te_vec_get(&iface->stats, n_stats - 1);
            prev = n_stats > 1 ? te_vec_get(&iface->stats, n_stats - 2) : NULL;

            rc = tapi_cfg_stats_if_stats_print_diff(last, prev,
                    "Interface '%s' (%s) stats diff on host '%s' (TA %s)",
                    iface->name, iface->if_info.if_name,
                    iface->host->name, iface->host->ta);
            if (rc != 0)
                return rc;
        }
        if (n_xstats != 0)
        {
            xlast = te_vec_get(&iface->xstats, n_xstats - 1);
            xprev = n_xstats > 1 ? te_vec_get(&iface->xstats, n_xstats - 2) :
                                   NULL;

            rc = tapi_cfg_stats_if_xstats_print_diff(xlast, xprev,
                    "Interface '%s' (%s) extended stats diff on "
                    "host '%s' (TA %s)", iface->name, iface->if_info.if_name,
                    iface->host->name, iface->host->ta);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

te_errno
tapi_env_stats_gather_and_log_diff(tapi_env *env)
{
    te_errno rc;

    rc = tapi_env_stats_gather(env);
    if (rc != 0)
        return rc;

    return tapi_env_stats_log_diff(env);
}
