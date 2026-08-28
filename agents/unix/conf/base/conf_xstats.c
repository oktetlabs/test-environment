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

#include "te_alloc.h"
#include "rcf_pch_tree.h"
#include "conf_ethtool.h"

#ifdef HAVE_LINUX_ETHTOOL_H
static te_errno
xstat_get(const ta_conf_ctx *ctx, uint64_t *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *xstat_name = ta_conf_ctx_inst(ctx, "xstat");
    const ta_ethtool_strings *xstat_names = NULL;
    const uint64_t *xstat_values = NULL;
    int i;
    te_errno rc;

    rc = ta_ethtool_get_strings_stats(ta_conf_ctx_gid(ctx), if_name,
                                      &xstat_names, &xstat_values);
    if (rc != 0)
        return rc;

    for (i = 0; i < xstat_names->num; i++)
    {
        if (strcmp(xstat_names->strings[i], xstat_name) == 0)
        {
            *val = xstat_values[i];
            return 0;
        }
    }
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

static te_errno
xstat_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    char *list = NULL;
    char *copy;
    char *saveptr;
    char *tok;
    te_errno rc;

    rc = ta_ethtool_get_strings_list(ta_conf_ctx_gid(ctx), if_name,
                                     ETH_SS_STATS, &list);
    if (rc != 0)
    {
        int failed_ethtool_cmd;

        failed_ethtool_cmd = ta_ethtool_failed_cmd();

        if (failed_ethtool_cmd < 0)
        {
            ERROR("%s(): error %r occurred while getting statistics for %s",
                  __FUNCTION__, rc, if_name);
            return rc;
        }
        else if (rc != TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP) ||
                 failed_ethtool_cmd != ETHTOOL_GSTRINGS)
        {
            ERROR("%s(): error %r occurred while getting statistics for %s; "
                  "failed SIOCETHTOOL command is %d (%s)", __FUNCTION__, rc,
                  if_name, failed_ethtool_cmd,
                  ta_ethtool_cmd2str(failed_ethtool_cmd));
            return rc;
        }

        /*
         * If statistics are not supported, report an empty list
         * to avoid error messages.
         */
        return 0;
    }

    if (list == NULL)
        return 0;

    copy = TE_STRDUP(list);
    for (tok = strtok_r(copy, " ", &saveptr); tok != NULL;
         tok = strtok_r(NULL, " ", &saveptr))
    {
        char *name = TE_STRDUP(tok);

        TE_VEC_APPEND(names, name);
    }
    free(copy);
    free(list);

    return 0;
}

static const ta_conf_node *const node_xstats =
    TA_CONF_NA("xstats",
        TA_CONF_RO_COLL_UINT64("xstat", xstat_get, xstat_list));

/**
 * Add a child nodes for ethtool statistics to the statistic interface object.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_eth_xstats_init(void)
{
    return ta_conf_register("/agent/interface", node_xstats);
}
#else
te_errno
ta_unix_conf_eth_xstats_init(void)
{
    INFO("Extra ethernet interface statistics are not supported");
    return 0;
}
#endif /* !HAVE_LINUX_ETHTOOL_H */
