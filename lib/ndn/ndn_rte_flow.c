/** @file
 * @brief NDN
 *
 * Definitions of ASN.1 types for NDN of RTE flow
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
    { "group", &asn_base_uint32_s,
        {PRIVATE, NDN_FLOW_ATTR_GROUP} },
    { "priority", &asn_base_uint32_s,
        {PRIVATE, NDN_FLOW_ATTR_PRIORITY} },
    { "ingress", &asn_base_int1_s,
        {PRIVATE, NDN_FLOW_ATTR_INGRESS} },
    { "egress", &asn_base_int1_s,
        {PRIVATE, NDN_FLOW_ATTR_EGRESS} },
    { "transfer", &asn_base_int1_s,
        {PRIVATE, NDN_FLOW_ATTR_TRANSFER} },
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
    {"rss", NDN_FLOW_ACTION_TYPE_RSS},
    {"drop", NDN_FLOW_ACTION_TYPE_DROP},
    {"flag", NDN_FLOW_ACTION_TYPE_FLAG},
    {"mark", NDN_FLOW_ACTION_TYPE_MARK},
    {"count", NDN_FLOW_ACTION_TYPE_COUNT},
    {"vxlan-encap", NDN_FLOW_ACTION_TYPE_VXLAN_ENCAP},
    {"vxlan-decap", NDN_FLOW_ACTION_TYPE_VXLAN_DECAP},
    {"of-pop-vlan", NDN_FLOW_ACTION_TYPE_OF_POP_VLAN},
    {"of-push-vlan", NDN_FLOW_ACTION_TYPE_OF_PUSH_VLAN},
    {"of-set-vlan-vid", NDN_FLOW_ACTION_TYPE_OF_SET_VLAN_VID},
    {"port-id", NDN_FLOW_ACTION_TYPE_PORT_ID},
    {"vf", NDN_FLOW_ACTION_TYPE_VF},
    {"phy-port", NDN_FLOW_ACTION_TYPE_PHY_PORT},
    {"jump", NDN_FLOW_ACTION_TYPE_JUMP},
};

asn_type ndn_rte_flow_action_type_s = {
    "Action-Type", {PRIVATE, NDN_FLOW_ACTION_TYPES}, ENUMERATED,
    TE_ARRAY_LEN(_ndn_rte_flow_action_type_enum_entries),
    {enum_entries: _ndn_rte_flow_action_type_enum_entries}
};

const asn_type * const ndn_rte_flow_action_type = &ndn_rte_flow_action_type_s;

static asn_named_entry_t _ndn_rte_flow_action_conf_rss_opt_hf_ne_array[] = {
    { "ipv4",               &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV4 } },
    { "frag-ipv4",          &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_FRAG_IPV4 } },
    { "nonfrag-ipv4-tcp",   &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_TCP } },
    { "nonfrag-ipv4-udp",   &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_UDP } },
    { "nonfrag-ipv4-sctp",  &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_SCTP } },
    { "nonfrag-ipv4-other", &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_OTHER } },
    { "ipv6",               &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6 } },
    { "frag-ipv6",          &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_FRAG_IPV6 } },
    { "nonfrag-ipv6-tcp",   &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_TCP } },
    { "nonfrag-ipv6-udp",   &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_UDP } },
    { "nonfrag-ipv6-sctp",  &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_SCTP } },
    { "nonfrag-ipv6-other", &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_OTHER } },
    { "l2-payload",         &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_L2_PAYLOAD } },
    { "ipv6-ex",            &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6_EX } },
    { "ipv6-tcp-ex",        &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6_TCP_EX } },
    { "ipv6-udp-ex",        &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6_UDP_EX } },
    { "port",               &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_PORT } },
    { "vxlan",              &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_VXLAN } },
    { "geneve",             &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_GENEVE } },
    { "nvgre",              &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NVGRE } },
    { "ip",                 &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IP } },
    { "tcp",                &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_TCP } },
    { "udp",                &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_UDP } },
    { "sctp",               &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_SCTP } },
    { "tunnel",             &asn_base_int1_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF_TUNNEL } },
};

asn_type ndn_rte_flow_action_conf_rss_opt_hf_s = {
    "Action-Conf-RSS-Opt-HF",
    { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_rte_flow_action_conf_rss_opt_hf_ne_array),
    {_ndn_rte_flow_action_conf_rss_opt_hf_ne_array}
};

const asn_type * const ndn_rte_flow_action_conf_rss_opt_hf =
    &ndn_rte_flow_action_conf_rss_opt_hf_s;

static asn_named_entry_t _ndn_rte_flow_action_conf_rss_opt_ne_array[] = {
    { "rss-key", &asn_base_octstring_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_KEY } },
    { "rss-hf",  &ndn_rte_flow_action_conf_rss_opt_hf_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT_HF } },
};

asn_type ndn_rte_flow_action_conf_rss_opt_s = {
    "Action-Conf-RSS-Opt", { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_rte_flow_action_conf_rss_opt_ne_array),
    {_ndn_rte_flow_action_conf_rss_opt_ne_array}
};

const asn_type * const ndn_rte_flow_action_conf_rss_opt =
    &ndn_rte_flow_action_conf_rss_opt_s;

asn_type ndn_rte_flow_action_conf_rss_queue_s = {
    "Action-Conf-RSS-Queue",
    { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_QUEUE }, SEQUENCE_OF,
    0, {subtype: &asn_base_int16_s}
};

const asn_type * const ndn_rte_flow_action_conf_rss_queue =
    &ndn_rte_flow_action_conf_rss_queue_s;

static asn_named_entry_t _ndn_rte_flow_action_conf_rss_ne_array[] = {
    { "rss-conf", &ndn_rte_flow_action_conf_rss_opt_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_OPT } },
    { "queue",    &ndn_rte_flow_action_conf_rss_queue_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_RSS_QUEUE } },
};

asn_type ndn_rte_flow_action_conf_rss_s = {
    "Action-Conf-RSS", { PRIVATE, NDN_FLOW_ACTION_CONF_RSS }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_rte_flow_action_conf_rss_ne_array),
    {_ndn_rte_flow_action_conf_rss_ne_array}
};

const asn_type * const ndn_rte_flow_action_conf_rss =
    &ndn_rte_flow_action_conf_rss_s;

static asn_named_entry_t _ndn_rte_flow_action_conf_count_ne_array[] = {
    { "counter-id", &asn_base_uint32_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_COUNT_ID } },
    { "shared", &asn_base_boolean_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_COUNT_SHARED } },
};

asn_type ndn_rte_flow_action_conf_count_s = {
    "Action-Conf-COUNT", { PRIVATE, NDN_FLOW_ACTION_CONF_COUNT }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_rte_flow_action_conf_count_ne_array),
    {_ndn_rte_flow_action_conf_count_ne_array}
};

const asn_type * const ndn_rte_flow_action_conf_count =
    &ndn_rte_flow_action_conf_count_s;

static asn_named_entry_t _ndn_rte_flow_action_conf_id_original_ne_array[] = {
    { "id", &asn_base_uint32_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_IO_ID } },
    { "original", &asn_base_boolean_s,
      { PRIVATE, NDN_FLOW_ACTION_CONF_IO_ORIGINAL } },
};

asn_type ndn_rte_flow_action_conf_id_original_s = {
    "Action-Conf-ID-Original", { PRIVATE, NDN_FLOW_ACTION_CONF_ID_ORIGINAL },
    SEQUENCE,
    TE_ARRAY_LEN(_ndn_rte_flow_action_conf_id_original_ne_array),
    {_ndn_rte_flow_action_conf_id_original_ne_array}
};

const asn_type * const ndn_rte_flow_action_conf_port =
    &ndn_rte_flow_action_conf_id_original_s;

static asn_named_entry_t _ndn_rte_flow_action_conf_ne_array[] = {
    { "index", &asn_base_int16_s,
        {PRIVATE, NDN_FLOW_ACTION_QID} },
    { "rss",   &ndn_rte_flow_action_conf_rss_s,
        {PRIVATE, NDN_FLOW_ACTION_CONF_RSS} },
    { "id", &asn_base_int32_s,
        {PRIVATE, NDN_FLOW_ACTION_MARK_ID} },
    { "count", &ndn_rte_flow_action_conf_count_s,
        {PRIVATE, NDN_FLOW_ACTION_CONF_COUNT} },
    { "encap-hdr", &ndn_generic_pdu_sequence_s,
        {PRIVATE, NDN_FLOW_ACTION_ENCAP_HDR} },
    { "ethertype", &asn_base_int16_s,
        {PRIVATE, NDN_FLOW_ACTION_OF_PUSH_VLAN_ETHERTYPE} },
    { "vlan-id", &asn_base_int16_s,
        {PRIVATE, NDN_FLOW_ACTION_OF_SET_VLAN_VID} },
    { "port-id", &ndn_rte_flow_action_conf_id_original_s,
        {PRIVATE, NDN_FLOW_ACTION_CONF_PORT_ID} },
    { "vf", &ndn_rte_flow_action_conf_id_original_s,
        {PRIVATE, NDN_FLOW_ACTION_CONF_VF} },
    { "phy-port", &ndn_rte_flow_action_conf_id_original_s,
        {PRIVATE, NDN_FLOW_ACTION_CONF_PHY_PORT} },
    { "group", &asn_base_int32_s,
        {PRIVATE, NDN_FLOW_ACTION_GROUP} },
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

static asn_named_entry_t _ndn_rte_flow_item_conf_ne_array[] = {
    { "id", &ndn_data_unit_uint32_s,
        {PRIVATE, NDN_FLOW_ITEM_CONF_PORT_ID_ID} },
    { "index", &ndn_data_unit_uint32_s,
        {PRIVATE, NDN_FLOW_ITEM_CONF_PHY_PORT_INDEX} },
};

asn_type ndn_rte_flow_item_conf_s = {
    "Item-Conf", {PRIVATE, 0}, CHOICE,
    TE_ARRAY_LEN(_ndn_rte_flow_item_conf_ne_array),
    {_ndn_rte_flow_item_conf_ne_array}
};

const asn_type * const ndn_rte_flow_item_conf = &ndn_rte_flow_item_conf_s;

asn_enum_entry_t _ndn_rte_flow_item_type_enum_entries[] = {
    {"port-id", NDN_FLOW_ITEM_TYPE_PORT_ID},
    {"phy-port", NDN_FLOW_ITEM_TYPE_PHY_PORT},
};

asn_type ndn_rte_flow_item_type_s = {
    "Item-Type", {PRIVATE, NDN_FLOW_ITEM_TYPES}, ENUMERATED,
    TE_ARRAY_LEN(_ndn_rte_flow_item_type_enum_entries),
    {enum_entries: _ndn_rte_flow_item_type_enum_entries}
};

const asn_type * const ndn_rte_flow_item_type = &ndn_rte_flow_item_type_s;

static asn_named_entry_t _ndn_rte_flow_item_ne_array[] = {
    { "type", &ndn_rte_flow_item_type_s,
        {PRIVATE, NDN_FLOW_ITEM_TYPE} },
    { "conf", &ndn_rte_flow_item_conf_s,
        {PRIVATE, NDN_FLOW_ITEM_CONF} },
};

asn_type ndn_rte_flow_item_s = {
    "Item", {PRIVATE, NDN_FLOW_ITEM}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_rte_flow_item_ne_array),
    {_ndn_rte_flow_item_ne_array}
};

const asn_type * const ndn_rte_flow_item = &ndn_rte_flow_item_s;

asn_type ndn_rte_flow_items_s = {
    "Items", {PRIVATE, 0}, SEQUENCE_OF,
    0, {subtype: &ndn_rte_flow_item_s}
};

const asn_type * const ndn_rte_flow_items = &ndn_rte_flow_items_s;

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

const asn_type * const ndn_rte_flow_pattern = &ndn_generic_pdu_sequence_s;
const asn_type * const ndn_rte_flow_rule = &ndn_rte_flow_rule_s;
