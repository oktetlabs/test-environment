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

void
tapi_rte_flow_add_ndn_action_encap(asn_value *ndn_actions,
                                   int action_index,
                                   tarpc_rte_eth_tunnel_type type,
                                   const asn_value *encap_hdr)
{
    asn_value *action;
    asn_value *hdr;

    if (type != TARPC_RTE_TUNNEL_TYPE_VXLAN)
        TEST_FAIL("Invalid tunnel type");

    action = asn_init_value(ndn_rte_flow_action);
    CHECK_NOT_NULL(action);
    hdr = asn_copy_value(encap_hdr);
    CHECK_NOT_NULL(hdr);

    CHECK_RC(asn_write_int32(action, NDN_FLOW_ACTION_TYPE_VXLAN_ENCAP, "type"));
    CHECK_RC(asn_put_descendent(action, hdr, "conf.#encap-hdr"));

    CHECK_RC(asn_insert_indexed(ndn_actions, action, action_index, ""));
}

void
tapi_rte_flow_add_ndn_action_decap(asn_value *ndn_actions,
                                   int action_index,
                                   tarpc_rte_eth_tunnel_type type)
{
    asn_value *action;

    if (type != TARPC_RTE_TUNNEL_TYPE_VXLAN)
        TEST_FAIL("Invalid tunnel type");

    action = asn_init_value(ndn_rte_flow_action);
    CHECK_NOT_NULL(action);

    CHECK_RC(asn_write_int32(action, NDN_FLOW_ACTION_TYPE_VXLAN_DECAP, "type"));

    CHECK_RC(asn_insert_indexed(ndn_actions, action, action_index, ""));
}

void
tapi_rte_flow_add_ndn_action_of_push_vlan(asn_value *ndn_actions,
                                          int action_index, uint16_t ethertype)
{
    asn_value *action;

    CHECK_NOT_NULL(action = asn_init_value(ndn_rte_flow_action));
    CHECK_RC(asn_write_int32(action, NDN_FLOW_ACTION_TYPE_OF_PUSH_VLAN,
                             "type"));
    CHECK_RC(asn_write_value_field(action, &ethertype, sizeof(ethertype),
                                   "conf.#ethertype"));

    CHECK_RC(asn_insert_indexed(ndn_actions, action, action_index, ""));
}

void
tapi_rte_flow_add_ndn_action_of_set_vlan_vid(asn_value *ndn_actions,
                                             int action_index,
                                             uint16_t vlan_vid)
{
    asn_value *action;

    CHECK_NOT_NULL(action = asn_init_value(ndn_rte_flow_action));
    CHECK_RC(asn_write_int32(action, NDN_FLOW_ACTION_TYPE_OF_SET_VLAN_VID,
                             "type"));
    CHECK_RC(asn_write_value_field(action, &vlan_vid, sizeof(vlan_vid),
                                   "conf.#vlan-id"));

    CHECK_RC(asn_insert_indexed(ndn_actions, action, action_index, ""));
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
    if (rc == -TE_RC(TE_RPC, TE_ENOSYS) || rc == -TE_RC(TE_RPC, TE_EOPNOTSUPP))
        TEST_SKIP("'rte_flow_validate' operation failed: %s", error.message);
    else if (rc != 0)
        TEST_VERDICT("'rte_flow_validate' operation failed: %s", error.message);

    RPC_AWAIT_IUT_ERROR(rpcs);
    flow = rpc_rte_flow_create(rpcs, port_id, attr, pattern, actions, &error);
    if (flow == RPC_NULL)
    {
        TEST_VERDICT("'rpc_rte_flow_create' operation failed: %s",
                     error.message);
    }

    return flow;
}

void
tapi_rte_flow_make_attr(rcf_rpc_server *rpcs, uint32_t group, uint32_t priority,
                        te_bool ingress, te_bool egress, te_bool transfer,
                        rpc_rte_flow_attr_p *attr)
{
    asn_value *attr_pdu;

    attr_pdu = asn_init_value(ndn_rte_flow_attr);
    if (attr_pdu == NULL)
        TEST_FAIL("Failed to init ASN.1 value");

    CHECK_RC(asn_write_uint32(attr_pdu, group, "group"));
    CHECK_RC(asn_write_uint32(attr_pdu, priority, "priority"));
    CHECK_RC(asn_write_int32(attr_pdu, !!egress, "egress"));
    CHECK_RC(asn_write_int32(attr_pdu, !!ingress, "ingress"));
    CHECK_RC(asn_write_int32(attr_pdu, !!transfer, "transfer"));

    CHECK_RC(rpc_rte_mk_flow_rule_components(rpcs, attr_pdu, attr, NULL, NULL));

    asn_free_value(attr_pdu);
}
