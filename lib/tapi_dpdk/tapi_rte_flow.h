/** @file
 * @brief DPDK RTE flow helper functions TAPI
 *
 * @defgroup tapi_rte_flow DPDK RTE flow helper functions TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * RTE flow helper functions TAPI
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
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
 * @param[inout]  ndn_actions   Action list
 * @param[in]     action_index  Index at which QUEUE action is put to list
 * @param[in]     queue         Queue index of QUEUE action
 */
extern void tapi_rte_flow_add_ndn_action_queue(asn_value *ndn_actions,
                                               int action_index,
                                               uint16_t queue);

/**
 * Add a DROP action to an action list at specified index.
 *
 * @param[inout]  ndn_actions   Action list
 * @param[in]     action_index  Index at which DROP action is put to list
 */
extern void tapi_rte_flow_add_ndn_action_drop(asn_value *ndn_actions,
                                              int action_index);

/**
 * Add a COUNT action to an action list at specified index.
 *
 * @param[inout]  ndn_actions   Action list
 * @param[in]     action_index  Index at which DROP action is put to list
 * @param[in]     counter_id    Counter index
 * @param[in]     shared        Shared counter if @c TRUE
 */
extern void tapi_rte_flow_add_ndn_action_count(asn_value *ndn_actions,
                                               int action_index,
                                               uint32_t counter_id,
                                               te_bool shared);

/**
 * Add a encap action to an action list at specified index.
 *
 * @param[inout]  ndn_actions       Action list
 * @param[in]     action_index      Index at which DROP action is put to list
 * @paran[in]     type              Type of the encapsulation
 * @param[in]     encap_hdr         Flow rule pattern that is used as a
 *                                  encapsulated packet's header definition
 */
extern void tapi_rte_flow_add_ndn_action_encap(asn_value *ndn_actions,
                                               int action_index,
                                               tarpc_rte_eth_tunnel_type type,
                                               const asn_value *encap_hdr);

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_RTE_FLOW_H__ */

/**@} <!-- END tapi_rte_flow --> */
