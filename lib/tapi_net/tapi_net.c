/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Network setup library
 *
 * Implementation of test API to define and set up test network.
 */

/** User name of the network setup library */
#define TE_LGR_USER     "Network library"

#include "te_config.h"
#include "te_enum.h"
#include "te_alloc.h"
#include "te_str.h"
#include "tapi_cfg_base.h"
#include "logger_api.h"
#include "tapi_net.h"

/** Minimal possible VLAN ID. */
#define TAPI_NET_VLAN_ID_MIN 1
/** Maximal possible VLAN ID. */
#define TAPI_NET_VLAN_ID_MAX 4094

static const te_enum_map iface_type_map[] = {
    {.name = "base",  .value = TAPI_NET_IFACE_TYPE_BASE},
    {.name = "vlan",  .value = TAPI_NET_IFACE_TYPE_VLAN},
    {.name = "qinq",  .value = TAPI_NET_IFACE_TYPE_QINQ},
    {.name = "gre",   .value = TAPI_NET_IFACE_TYPE_GRE},
    TE_ENUM_MAP_END
};

/* Free a stack of interfaces. */
static void
tapi_vec_destroy_iface_stack(const void *item)
{
    tapi_net_iface_head *head = (tapi_net_iface_head *)item;
    tapi_net_iface *iface;

    if (head == NULL)
        return;

    while (!SLIST_EMPTY(head))
    {
        iface = SLIST_FIRST(head);
        SLIST_REMOVE_HEAD(head, iface_next);
        free(iface->name);
        free(iface);
    }
}

/* Free agent resources. */
static void
tapi_vec_destroy_agent(const void *item)
{
    tapi_net_ta *agent = (tapi_net_ta *)item;

    free(agent->ta_name);
    te_vec_free(&agent->ifaces);
}

/* Free network endpoint strings. */
static void
tapi_vec_destroy_net(const void *item)
{
    tapi_net_link *net = (tapi_net_link *)item;
    size_t i;

    for (i = 0; i < TAPI_NET_EP_NUM; i++)
    {
        free(net->endpoints[i].ta_name);
        free(net->endpoints[i].if_name);
    }
}

/* Allocate memory for interface instanse and fill fields. */
static tapi_net_iface *
iface_init(const char *if_name, tapi_net_iface_type iface_type)
{
    tapi_net_iface *iface_tmp = NULL;

    iface_tmp = TE_ALLOC(sizeof(*iface_tmp));
    iface_tmp->type = iface_type;
    iface_tmp->name = TE_STRDUP(if_name);

    return iface_tmp;
}

te_errno
tapi_net_logical_iface_add(tapi_net_iface_type iface_type, const char *if_name,
                           tapi_net_iface *base_iface, tapi_net_iface **iface)
{
    tapi_net_iface *iface_tmp = NULL;

    assert(if_name != NULL);
    assert(base_iface != NULL);
    assert(iface != NULL);

    if (iface_type == TAPI_NET_IFACE_TYPE_BASE)
    {
        ERROR("%s: logical interface can not have 'base' type",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (iface_type == TAPI_NET_IFACE_TYPE_UNKNOWN)
    {
        ERROR("%s: unsupported interface type", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    iface_tmp = iface_init(if_name, iface_type);

    SLIST_INSERT_AFTER(base_iface, iface_tmp, iface_next);

    *iface = iface_tmp;

    return 0;
}

te_errno
tapi_net_iface_set_vlan_conf(tapi_net_iface *iface, const tapi_net_vlan *vlan)
{
    assert(iface != NULL);
    assert(iface->type == TAPI_NET_IFACE_TYPE_VLAN);

    if (vlan->vlan_id < TAPI_NET_VLAN_ID_MIN ||
        vlan->vlan_id > TAPI_NET_VLAN_ID_MAX)
    {
        ERROR("%s: invalid VLAN ID: %d", __FUNCTION__, vlan->vlan_id);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    iface->conf.vlan = *vlan;

    return 0;
}

te_errno
tapi_net_iface_set_qinq_conf(tapi_net_iface *iface, const tapi_net_qinq *qinq)
{
    assert(iface != NULL);
    assert(iface->type == TAPI_NET_IFACE_TYPE_QINQ);

    if (qinq->inner_id < TAPI_NET_VLAN_ID_MIN ||
        qinq->inner_id > TAPI_NET_VLAN_ID_MAX)
    {
        ERROR("%s: invalid QinQ inner ID: %d", __FUNCTION__, qinq->inner_id);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (qinq->outer_id < TAPI_NET_VLAN_ID_MIN ||
        qinq->outer_id > TAPI_NET_VLAN_ID_MAX)
    {
        ERROR("%s: invalid QinQ outer ID: %d", __FUNCTION__, qinq->outer_id);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    iface->conf.qinq = *qinq;

    return 0;
}

void
tapi_net_ta_init(const char *ta_name, tapi_net_ta *net_cfg_ta)
{
    assert(ta_name != NULL);
    assert(net_cfg_ta != NULL);

    memset(net_cfg_ta, 0, sizeof(*net_cfg_ta));
    net_cfg_ta->ta_name = TE_STRDUP(ta_name);

    net_cfg_ta->ifaces = TE_VEC_INIT(tapi_net_iface_head);
    te_vec_set_destroy_fn_safe(&net_cfg_ta->ifaces,
                               tapi_vec_destroy_iface_stack);
}

void
tapi_net_ta_set_ifaces(tapi_net_ta *net_cfg_ta, const char **if_name_list)
{
    size_t i;

    assert(net_cfg_ta != NULL);
    assert(if_name_list != NULL);

    for (i = 0; if_name_list[i] != NULL; i++)
    {
        tapi_net_iface_head head = SLIST_HEAD_INITIALIZER(head);
        tapi_net_iface *base_iface;

        base_iface = iface_init(if_name_list[i], TAPI_NET_IFACE_TYPE_BASE);
        SLIST_INIT(&head);
        SLIST_INSERT_HEAD(&head, base_iface, iface_next);
        TE_VEC_APPEND(&net_cfg_ta->ifaces, head);
    }
}

void
tapi_net_ta_destroy(tapi_net_ta *net_cfg_ta)
{
    free(net_cfg_ta->ta_name);
    te_vec_free(&net_cfg_ta->ifaces);
}

void
tapi_net_ctx_init(tapi_net_ctx *net_ctx)
{
    if (net_ctx == NULL)
        return;

    net_ctx->agents = TE_VEC_INIT(tapi_net_ta);
    te_vec_set_destroy_fn_safe(&net_ctx->agents, tapi_vec_destroy_agent);

    net_ctx->nets = TE_VEC_INIT(tapi_net_link);
    te_vec_set_destroy_fn_safe(&net_ctx->nets, tapi_vec_destroy_net);
}

void
tapi_net_ctx_release(tapi_net_ctx *net_ctx)
{
    if (net_ctx == NULL)
        return;

    te_vec_free(&net_ctx->agents);
    te_vec_free(&net_ctx->nets);
}

tapi_net_ta *
tapi_net_find_agent_by_name(tapi_net_ctx *net_ctx, const char *ta_name)
{
    tapi_net_ta *agent;

    if (net_ctx == NULL || ta_name == NULL)
        return NULL;

    TE_VEC_FOREACH(&net_ctx->agents, agent)
    {
        if (agent->ta_name != NULL && strcmp(agent->ta_name, ta_name) == 0)
            return agent;
    }

    return NULL;
}

tapi_net_iface *
tapi_net_find_iface_by_name(tapi_net_ta *net_cfg_ta,
                                     const char *if_name)
{
    tapi_net_iface *iface = NULL;
    tapi_net_iface *tmp = NULL;
    tapi_net_iface_head *iface_head;

    if (net_cfg_ta == NULL || if_name == NULL)
        return NULL;

    TE_VEC_FOREACH(&net_cfg_ta->ifaces, iface_head)
    {
        SLIST_FOREACH_SAFE(iface, iface_head, iface_next, tmp)
        {
            if (iface->name != NULL && strcmp(iface->name, if_name) == 0)
                return iface;
        }
    }

    return NULL;
}

tapi_net_iface_type
tapi_net_iface_type_by_name(const char *iface_type_str)
{
    return te_enum_map_from_str(iface_type_map, iface_type_str,
                                TAPI_NET_IFACE_TYPE_UNKNOWN);
}
