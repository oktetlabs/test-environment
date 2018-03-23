/** @file
 * @brief RPC client API for RTE flow
 *
 * RPC client API for RTE flow functions
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Roman Zhukov <Roman.Zhukov@oktetlabs.ru>
 */

#include "te_config.h"

#include "log_bufs.h"
#include "tapi_mem.h"
#include "tapi_rpc_internal.h"
#include "tapi_test_log.h"

#include "tarpc.h"

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
    uint8_t                                 component_flags;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    flow_len = asn_count_txt_len(flow_rule_components, 0) + 1;
    in.flow_rule_components = tapi_calloc(1, flow_len);

    component_flags =
        tarpc_rte_flow_type2rpc_flags(asn_get_type(flow_rule_components));
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

    TAPI_RPC_LOG(rpcs, rte_mk_flow_rule_components, "\n%s,\n",
                 "{"RPC_PTR_FMT", "RPC_PTR_FMT", "RPC_PTR_FMT"}, "
                 NEG_ERRNO_FMT, in.flow_rule_components, RPC_PTR_VAL(out.attr),
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
        CASE_ERROR_TYPE2STR(ATTR, "attributes structure");
        CASE_ERROR_TYPE2STR(ITEM_NUM, "pattern length");
        CASE_ERROR_TYPE2STR(ITEM, "specific pattern item");
        CASE_ERROR_TYPE2STR(ACTION_NUM, "number of actions");
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
