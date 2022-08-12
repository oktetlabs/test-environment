/** @file
 * @brief DPDK RTE flow helper functions TAPI
 *
 * RTE flow helper functions TAPI (implementation)
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "TAPI RTE flow"

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
                                   uint32_t counter_id)
{
    asn_value *count_action;

    count_action = asn_init_value(ndn_rte_flow_action);
    CHECK_NOT_NULL(count_action);

    CHECK_RC(asn_write_int32(count_action, NDN_FLOW_ACTION_TYPE_COUNT,
                             "type"));
    CHECK_RC(asn_write_int32(count_action, counter_id, "conf.#count.counter-id"));

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
tapi_rte_flow_add_ndn_action_of_pop_vlan(asn_value *ndn_actions,
                                         int action_index)
{
    asn_value *action;

    CHECK_NOT_NULL(action = asn_init_value(ndn_rte_flow_action));
    CHECK_RC(asn_write_int32(action, NDN_FLOW_ACTION_TYPE_OF_POP_VLAN, "type"));

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

void
tapi_rte_flow_add_ndn_action_port(ndn_rte_flow_action_type_t type,
                                  uint32_t ethdev_port_id,
                                  asn_value *actions,
                                  int entry_idx)
{
    asn_value *entry;

    if (type != NDN_FLOW_ACTION_TYPE_PORT_REPRESENTOR &&
        type != NDN_FLOW_ACTION_TYPE_REPRESENTED_PORT)
        CHECK_RC(TE_EINVAL);

    CHECK_NOT_NULL(entry = asn_init_value(ndn_rte_flow_action));

    CHECK_RC(asn_write_int32(entry, type, "type"));

    CHECK_RC(asn_write_value_field_fmt(entry, &ethdev_port_id,
                                       sizeof(ethdev_port_id),
                                       "conf.#ethdev-port-id"));

    CHECK_RC(asn_insert_indexed(actions, entry, entry_idx, ""));
}

void
tapi_rte_flow_add_ndn_action_jump(asn_value *ndn_actions,
                                  int action_index,
                                  uint32_t group)
{
    asn_value *action;

    CHECK_NOT_NULL(action = asn_init_value(ndn_rte_flow_action));
    CHECK_RC(asn_write_int32(action, NDN_FLOW_ACTION_TYPE_JUMP, "type"));
    CHECK_RC(asn_write_value_field_fmt(action, &group, sizeof(group),
                                       "conf.#group"));

    CHECK_RC(asn_insert_indexed(ndn_actions, action, action_index, ""));
}

void
tapi_rte_flow_add_ndn_item_port(ndn_rte_flow_item_type_t type,
                                uint32_t ethdev_port_id,
                                asn_value *items,
                                int entry_idx)
{
    asn_value *entry;

    CHECK_NOT_NULL(entry = asn_init_value(ndn_rte_flow_item));

    CHECK_RC(asn_write_int32(entry, type, "type"));

    CHECK_RC(asn_write_value_field(entry, &ethdev_port_id,
                                   sizeof(ethdev_port_id),
                                   "conf.#ethdev-port-id.#plain"));

    CHECK_RC(asn_insert_indexed(items, entry, entry_idx, ""));
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

void
tapi_rte_flow_isolate(rcf_rpc_server *rpcs, uint16_t port_id, int set)
{
    te_errno rc;

    RPC_AWAIT_IUT_ERROR(rpcs);

    rc = rpc_rte_flow_isolate(rpcs, port_id, set, NULL);
    if (rc == -TE_RC(TE_RPC, TE_EOPNOTSUPP) ||
        rc == -TE_RC(TE_RPC, TE_ENOSYS))
    {
        TEST_SKIP("rte_flow_isolate() RPC is unavailable");
    }

    CHECK_RC(rc);
}
