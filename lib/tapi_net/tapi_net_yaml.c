/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Auxiliary library to define network in YAML format
 *
 * Implementation of test API to provide a way to set up test network defined in YAML files.
 */

/** User name of the test network configuration parser */
#define TE_LGR_USER     "Network YAML library"

#include <yaml.h>

#include "te_config.h"
#include "logger_api.h"
#include "conf_api.h"
#include "te_str.h"
#include "te_alloc.h"
#include "te_enum.h"
#include "te_expand.h"

#include "tapi_net_yaml.h"

#define YAML_ERR_PREFIX "YAML network file parser "

#define YAML_NODE_LINE_COLUMN_FMT   "line %lu column %lu"
#define YAML_NODE_LINE_COLUMN(_n)   \
    (_n)->start_mark.line + 1, (_n)->start_mark.column + 1

typedef enum cfg_yaml_root_field {
    CFG_YAML_ROOT_FIELD_UNKNOWN = -1,
    CFG_YAML_ROOT_FIELD_IFACE_LIST,
    CFG_YAML_ROOT_FIELD_LAG_LIST,
    CFG_YAML_ROOT_FIELD_NET_LIST,
    CFG_YAML_ROOT_FIELD_NAT_LIST,
} cfg_yaml_root_field;

static const te_enum_map cfg_yaml_root_field_map[] = {
    {.name = "interfaces",  .value = CFG_YAML_ROOT_FIELD_IFACE_LIST},
    {.name = "aggregates",  .value = CFG_YAML_ROOT_FIELD_LAG_LIST},
    {.name = "networks",    .value = CFG_YAML_ROOT_FIELD_NET_LIST},
    {.name = "nat",         .value = CFG_YAML_ROOT_FIELD_NAT_LIST},
    TE_ENUM_MAP_END
};

typedef enum cfg_yaml_iface_field {
    CFG_YAML_IFACE_FIELD_UNKNOWN = -1,
    CFG_YAML_IFACE_FIELD_AGENT,
    CFG_YAML_IFACE_FIELD_NAME_LIST,
} cfg_yaml_iface_field;

static const te_enum_map cfg_yaml_iface_field_map[] = {
    {.name = "agent", .value = CFG_YAML_IFACE_FIELD_AGENT},
    {.name = "names", .value = CFG_YAML_IFACE_FIELD_NAME_LIST},
    TE_ENUM_MAP_END
};

typedef enum cfg_yaml_lag_field {
    CFG_YAML_LAG_FIELD_UNKNOWN = -1,
    CFG_YAML_LAG_FIELD_AGENT,
    CFG_YAML_LAG_FIELD_NAME,
    CFG_YAML_LAG_FIELD_TYPE,
    CFG_YAML_LAG_FIELD_MODE,
    CFG_YAML_LAG_FIELD_SLAVES,
} cfg_yaml_lag_field;

typedef enum cfg_yaml_ep_field {
    CFG_YAML_EP_FIELD_UNKNOWN = -1,
    CFG_YAML_EP_FIELD_AGENT,
    CFG_YAML_EP_FIELD_BASE_IFACE,
    CFG_YAML_EP_FIELD_IFACE,
} cfg_yaml_ep_field;

static const te_enum_map cfg_yaml_ep_field_map[] = {
    {.name = "agent",      .value = CFG_YAML_EP_FIELD_AGENT},
    {.name = "base_iface", .value = CFG_YAML_EP_FIELD_BASE_IFACE},
    {.name = "iface",      .value = CFG_YAML_EP_FIELD_IFACE},
    TE_ENUM_MAP_END
};

typedef enum cfg_yaml_nat_field {
    CFG_YAML_NAT_FIELD_UNKNOWN = -1,
    CFG_YAML_NAT_FIELD_AGENT,
    CFG_YAML_NAT_FIELD_RULES,
} cfg_yaml_nat_field;

static const te_enum_map cfg_yaml_nat_field_map[] = {
    { .name = "agent", .value = CFG_YAML_NAT_FIELD_AGENT },
    { .name = "rules", .value = CFG_YAML_NAT_FIELD_RULES },
    TE_ENUM_MAP_END
};

typedef enum cfg_yaml_nat_rule_field {
    CFG_YAML_NAT_RULE_FIELD_UNKNOWN = -1,
    CFG_YAML_NAT_RULE_FIELD_TYPE,
    CFG_YAML_NAT_RULE_FIELD_MODE,
    CFG_YAML_NAT_RULE_FIELD_FROM,
    CFG_YAML_NAT_RULE_FIELD_TO,
} cfg_yaml_nat_rule_field;

static const te_enum_map cfg_yaml_nat_rule_field_map[] = {
    { .name = "type", .value = CFG_YAML_NAT_RULE_FIELD_TYPE },
    { .name = "mode", .value = CFG_YAML_NAT_RULE_FIELD_MODE },
    { .name = "from", .value = CFG_YAML_NAT_RULE_FIELD_FROM },
    { .name = "to",   .value = CFG_YAML_NAT_RULE_FIELD_TO },
    TE_ENUM_MAP_END
};

static const te_enum_map cfg_yaml_nat_rule_type_map[] = {
    { .name = "dnat", .value = TAPI_NET_NAT_RULE_TYPE_DNAT },
    { .name = "snat", .value = TAPI_NET_NAT_RULE_TYPE_SNAT },
    TE_ENUM_MAP_END
};

static const te_enum_map cfg_yaml_nat_rule_mode_map[] = {
    { .name = "address",    .value = TAPI_NET_NAT_RULE_MODE_ADDRESS },
    { .name = "masquerade", .value = TAPI_NET_NAT_RULE_MODE_MASQUERADE },
    TE_ENUM_MAP_END
};

static bool
expand_local_env_or_unix(const char *param_name, const void *ctx,
                         te_string *dest)
{
    const char *value = NULL;
    char *cfg_value = NULL;
    bool has_value;
    te_errno rc;

    UNUSED(ctx);

    rc = cfg_get_string(&cfg_value, "/local:/env:%s", param_name);
    if (rc == 0)
        value = cfg_value;
    else
        value = getenv(param_name);

    has_value = value != NULL;

    if (has_value)
        te_string_append(dest, "%s", value);

    free(cfg_value);

    return has_value;
}

static te_errno
expanded_val_get_str(const char *src, char **retval)
{
    te_string tmp = TE_STRING_INIT;
    te_errno rc;

    rc = te_string_expand_parameters(src, expand_local_env_or_unix, NULL,
                                     &tmp);
    if (rc != 0)
    {
        te_string_free(&tmp);
        return rc;
    }

    te_string_move(retval, &tmp);

    return 0;
}

static te_errno
expanded_val_get_int(const char *src, int *retval)
{
    te_errno rc;
    char *tmp;
    int val;

    rc = expanded_val_get_str(src, &tmp);
    if (rc != 0)
        return rc;

    rc = te_strtoi(tmp, 0, &val);
    free(tmp);
    if (rc != 0)
        return rc;

    *retval = val;

    return 0;
}

typedef struct cfg_net_node_ctx {
    tapi_net_iface_type iface_type;
    int af;
    union {
        tapi_net_vlan vlan;
        tapi_net_qinq qinq;
    } spec;
    yaml_node_t *ep_list_node;
} cfg_net_node_ctx;

const cfg_net_node_ctx cfg_net_node_ctx_def = {
    .iface_type = TAPI_NET_IFACE_TYPE_UNKNOWN,
    .af = -1,
    .spec = { { 0 } },
    .ep_list_node = NULL,
};

typedef struct cfg_yaml_lag_ctx {
    char *agent;
    char *name;
    tapi_net_lag_type type;
    tapi_net_lag_mode mode;
    yaml_node_t *slaves_node;
} cfg_yaml_lag_ctx;

static const cfg_yaml_lag_ctx cfg_yaml_lag_ctx_def = {
    .agent = NULL,
    .name = NULL,
    .type = TAPI_NET_LAG_TYPE_UNKNOWN,
    .mode = TAPI_NET_LAG_MODE_UNKNOWN,
    .slaves_node = NULL,
};

/* Callback type for handling netowork node fields. */
typedef te_errno (*cfg_yaml_net_field_handler)(yaml_node_t *v, void *ctx);

/* Callback type for handling netowork node fields. */
typedef te_errno (*cfg_yaml_lag_field_handler)(yaml_node_t *v, void *ctx);

/* Handle agent name field in network node. */
static te_errno
lag_node_agent_field_hadler(yaml_node_t *v, void *ctx)
{
    cfg_yaml_lag_ctx *lag_ctx = ctx;
    te_errno rc;

    if (v->type != YAML_SCALAR_NODE)
    {
        ERROR(YAML_ERR_PREFIX "'agent' in 'aggregates' must be scalar at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(v));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    rc = expanded_val_get_str((const char *)v->data.scalar.value,
                              &lag_ctx->agent);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX, "failed to expand agent name: %r", rc);
        return rc;
    }

    return 0;
}

/* Handle aggreagtion interface name field in network node. */
static te_errno
lag_node_name_field_hadler(yaml_node_t *v, void *ctx)
{
    cfg_yaml_lag_ctx *lag_ctx = ctx;
    te_errno rc;

    if (v->type != YAML_SCALAR_NODE)
    {
        ERROR(YAML_ERR_PREFIX "'name' in 'aggregates' must be scalar at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(v));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    rc = expanded_val_get_str((const char *)v->data.scalar.value,
                              &lag_ctx->name);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX, "failed to expand name: %r", rc);
        return rc;
    }

    return 0;
}

/* Handle aggregation type in network node. */
static te_errno
lag_node_type_field_hadler(yaml_node_t *v, void *ctx)
{
    cfg_yaml_lag_ctx *lag_ctx = ctx;
    char *type_str = NULL;
    te_errno rc;

    if (v->type != YAML_SCALAR_NODE)
    {
        ERROR(YAML_ERR_PREFIX "'type' in 'aggregates' must be scalar at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(v));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = expanded_val_get_str((const char *)v->data.scalar.value, &type_str);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX, "failed to aggreagation type: %r", rc);
        return rc;
    }

    lag_ctx->type = te_enum_map_from_str(tapi_net_lag_type_map,
                                         type_str, TAPI_NET_LAG_TYPE_UNKNOWN);
    if (lag_ctx->type == TAPI_NET_LAG_TYPE_UNKNOWN)
    {
        ERROR(YAML_ERR_PREFIX "unknown LAG type '%s' in 'aggregates'",
              type_str);
        free(type_str);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    free(type_str);

    return 0;
}

/* Handle aggreagation mode in network node. */
static te_errno
lag_node_mode_field_hadler(yaml_node_t *v, void *ctx)
{
    cfg_yaml_lag_ctx *lag_ctx = ctx;
    char *mode_str = NULL;
    te_errno rc;

    if (v->type != YAML_SCALAR_NODE)
    {
        ERROR(YAML_ERR_PREFIX "'mode' in 'aggregates' must be scalar at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(v));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = expanded_val_get_str((const char *)v->data.scalar.value, &mode_str);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX, "failed to aggreagation mode: %r", rc);
        return rc;
    }

    lag_ctx->mode = te_enum_map_from_str(tapi_net_lag_mode_map,
                                         mode_str, TAPI_NET_LAG_MODE_UNKNOWN);
    if (lag_ctx->mode == TAPI_NET_LAG_MODE_UNKNOWN)
    {
        ERROR(YAML_ERR_PREFIX "unknown LAG mode '%s' in 'aggregates'",
              mode_str);
        free(mode_str);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    free(mode_str);

    return 0;
}

/* Handle aggregation slaves in network node. */
static te_errno
lag_node_slaves_node_field_hadler(yaml_node_t *v, void *ctx)
{
    cfg_yaml_lag_ctx *lag_ctx = ctx;

    if (v->type != YAML_SEQUENCE_NODE)
    {
        ERROR(YAML_ERR_PREFIX "'slaves' in 'aggregates' must be a sequence at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(v));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    lag_ctx->slaves_node = v;

    return 0;
}

/* Default handler for unknown LAG field.*/
static te_errno
lag_node_unknown_field_handler(yaml_node_t *v, void *user_ctx)
{
    UNUSED(v);
    UNUSED(user_ctx);

    ERROR(YAML_ERR_PREFIX "unknown field in LAG node");

    return TE_RC(TE_TAPI, TE_EINVAL);
}

/* Handle interface type field in network node. */
static te_errno
net_node_handle_iface_type(yaml_node_t *v, void *ctx)
{
    cfg_net_node_ctx *node_ctx = ctx;

    if (v->type != YAML_SCALAR_NODE)
    {
        ERROR(YAML_ERR_PREFIX "interface type must be scalar at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(v));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    node_ctx->iface_type = tapi_net_iface_type_by_name(
                               (const char *)v->data.scalar.value);
    if (node_ctx->iface_type == TAPI_NET_IFACE_TYPE_UNKNOWN)
    {
        ERROR(YAML_ERR_PREFIX "unsupported interface type '%s'",
              (char *)v->data.scalar.value);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
}

/* Handle address family in network node. */
static te_errno
net_node_handle_af(yaml_node_t *v, void *ctx)
{
    cfg_net_node_ctx *node_ctx = ctx;
    char *af_str = NULL;
    te_errno rc = 0;
    int af;

    if (v->type != YAML_SCALAR_NODE)
    {
        ERROR(YAML_ERR_PREFIX "address family must be scalar at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(v));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = expanded_val_get_str((const char *)v->data.scalar.value, &af_str);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to expand address family: %r", rc);
        goto out;
    }

    af = te_enum_map_from_str(tapi_net_yaml_af_map, af_str, -1);
    if (af < 0)
    {
        ERROR(YAML_ERR_PREFIX "unknown address family '%s'", af_str);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto out;
    }

    node_ctx->af = af;

out:
    free(af_str);
    return rc;
}

/* Handle VLAN ID field in network node. */
static te_errno
net_node_handle_vlan_id(yaml_node_t *v, void *ctx)
{
    cfg_net_node_ctx *node_ctx = ctx;
    te_errno rc;

    if (v->type != YAML_SCALAR_NODE)
    {
        ERROR(YAML_ERR_PREFIX "VLAN ID must be scalar at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(v));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = expanded_val_get_int((const char *)v->data.scalar.value,
                              &node_ctx->spec.vlan.vlan_id);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to parse VLAN ID '%s': %r",
              (char *)v->data.scalar.value, rc);
        return rc;
    }

    return 0;
}

/* Handle QinQ outer VLAN ID field in network node. */
static te_errno
net_node_handle_qinq_outer_id(yaml_node_t *v, void *ctx)
{
    cfg_net_node_ctx *node_ctx = ctx;
    te_errno rc;

    if (v->type != YAML_SCALAR_NODE)
    {
        ERROR(YAML_ERR_PREFIX "QinQ outer ID must be scalar at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(v));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = expanded_val_get_int((const char *)v->data.scalar.value,
                              &node_ctx->spec.qinq.outer_id);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to parse QinQ outer ID '%s': %r",
              (char *)v->data.scalar.value, rc);
        return rc;
    }

    return 0;
}

/* Handle QinQ inner VLAN ID field in network node. */
static te_errno
net_node_handle_qinq_inner_id(yaml_node_t *v, void *ctx)
{
    cfg_net_node_ctx *node_ctx = ctx;
    te_errno rc;

    if (v->type != YAML_SCALAR_NODE)
    {
        ERROR(YAML_ERR_PREFIX "QinQ inner ID must be scalar at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(v));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = expanded_val_get_int((const char *)v->data.scalar.value,
                              &node_ctx->spec.qinq.inner_id);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to parse QinQ inner ID '%s': %r",
              (char *)v->data.scalar.value, rc);
        return rc;
    }

    return 0;
}

/* Handle endpoint list in network node. */
static te_errno
net_node_handle_ep_list(yaml_node_t *v, void *ctx)
{
    cfg_net_node_ctx *node_ctx = ctx;

    node_ctx->ep_list_node = v;

    return 0;
}

/* Default handler for unknown network field in network node.*/
static te_errno
net_node_unknown_field_handler(yaml_node_t *v, void *user_ctx)
{
    UNUSED(v);
    UNUSED(user_ctx);

    ERROR(YAML_ERR_PREFIX "unknown network field in network node");

    return TE_RC(TE_TAPI, TE_EINVAL);
}

/* Mapping between YAML network field names and their handlers. */
static const TE_ENUM_MAP_ACTION(cfg_yaml_net_field_handler) cfg_fld_acts[] = {
    { "iface_type",      net_node_handle_iface_type },
    { "af",              net_node_handle_af },
    { "vlan_id",         net_node_handle_vlan_id },
    { "qinq_outer_id",   net_node_handle_qinq_outer_id },
    { "qinq_inner_id",   net_node_handle_qinq_inner_id },
    { "endpoints",       net_node_handle_ep_list },
    TE_ENUM_MAP_END
};

/* Mapping between YAML LAG field names and their handlers. */
static const TE_ENUM_MAP_ACTION(cfg_yaml_lag_field_handler)
cfg_lag_fld_acts[] = {
    { "agent",           lag_node_agent_field_hadler },
    { "name",            lag_node_name_field_hadler },
    { "type",            lag_node_type_field_hadler },
    { "mode",            lag_node_mode_field_hadler },
    { "slaves",          lag_node_slaves_node_field_hadler },
    TE_ENUM_MAP_END
};

static te_errno
endpoint_process(const cfg_net_node_ctx *net_node_ctx, const char *ta_name,
                 const char *base_if_name, const char *if_name,
                 tapi_net_ctx *net_ctx)
{
    tapi_net_iface *base_iface = NULL;
    tapi_net_iface_type iface_type;
    tapi_net_iface *iface = NULL;
    tapi_net_ta *net_ta = NULL;
    te_errno rc;

    assert(net_node_ctx != NULL);
    assert(base_if_name != NULL);
    assert(net_ctx != NULL);
    assert(ta_name != NULL);

    net_ta = tapi_net_find_agent_by_name(net_ctx, ta_name);
    if (net_ta == NULL)
    {
        ERROR(YAML_ERR_PREFIX "unknown agent '%s'", ta_name);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    base_iface = tapi_net_find_iface_by_name(net_ta, base_if_name);
    if (base_iface == NULL)
    {
        ERROR(YAML_ERR_PREFIX "base interface '%s' not found for agent '%s'",
              base_if_name, ta_name);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    iface_type = net_node_ctx->iface_type;

    if (iface_type == TAPI_NET_IFACE_TYPE_BASE)
        return 0;

    rc = tapi_net_logical_iface_add(iface_type, if_name, base_iface, &iface);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to add logical interface '%s': %r",
              if_name, rc);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    switch (iface_type)
    {
        case TAPI_NET_IFACE_TYPE_GRE:
            break;

        case TAPI_NET_IFACE_TYPE_VLAN:
            return tapi_net_iface_set_vlan_conf(iface,
                                                &net_node_ctx->spec.vlan);

        case TAPI_NET_IFACE_TYPE_QINQ:
            return tapi_net_iface_set_qinq_conf(iface,
                                                &net_node_ctx->spec.qinq);

        default:
            assert(0);
    }

    return 0;
}

/* Fill epdpoint information required for configuring network. */
static void
fill_ep_info(tapi_net_endpoint *ep, const char *ta_name,
             const char *if_name)
{
    assert(ep != NULL);

    ep->if_name = TE_STRDUP(if_name);
    ep->ta_name = TE_STRDUP(ta_name);
}

static te_errno
endpoint_node_parse(yaml_node_t *ep_node, yaml_document_t *doc,
                    char **ta_name, char **base_iface, char **iface)
{
    yaml_node_pair_t *pair = NULL;
    char *base_iface_tmp = NULL;
    cfg_yaml_ep_field ep_field;
    char *ta_name_tmp = NULL;
    char *iface_tmp = NULL;
    te_errno rc = 0;

    assert(ep_node != NULL);
    assert(doc != NULL);

    if (ep_node->type != YAML_MAPPING_NODE)
    {
        ERROR(YAML_ERR_PREFIX "unexpected endpoint node type at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(ep_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    pair = ep_node->data.mapping.pairs.start;

    do {
        yaml_node_t *k = yaml_document_get_node(doc, pair->key);
        yaml_node_t *v = yaml_document_get_node(doc, pair->value);

        if (k->type != YAML_SCALAR_NODE)
        {
            ERROR(YAML_ERR_PREFIX "unexpected endpoint field type at " YAML_NODE_LINE_COLUMN_FMT,
                  YAML_NODE_LINE_COLUMN(k));
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            goto err;
        }

        if (v->type != YAML_SCALAR_NODE)
        {
            ERROR(YAML_ERR_PREFIX "unexpected endpoint field value type at " YAML_NODE_LINE_COLUMN_FMT,
                  YAML_NODE_LINE_COLUMN(k));
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            goto err;
        }

        ep_field = te_enum_map_from_str(cfg_yaml_ep_field_map,
                                        (char *)k->data.scalar.value,
                                        CFG_YAML_EP_FIELD_UNKNOWN);

        switch (ep_field)
        {
            case CFG_YAML_EP_FIELD_AGENT:
                rc = expanded_val_get_str((char *)v->data.scalar.value,
                                          &ta_name_tmp);
                if (rc != 0)
                {
                    ERROR(YAML_ERR_PREFIX "failed to parse endpoint agent name '%s' at " YAML_NODE_LINE_COLUMN_FMT ": %r",
                          (char *)v->data.scalar.value,
                           YAML_NODE_LINE_COLUMN(v), rc);
                    goto err;
                }
                break;

            case CFG_YAML_EP_FIELD_BASE_IFACE:
                rc = expanded_val_get_str((char *)v->data.scalar.value,
                                          &base_iface_tmp);
                if (rc != 0)
                {
                    ERROR(YAML_ERR_PREFIX "failed to parse base interface name '%s' at " YAML_NODE_LINE_COLUMN_FMT ": %r",
                          (char *)v->data.scalar.value,
                          YAML_NODE_LINE_COLUMN(v), rc);
                    goto err;
                }
                break;

            case CFG_YAML_EP_FIELD_IFACE:
                rc = expanded_val_get_str((char *)v->data.scalar.value,
                                          &iface_tmp);
                if (rc != 0)
                {
                    ERROR(YAML_ERR_PREFIX "failed to parse interface name '%s' at " YAML_NODE_LINE_COLUMN_FMT ": %r",
                          (char *)v->data.scalar.value,
                          YAML_NODE_LINE_COLUMN(v), rc);
                    goto err;
                }
                break;

            default:
                ERROR(YAML_ERR_PREFIX "unsupported endpoint field '%s' at " YAML_NODE_LINE_COLUMN_FMT,
                      (char *)k->data.scalar.value,
                      YAML_NODE_LINE_COLUMN(k));
                rc = TE_RC(TE_TAPI, TE_EINVAL);
                goto err;
        }
    } while (++pair < ep_node->data.mapping.pairs.top);

    if (base_iface != NULL)
        *base_iface = base_iface_tmp;
    if (ta_name != NULL)
        *ta_name = ta_name_tmp;
    if (iface != NULL)
        *iface = iface_tmp;

    return 0;

err:
    free(base_iface_tmp);
    free(ta_name_tmp);
    free(iface_tmp);

    return rc;
}

/* Parse endpoint list of specific network */
static te_errno
endpoint_list_node_parse(yaml_document_t *doc,
                         const cfg_net_node_ctx *net_node_ctx,
                         tapi_net_link *net, tapi_net_ctx *net_ctx)
{
    const yaml_node_t *ep_list_node;
    yaml_node_item_t *item_top;
    char *base_iface = NULL;
    yaml_node_item_t *item;
    char *ta_name = NULL;
    char *iface = NULL;
    te_errno rc = 0;
    size_t ep_num;
    size_t i;

    assert(net_node_ctx != NULL);
    assert(net_ctx != NULL);
    assert(doc != NULL);
    assert(net != NULL);

    ep_list_node = net_node_ctx->ep_list_node;

    if (ep_list_node->type != YAML_SEQUENCE_NODE)
    {
        ERROR(YAML_ERR_PREFIX "endpoints must be a sequence at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(ep_list_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    item = ep_list_node->data.sequence.items.start;
    item_top = ep_list_node->data.sequence.items.top;
    ep_num = item_top - item;

    if (ep_num != TAPI_NET_EP_NUM)
    {
        ERROR(YAML_ERR_PREFIX "network must contain exactly %d endpoints",
              TAPI_NET_EP_NUM);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    for (i = 0; i < ep_num; i++, item++)
    {
        yaml_node_t *ep_node = NULL;

        ep_node = yaml_document_get_node(doc, *item);

        rc = endpoint_node_parse(ep_node, doc, &ta_name, &base_iface, &iface);
        if (rc != 0)
        {
            ERROR(YAML_ERR_PREFIX "failed to parse endpoint node: %r", rc);
            return rc;
        }

        if (ta_name == NULL || base_iface == NULL)
        {
            ERROR(YAML_ERR_PREFIX "endpoint is missing required fields");
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            goto err;
        }

        if (net_node_ctx->iface_type == TAPI_NET_IFACE_TYPE_BASE)
            iface = TE_STRDUP(base_iface);

        rc = endpoint_process(net_node_ctx, ta_name, base_iface, iface,
                              net_ctx);
        if (rc != 0)
        {
            ERROR(YAML_ERR_PREFIX "failed to process endpoint: %r", rc);
            goto err;
        }

        fill_ep_info(&net->endpoints[i], ta_name, iface);

        free(base_iface);
        free(ta_name);
        free(iface);
    }

    return 0;

err:
    free(base_iface);
    free(ta_name);
    free(iface);

    return rc;
}

static te_errno
net_node_ctx_validate(const cfg_net_node_ctx *net_node_ctx,
                      yaml_node_t *net_node)
{
    if (net_node_ctx->ep_list_node == NULL)
    {
        ERROR(YAML_ERR_PREFIX "endpoint list is required for network node at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(net_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (net_node_ctx->af < 0)
    {
        ERROR(YAML_ERR_PREFIX "address family is required for network node at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(net_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    switch (net_node_ctx->iface_type)
    {
        /* No additional checks. */
        case TAPI_NET_IFACE_TYPE_BASE:
            /*@fallthrough@*/

        case TAPI_NET_IFACE_TYPE_GRE:
            break;

        case TAPI_NET_IFACE_TYPE_VLAN:
            /*@fallthrough@*/

        /* Check type-specific field initialization. */
        case TAPI_NET_IFACE_TYPE_QINQ:
            if (memcmp(&net_node_ctx->spec, &cfg_net_node_ctx_def.spec,
                       sizeof(net_node_ctx->spec)) == 0)
            {
                ERROR(YAML_ERR_PREFIX "type-specific fields are missing at " YAML_NODE_LINE_COLUMN_FMT,
                      YAML_NODE_LINE_COLUMN(net_node));
            }
            break;

        default:
            ERROR(YAML_ERR_PREFIX "unsupported inteface type for network node at " YAML_NODE_LINE_COLUMN_FMT,
                  YAML_NODE_LINE_COLUMN(net_node));
            return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
}

/* Parse node with specific netowrk and fill network configuration context .*/
static te_errno
net_node_parse(yaml_node_t *net_node, yaml_document_t *doc,
               tapi_net_ctx *net_ctx)
{
    cfg_net_node_ctx net_node_ctx = cfg_net_node_ctx_def;
    yaml_node_pair_t *pair_top = NULL;
    yaml_node_pair_t *pair = NULL;
    tapi_net_link net = {};
    te_errno rc;

    assert(net_ctx != NULL);
    assert(net_node != NULL);
    assert(doc != NULL);

    if (net_node->type != YAML_MAPPING_NODE)
    {
        ERROR(YAML_ERR_PREFIX "unexpected network node type at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(net_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    pair_top = net_node->data.mapping.pairs.top;
    pair = net_node->data.mapping.pairs.start;

    if (pair_top == pair)
    {
        ERROR(YAML_ERR_PREFIX "empty network mapping at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(net_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    do {
        yaml_node_t *k = yaml_document_get_node(doc, pair->key);
        yaml_node_t *v = yaml_document_get_node(doc, pair->value);

        if (k->type != YAML_SCALAR_NODE)
        {
            ERROR(YAML_ERR_PREFIX "unexpected network node type for field at " YAML_NODE_LINE_COLUMN_FMT,
                  YAML_NODE_LINE_COLUMN(k));
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        TE_ENUM_DISPATCH(cfg_fld_acts, net_node_unknown_field_handler,
                         (char *)k->data.scalar.value, rc, v, &net_node_ctx);
        if (rc != 0)
            return rc;
    } while (++pair < net_node->data.mapping.pairs.top);

    rc = net_node_ctx_validate(&net_node_ctx, net_node);
    if (rc != 0)
        return rc;

    TE_SPRINTF(net.name, "test_net_%zu", te_vec_size(&net_ctx->nets));
    net.af = net_node_ctx.af;

    rc = endpoint_list_node_parse(doc, &net_node_ctx, &net, net_ctx);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to parse endpoint list node: %r", rc);
        return rc;
    }

    TE_VEC_APPEND(&net_ctx->nets, net);

    return 0;
}

/* Parse node with netowrks and fill network configuration context .*/
static te_errno
net_list_node_parse(yaml_node_t *net_list_node, yaml_document_t *doc,
                    tapi_net_ctx *net_ctx)
{
    yaml_node_item_t *item = net_list_node->data.sequence.items.start;
    te_errno rc;

    assert(net_list_node != NULL);
    assert(net_ctx != NULL);
    assert(doc != NULL);

    do {
        yaml_node_t *net_node = yaml_document_get_node(doc, *item);

        rc = net_node_parse(net_node, doc, net_ctx);
        if (rc != 0)
        {
            ERROR(YAML_ERR_PREFIX "failed to parse network node: %r", rc);
            return rc;
        }
    } while (++item < net_list_node->data.sequence.items.top);

    return 0;
}
/* Parse interfaces list. */
static te_errno
parse_if_name_list(yaml_node_t *if_name_list_node, yaml_document_t *doc,
                   char ***if_name_list)
{
    yaml_node_item_t *item_top;
    char **name_list = NULL;
    yaml_node_item_t *item;
    size_t name_list_size;
    te_errno rc = 0;
    size_t i;

    assert(if_name_list_node != NULL);
    assert(if_name_list != NULL);
    assert(doc != NULL);

    if (if_name_list_node->type != YAML_SEQUENCE_NODE)
    {
        ERROR(YAML_ERR_PREFIX "unexpected node type in interface name list at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(if_name_list_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    item_top = if_name_list_node->data.sequence.items.top;
    item = if_name_list_node->data.sequence.items.start;
    name_list_size = item_top - item;

    if (name_list_size == 0)
    {
        ERROR(YAML_ERR_PREFIX "empty list of iterface names");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* The list must be NULL-terminated. */
    name_list = TE_ALLOC((name_list_size + 1) * sizeof(*name_list));

    for (i = 0; i < name_list_size; i++, item++)
    {
        yaml_node_t *if_name_node = yaml_document_get_node(doc, *item);

        if (if_name_node->type != YAML_SCALAR_NODE)
        {
            ERROR(YAML_ERR_PREFIX "unexpected node type in interface name list at " YAML_NODE_LINE_COLUMN_FMT,
                  YAML_NODE_LINE_COLUMN(if_name_node));
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            goto err;
        }

        rc = expanded_val_get_str(
                 (const char *)if_name_node->data.scalar.value, &name_list[i]);
        if (rc != 0)
        {
            ERROR(YAML_ERR_PREFIX "failed to expand interface name at " YAML_NODE_LINE_COLUMN_FMT ": %r",
                  YAML_NODE_LINE_COLUMN(if_name_node), rc);
            goto err;
        }
    }

    name_list[name_list_size] = NULL;
    *if_name_list = name_list;

    return 0;

err:
    te_str_free_array(name_list);

    return rc;
}

/* Parse node that represents interfaces associated with a specific agent. */
static te_errno
iface_node_parse(yaml_node_t *iface_list_node, yaml_document_t *doc,
                 char **ta_name, char ***if_name_list)
{
    yaml_node_t *if_name_list_node;
    yaml_node_pair_t *pair = NULL;
    cfg_yaml_iface_field field;
    char *ta_name_tmp = NULL;
    char **name_list = NULL;
    te_errno rc = 0;

    assert(iface_list_node != NULL);
    assert(if_name_list != NULL);
    assert(ta_name != NULL);
    assert(doc != NULL);

    if (iface_list_node->type != YAML_MAPPING_NODE)
    {
        ERROR(YAML_ERR_PREFIX "unexpected interface list node type at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(iface_list_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    pair = iface_list_node->data.mapping.pairs.start;

    do {
        yaml_node_t *k = yaml_document_get_node(doc, pair->key);
        yaml_node_t *v = yaml_document_get_node(doc, pair->value);

        if (k->type != YAML_SCALAR_NODE)
        {
            ERROR(YAML_ERR_PREFIX "unexpected interface node type at " YAML_NODE_LINE_COLUMN_FMT,
                  YAML_NODE_LINE_COLUMN(k));
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            goto err;
        }

        field = te_enum_map_from_str(cfg_yaml_iface_field_map,
                                     (char *)k->data.scalar.value,
                                     CFG_YAML_IFACE_FIELD_UNKNOWN);

        switch (field)
        {
            case CFG_YAML_IFACE_FIELD_AGENT:
                if (v->type != YAML_SCALAR_NODE)
                {
                    ERROR(YAML_ERR_PREFIX "unexpected field type in interface list at " YAML_NODE_LINE_COLUMN_FMT,
                          YAML_NODE_LINE_COLUMN(v));
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }
                rc = expanded_val_get_str(
                         (const char *)v->data.scalar.value, &ta_name_tmp);
                if (rc != 0)
                {
                    ERROR(YAML_ERR_PREFIX, "failed to expand test agent name: %r", rc);
                    goto err;
                }

                break;

            case CFG_YAML_IFACE_FIELD_NAME_LIST:
                if_name_list_node = v;
                break;

            default:
                ERROR(YAML_ERR_PREFIX "unexpected field in interface list node at " YAML_NODE_LINE_COLUMN_FMT,
                      YAML_NODE_LINE_COLUMN(k));
                rc = TE_RC(TE_TAPI, TE_EINVAL);
                goto err;
        }
    } while (++pair < iface_list_node->data.mapping.pairs.top);

    if (ta_name_tmp == NULL)
    {
        ERROR(YAML_ERR_PREFIX "interface agent missing name");
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto err;
    }

    if (if_name_list_node == NULL)
    {
        ERROR(YAML_ERR_PREFIX "interface name list node is missing for agent %s",
              ta_name_tmp);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto err;
    }

    rc = parse_if_name_list(if_name_list_node, doc, &name_list);
    if (rc != 0)
        goto err;

    *if_name_list = name_list;
    *ta_name = ta_name_tmp;

    return 0;

err:
    free(ta_name_tmp);

    return rc;

}

/* Parse list of interfaces and init network configuration for each agent. */
static te_errno
iface_list_node_parse(yaml_node_t *iface_list_node, yaml_document_t *doc,
                      tapi_net_ctx *net_ctx)
{
    yaml_node_item_t *item = iface_list_node->data.sequence.items.start;
    te_errno rc = 0;

    assert(net_ctx != NULL);
    assert(iface_list_node != NULL);
    assert(doc != NULL);

    do {
        tapi_net_ta agent = {};
        char **if_name_list = NULL;
        char *ta_name = NULL;

        yaml_node_t *iface_node = yaml_document_get_node(doc, *item);

        rc = iface_node_parse(iface_node, doc, &ta_name, &if_name_list);
        if (rc != 0)
        {
            ERROR(YAML_ERR_PREFIX "failed to parse interface node: %r", rc);
            return rc;
        }

        tapi_net_ta_init(ta_name, &agent);
        free(ta_name);

        tapi_net_ta_set_ifaces(&agent, (const char **)if_name_list);
        te_str_free_array(if_name_list);

        TE_VEC_APPEND(&net_ctx->agents, agent);
    } while(++item < iface_list_node->data.sequence.items.top);

    return 0;
}

static te_errno
lag_info_validate(const cfg_yaml_lag_ctx *lag_ctx)
{
    if (lag_ctx->agent == NULL || lag_ctx->name == NULL ||
        lag_ctx->type == TAPI_NET_LAG_TYPE_UNKNOWN ||
        lag_ctx->mode == TAPI_NET_LAG_MODE_UNKNOWN ||
        lag_ctx->slaves_node == NULL)
    {
        ERROR(YAML_ERR_PREFIX "incomplete LAG entry in 'aggregates'");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
}

/* Parse mapping fields of a single LAG entry. */
static te_errno
lag_node_parse_fields(yaml_node_t *lag_node, yaml_document_t *doc,
                      cfg_yaml_lag_ctx *ctx)
{
    yaml_node_pair_t *pair;
    te_errno rc;

    assert(ctx != NULL);
    assert(lag_node != NULL);
    assert(doc != NULL);

    if (lag_node->type != YAML_MAPPING_NODE)
    {
        ERROR(YAML_ERR_PREFIX "LAG entry must be a mapping at "
              YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(lag_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    pair = lag_node->data.mapping.pairs.start;

    do {
        yaml_node_t *k = yaml_document_get_node(doc, pair->key);
        yaml_node_t *v = yaml_document_get_node(doc, pair->value);

        if (k->type != YAML_SCALAR_NODE)
        {
            ERROR(YAML_ERR_PREFIX "unexpected key type in 'aggregates' at "
                  YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(k));
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        TE_ENUM_DISPATCH(cfg_lag_fld_acts, lag_node_unknown_field_handler,
                         (char *)k->data.scalar.value, rc, v, ctx);
        if (rc != 0)
            return rc;
    } while (++pair < lag_node->data.mapping.pairs.top);

    return 0;
}

/* Build NULL-terminated array of slave names. */
static te_errno
lag_node_build_slaves(yaml_document_t *doc,
                      const cfg_yaml_lag_ctx *ctx,
                      char ***slaves_out)
{
    yaml_node_item_t *item;
    yaml_node_item_t *top;
    char **slaves = NULL;
    size_t slaves_num;
    te_errno rc;
    size_t i;

    assert(doc != NULL);
    assert(ctx != NULL);
    assert(slaves_out != NULL);

    if (ctx->slaves_node == NULL)
    {
        ERROR(YAML_ERR_PREFIX "LAG 'slaves' list is required in 'aggregates'");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    item = ctx->slaves_node->data.sequence.items.start;
    top  = ctx->slaves_node->data.sequence.items.top;

    slaves_num = top - item;
    if (slaves_num == 0)
    {
        ERROR(YAML_ERR_PREFIX "empty 'slaves' list in 'aggregates'");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    slaves = TE_ALLOC((slaves_num + 1) * sizeof(*slaves));

    for (i = 0; item < top; ++item, ++i)
    {
        yaml_node_t *slave_node = yaml_document_get_node(doc, *item);

        if (slave_node->type != YAML_SCALAR_NODE)
        {
            ERROR(YAML_ERR_PREFIX "unexpected node type in 'slaves' at "
                  YAML_NODE_LINE_COLUMN_FMT, YAML_NODE_LINE_COLUMN(slave_node));
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            goto error;
        }

        rc = expanded_val_get_str((const char *)slave_node->data.scalar.value,
                                  &slaves[i]);
        if (rc != 0)
        {
            ERROR(YAML_ERR_PREFIX "failed to expand slave name in 'aggregates': %r",
                  rc);
            goto error;
        }
    }

    slaves[i] = NULL;
    *slaves_out = slaves;

    return 0;

error:
    te_str_free_array(slaves);

    return rc;
}

static te_errno
lag_node_parse(yaml_node_t *lag_node, yaml_document_t *doc,
               tapi_net_ctx *net_ctx)
{
    cfg_yaml_lag_ctx ctx = cfg_yaml_lag_ctx_def;
    char **slave_list = NULL;
    te_errno rc = 0;

    tapi_net_ta *ta;

    assert(net_ctx != NULL);
    assert(lag_node != NULL);
    assert(doc != NULL);

    rc = lag_node_parse_fields(lag_node, doc, &ctx);
    if (rc != 0)
        goto out;

    rc = lag_info_validate(&ctx);
    if (rc != 0)
        goto out;

    rc = lag_node_build_slaves(doc, &ctx, &slave_list);
    if (rc != 0)
        goto out;

    ta = tapi_net_find_agent_by_name(net_ctx, ctx.agent);
    if (ta == NULL)
    {
        ERROR(YAML_ERR_PREFIX "unknown agent '%s' in 'aggregates'", ctx.agent);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto out;
    }

    rc = tapi_net_ta_add_lag(ta, ctx.name, ctx.type, ctx.mode,
                             (const char **)slave_list);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to add LAG '%s' for agent '%s': %r",
              ctx.name, ctx.agent, rc);
        goto out;
    }

out:
    te_str_free_array(slave_list);

    free(ctx.agent);
    free(ctx.name);

    return rc;
}

/* Parse list of LAG definitions from 'aggregates' section. */
static te_errno
lag_list_node_parse(yaml_node_t *lag_list_node, yaml_document_t *doc,
                    tapi_net_ctx *net_ctx)
{
    yaml_node_item_t *item;
    te_errno rc;

    assert(net_ctx != NULL);
    assert(doc != NULL);

    if (lag_list_node == NULL)
        return 0;

    item = lag_list_node->data.sequence.items.start;

    do {
        yaml_node_t *lag_node = yaml_document_get_node(doc, *item);

        rc = lag_node_parse(lag_node, doc, net_ctx);
        if (rc != 0)
        {
            ERROR(YAML_ERR_PREFIX "failed to parse LAG node: %r", rc);
            return rc;
        }
    } while (++item < lag_list_node->data.sequence.items.top);

    return 0;
}

static te_errno
nat_rule_node_parse(yaml_node_t *rule_node, yaml_document_t *doc,
                    tapi_net_nat_rule *rule)
{
    yaml_node_pair_t *pair;
    te_errno rc;

    assert(rule_node != NULL);
    assert(rule != NULL);
    assert(doc != NULL);

    if (rule_node->type != YAML_MAPPING_NODE)
    {
        ERROR(YAML_ERR_PREFIX "NAT rule must be a mapping at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(rule_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    pair = rule_node->data.mapping.pairs.start;
    do {
        yaml_node_t *v = yaml_document_get_node(doc, pair->value);
        yaml_node_t *k = yaml_document_get_node(doc, pair->key);
        cfg_yaml_nat_rule_field field;
        tapi_net_endpoint *ep = NULL;
        char *ta_name = NULL;
        char *iface = NULL;

        if (k->type != YAML_SCALAR_NODE)
        {
            ERROR(YAML_ERR_PREFIX "unexpected NAT rule key type at " YAML_NODE_LINE_COLUMN_FMT,
                  YAML_NODE_LINE_COLUMN(k));
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        field = te_enum_map_from_str(cfg_yaml_nat_rule_field_map,
                                     (char *)k->data.scalar.value,
                                     CFG_YAML_NAT_RULE_FIELD_UNKNOWN);

        switch (field)
        {
            case CFG_YAML_NAT_RULE_FIELD_TYPE:
            {
                char *nat_type_str = NULL;

                if (v->type != YAML_SCALAR_NODE)
                {
                    ERROR(YAML_ERR_PREFIX "NAT 'type' must be scalar at " YAML_NODE_LINE_COLUMN_FMT,
                          YAML_NODE_LINE_COLUMN(v));
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }

                rc = expanded_val_get_str((char *)v->data.scalar.value,
                                          &nat_type_str);
                if (rc != 0)
                {
                    ERROR(YAML_ERR_PREFIX "failed to parse NAT type '%s' at " YAML_NODE_LINE_COLUMN_FMT ": %r",
                          (char *)v->data.scalar.value,
                           YAML_NODE_LINE_COLUMN(v), rc);
                    return rc;
                }

                rule->type = te_enum_map_from_str(
                                 cfg_yaml_nat_rule_type_map, nat_type_str,
                                 TAPI_NET_NAT_RULE_TYPE_UNKNOWN);
                free(nat_type_str);
                break;
            }

            case CFG_YAML_NAT_RULE_FIELD_MODE:
            {
                char *nat_mode_str = NULL;

                if (v->type != YAML_SCALAR_NODE)
                {
                    ERROR(YAML_ERR_PREFIX "NAT 'mode' must be scalar at " YAML_NODE_LINE_COLUMN_FMT,
                          YAML_NODE_LINE_COLUMN(v));
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }

                rc = expanded_val_get_str((char *)v->data.scalar.value,
                                          &nat_mode_str);
                if (rc != 0)
                {
                    ERROR(YAML_ERR_PREFIX "failed to parse NAT mode '%s' at " YAML_NODE_LINE_COLUMN_FMT ": %r",
                          (char *)v->data.scalar.value,
                          YAML_NODE_LINE_COLUMN(v), rc);
                    return rc;
                }

                rule->mode = te_enum_map_from_str(cfg_yaml_nat_rule_mode_map,
                                                  nat_mode_str,
                                                  TAPI_NET_NAT_RULE_MODE_UNKNOWN);
                free(nat_mode_str);
                break;
            }

            case CFG_YAML_NAT_RULE_FIELD_FROM:
                ep = &rule->from;
                /*@fallthrough@*/

            case CFG_YAML_NAT_RULE_FIELD_TO:
                if (ep == NULL)
                    ep = &rule->to;

                rc = endpoint_node_parse(v, doc, &ta_name, NULL, &iface);
                if (rc != 0)
                {
                    ERROR(YAML_ERR_PREFIX "failed to parse endpoint node: %r", rc);
                    return rc;
                }

                fill_ep_info(ep, ta_name, iface);
                free(ta_name);
                free(iface);

                break;

            default:
                ERROR(YAML_ERR_PREFIX "unsupported field in NAT rule '%s' at " YAML_NODE_LINE_COLUMN_FMT,
                      (char *)k->data.scalar.value, YAML_NODE_LINE_COLUMN(k));
                return TE_RC(TE_TAPI, TE_EINVAL);
        }
    } while (++pair < rule_node->data.mapping.pairs.top);

    rc = tapi_net_nat_rule_validate(rule);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to validate parsed NAT rule: %r", rc);
        return rc;
    }

    return 0;
}

static te_errno
nat_rule_list_node_parse(yaml_node_t *rule_list, yaml_document_t *doc,
                         const char *ta_name, tapi_net_ctx *net_ctx)
{
    yaml_node_item_t *item = rule_list->data.sequence.items.start;
    tapi_net_ta *agent = NULL;
    te_errno rc;

    assert(rule_list != NULL);
    assert(net_ctx != NULL);
    assert(ta_name != NULL);
    assert(doc != NULL);

    agent = tapi_net_find_agent_by_name(net_ctx, ta_name);
    if (agent == NULL)
    {
        ERROR(YAML_ERR_PREFIX "unknown NAT agent '%s'", ta_name);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    do {
        tapi_net_nat_rule rule;
        yaml_node_t *rule_node = yaml_document_get_node(doc, *item);

        tapi_net_nat_rule_init(&rule);
        rc = nat_rule_node_parse(rule_node, doc, &rule);
        if (rc != 0)
        {
            ERROR(YAML_ERR_PREFIX "failed to parse NAT rule at " YAML_NODE_LINE_COLUMN_FMT ": %r",
                  YAML_NODE_LINE_COLUMN(rule_node), rc);
            return rc;
        }

        rc = tapi_net_nat_rule_check_dup(agent, &rule);
        if (rc != 0)
            return rc;

        TE_VEC_APPEND(&agent->nat_rules, rule);
    } while (++item < rule_list->data.sequence.items.top);

    return 0;
}

static te_errno
nat_node_parse(yaml_node_t *nat_node, yaml_document_t *doc,
               tapi_net_ctx *net_ctx)
{
    yaml_node_t *rules_node = NULL;
    yaml_node_pair_t *pair;
    char *nat_agent = NULL;
    te_errno rc = 0;

    if (nat_node->type != YAML_MAPPING_NODE)
    {
        ERROR(YAML_ERR_PREFIX "unexpected NAT node type at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(nat_node));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    pair = nat_node->data.mapping.pairs.start;
    do {
        yaml_node_t *k = yaml_document_get_node(doc, pair->key);
        yaml_node_t *v = yaml_document_get_node(doc, pair->value);
        cfg_yaml_nat_rule_field field;

        if (k->type != YAML_SCALAR_NODE)
        {
            ERROR(YAML_ERR_PREFIX "unexpected NAT field type at " YAML_NODE_LINE_COLUMN_FMT,
                  YAML_NODE_LINE_COLUMN(k));
            rc = TE_EINVAL;
            goto out;
        }

        field = te_enum_map_from_str(cfg_yaml_nat_field_map,
                                     (char *)k->data.scalar.value,
                                     CFG_YAML_NAT_FIELD_UNKNOWN);

        switch (field)
        {
            case CFG_YAML_NAT_FIELD_AGENT:
                if (v->type != YAML_SCALAR_NODE)
                {
                    ERROR(YAML_ERR_PREFIX "NAT agent must be scalar at " YAML_NODE_LINE_COLUMN_FMT,
                          YAML_NODE_LINE_COLUMN(v));
                    rc = TE_EINVAL;
                    goto out;
                }

                rc = expanded_val_get_str((char *)v->data.scalar.value, &nat_agent);
                if (rc != 0)
                {
                    ERROR(YAML_ERR_PREFIX "failed to parse NAT agent name '%s' at " YAML_NODE_LINE_COLUMN_FMT ": %r",
                          (char *)v->data.scalar.value,
                          YAML_NODE_LINE_COLUMN(v), rc);
                    goto out;
                }
                break;

            case CFG_YAML_NAT_FIELD_RULES:
                if (v->type != YAML_SEQUENCE_NODE)
                {
                    ERROR(YAML_ERR_PREFIX "NAT rules must be a sequence at " YAML_NODE_LINE_COLUMN_FMT,
                          YAML_NODE_LINE_COLUMN(v));
                    rc = TE_EINVAL;
                    goto out;
                }

                rules_node = v;
                break;

            default:
                ERROR(YAML_ERR_PREFIX "unsupported field '%s' in NAT section at " YAML_NODE_LINE_COLUMN_FMT,
                      (char *)k->data.scalar.value, YAML_NODE_LINE_COLUMN(k));
                rc = TE_EINVAL;
                goto out;
        }
    } while (++pair < nat_node->data.mapping.pairs.top);

    if (nat_agent == NULL || rules_node == NULL)
    {
        ERROR(YAML_ERR_PREFIX "NAT is missing required fields");
        rc = TE_EINVAL;
        goto out;
    }

    rc = nat_rule_list_node_parse(rules_node, doc, nat_agent, net_ctx);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to parse node with list of NAT rules: %r",
              rc);
        return rc;
    }

out:
    free(nat_agent);
    return rc;
}

/* Parse root 'nat' list. */
static te_errno
nat_list_node_parse(yaml_node_t *nat_list_node, yaml_document_t *doc,
                    tapi_net_ctx *net_ctx)
{
    yaml_node_item_t *item;
    te_errno rc;

    assert(net_ctx != NULL);
    assert(doc != NULL);

    if (nat_list_node == NULL)
        return 0;

    item = nat_list_node->data.sequence.items.start;

    do {
        yaml_node_t *nat_node = yaml_document_get_node(doc, *item);

        rc = nat_node_parse(nat_node, doc, net_ctx);
        if (rc != 0)
            return rc;
    } while (++item < nat_list_node->data.sequence.items.top);

    return 0;
}

/*
 * Parse root node and fill network configuration context.
 * The function allocates memory for netwroks and agents mentioned
 * in the YAML config file.
 */
static te_errno
root_node_parse(yaml_node_t *root, yaml_document_t *doc,
                tapi_net_ctx *net_ctx)
{
    yaml_node_t *iface_list_node = NULL;
    yaml_node_t *lag_list_node = NULL;
    yaml_node_t *net_list_node = NULL;
    yaml_node_t *nat_list_node = NULL;
    cfg_yaml_root_field type;
    yaml_node_pair_t *pair;
    size_t num_agents;
    size_t num_nets;
    te_errno rc;

    assert(net_ctx != NULL);
    assert(root != NULL);
    assert(doc != NULL);

    if (root->type != YAML_MAPPING_NODE) {
        ERROR(YAML_ERR_PREFIX "unexpected root type at " YAML_NODE_LINE_COLUMN_FMT,
              YAML_NODE_LINE_COLUMN(root));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    pair = root->data.mapping.pairs.start;

    do {
        yaml_node_t *k = yaml_document_get_node(doc, pair->key);
        yaml_node_t *v = yaml_document_get_node(doc, pair->value);

        if (k->type != YAML_SCALAR_NODE)
        {
            ERROR(YAML_ERR_PREFIX "unexpected root node type at " YAML_NODE_LINE_COLUMN_FMT,
                  YAML_NODE_LINE_COLUMN(k));
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        type = te_enum_map_from_str(cfg_yaml_root_field_map,
                                    (char *)k->data.scalar.value,
                                    CFG_YAML_ROOT_FIELD_UNKNOWN);

        switch (type)
        {
            case CFG_YAML_ROOT_FIELD_IFACE_LIST:
                iface_list_node = v;

                if (iface_list_node->type != YAML_SEQUENCE_NODE)
                {
                    ERROR(YAML_ERR_PREFIX "interface list node is not a sequence");
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }

                num_agents = iface_list_node->data.sequence.items.top -
                             iface_list_node->data.sequence.items.start;

                if (num_agents == 0)
                {
                    ERROR(YAML_ERR_PREFIX "interface list node is empty");
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }

                break;

            case CFG_YAML_ROOT_FIELD_LAG_LIST:
                lag_list_node = v;

                if (lag_list_node->type != YAML_SEQUENCE_NODE)
                {
                    ERROR(YAML_ERR_PREFIX "aggregates list node is not a sequence");
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }

                break;

            case CFG_YAML_ROOT_FIELD_NET_LIST:
                net_list_node = v;

                if (net_list_node->type != YAML_SEQUENCE_NODE)
                {
                    ERROR(YAML_ERR_PREFIX "network list node is not a sequence");
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }

                num_nets = net_list_node->data.sequence.items.top -
                           net_list_node->data.sequence.items.start;

                if (num_nets == 0)
                {
                    ERROR(YAML_ERR_PREFIX "network list is empty");
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }

                break;

            case CFG_YAML_ROOT_FIELD_NAT_LIST:
                nat_list_node = v;

                if (nat_list_node->type != YAML_SEQUENCE_NODE)
                {
                    ERROR(YAML_ERR_PREFIX "NAT list node is not a sequence");
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }

                break;

            default:
                ERROR(YAML_ERR_PREFIX "unexpected root field '%s' at " YAML_NODE_LINE_COLUMN_FMT,
                      (char *)k->data.scalar.value, YAML_NODE_LINE_COLUMN(k));
                return TE_RC(TE_TAPI, TE_EINVAL);
        }
    } while (++pair < root->data.mapping.pairs.top);

    if (iface_list_node == NULL || net_list_node == NULL)
    {
        ERROR(YAML_ERR_PREFIX "'nets' or 'interfaces' are missing");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = iface_list_node_parse(iface_list_node, doc, net_ctx);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to parse interface list node");
        return rc;
    }

    rc = lag_list_node_parse(lag_list_node, doc, net_ctx);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to parse aggregates list node");
        return rc;
    }

    rc = net_list_node_parse(net_list_node, doc, net_ctx);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to parse network list node");
        return rc;
    }

    rc = nat_list_node_parse(nat_list_node, doc, net_ctx);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to parse NAT list node");
        return rc;
    }

    return 0;
}

te_errno
tapi_net_yaml_parse(const char *filename, tapi_net_ctx *net_ctx)
{
    yaml_node_t *root = NULL;
    yaml_parser_t parser;
    yaml_document_t doc;
    te_errno rc = 0;
    FILE *f = NULL;

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        ERROR(YAML_ERR_PREFIX "failed to open target file '%s'",
              filename);
        return te_rc_os2te(errno);
    }

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);
    yaml_parser_load(&parser, &doc);
    fclose(f);

    root = yaml_document_get_root_node(&doc);
    if (root == NULL)
    {
        ERROR(YAML_ERR_PREFIX "failed to get root node in file %s'", filename);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto out;
    }

    rc = root_node_parse(root, &doc, net_ctx);
    if (rc != 0)
    {
        ERROR(YAML_ERR_PREFIX "failed to parse root node: %r", rc);
        goto out;
    }

out:
    yaml_document_delete(&doc);
    yaml_parser_delete(&parser);

    return rc;
}
