/** @file
 * @brief Unix TA Traffic Control configuration support
 *
 * Implementation of configuration nodes traffic control support
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#define TE_LGR_USER     "Unix Conf TC"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_defs.h"
#include "rcf_pch.h"
#include "rcf_ch_api.h"

#include "conf_tc.h"
#include "conf_qdisc.h"
#include "conf_netem.h"
#include "conf_tc_internal.h"

RCF_PCH_CFG_NODE_RW_COLLECTION(node_qdisc_param, "param", NULL, NULL,
                               conf_netem_param_get,
                               conf_netem_param_set,
                               conf_netem_param_add,
                               conf_netem_param_del,
                               conf_netem_param_list, NULL);

/* TODO: add handle support */
RCF_PCH_CFG_NODE_RW(node_qdisc_handle, "handle", NULL,
                    &node_qdisc_param, conf_qdisc_handle_get, NULL);

/* FIXME: only root support */
RCF_PCH_CFG_NODE_RW(node_qdisc_parent, "parent", NULL,
                    &node_qdisc_handle, conf_qdisc_parent_get, NULL);

RCF_PCH_CFG_NODE_RW(node_qdisc_kind, "kind", NULL,
                    &node_qdisc_parent,
                    conf_qdics_kind_get,
                    conf_qdics_kind_set);

RCF_PCH_CFG_NODE_RW(node_qdisc_enabled, "enabled", NULL,
                    &node_qdisc_kind,
                    conf_qdisc_enabled_get,
                    conf_qdisc_enabled_set);

RCF_PCH_CFG_NODE_NA(node_qdisc, "qdisc", &node_qdisc_enabled, NULL);

RCF_PCH_CFG_NODE_NA(node_tc, "tc", &node_qdisc, NULL);

/* See the description in conf_tc.h */
te_errno
ta_unix_conf_tc_init(void)
{
    te_errno rc;

    if ((rc = conf_tc_internal_init()) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if ((rc = rcf_pch_add_node("/agent/interface", &node_tc)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return 0;
}

/* See the description in conf_tc.h */
void
ta_unix_conf_tc_fini(void)
{
    conf_tc_internal_fini();
}
