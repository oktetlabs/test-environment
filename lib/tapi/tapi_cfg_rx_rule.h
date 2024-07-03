/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Rx rules configuration TAPI
 *
 * Definition of test API for Network Interface Rx classification
 * rules (doc/cm/cm_rx_rules.xml).
 */

#ifndef __TE_TAPI_CFG_RX_RULE_H__
#define __TE_TAPI_CFG_RX_RULE_H__

#include "te_errno.h"
#include "te_stdint.h"
#include "te_sockaddr.h"
#include "te_rpc_sys_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Special rule insertion locations */
typedef enum tapi_cfg_rx_rule_spec_loc {
    TAPI_CFG_RX_RULE_ANY = -1,    /**< Insert at any available position */
    TAPI_CFG_RX_RULE_FIRST = -2,  /**< Insert at the first available
                                       position */
    TAPI_CFG_RX_RULE_LAST = -3,   /**< Insert at the last available
                                       position */
} tapi_cfg_rx_rule_spec_loc;

/** Supported types of flow for Rx rules */
typedef enum tapi_cfg_rx_rule_flow {
    TAPI_CFG_RX_RULE_FLOW_UNKNOWN,    /**< unknown flow type */
    TAPI_CFG_RX_RULE_FLOW_TCP_V4,     /**< TCP/IPv4 */
    TAPI_CFG_RX_RULE_FLOW_UDP_V4,     /**< UDP/IPv4 */
    TAPI_CFG_RX_RULE_FLOW_SCTP_V4,    /**< SCTP/IPv4 */
    TAPI_CFG_RX_RULE_FLOW_AH_V4,      /**< AH/IPv4 */
    TAPI_CFG_RX_RULE_FLOW_ESP_V4,     /**< ESP/IPv4 */
    TAPI_CFG_RX_RULE_FLOW_IPV4_USER,  /**< IPv4 */
    TAPI_CFG_RX_RULE_FLOW_TCP_V6,     /**< TCP/IPv6 */
    TAPI_CFG_RX_RULE_FLOW_UDP_V6,     /**< UDP/IPv6 */
    TAPI_CFG_RX_RULE_FLOW_SCTP_V6,    /**< SCTP/IPv6 */
    TAPI_CFG_RX_RULE_FLOW_AH_V6,      /**< AH/IPv6 */
    TAPI_CFG_RX_RULE_FLOW_ESP_V6,     /**< ESP/IPv6 */
    TAPI_CFG_RX_RULE_FLOW_IPV6_USER,  /**< IPv6 */
    TAPI_CFG_RX_RULE_FLOW_ETHER,      /**< Ethernet */
} tapi_cfg_rx_rule_flow;


/**
 * Add Rx classification rule.
 *
 * @note Change is local and should be committed.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Rule location (may be a nonnegative number or
 *                    a value from tapi_cfg_rx_rule_spec_loc)
 * @param flow_type   Flow type (may be left unspecified and set later
 *                    with tapi_cfg_rx_rule_flow_type_set())
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_add(const char *ta,
                                     const char *if_name,
                                     int64_t location,
                                     tapi_cfg_rx_rule_flow flow_type);

/**
 * Set Rx queue.
 *
 * @note Change is local and should be committed.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Rule location (may be a nonnegative number or
 *                    a value from tapi_cfg_rx_rule_spec_loc)
 * @param rxq         Rx queue number or @c -1 if packets matching to
 *                    the rule should be discarded
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_rx_queue_set(const char *ta,
                                              const char *if_name,
                                              int64_t location,
                                              int64_t rxq);

/**
 * Set RSS context.
 *
 * @note Change is local and should be committed.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Rule location (may be a nonnegative number or
 *                    a value from tapi_cfg_rx_rule_spec_loc)
 * @param context_id  RSS context number
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_rss_context_set(const char *ta,
                                                 const char *if_name,
                                                 int64_t location,
                                                 int64_t context_id);

/**
 * Get flow type for a given socket type and address family.
 *
 * @param af            Address family (@c AF_INET, @c AF_INET6)
 * @param sock_type     Socket type (@c RPC_SOCK_STREAM,
 *                      @c RPC_SOCK_DGRAM)
 *
 * @return Flow type.
 */
extern tapi_cfg_rx_rule_flow tapi_cfg_rx_rule_flow_by_socket(
                                        int af, rpc_socket_type sock_type);

/**
 * Set flow type.
 *
 * @note Change is local and should be committed.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Rule location (may be a nonnegative number or
 *                    a value from tapi_cfg_rx_rule_spec_loc)
 * @param flow_type   Flow type to set
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_flow_type_set(
                                        const char *ta,
                                        const char *if_name,
                                        int64_t location,
                                        tapi_cfg_rx_rule_flow flow_type);

/*
 * Here setters and getters for flow specification fields are defined.
 * Setters make only local changes which should be committed with
 * tapi_cfg_rx_rule_commit().
 *
 * For example, for "src_mac" field the following accessors are defined:
 * tapi_cfg_rx_rule_src_mac_setv(ta, if_name, location, value);
 * tapi_cfg_rx_rule_src_mac_setm(ta, if_name, location, value);
 * tapi_cfg_rx_rule_src_mac_getv(ta, if_name, location, value);
 * tapi_cfg_rx_rule_src_mac_getm(ta, if_name, location, value);
 */

#define TAPI_CFG_RX_RULE_FIELD_ACC(field_, type_) \
      /* Set field value */                                           \
      extern te_errno tapi_cfg_rx_rule_ ## field_ ## _setv(           \
                                const char *ta,                       \
                                const char *if_name,                  \
                                int64_t location,                     \
                                const type_ value);                   \
      /* Set field mask */                                            \
      extern te_errno tapi_cfg_rx_rule_ ## field_ ## _setm(           \
                                const char *ta,                       \
                                const char *if_name,                  \
                                int64_t location,                     \
                                const type_ value);                   \
      /* Get field value */                                           \
      extern te_errno tapi_cfg_rx_rule_ ## field_ ## _getv(           \
                                const char *ta,                       \
                                const char *if_name,                  \
                                int64_t location,                     \
                                type_ *value);                        \
      /* Get field mask */                                            \
      extern te_errno tapi_cfg_rx_rule_ ## field_ ## _getm(           \
                                const char *ta,                       \
                                const char *if_name,                  \
                                int64_t location,                     \
                                type_ *value)

TAPI_CFG_RX_RULE_FIELD_ACC(src_mac, struct sockaddr *);
TAPI_CFG_RX_RULE_FIELD_ACC(dst_mac, struct sockaddr *);
TAPI_CFG_RX_RULE_FIELD_ACC(ether_type, uint16_t);
TAPI_CFG_RX_RULE_FIELD_ACC(vlan_tpid, uint16_t);
TAPI_CFG_RX_RULE_FIELD_ACC(vlan_tci, uint16_t);
TAPI_CFG_RX_RULE_FIELD_ACC(data0, uint32_t);
TAPI_CFG_RX_RULE_FIELD_ACC(data1, uint32_t);
TAPI_CFG_RX_RULE_FIELD_ACC(src_l3_addr, struct sockaddr *);
TAPI_CFG_RX_RULE_FIELD_ACC(dst_l3_addr, struct sockaddr *);
TAPI_CFG_RX_RULE_FIELD_ACC(src_port, uint16_t);
TAPI_CFG_RX_RULE_FIELD_ACC(dst_port, uint16_t);
TAPI_CFG_RX_RULE_FIELD_ACC(tos_or_tclass, uint8_t);
TAPI_CFG_RX_RULE_FIELD_ACC(spi, uint32_t);
TAPI_CFG_RX_RULE_FIELD_ACC(l4_4_bytes, uint32_t);
TAPI_CFG_RX_RULE_FIELD_ACC(l4_proto, uint8_t);

#undef TAPI_CFG_RX_RULE_FIELD_ACC

/**
 * Fill address/port fields for TCP/UDP flow types.
 *
 * @note Change is local and should be committed.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Rule location (may be a nonnegative number or
 *                    a value from tapi_cfg_rx_rule_spec_loc)
 * @param af          @c AF_INET or @c AF_INET6
 * @param src         Source address/port or @c NULL
 * @param src_mask    Mask for source address/port or @c NULL.
 *                    Null mask for non-null @p src is treated as
 *                    all-ones value
 * @param dst         Destination address/port or @c NULL
 * @param dst_mask    Mask for destination address/port or @c NULL.
 *                    Null mask for non-null @p dst is treated as
 *                    all-ones value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_fill_ip_addrs_ports(
                                           const char *ta,
                                           const char *if_name,
                                           int64_t location, int af,
                                           const struct sockaddr *src,
                                           const struct sockaddr *src_mask,
                                           const struct sockaddr *dst,
                                           const struct sockaddr *dst_mask);

/**
 * Commit changes made to Rx rule.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Rule location (may be a nonnegative number or
 *                    a value from tapi_cfg_rx_rule_spec_loc)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_commit(const char *ta,
                                        const char *if_name,
                                        int64_t location);

/**
 * Get location of the rule added the last time (useful when special
 * insertion location was used when adding the rule and you need to
 * know the real location of added rule in rules table).
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Where to save obtained location (may be @c -1
 *                    if the last rule was added for another interface
 *                    or no rule was added at all)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_get_last_added(const char *ta,
                                                const char *if_name,
                                                int64_t *location);

/**
 * Remove existing Rx classification rule.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Rule location
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_del(const char *ta,
                                     const char *if_name,
                                     int64_t location);

/**
 * Get flow type of existing Rx rule.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Rule location
 * @param flow_type   Where to save obtained flow type
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_flow_type_get(
                                        const char *ta,
                                        const char *if_name,
                                        int64_t location,
                                        tapi_cfg_rx_rule_flow *flow_type);

/**
 * Get Rx queue assigned to a rule.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Rule location
 * @param rxq         Where to save Rx queue
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_rx_queue_get(const char *ta,
                                              const char *if_name,
                                              int64_t location,
                                              int64_t *rxq);

/**
 * Get RSS context assigned to a rule.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param location    Rule location
 * @param context_id  Where to save RSS context
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_rss_context_get(const char *ta,
                                                 const char *if_name,
                                                 int64_t location,
                                                 int64_t *context_id);

/**
 * Check whether special insertion locations are supported for
 * Rx classification rules.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param value       Will be set to @c true if special locations are
 *                    supported, to @c false otherwise
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_spec_loc_get(
                                        const char *ta,
                                        const char *if_name,
                                        bool *value);

/**
 * Get size of Rx classification rules table.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param size        Where to save rules table size
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_table_size_get(
                                        const char *ta,
                                        const char *if_name,
                                        uint32_t *size);

/**
 * Find a free place to insert a new Rx rule.
 *
 * @param ta          Test Agent
 * @param if_name     Interface name
 * @param start       Index from which to start search
 * @param end         Last acceptable index plus one
 *                    (if zero, rules table size will be used)
 * @param location    Found free location
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_rx_rule_find_location(const char *ta,
                                               const char *if_name,
                                               unsigned int start,
                                               unsigned int end,
                                               int64_t *location);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_CFG_RX_RULE_H__ */
