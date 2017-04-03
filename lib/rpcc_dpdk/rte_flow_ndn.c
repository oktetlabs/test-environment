/** @file
 * @brief RPC client API for RTE flow
 *
 * RPC client API for RTE flow functions
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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

int
rpc_rte_mk_flow_rule_from_str(rcf_rpc_server *rpcs,
                              const asn_value  *flow_rule,
                              rpc_rte_flow_attr_p *attr,
                              rpc_rte_flow_item_p *pattern,
                              rpc_rte_flow_action_p *actions)
{
    tarpc_rte_mk_flow_rule_from_str_in  in;
    tarpc_rte_mk_flow_rule_from_str_out out;
    size_t                          flow_len;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    flow_len = asn_count_txt_len(flow_rule, 0) + 1;
    in.flow_rule = tapi_calloc(1, flow_len);

    if (asn_sprint_value(flow_rule, in.flow_rule, flow_len, 0) <= 0)
        TEST_FAIL("Failed to prepare textual representation of ASN.1 flow rule");

    rcf_rpc_call(rpcs, "rte_mk_flow_rule_from_str", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_mk_flow_rule_from_str, out.retval);

    TAPI_RPC_LOG(rpcs, rte_mk_flow_rule_from_str, "\n%s,\n",
                 "{"RPC_PTR_FMT", "RPC_PTR_FMT", "RPC_PTR_FMT"}, "
                 NEG_ERRNO_FMT, in.flow_rule, RPC_PTR_VAL(out.attr),
                 RPC_PTR_VAL(out.pattern), RPC_PTR_VAL(out.actions),
                 NEG_ERRNO_ARGS(out.retval));

    *attr = (rpc_rte_flow_attr_p)out.attr;
    *pattern = (rpc_rte_flow_item_p)out.pattern;
    *actions = (rpc_rte_flow_action_p)out.actions;

    RETVAL_ZERO_INT(rte_mk_flow_rule_from_str, out.retval);
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

int
rpc_rte_flow_validate(rcf_rpc_server *rpcs,
                      uint8_t port_id,
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
                 "%hhu, "RPC_PTR_FMT", "RPC_PTR_FMT", "RPC_PTR_FMT,
                 NEG_ERRNO_FMT "%s", in.port_id, RPC_PTR_VAL(in.attr),
                 RPC_PTR_VAL(in.pattern), RPC_PTR_VAL(in.actions),
                 NEG_ERRNO_ARGS(out.retval), (out.retval != 0) ?
                 tarpc_rte_flow_error2str(tlbp, &out.error) : "");
    te_log_buf_free(tlbp);

    if (error != NULL)
    {
        error->type = out.error.type;

        if (out.error.message != NULL)
            error->message = tapi_strdup(out.error.message);
    }

    RETVAL_ZERO_INT(rte_flow_validate, out.retval);
}
