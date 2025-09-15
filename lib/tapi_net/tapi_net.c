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
#include "tapi_cfg_net.h"
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

typedef te_errno (*setup_iface_handler)(const char *ta,
                                        const tapi_net_iface *iface,
                                        const tapi_net_iface *base_iface);

static te_errno
setup_base_iface(const char *ta, const tapi_net_iface *iface,
                 const tapi_net_iface *base_iface)
{
    cfg_handle *handles = NULL;
    unsigned int count = 0;

    UNUSED(base_iface);

    cfg_find_pattern_fmt(&count, &handles, "/agent:%s/interface:%s/",
                         ta, iface->name);
    if (count != 1)
    {
        ERROR("Failed to find base interface '%s' in Configurator Tree",
              iface->name);
        return TE_RC(TE_TAPI, TE_ENOENT);
    }

    return 0;
}

static te_errno
unknown_iface_handler(const char *ta,
                      const tapi_net_iface *iface,
                      const tapi_net_iface *base_iface)
{
    UNUSED(ta);
    UNUSED(iface);
    UNUSED(base_iface);

    ERROR("Unsupported interface type");

    return TE_RC(TE_TAPI, TE_EINVAL);
}

static te_errno
setup_vlan_iface(const char *ta, const tapi_net_iface *iface,
                 const tapi_net_iface *base_iface)
{
    char *iface_real_name = NULL;
    te_errno rc;

    if (base_iface == NULL)
    {
        ERROR("Base interface must specified for VLAN interface");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_cfg_base_if_add_vlan(ta, base_iface->name,
                                   iface->conf.vlan.vlan_id, &iface_real_name);
    if (rc != 0)
    {
        ERROR("Failed to add VLAN interface: %r", rc);
        return rc;
    }

    if (strcmp(iface->name, iface_real_name) != 0)
    {
        ERROR("Created VLAN interface has different name");
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    return 0;
}

static te_errno
setup_qinq_iface(const char *ta, const tapi_net_iface *iface,
                 const tapi_net_iface *base_iface)
{
    UNUSED(ta);
    UNUSED(iface);

    if (base_iface == NULL)
    {
        ERROR("Base interface must specified for QinQ interface");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    ERROR("QinQ setup is not supported yet");

    return TE_RC(TE_TAPI, TE_EINVAL);
}

static te_errno
setup_gre_iface(const char *ta, const tapi_net_iface *iface,
                const tapi_net_iface *base_iface)
{
    UNUSED(ta);
    UNUSED(iface);

    if (base_iface == NULL)
    {
        ERROR("Base interface must specified for GRE interface");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    ERROR("GRE setup is not supported yet");

    return TE_RC(TE_TAPI, TE_EINVAL);
}

static const TE_ENUM_MAP_ACTION(setup_iface_handler) setup_iface_actions[] = {
    { "base", setup_base_iface },
    { "vlan", setup_vlan_iface },
    { "qinq", setup_qinq_iface },
    { "gre",  setup_gre_iface },
    TE_ENUM_MAP_END
};

static te_errno
setup_iface_stack(const char *ta, const tapi_net_iface_head *iface_stack)
{
    const tapi_net_iface *iface_prev = NULL;
    const tapi_net_iface *iface = NULL;
    const tapi_net_iface *tmp = NULL;
    te_errno rc;

    SLIST_FOREACH_SAFE(iface, iface_stack, iface_next, tmp)
    {
        const char *type_name = te_enum_map_from_any_value(iface_type_map,
                                                           iface->type,
                                                           "unknown");

        if (iface->type == TAPI_NET_IFACE_TYPE_BASE && iface_prev != NULL)
        {
            ERROR("Base interface must come first in the interface stack");
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        TE_ENUM_DISPATCH(setup_iface_actions, unknown_iface_handler,
                         type_name, rc, ta, iface, iface_prev);
        if (rc != 0)
        {
            ERROR("Failed to set up %s interface on %s: %r", type_name, ta, rc);
            return rc;
        }

        iface_prev = iface;
    }

    return 0;
}

te_errno
tapi_net_setup_ifaces(const tapi_net_ctx *net_ctx)
{
    const tapi_net_iface_head *iface;
    const tapi_net_ta *agent;
    te_errno rc;

    if (net_ctx == NULL)
        return 0;

    TE_VEC_FOREACH(&net_ctx->agents, agent)
    {
        TE_VEC_FOREACH(&agent->ifaces, iface)
        {
            rc = setup_iface_stack(agent->ta_name, iface);
            if (rc != 0)
            {
                ERROR("%s: failed to setup one of interfaces on %s",
                      __FUNCTION__, agent->ta_name);
                return rc;
            }
        }
    }

    return 0;
}

const char *
tapi_net_get_top_iface_name(const tapi_net_iface_head *iface_head)
{
    const tapi_net_iface *iface_cur;

    if (iface_head == NULL)
        return NULL;

    iface_cur = SLIST_FIRST(iface_head);

    while (SLIST_NEXT(iface_cur, iface_next) != NULL)
        iface_cur = SLIST_NEXT(iface_cur, iface_next);

    return iface_cur->name;
}

te_errno
tapi_net_get_top_iface_addr(const tapi_net_iface_head *iface_head,
                            const struct sockaddr **addr)
{
    const tapi_net_iface *iface_cur;

    assert(iface_head != NULL);
    assert(addr != NULL);

    iface_cur = SLIST_FIRST(iface_head);

    while (SLIST_NEXT(iface_cur, iface_next) != NULL)
        iface_cur = SLIST_NEXT(iface_cur, iface_next);

    if (iface_cur->addr == NULL)
    {
        ERROR("%s: no address is assigned to requested interface",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_ENOENT);
    }

    *addr = iface_cur->addr;

    return 0;
}

/* Match net from Configurator tree with one from network context. */
static bool
match_cfg_net(const cfg_net_t *cfg_net, const tapi_net_link *net_link)
{
    char *ifname[TAPI_NET_EP_NUM] = {};
    char *ta[TAPI_NET_EP_NUM] = {};
    char *oid_str;
    cfg_oid *oid;
    bool found = false;
    te_errno rc;
    int i;

    assert(cfg_net != NULL);
    assert(net_link != NULL);

    if (cfg_net->n_nodes != TAPI_NET_EP_NUM)
        return false;

    for (i = 0; i < TAPI_NET_EP_NUM; i++)
    {
        cfg_val_type type = CVT_STRING;

        rc = cfg_get_instance(cfg_net->nodes[i].handle, &type, &oid_str);
        if (rc != 0)
            return false;

        oid = cfg_convert_oid_str(oid_str);
        free(oid_str);

        if (!oid->inst)
            return false;

        /* TODO: Anti-pattern. Changes in CFG API are required to fix it. */
        if (strcmp(cfg_oid_inst_subid(oid, 1), "agent") != 0 ||
            strcmp(cfg_oid_inst_subid(oid, 2), "interface") != 0)
        {
            cfg_free_oid(oid);
            return false;
        }

        ta[i] = TE_STRDUP(CFG_OID_GET_INST_NAME(oid, 1));
        ifname[i] = TE_STRDUP(CFG_OID_GET_INST_NAME(oid, 2));

        cfg_free_oid(oid);
    }

    for (i = 0; i < TAPI_NET_EP_NUM; i++)
    {
        int flip = TAPI_NET_EP_NUM - i - 1;

        if (strcmp(ta[0], net_link->endpoints[i].ta_name) == 0 &&
            strcmp(ifname[0], net_link->endpoints[i].if_name) == 0 &&
            strcmp(ta[1], net_link->endpoints[flip].ta_name) == 0 &&
            strcmp(ifname[1], net_link->endpoints[flip].if_name) == 0)
        {
            found = true;
            break;
        }
    }

    for (i = 0; i < TAPI_NET_EP_NUM; i++)
    {
        free(ifname[i]);
        free(ta[i]);
    }

    return found;
}

/* Check if a network with the same two endpoints already exists in Configurator. */
static bool
net_link_exists(const tapi_net_link *net_link)
{
    cfg_nets_t nets;
    unsigned int i;
    te_errno rc;

    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
        return false;

    for (i = 0; i < nets.n_nets; i++)
    {
        if (match_cfg_net(&nets.nets[i], net_link))
        {
            tapi_cfg_net_free_nets(&nets);
            return true;
        }
    }

    tapi_cfg_net_free_nets(&nets);

    return false;
}

/** Register network in Configurator. */
static te_errno
net_register(const tapi_net_link *net_link, cfg_net_t **cfg_net)
{
    te_string oid_ep0_str = TE_STRING_INIT_STATIC(CFG_OID_MAX);
    te_string oid_ep1_str = TE_STRING_INIT_STATIC(CFG_OID_MAX);
    cfg_net_t *cfg_net_tmp;
    te_errno rc;

    assert(net_link != NULL);
    assert(cfg_net != NULL);

    cfg_net_tmp = TE_ALLOC(sizeof(*cfg_net_tmp));

    te_string_append(&oid_ep0_str, "/agent:%s/interface:%s",
                     net_link->endpoints[0].ta_name,
                     net_link->endpoints[0].if_name);
    te_string_append(&oid_ep1_str, "/agent:%s/interface:%s",
                     net_link->endpoints[1].ta_name,
                     net_link->endpoints[1].if_name);

    rc = tapi_cfg_net_register_net(net_link->name, cfg_net_tmp,
                                   oid_ep0_str.ptr, NET_NODE_TYPE_AGENT,
                                   oid_ep1_str.ptr, NET_NODE_TYPE_AGENT,
                                   NULL);
    if (rc != 0)
    {
        ERROR("Failed to register network '%s': %r", net_link->name, rc);
        return rc;
    }

    *cfg_net = cfg_net_tmp;

    return 0;
}

static unsigned
inet_version(int af)
{
    switch (af)
    {
       case AF_INET:
           return 4;

       case AF_INET6:
           return 6;

       default:
           TE_FATAL_ERROR("Unsupported address family: %d", af);
    }
}

static te_errno
net_get_node_info(cfg_handle node_handle, int af, struct sockaddr **addr,
                  char **ta_name, char **iface_name)
{
    cfg_val_type type = CVT_ADDRESS;
    cfg_handle *ip_handles = NULL;
    char *node_oid = NULL;
    char *oid_str = NULL;
    cfg_oid *oid = NULL;
    unsigned int ip_num;
    te_errno rc = 0;

    rc = cfg_get_oid_str(node_handle, &node_oid);
    if (rc != 0)
    {
        ERROR("Failed to get OID of network node %u: %r", node_handle, rc);
        goto out;
    }

    rc = cfg_get_string(&oid_str, "%s", node_oid);
    if (rc != 0)
    {
        ERROR("Failed to get interface of network node %s: %r", node_oid, rc);
        goto out;
    }

    oid = cfg_convert_oid_str(oid_str);
    if (oid == NULL)
    {
        ERROR("Failed to convert OID of network node %u: %r", node_handle, rc);
        goto out;
    }

    if (ta_name != NULL)
        *ta_name = cfg_oid_get_inst_name(oid, 1);
    if (iface_name != NULL)
        *iface_name = cfg_oid_get_inst_name(oid, 2);

    if (addr != NULL)
    {
        rc = cfg_find_pattern_fmt(&ip_num, &ip_handles, "%s/ip%u_address:*",
                                  node_oid, inet_version(af));
        if (rc != 0)
        {
            ERROR("Failed to find IPv%u address of network node %s: %r",
                  inet_version(af), node_oid, rc);
            goto out;
        }

        if (ip_num != 1)
        {
            ERROR("Node %s has %d IPv%u addresses, unsupported",
                  node_oid, ip_num, inet_version(af));
            rc = TE_RC(TE_TAPI, TE_EINVAL);
        }

        rc = cfg_get_instance(ip_handles[0], &type, addr);
        if (rc != 0)
        {
            ERROR("Failed to get IPv%u address of network node %s: %r",
                  inet_version(af), node_oid, rc);
        }
    }

out:
    free(ip_handles);
    free(oid_str);
    cfg_free_oid(oid);
    free(node_oid);

    return rc;
}

static const tapi_net_link *
find_net_link_by_cfg_net(const cfg_net_t *cfg_net,
                         const tapi_net_ctx *net_ctx)
{
    const tapi_net_link *net_link;

    TE_VEC_FOREACH(&net_ctx->nets, net_link)
    {
        if (match_cfg_net(cfg_net, net_link))
            return net_link;
    }

    return NULL;
}

static te_errno
agent_if_addr_set_by_net(const cfg_net_t *cfg_net, tapi_net_ctx *net_ctx)
{
    const tapi_net_link *net_link;
    struct sockaddr *addr;
    tapi_net_iface *iface;
    tapi_net_ta *agent;
    unsigned int i;
    char *ta_name;
    char *if_name;
    te_errno rc;
    int af;

    assert(cfg_net != NULL);
    assert(net_ctx != NULL);

    net_link = find_net_link_by_cfg_net(cfg_net, net_ctx);
    if (net_link == NULL)
        return 0;

    af = net_link->af;

    for (i = 0; i < cfg_net->n_nodes; i++)
    {
        rc = net_get_node_info(cfg_net->nodes[i].handle, af, &addr,
                               &ta_name, &if_name);
        if (rc != 0)
        {
            ERROR("Failed to get information about one of network nodes: %r",
                  rc);
            free(ta_name);
            free(if_name);
            return rc;
        }

        agent = tapi_net_find_agent_by_name(net_ctx, ta_name);
        if (agent == NULL)
        {
            ERROR("Agent %s is missing in test network configuration");
            return TE_RC(TE_TAPI, TE_ENOENT);
        }

        iface = tapi_net_find_iface_by_name(agent, if_name);
        if (iface == NULL)
        {
            ERROR("Interface %s is missing on %s agent in test network configuration",
                  if_name, ta_name);
            return TE_RC(TE_TAPI, TE_ENOENT);
        }

        iface->addr = addr;

        free(ta_name);
        free(if_name);
    }

    return 0;
}

/**
 * Fill in IP addresses for logical interfaces based on netowrks in Configurator.
 *
 * This function set appropriate IP addresses for the logical interface
 * structures mentioned in Configurator to use them after in tests.
 */
static te_errno
net_addr_fill(tapi_net_ctx *net_ctx)
{
    cfg_nets_t nets;
    te_errno rc = 0;
    unsigned int i;

    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        ERROR("Failed to get nets configuration: %r", rc);
        return rc;
    }

    for (i = 0; i < nets.n_nets; i++)
    {
        rc = agent_if_addr_set_by_net(&nets.nets[i], net_ctx);
        if (rc != 0)
        {
            ERROR("Failed to process %s network: %r", nets.nets[i].name, rc);
            goto out;
        }
    }

out:
    tapi_cfg_net_free_nets(&nets);

    return rc;
}

te_errno
tapi_net_setup(tapi_net_ctx *net_ctx)
{
    const tapi_net_link *net_link;
    te_errno rc;

    assert(net_ctx != NULL);

    rc = tapi_net_setup_ifaces(net_ctx);
    if (rc != 0)
    {
        ERROR("%s: failed to setup interfaces specified in network context: %r",
              __FUNCTION__, rc);
    }

    TE_VEC_FOREACH(&net_ctx->nets, net_link)
    {
        cfg_net_t *cfg_net;

        if (net_link_exists(net_link))
            continue;

        rc = net_register(net_link, &cfg_net);
        if (rc != 0)
            return rc;

        rc = tapi_cfg_net_assign_ip(net_link->af, cfg_net, NULL);
        if (rc != 0)
        {
            ERROR("%s: failed to assign IPs to net %s: %r", __FUNCTION__,
                  net_link->name, rc);
            return rc;
        }
    }

    rc = net_addr_fill(net_ctx);
    if (rc != 0)
    {
        ERROR("%s: failed to obtain interface addresses from Configurator: %r",
              __FUNCTION__, rc);
    }

    rc = tapi_cfg_net_all_up(false);
    if (rc != 0)
    {
        ERROR("%s: failed to up all interfaces mentioned in networks configuration: %r",
              __FUNCTION__, rc);
    }

    return 0;
}
