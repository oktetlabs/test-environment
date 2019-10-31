/** @file
 * @brief DPDK RTE flow helper functions TAPI
 *
 * RTE flow helper functions TAPI (implementation)
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#include "te_config.h"

#include "tapi_test.h"
#include "tapi_test_log.h"
#include "rte_flow_ndn.h"
#include "tapi_rte_flow.h"
#include "tapi_rpc_rte_flow.h"

void
tapi_rte_flow_add_ndn_action_queue(asn_value *ndn_actions, int action_index,
                                   uint16_t queue)
{
    asn_value *queue_action;

    queue_action = asn_init_value(ndn_rte_flow_action);
    CHECK_NOT_NULL(queue_action);

    CHECK_RC(asn_write_int32(queue_action, NDN_FLOW_ACTION_TYPE_QUEUE,
                             "type"));
    CHECK_RC(asn_write_int32(queue_action, queue, "conf.#index"));

    CHECK_RC(asn_insert_indexed(ndn_actions, queue_action, action_index, ""));
}

void
tapi_rte_flow_add_ndn_action_drop(asn_value *ndn_actions, int action_index)
{
    asn_value *drop_action;

    drop_action = asn_init_value(ndn_rte_flow_action);
    CHECK_NOT_NULL(drop_action);

    CHECK_RC(asn_write_int32(drop_action, NDN_FLOW_ACTION_TYPE_DROP, "type"));

    CHECK_RC(asn_insert_indexed(ndn_actions, drop_action, action_index, ""));
}

void
tapi_rte_flow_add_ndn_action_count(asn_value *ndn_actions, int action_index,
                                   uint32_t counter_id, te_bool shared)
{
    asn_value *count_action;

    count_action = asn_init_value(ndn_rte_flow_action);
    CHECK_NOT_NULL(count_action);

    CHECK_RC(asn_write_int32(count_action, NDN_FLOW_ACTION_TYPE_COUNT,
                             "type"));
    CHECK_RC(asn_write_int32(count_action, counter_id, "conf.#count.counter-id"));
    CHECK_RC(asn_write_bool(count_action, shared, "conf.#count.shared"));

    CHECK_RC(asn_insert_indexed(ndn_actions, count_action, action_index, ""));
}

rpc_rte_flow_p
tapi_rte_flow_validate_and_create_rule(rcf_rpc_server *rpcs, uint16_t port_id,
                                       rpc_rte_flow_attr_p attr,
                                       rpc_rte_flow_item_p pattern,
                                       rpc_rte_flow_action_p actions)
{
    tarpc_rte_flow_error error;
    rpc_rte_flow_p flow;
    te_errno rc;

    RPC_AWAIT_IUT_ERROR(rpcs);
    rc = rpc_rte_flow_validate(rpcs, port_id, attr, pattern, actions, &error);
    if (rc == -TE_RC(TE_RPC, TE_ENOSYS))
        TEST_SKIP("Flow validate operation failed: %s", error.message);
    if (rc != 0)
        TEST_VERDICT("'rte_flow_validate' operation failed: %s", error.message);

    flow = rpc_rte_flow_create(rpcs, port_id, attr, pattern, actions, &error);
    if (flow == RPC_NULL)
    {
        TEST_VERDICT("'rpc_rte_flow_create' operation failed: %s",
                     error.message);
    }

    return flow;
}
