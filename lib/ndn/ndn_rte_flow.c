/** @file
 * @brief NDN
 *
 * Definitions of ASN.1 types for NDN of RTE flow
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

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "tad_common.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_rte_flow.h"

static asn_named_entry_t _ndn_rte_flow_attr_ne_array[] = {
    { "group", &asn_base_int32_s,
        {PRIVATE, NDN_FLOW_ATTR_GROUP} },
    { "priority", &asn_base_int32_s,
        {PRIVATE, NDN_FLOW_ATTR_PRIORITY} },
    { "ingress", &asn_base_int1_s,
        {PRIVATE, NDN_FLOW_ATTR_INGRESS} },
    { "egress", &asn_base_int1_s,
        {PRIVATE, NDN_FLOW_ATTR_EGRESS} },
};

asn_type ndn_rte_flow_attr_s = {
    "Attributes", {PRIVATE, NDN_FLOW_ATTR}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_rte_flow_attr_ne_array),
    {_ndn_rte_flow_attr_ne_array}
};

const asn_type * const ndn_rte_flow_attr = &ndn_rte_flow_attr_s;

asn_enum_entry_t _ndn_rte_flow_action_type_enum_entries[] = {
    {"void", NDN_FLOW_ACTION_TYPE_VOID},
    {"queue", NDN_FLOW_ACTION_TYPE_QUEUE},
};

asn_type ndn_rte_flow_action_type_s = {
    "Action-Type", {PRIVATE, NDN_FLOW_ACTION_TYPES}, ENUMERATED,
    TE_ARRAY_LEN(_ndn_rte_flow_action_type_enum_entries),
    {enum_entries: _ndn_rte_flow_action_type_enum_entries}
};

const asn_type * const ndn_rte_flow_action_type = &ndn_rte_flow_action_type_s;

static asn_named_entry_t _ndn_rte_flow_action_conf_ne_array[] = {
    { "index", &asn_base_int16_s,
        {PRIVATE, NDN_FLOW_ACTION_QID} },
};

asn_type ndn_rte_flow_action_conf_s = {
    "Action-Conf", {PRIVATE, 0}, CHOICE,
    TE_ARRAY_LEN(_ndn_rte_flow_action_conf_ne_array),
    {_ndn_rte_flow_action_conf_ne_array}
};

const asn_type * const ndn_rte_flow_action_conf = &ndn_rte_flow_action_conf_s;

static asn_named_entry_t _ndn_rte_flow_action_ne_array[] = {
    { "type", &ndn_rte_flow_action_type_s,
        {PRIVATE, NDN_FLOW_ACTION_TYPE} },
    { "conf", &ndn_rte_flow_action_conf_s,
        {PRIVATE, NDN_FLOW_ACTION_CONF} },
};

asn_type ndn_rte_flow_action_s = {
    "Action", {PRIVATE, NDN_FLOW_ACTION}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_rte_flow_action_ne_array),
    {_ndn_rte_flow_action_ne_array}
};

const asn_type * const ndn_rte_flow_action = &ndn_rte_flow_action_s;

asn_type ndn_rte_flow_actions_s = {
    "Actions", {PRIVATE, 0}, SEQUENCE_OF,
    0, {subtype: &ndn_rte_flow_action_s}
};

const asn_type * const ndn_rte_flow_actions = &ndn_rte_flow_actions_s;

static asn_named_entry_t _ndn_rte_flow_rule_ne_array[] = {
    { "attr", &ndn_rte_flow_attr_s,
        {PRIVATE, NDN_FLOW_RULE_ATTR} },
    { "pattern", &ndn_generic_pdu_sequence_s,
        {PRIVATE, NDN_FLOW_RULE_PATTERN} },
    { "actions", &ndn_rte_flow_actions_s,
        {PRIVATE, NDN_FLOW_RULE_ACTIONS} },
};

asn_type ndn_rte_flow_rule_s = {
    "Flow-Rule", {PRIVATE, NDN_FLOW_RULE}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_rte_flow_rule_ne_array),
    {_ndn_rte_flow_rule_ne_array}
};

const asn_type * const ndn_rte_flow_rule = &ndn_rte_flow_rule_s;
