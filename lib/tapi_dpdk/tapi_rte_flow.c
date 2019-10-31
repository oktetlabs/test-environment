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
#include "rte_flow_ndn.h"
#include "tapi_rte_flow.h"

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
