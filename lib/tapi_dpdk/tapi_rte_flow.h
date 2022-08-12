/** @file
 * @brief DPDK RTE flow helper functions TAPI
 *
 * @defgroup tapi_rte_flow DPDK RTE flow helper functions TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * RTE flow helper functions TAPI
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#ifndef __TAPI_RTE_FLOW_H__
#define __TAPI_RTE_FLOW_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h"
#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"
#include "tapi_ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add a QUEUE action to an action list at specified index.
 *
 * @param[in,out] ndn_actions   Action list
 * @param[in]     action_index  Index at which the action is put to list
 * @param[in]     queue         Queue index of the action
 */
extern void tapi_rte_flow_add_ndn_action_queue(asn_value *ndn_actions,
                                               int action_index,
                                               uint16_t queue);

/**
 * Add a DROP action to an action list at specified index.
 *
 * @param[in,out] ndn_actions   Action list
 * @param[in]     action_index  Index at which the action is put to list
 */
extern void tapi_rte_flow_add_ndn_action_drop(asn_value *ndn_actions,
                                              int action_index);

/**
 * Add a COUNT action to an action list at specified index.
 *
 * @param[in,out] ndn_actions   Action list
 * @param[in]     action_index  Index at which the action is put to list
 * @param[in]     counter_id    Counter index
 */
extern void tapi_rte_flow_add_ndn_action_count(asn_value *ndn_actions,
                                               int action_index,
                                               uint32_t counter_id);

/**
 * Add a encap action to an action list at specified index.
 *
 * @param[in,out] ndn_actions       Action list
 * @param[in]     action_index      Index at which the action is put to list
 * @paran[in]     type              Type of the encapsulation
 * @param[in]     encap_hdr         Flow rule pattern that is used as a
 *                                  encapsulated packet's header definition
 */
extern void tapi_rte_flow_add_ndn_action_encap(asn_value *ndn_actions,
                                               int action_index,
                                               tarpc_rte_eth_tunnel_type type,
                                               const asn_value *encap_hdr);

/**
 * Add a decap action to an action list at specified index.
 *
 * @param[in,out] ndn_actions       Action list
 * @param[in]     action_index      Index at which the action is put to list
 * @paran[in]     type              Type of the encapsulation
 */
extern void tapi_rte_flow_add_ndn_action_decap(asn_value *ndn_actions,
                                               int action_index,
                                               tarpc_rte_eth_tunnel_type type);

/**
 * Add a pop VLAN action to an action list at specified index.
 *
 * @param[in,out] ndn_actions       Action list
 * @param[in]     action_index      Index at which the action is put to list
 */
extern void tapi_rte_flow_add_ndn_action_of_pop_vlan(asn_value *ndn_actions,
                                                     int action_index);

/**
 * Add a push vlan action to an action list at specified index.
 *
 * @param[in,out] ndn_actions       Action list
 * @param[in]     action_index      Index at which the action is put to list
 * @paran[in]     ethertype         VLAN EtherType
 */
extern void tapi_rte_flow_add_ndn_action_of_push_vlan(asn_value *ndn_actions,
                                                      int action_index,
                                                      uint16_t ethertype);

/**
 * Add a set vlan vid action to an action list at specified index.
 *
 * @param[in,out] ndn_actions       Action list
 * @param[in]     action_index      Index at which the action is put to list
 * @paran[in]     vlan_vid          VLAN ID
 */
extern void tapi_rte_flow_add_ndn_action_of_set_vlan_vid(asn_value *ndn_actions,
                                                         int action_index,
                                                         uint16_t vlan_vid);

/**
 * Add an action of type PORT_REPRESENTOR / REPRESENTED_PORT to the action list.
 *
 * @param[in]     type            The action type
 * @param[in]     ethdev_port_id  The representor's ethdev port ID
 * @param[in,out] actions         The list of actions
 * @param[in]     entry_idx       The entry index
 */
extern void tapi_rte_flow_add_ndn_action_port(ndn_rte_flow_action_type_t type,
                                              uint32_t ethdev_port_id,
                                              asn_value *actions,
                                              int entry_idx);

/**
 * Add a JUMP action to an action list at specified index.
 *
 * @param[in,out] ndn_actions       Action list
 * @param[in]     action_index      Index at which the action is put to list
 * @param[in]     group             Group to redirect packets to
 */
extern void tapi_rte_flow_add_ndn_action_jump(asn_value *ndn_actions,
                                              int action_index,
                                              uint32_t group);

/**
 * Add an item of type PORT_REPRESENTOR / REPRESENTED_PORT to the item list.
 *
 * @param[in]     type            The item type
 * @param[in]     ethdev_port_id  The representor's ethdev port ID
 * @param[in,out] items           The list of items
 * @param[in]     entry_idx       The entry index
 */
extern void tapi_rte_flow_add_ndn_item_port(ndn_rte_flow_item_type_t type,
                                            uint32_t ethdev_port_id,
                                            asn_value *items,
                                            int entry_idx);

/**
 * Convert an ASN value representing a flow rule pattern into
 * RTE flow rule pattern and a template that matches the pattern.
 *
 * @param[in]     rpcs      RPC server handle
 * @param[in]     port_id   The port identifier of the device
 * @param[in]     attr      RTE flow attr pointer
 * @param[in]     pattern   RTE flow item pointer to the array of items
 * @param[in]     actions   RTE flow action pointer to the array of actions
 *
 * @return RTE flow pointer on success; jumps out on failure
 */
extern rpc_rte_flow_p tapi_rte_flow_validate_and_create_rule(
                                              rcf_rpc_server *rpcs,
                                              uint16_t port_id,
                                              rpc_rte_flow_attr_p attr,
                                              rpc_rte_flow_item_p pattern,
                                              rpc_rte_flow_action_p actions);

/**
 * Make RTE flow rule attributes
 *
 * @param[in]   rpcs        RPC server handle
 * @param[in]   group       Rule priority group
 * @param[in]   priority    Rule priority level within group
 * @param[in]   ingress     Rule applies to ingress traffic
 * @param[in]   egress      Rule applies to egress traffic
 * @param[in]   transfer    Transfer the rule to the lowest possible level
 *                          of any device endpoints found in the pattern
 * @param[out]  attr        RTE flow attr pointer
 *
 * @exception   TEST_FAIL
 */
extern void tapi_rte_flow_make_attr(rcf_rpc_server *rpcs, uint32_t group,
                                    uint32_t priority, te_bool ingress,
                                    te_bool egress, te_bool transfer,
                                    rpc_rte_flow_attr_p *attr);

/**
 * Isolate RTE flow
 *
 * @param[in]   rpcs        RPC server handle
 * @param[in]   port_id     The port identifier of the device
 * @param[in]   set         Nonzero to enter isolated mode, leave it otherwise
 *
 * @exception   TEST_FAIL
 */
extern void tapi_rte_flow_isolate(rcf_rpc_server *rpcs, uint16_t port_id,
                                  int set);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_RTE_FLOW_H__ */

/**@} <!-- END tapi_rte_flow --> */
