/** @file
 * @brief NDN
 *
 * Declarations of ASN.1 types for NDN of RTE flow
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
    NDN_FLOW_ACTION_TYPE_RSS,
    NDN_FLOW_ACTION_TYPE_DROP,
    NDN_FLOW_ACTION_TYPE_FLAG,
    NDN_FLOW_ACTION_TYPE_MARK,
    NDN_FLOW_ACTION_TYPE_COUNT,
} ndn_rte_flow_action_type_t;

typedef enum {
    NDN_FLOW_ACTION_CONF_COUNT,
    NDN_FLOW_ACTION_CONF_COUNT_ID,
    NDN_FLOW_ACTION_CONF_COUNT_SHARED,
    NDN_FLOW_ACTION_CONF_RSS,
    NDN_FLOW_ACTION_CONF_RSS_QUEUE,
    NDN_FLOW_ACTION_CONF_RSS_OPT,
    NDN_FLOW_ACTION_CONF_RSS_OPT_KEY,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV4,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_FRAG_IPV4,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_TCP,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_UDP,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_SCTP,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_OTHER,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_FRAG_IPV6,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_TCP,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_UDP,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_SCTP,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_OTHER,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_L2_PAYLOAD,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6_EX,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6_TCP_EX,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6_UDP_EX,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_PORT,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_VXLAN,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_GENEVE,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NVGRE,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IP,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_TCP,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_UDP,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_SCTP,
    NDN_FLOW_ACTION_CONF_RSS_OPT_HF_TUNNEL,
} ndn_rte_flow_action_conf_t;

typedef enum {
    NDN_FLOW_ACTION_TYPE,
    NDN_FLOW_ACTION_CONF,
    NDN_FLOW_ACTION_QID,
    NDN_FLOW_ACTION_MARK_ID,
} ndn_rte_flow_action_t;

typedef enum {
    NDN_FLOW_RULE_ATTR,
    NDN_FLOW_RULE_PATTERN,
    NDN_FLOW_RULE_ACTIONS,
} ndn_rte_flow_rule_t;

extern const asn_type * const ndn_rte_flow_attr;
extern const asn_type * const ndn_rte_flow_pattern;
extern const asn_type * const ndn_rte_flow_actions;
extern const asn_type * const ndn_rte_flow_rule;
extern const asn_type * const ndn_rte_flow_action;
extern const asn_type * const ndn_rte_flow_action_conf_rss;
extern const asn_type * const ndn_rte_flow_action_conf_rss_queue;
extern const asn_type * const ndn_rte_flow_action_conf_rss_opt;
extern const asn_type * const ndn_rte_flow_action_conf_rss_opt_hf;

extern asn_type ndn_rte_flow_rule_s;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_RTE_FLOW_H__ */
