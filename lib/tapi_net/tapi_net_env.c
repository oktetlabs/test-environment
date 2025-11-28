/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Auxiliary library for interacting with test environment.
 *
 * Implementation of helpers for interacting with test environment.
 */

/** User name of the network setup library */
#define TE_LGR_USER     "Network ENV library"

#include "te_config.h"
#include "te_vector.h"
#include "tapi_cfg_net.h"
#include "tapi_net_env.h"

const tapi_net_iface_head *
tapi_net_env_get_iface_stack(tapi_env *env, tapi_net_ctx *net_ctx,
                             const char *base_iface)
{
    const tapi_net_iface_head *iface_head;
    const tapi_env_if *env_if;
    const tapi_net_ta *agent;
    const char *ta_name;

    assert(base_iface != NULL);
    assert(net_ctx != NULL);
    assert(env != NULL);

    env_if = tapi_env_get_env_if(env, base_iface);
    if (env_if == NULL)
        return NULL;

    ta_name = env_if->host->ta;
    if (ta_name == NULL)
        return NULL;

    agent = tapi_net_find_agent_by_name(net_ctx, ta_name);
    if (agent == NULL)
        return NULL;

    TE_VEC_FOREACH(&agent->ifaces, iface_head)
    {
        tapi_net_iface *iface = SLIST_FIRST(iface_head);

        if (iface != NULL && iface->name != NULL &&
            strcmp(iface->name, env_if->if_info.if_name) == 0)
        {
            return iface_head;
        }
    }

    /* The requested base interface may be a slave in aggregation. */
    return tapi_net_find_iface_stack_by_aggr_slave(
                agent, env_if->if_info.if_name);
}
