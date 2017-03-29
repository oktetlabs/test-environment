/** @file
 * @brief NDN
 *
 * Declarations of ASN.1 types for NDN of RTE flow
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
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

#ifndef __TE_NDN_RTE_FLOW_H__
#define __TE_NDN_RTE_FLOW_H__

#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NDN_FLOW_ATTR,
    NDN_FLOW_ACTION_TYPES,
    NDN_FLOW_ACTION,
    NDN_FLOW_RULE,
} ndn_rte_flow_tags_t;

typedef enum {
    NDN_FLOW_ATTR_GROUP,
    NDN_FLOW_ATTR_PRIORITY,
    NDN_FLOW_ATTR_INGRESS,
    NDN_FLOW_ATTR_EGRESS,
} ndn_rte_flow_attr_t;

typedef enum {
    NDN_FLOW_ACTION_TYPE_VOID,
    NDN_FLOW_ACTION_TYPE_QUEUE,
} ndn_rte_flow_action_type_t;

typedef enum {
    NDN_FLOW_ACTION_TYPE,
    NDN_FLOW_ACTION_CONF,
    NDN_FLOW_ACTION_QID,
} ndn_rte_flow_action_t;

typedef enum {
    NDN_FLOW_RULE_ATTR,
    NDN_FLOW_RULE_PATTERN,
    NDN_FLOW_RULE_ACTIONS,
} ndn_rte_flow_rule_t;

extern const asn_type * const ndn_rte_flow_rule;

extern asn_type ndn_rte_flow_rule_s;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_RTE_FLOW_H__ */
