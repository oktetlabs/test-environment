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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_RTE_FLOW_H__ */

/**@} <!-- END tapi_rte_flow --> */
