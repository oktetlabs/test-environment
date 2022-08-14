/** @file
 * @brief RPC client API for RTE flow
 *
 * RPC client API for RTE flow functions
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include "log_bufs.h"
#include "tapi_mem.h"
#include "tapi_rpc_internal.h"
#include "tapi_test_log.h"

#include "tarpc.h"

#include "tapi_rpc_rte_ethdev.h"
#include "tapi_rpc_rte_flow.h"
#include "rte_flow_ndn.h"
#include "rpcc_dpdk.h"

void
rpc_rte_free_flow_rule(rcf_rpc_server *rpcs,
                       rpc_rte_flow_attr_p attr,
                       rpc_rte_flow_item_p pattern,
                       rpc_rte_flow_action_p actions)
{
    tarpc_rte_free_flow_rule_in  in;
    tarpc_rte_free_flow_rule_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.attr = (tarpc_rte_flow_attr)attr;
    in.pattern = (tarpc_rte_flow_item)pattern;
    in.actions = (tarpc_rte_flow_action)actions;

    rcf_rpc_call(rpcs, "rte_free_flow_rule", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_free_flow_rule,
                 RPC_PTR_FMT", "RPC_PTR_FMT", "RPC_PTR_FMT, "",
                 RPC_PTR_VAL(in.attr), RPC_PTR_VAL(in.pattern),
                 RPC_PTR_VAL(in.actions));

    RETVAL_VOID(rte_free_flow_rule);
}

static uint8_t
tarpc_rte_flow_type2rpc_flags(const asn_type *type)
{
    if (type == ndn_rte_flow_attr)
        return TARPC_RTE_FLOW_ATTR_FLAG;
    else if (type == ndn_rte_flow_pattern)
        return TARPC_RTE_FLOW_PATTERN_FLAG;
    else if (type == ndn_rte_flow_actions)
        return TARPC_RTE_FLOW_ACTIONS_FLAG;
    else if (type == ndn_rte_flow_rule)
        return TARPC_RTE_FLOW_RULE_FLAGS;

    return 0;
}

int
rpc_rte_mk_flow_rule_components(rcf_rpc_server *rpcs,
                                const asn_value *flow_rule_components,
                                rpc_rte_flow_attr_p *attr,
                                rpc_rte_flow_item_p *pattern,
                                rpc_rte_flow_action_p *actions)
{
    tarpc_rte_mk_flow_rule_components_in    in;
    tarpc_rte_mk_flow_rule_components_out   out;
    size_t                                  flow_len;
    const asn_type                         *flow_rule_components_type;
    uint8_t                                 component_flags;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    flow_len = asn_count_txt_len(flow_rule_components, 0) + 1;
    in.flow_rule_components = tapi_calloc(1, flow_len);

    flow_rule_components_type = asn_get_type(flow_rule_components);
    component_flags = tarpc_rte_flow_type2rpc_flags(flow_rule_components_type);
    if (component_flags == 0)
    {
        ERROR("%s(): invalid flow rule components ASN.1 type", __FUNCTION__);
        RETVAL_ZERO_INT(rte_mk_flow_rule_components, -EINVAL);
    }
    in.component_flags = component_flags;

    if (((TARPC_RTE_FLOW_ATTR_FLAG & component_flags) && (attr == NULL)) ||
        ((TARPC_RTE_FLOW_PATTERN_FLAG & component_flags) && (pattern == NULL)) ||
        ((TARPC_RTE_FLOW_ACTIONS_FLAG & component_flags) && (actions == NULL)))
    {
        ERROR("%s(): no RPC pointer for rte flow rule component", __FUNCTION__);
        RETVAL_ZERO_INT(rte_mk_flow_rule_components, -EINVAL);
    }

    if (asn_sprint_value(flow_rule_components, in.flow_rule_components,
                         flow_len, 0) <= 0)
        TEST_FAIL("Failed to prepare textual representation of ASN.1 flow rule");

    rcf_rpc_call(rpcs, "rte_mk_flow_rule_components", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_mk_flow_rule_components, out.retval);

    TAPI_RPC_LOG(rpcs, rte_mk_flow_rule_components, "type=%s,\n%s\n",
                 "{"RPC_PTR_FMT", "RPC_PTR_FMT", "RPC_PTR_FMT"}, "
                 NEG_ERRNO_FMT, asn_get_type_name(flow_rule_components_type),
                 in.flow_rule_components, RPC_PTR_VAL(out.attr),
                 RPC_PTR_VAL(out.pattern), RPC_PTR_VAL(out.actions),
                 NEG_ERRNO_ARGS(out.retval));

    if (TARPC_RTE_FLOW_ATTR_FLAG & component_flags)
        *attr = (rpc_rte_flow_attr_p)out.attr;
    if (TARPC_RTE_FLOW_PATTERN_FLAG & component_flags)
        *pattern = (rpc_rte_flow_item_p)out.pattern;
    if (TARPC_RTE_FLOW_ACTIONS_FLAG & component_flags)
        *actions = (rpc_rte_flow_action_p)out.actions;

    RETVAL_ZERO_INT(rte_mk_flow_rule_components, out.retval);
}

static const char *
tarpc_rte_flow_error2str(te_log_buf *tlbp, struct tarpc_rte_flow_error *error)
{
    te_log_buf_append(tlbp, ", rte flow error type %d(", error->type);

#define CASE_ERROR_TYPE2STR(_type, _str) \
    case TARPC_RTE_FLOW_ERROR_TYPE_##_type:     \
            te_log_buf_append(tlbp, _str);      \
            break

    switch(error->type)
    {
        CASE_ERROR_TYPE2STR(NONE, "no error");
        CASE_ERROR_TYPE2STR(UNSPECIFIED, "cause unspecified");
        CASE_ERROR_TYPE2STR(HANDLE, "flow rule (handle)");
        CASE_ERROR_TYPE2STR(ATTR_GROUP, "group field");
        CASE_ERROR_TYPE2STR(ATTR_PRIORITY, "priority field");
        CASE_ERROR_TYPE2STR(ATTR_INGRESS, "ingress field");
        CASE_ERROR_TYPE2STR(ATTR_EGRESS, "egress field");
        CASE_ERROR_TYPE2STR(ATTR_TRANSFER, "transfer field");
        CASE_ERROR_TYPE2STR(ATTR, "attributes structure");
        CASE_ERROR_TYPE2STR(ITEM_NUM, "pattern length");
        CASE_ERROR_TYPE2STR(ITEM_SPEC, "item specification");
        CASE_ERROR_TYPE2STR(ITEM_LAST, "item specification range");
        CASE_ERROR_TYPE2STR(ITEM_MASK, "item specification mask");
        CASE_ERROR_TYPE2STR(ITEM, "specific pattern item");
        CASE_ERROR_TYPE2STR(ACTION_NUM, "number of actions");
        CASE_ERROR_TYPE2STR(ACTION_CONF, "action configuration");
        CASE_ERROR_TYPE2STR(ACTION, "specific action");
        default:
             te_log_buf_append(tlbp, "unknown type");
    }
#undef CASE_ERROR_TYPE2STR

    te_log_buf_append(tlbp, "): %s", (error->message != NULL) ?
                      error->message : "no stated reason");

    return te_log_buf_get(tlbp);
}

static void
tarpc_rte_flow_error_copy(struct tarpc_rte_flow_error *dst,
                          struct tarpc_rte_flow_error *src)
{
    if (dst != NULL && src != NULL)
    {
        dst->type = src->type;

        if (src->message != NULL)
            dst->message = tapi_strdup(src->message);
    }
}

int
rpc_rte_flow_validate(rcf_rpc_server *rpcs,
                      uint16_t port_id,
                      rpc_rte_flow_attr_p attr,
                      rpc_rte_flow_item_p pattern,
                      rpc_rte_flow_action_p actions,
                      struct tarpc_rte_flow_error *error)
{
    tarpc_rte_flow_validate_in      in;
    tarpc_rte_flow_validate_out     out;
    te_log_buf                     *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.attr = (tarpc_rte_flow_attr)attr;
    in.pattern = (tarpc_rte_flow_item)pattern;
    in.actions = (tarpc_rte_flow_action)actions;

    rcf_rpc_call(rpcs, "rte_flow_validate", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_flow_validate, out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_flow_validate,
                 "%hu, "RPC_PTR_FMT", "RPC_PTR_FMT", "RPC_PTR_FMT,
                 NEG_ERRNO_FMT "%s", in.port_id, RPC_PTR_VAL(in.attr),
                 RPC_PTR_VAL(in.pattern), RPC_PTR_VAL(in.actions),
                 NEG_ERRNO_ARGS(out.retval), (out.retval != 0) ?
                 tarpc_rte_flow_error2str(tlbp, &out.error) : "");
    te_log_buf_free(tlbp);

    tarpc_rte_flow_error_copy(error, &out.error);

    RETVAL_ZERO_INT(rte_flow_validate, out.retval);
}

rpc_rte_flow_p
rpc_rte_flow_create(rcf_rpc_server *rpcs,
                    uint16_t port_id,
                    rpc_rte_flow_attr_p attr,
                    rpc_rte_flow_item_p pattern,
                    rpc_rte_flow_action_p actions,
                    struct tarpc_rte_flow_error *error)
{
    tarpc_rte_flow_create_in        in;
    tarpc_rte_flow_create_out       out;
    te_log_buf                     *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.attr = (tarpc_rte_flow_attr)attr;
    in.pattern = (tarpc_rte_flow_item)pattern;
    in.actions = (tarpc_rte_flow_action)actions;

    rcf_rpc_call(rpcs, "rte_flow_create", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_flow_create, out.flow);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_flow_create,
                 "%hu, "RPC_PTR_FMT", "RPC_PTR_FMT", "RPC_PTR_FMT,
                 RPC_PTR_FMT "%s", in.port_id, RPC_PTR_VAL(in.attr),
                 RPC_PTR_VAL(in.pattern), RPC_PTR_VAL(in.actions),
                 RPC_PTR_VAL(out.flow), (out.error.type != 0) ?
                 tarpc_rte_flow_error2str(tlbp, &out.error) : "");
    te_log_buf_free(tlbp);

    tarpc_rte_flow_error_copy(error, &out.error);

    RETVAL_RPC_PTR(rte_flow_create, out.flow);
}

int
rpc_rte_flow_destroy(rcf_rpc_server *rpcs,
                     uint16_t port_id,
                     rpc_rte_flow_p flow,
                     struct tarpc_rte_flow_error *error)
{
    tarpc_rte_flow_destroy_in       in;
    tarpc_rte_flow_destroy_out      out;
    te_log_buf                     *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.flow = (tarpc_rte_flow)flow;

    rcf_rpc_call(rpcs, "rte_flow_destroy", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_flow_destroy, out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_flow_destroy, "%hu, "RPC_PTR_FMT,
                 NEG_ERRNO_FMT "%s", in.port_id, RPC_PTR_VAL(in.flow),
                 NEG_ERRNO_ARGS(out.retval), (out.retval != 0) ?
                 tarpc_rte_flow_error2str(tlbp, &out.error) : "");
    te_log_buf_free(tlbp);

    tarpc_rte_flow_error_copy(error, &out.error);

    RETVAL_ZERO_INT(rte_flow_destroy, out.retval);
}

static const char *
tarpc_rte_flow_query_data2str(te_log_buf *tlbp, const struct tarpc_rte_flow_query_data *data)
{
    switch (data->type)
    {
        case TARPC_RTE_FLOW_QUERY_DATA_COUNT:
            te_log_buf_append(tlbp,
                              "{ hits_set: %u, hits: %u, bytes_set: %u, bytes: %u }",
                              data->tarpc_rte_flow_query_data_u.count.hits_set,
                              data->tarpc_rte_flow_query_data_u.count.hits,
                              data->tarpc_rte_flow_query_data_u.count.bytes_set,
                              data->tarpc_rte_flow_query_data_u.count.bytes);
            break;
        default:
            te_log_buf_append(tlbp, "Unknown type");
            break;
    }

    return te_log_buf_get(tlbp);
}

int
rpc_rte_flow_query(rcf_rpc_server *rpcs, uint16_t port_id,
                   rpc_rte_flow_p flow, rpc_rte_flow_action_p action,
                   tarpc_rte_flow_query_data *data, tarpc_rte_flow_error *error)
{
    tarpc_rte_flow_query_in in;
    tarpc_rte_flow_query_out out;
    te_log_buf *tlbp_err;
    te_log_buf *tlbp_data;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.flow = (tarpc_rte_flow)flow;
    in.action = (tarpc_rte_flow_action)action;
    in.data = *data;

    rcf_rpc_call(rpcs, "rte_flow_query", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_flow_query, out.retval);

    tlbp_err = te_log_buf_alloc();
    tlbp_data = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_flow_query, "%hu, "RPC_PTR_FMT", "RPC_PTR_FMT", %s",
                 NEG_ERRNO_FMT "%s", in.port_id, RPC_PTR_VAL(in.flow),
                 RPC_PTR_VAL(in.action),
                 tarpc_rte_flow_query_data2str(tlbp_data, &out.data),
                 NEG_ERRNO_ARGS(out.retval), (out.retval != 0) ?
                 tarpc_rte_flow_error2str(tlbp_err, &out.error) : "");
    te_log_buf_free(tlbp_data);
    te_log_buf_free(tlbp_err);

    tarpc_rte_flow_error_copy(error, &out.error);
    *data = out.data;

    RETVAL_ZERO_INT(rte_flow_query, out.retval);
}


int
rpc_rte_flow_flush(rcf_rpc_server *rpcs,
                   uint16_t port_id,
                   struct tarpc_rte_flow_error *error)
{
    tarpc_rte_flow_flush_in         in;
    tarpc_rte_flow_flush_out        out;
    te_log_buf                     *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_flow_flush", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_flow_flush, out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_flow_flush, "%hu",
                 NEG_ERRNO_FMT "%s", in.port_id,
                 NEG_ERRNO_ARGS(out.retval), (out.retval != 0) ?
                 tarpc_rte_flow_error2str(tlbp, &out.error) : "");
    te_log_buf_free(tlbp);

    tarpc_rte_flow_error_copy(error, &out.error);

    RETVAL_ZERO_INT(rte_flow_flush, out.retval);
}

int
rpc_rte_flow_isolate(rcf_rpc_server              *rpcs,
                     uint16_t                     port_id,
                     int                          set,
                     struct tarpc_rte_flow_error *error)
{
    tarpc_rte_flow_isolate_in   in;
    tarpc_rte_flow_isolate_out  out;
    te_log_buf                 *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    in.set = set;

    rcf_rpc_call(rpcs, "rte_flow_isolate", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_flow_isolate, out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_flow_isolate, "%hu, %d", NEG_ERRNO_FMT "%s",
                 in.port_id, in.set, NEG_ERRNO_ARGS(out.retval),
                 (out.retval != 0) ?
                 tarpc_rte_flow_error2str(tlbp, &out.error) : "");
    te_log_buf_free(tlbp);

    tarpc_rte_flow_error_copy(error, &out.error);

    RETVAL_ZERO_INT(rte_flow_isolate, out.retval);
}

int
rpc_rte_insert_flow_rule_items(rcf_rpc_server *rpcs,
                               rpc_rte_flow_item_p *pattern,
                               const asn_value *items,
                               int index)
{
    tarpc_rte_insert_flow_rule_items_in in;
    tarpc_rte_insert_flow_rule_items_out out;
    size_t len;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.pattern = (tarpc_rte_flow_item)*pattern;
    len = asn_count_txt_len(items, 0) + 1;
    in.items = tapi_calloc(1, len);
    in.index = index;

    if (asn_sprint_value(items, in.items, len, 0) <= 0)
        TEST_FAIL("Failed to prepare text representation of ASN.1 flow items");

    rcf_rpc_call(rpcs, "rte_insert_flow_rule_items", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_insert_flow_rule_items, out.retval);

    TAPI_RPC_LOG(rpcs, rte_insert_flow_rule_items, RPC_PTR_FMT ",\n%s, %d",
                 "{ " RPC_PTR_FMT ", " NEG_ERRNO_FMT " }",
                 RPC_PTR_VAL(in.pattern), in.items, in.index,
                 RPC_PTR_VAL(out.pattern), NEG_ERRNO_ARGS(out.retval));

    if (out.retval == 0)
        *pattern = out.pattern;

    RETVAL_ZERO_INT(rte_insert_flow_rule_items, out.retval);
}

static const char *
tarpc_rte_flow_tunnel2str(te_log_buf                          *lb,
                          const struct tarpc_rte_flow_tunnel  *tunnel)
{
    te_log_buf_append(lb, "{ type=%s, tun_id=0x%" PRIx64 " }",
                      tarpc_rte_eth_tunnel_type2str(tunnel->type),
                      tunnel->tun_id);

    return te_log_buf_get(lb);
}

static const char *
tarpc_memidx_to_str(te_log_buf     *lb,
                    const rpc_ptr  *ptr)
{
    te_log_buf_append(lb, "(%#x)", *ptr);

    return te_log_buf_get(lb);
}

static const char *
tarpc_uint32_to_str(te_log_buf      *lb,
                    const uint32_t  *val)
{
    te_log_buf_append(lb, "%" PRIu32, *val);

    return te_log_buf_get(lb);
}

int
rpc_rte_flow_tunnel_decap_set(
                            rcf_rpc_server                      *rpcs,
                            uint16_t                             port_id,
                            const struct tarpc_rte_flow_tunnel  *tunnel,
                            rpc_rte_flow_action_p               *actions,
                            uint32_t                            *num_of_actions,
                            struct tarpc_rte_flow_error         *error)
{
    te_log_buf                           *lb_num_of_actions;
    te_log_buf                           *lb_actions;
    te_log_buf                           *lb_tunnel;
    te_log_buf                           *lb_error;
    tarpc_rte_flow_tunnel_decap_set_out   out = {};
    tarpc_rte_flow_tunnel_decap_set_in    in = {};

    TAPI_RPC_SET_IN_ARG_IF_PTR_NOT_NULL(num_of_actions);
    TAPI_RPC_SET_IN_ARG_IF_PTR_NOT_NULL(actions);
    TAPI_RPC_SET_IN_ARG_IF_PTR_NOT_NULL(tunnel);

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_flow_tunnel_decap_set", &in, &out);

    TAPI_RPC_CHECK_OUT_ARG_SINGLE_PTR(rte_flow_tunnel_decap_set, actions);
    TAPI_RPC_CHECK_OUT_ARG_SINGLE_PTR(rte_flow_tunnel_decap_set,
                                      num_of_actions);

    lb_num_of_actions = te_log_buf_alloc();
    lb_actions = te_log_buf_alloc();
    lb_tunnel = te_log_buf_alloc();
    lb_error = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, rte_flow_tunnel_decap_set, "port_id=%" PRIu16 ", "
                 "tunnel=%s", "actions=%s, num_of_actions=%s; "
                 NEG_ERRNO_FMT "%s", in.port_id,
                 TAPI_RPC_LOG_ARG_TO_STR(in, tunnel, lb_tunnel,
                                         tarpc_rte_flow_tunnel2str),
                 TAPI_RPC_LOG_ARG_TO_STR(out, actions, lb_actions,
                                         tarpc_memidx_to_str),
                 TAPI_RPC_LOG_ARG_TO_STR(out, num_of_actions, lb_num_of_actions,
                                         tarpc_uint32_to_str),
                 NEG_ERRNO_ARGS(out.retval),
                 (error != NULL) ?
                     tarpc_rte_flow_error2str(lb_error, &out.error) : "");

    te_log_buf_free(lb_num_of_actions);
    te_log_buf_free(lb_actions);
    te_log_buf_free(lb_tunnel);
    te_log_buf_free(lb_error);

    TAPI_RPC_COPY_OUT_ARG_IF_PTR_NOT_NULL(num_of_actions);
    TAPI_RPC_COPY_OUT_ARG_IF_PTR_NOT_NULL(actions);

    if (error != NULL)
        *error = out.error;

    RETVAL_ZERO_INT(rte_flow_tunnel_decap_set, out.retval);
}

int
rpc_rte_flow_tunnel_match(rcf_rpc_server                      *rpcs,
                          uint16_t                             port_id,
                          const struct tarpc_rte_flow_tunnel  *tunnel,
                          rpc_rte_flow_item_p                *items,
                          uint32_t                            *num_of_items,
                          struct tarpc_rte_flow_error         *error)
{
    te_log_buf                       *lb_num_of_items;
    te_log_buf                       *lb_tunnel;
    te_log_buf                       *lb_items;
    te_log_buf                       *lb_error;
    tarpc_rte_flow_tunnel_match_out   out = {};
    tarpc_rte_flow_tunnel_match_in    in = {};

    TAPI_RPC_SET_IN_ARG_IF_PTR_NOT_NULL(num_of_items);
    TAPI_RPC_SET_IN_ARG_IF_PTR_NOT_NULL(tunnel);
    TAPI_RPC_SET_IN_ARG_IF_PTR_NOT_NULL(items);

    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_flow_tunnel_match", &in, &out);

    TAPI_RPC_CHECK_OUT_ARG_SINGLE_PTR(rte_flow_tunnel_match, num_of_items);
    TAPI_RPC_CHECK_OUT_ARG_SINGLE_PTR(rte_flow_tunnel_match, items);

    lb_num_of_items = te_log_buf_alloc();
    lb_tunnel = te_log_buf_alloc();
    lb_items = te_log_buf_alloc();
    lb_error = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, rte_flow_tunnel_match, "port_id=%" PRIu16 ", "
                 "tunnel=%s", "items=%s, num_of_items=%s; "
                 NEG_ERRNO_FMT "%s", in.port_id,
                 TAPI_RPC_LOG_ARG_TO_STR(in, tunnel, lb_tunnel,
                                         tarpc_rte_flow_tunnel2str),
                 TAPI_RPC_LOG_ARG_TO_STR(out, items, lb_items,
                                         tarpc_memidx_to_str),
                 TAPI_RPC_LOG_ARG_TO_STR(out, num_of_items, lb_num_of_items,
                                         tarpc_uint32_to_str),
                 NEG_ERRNO_ARGS(out.retval),
                 (error != NULL) ?
                     tarpc_rte_flow_error2str(lb_error, &out.error) : "");

    te_log_buf_free(lb_num_of_items);
    te_log_buf_free(lb_tunnel);
    te_log_buf_free(lb_items);
    te_log_buf_free(lb_error);

    TAPI_RPC_COPY_OUT_ARG_IF_PTR_NOT_NULL(num_of_items);
    TAPI_RPC_COPY_OUT_ARG_IF_PTR_NOT_NULL(items);

    if (error != NULL)
        *error = out.error;

    RETVAL_ZERO_INT(rte_flow_tunnel_match, out.retval);
}

static const char *
tarpc_rte_flow_restore_info2str(te_log_buf                          *lb,
                                struct tarpc_rte_flow_restore_info  *info)
{
    const struct te_log_buf_bit2str  bit2str[] = {

#define TARPC_RTE_FLOW_RESTORE_INFO_BIT2STR(_bit)           \
        { TARPC_RTE_FLOW_RESTORE_INFO_##_bit##_BIT, #_bit }

        TARPC_RTE_FLOW_RESTORE_INFO_BIT2STR(ENCAPSULATED),
        TARPC_RTE_FLOW_RESTORE_INFO_BIT2STR(GROUP_ID),
        TARPC_RTE_FLOW_RESTORE_INFO_BIT2STR(TUNNEL),

#undef TARPC_RTE_FLOW_RESTORE_INFO_BIT2STR

        { 0, NULL }
    };

    te_log_buf_append(lb, "{ flags=");

    te_bit_mask2log_buf(lb, info->flags, bit2str);

    te_log_buf_append(lb, ", group_id=%" PRIu32 ", tunnel=", info->group_id);

    tarpc_rte_flow_tunnel2str(lb, &info->tunnel);

    te_log_buf_append(lb, " }");

    return te_log_buf_get(lb);
}

int
rpc_rte_flow_get_restore_info(rcf_rpc_server                      *rpcs,
                              uint16_t                             port_id,
                              rpc_rte_mbuf_p                       m,
                              struct tarpc_rte_flow_restore_info  *info,
                              struct tarpc_rte_flow_error         *error)
{
    te_log_buf                           *lb_error;
    te_log_buf                           *lb_info;
    tarpc_rte_flow_get_restore_info_out   out = {};
    tarpc_rte_flow_get_restore_info_in    in = {};

    TAPI_RPC_SET_IN_ARG_IF_PTR_NOT_NULL(info);

    in.port_id = port_id;
    in.m = m;

    rcf_rpc_call(rpcs, "rte_flow_get_restore_info", &in, &out);

    TAPI_RPC_CHECK_OUT_ARG_SINGLE_PTR(rte_flow_get_restore_info, info);

    lb_error = te_log_buf_alloc();
    lb_info = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, rte_flow_get_restore_info, "port_id=%" PRIu16 ", "
                 "m=" RPC_PTR_FMT, "info=%s; " NEG_ERRNO_FMT "%s",
                 in.port_id, RPC_PTR_VAL(in.m),
                 TAPI_RPC_LOG_ARG_TO_STR(out, info, lb_info,
                                         tarpc_rte_flow_restore_info2str),
                 NEG_ERRNO_ARGS(out.retval),
                 (error != NULL) ?
                     tarpc_rte_flow_error2str(lb_error, &out.error) : "");

    te_log_buf_free(lb_error);
    te_log_buf_free(lb_info);

    TAPI_RPC_COPY_OUT_ARG_IF_PTR_NOT_NULL(info);

    if (error != NULL)
        *error = out.error;

    return out.retval;
}

int
rpc_rte_flow_tunnel_action_decap_release(
                                   rcf_rpc_server               *rpcs,
                                   uint16_t                      port_id,
                                   rpc_rte_flow_action_p         actions,
                                   uint32_t                      num_of_actions,
                                   struct tarpc_rte_flow_error  *error)
{
    te_log_buf                                      *lb_error;
    tarpc_rte_flow_tunnel_action_decap_release_out   out = {};
    tarpc_rte_flow_tunnel_action_decap_release_in    in = {};

    in.num_of_actions = num_of_actions;
    in.actions = actions;
    in.port_id = port_id;

    rcf_rpc_call(rpcs, "rte_flow_tunnel_action_decap_release", &in, &out);

    lb_error = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, rte_flow_tunnel_action_decap_release,
                 "port_id=%" PRIu16 ", actions=" RPC_PTR_FMT ", "
                 "num_of_actions=%" PRIu32, "; " NEG_ERRNO_FMT "%s",
                 in.port_id, RPC_PTR_VAL(in.actions), in.num_of_actions,
                 NEG_ERRNO_ARGS(out.retval),
                 (error != NULL) ?
                     tarpc_rte_flow_error2str(lb_error, &out.error) : "");

    te_log_buf_free(lb_error);

    if (error != NULL)
        *error = out.error;

    RETVAL_ZERO_INT(rte_flow_tunnel_action_decap_release, out.retval);
}

int
rpc_rte_flow_tunnel_item_release(rcf_rpc_server               *rpcs,
                                 uint16_t                      port_id,
                                 rpc_rte_flow_item_p          items,
                                 uint32_t                      num_of_items,
                                 struct tarpc_rte_flow_error  *error)
{
    te_log_buf                              *lb_error;
    tarpc_rte_flow_tunnel_item_release_out   out = {};
    tarpc_rte_flow_tunnel_item_release_in    in = {};

    in.num_of_items = num_of_items;
    in.port_id = port_id;
    in.items = items;

    rcf_rpc_call(rpcs, "rte_flow_tunnel_item_release", &in, &out);

    lb_error = te_log_buf_alloc();

    TAPI_RPC_LOG(rpcs, rte_flow_tunnel_item_release,
                 "port_id=%" PRIu16 ", items=" RPC_PTR_FMT ", "
                 "num_of_items=%" PRIu32, "; " NEG_ERRNO_FMT "%s",
                 in.port_id, RPC_PTR_VAL(in.items), in.num_of_items,
                 NEG_ERRNO_ARGS(out.retval),
                 (error != NULL) ?
                     tarpc_rte_flow_error2str(lb_error, &out.error) : "");

    te_log_buf_free(lb_error);

    if (error != NULL)
        *error = out.error;

    RETVAL_ZERO_INT(rte_flow_tunnel_item_release, out.retval);
}

int
rpc_rte_flow_prepend_opaque_actions(rcf_rpc_server         *rpcs,
                                    rpc_rte_flow_action_p   flow_actions,
                                    rpc_rte_flow_action_p   opaque_actions,
                                    unsigned int            nb_opaque_actions,
                                    rpc_rte_flow_action_p  *united_actions)
{
    tarpc_rte_flow_prepend_opaque_actions_out  out = {};
    tarpc_rte_flow_prepend_opaque_actions_in   in = {};

    if (flow_actions == RPC_NULL || opaque_actions == RPC_NULL ||
        nb_opaque_actions == 0 || united_actions == NULL)
        RETVAL_ZERO_INT(rte_flow_prepend_opaque_actions, -EINVAL);

    in.nb_opaque_actions = nb_opaque_actions;
    in.opaque_actions = opaque_actions;
    in.flow_actions = flow_actions;

    rcf_rpc_call(rpcs, "rte_flow_prepend_opaque_actions", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_flow_prepend_opaque_actions,
                 "flow_actions=" RPC_PTR_FMT ", "
                 "opaque_actions=" RPC_PTR_FMT ", nb_opaque_actions=%u",
                 RPC_PTR_FMT "; " NEG_ERRNO_FMT, RPC_PTR_VAL(in.flow_actions),
                 RPC_PTR_VAL(in.opaque_actions), in.nb_opaque_actions,
                 RPC_PTR_VAL(out.united_actions), NEG_ERRNO_ARGS(out.retval));

    *united_actions = out.united_actions;

    RETVAL_ZERO_INT(rte_flow_prepend_opaque_actions, out.retval);
}

void
rpc_rte_flow_release_united_actions(rcf_rpc_server         *rpcs,
                                    rpc_rte_flow_action_p   united_actions)
{
    tarpc_rte_flow_release_united_actions_out  out = {};
    tarpc_rte_flow_release_united_actions_in   in = {};

    if (united_actions == RPC_NULL)
        RETVAL_VOID(rte_flow_release_united_actions);

    in.united_actions = united_actions;

    rcf_rpc_call(rpcs, "rte_flow_release_united_actions", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_flow_release_united_actions,
                 RPC_PTR_FMT, "", RPC_PTR_VAL(in.united_actions));

    RETVAL_VOID(rte_flow_release_united_actions);
}

int
rpc_rte_flow_prepend_opaque_items(rcf_rpc_server       *rpcs,
                                  rpc_rte_flow_item_p   flow_items,
                                  rpc_rte_flow_item_p   opaque_items,
                                  unsigned int          nb_opaque_items,
                                  rpc_rte_flow_item_p  *united_items)
{
    tarpc_rte_flow_prepend_opaque_items_out  out = {};
    tarpc_rte_flow_prepend_opaque_items_in   in = {};

    if (flow_items == RPC_NULL || opaque_items == RPC_NULL ||
        nb_opaque_items == 0 || united_items == NULL)
        RETVAL_ZERO_INT(rte_flow_prepend_opaque_items, -EINVAL);

    in.nb_opaque_items = nb_opaque_items;
    in.opaque_items = opaque_items;
    in.flow_items = flow_items;

    rcf_rpc_call(rpcs, "rte_flow_prepend_opaque_items", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_flow_prepend_opaque_items,
                 "flow_items=" RPC_PTR_FMT ", "
                 "opaque_items=" RPC_PTR_FMT ", nb_opaque_items=%u",
                 "united_items=" RPC_PTR_FMT "; " NEG_ERRNO_FMT,
                 RPC_PTR_VAL(in.flow_items), RPC_PTR_VAL(in.opaque_items),
                 in.nb_opaque_items, RPC_PTR_VAL(out.united_items),
                 NEG_ERRNO_ARGS(out.retval));

    *united_items = out.united_items;

    RETVAL_ZERO_INT(rte_flow_prepend_opaque_items, out.retval);
}

void
rpc_rte_flow_release_united_items(rcf_rpc_server       *rpcs,
                                  rpc_rte_flow_item_p   united_items)
{
    tarpc_rte_flow_release_united_items_out  out = {};
    tarpc_rte_flow_release_united_items_in   in = {};

    if (united_items == RPC_NULL)
        RETVAL_VOID(rte_flow_release_united_items);

    in.united_items = united_items;

    rcf_rpc_call(rpcs, "rte_flow_release_united_items", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_flow_release_united_items,
                 RPC_PTR_FMT, "", RPC_PTR_VAL(in.united_items));

    RETVAL_VOID(rte_flow_release_united_items);
}
