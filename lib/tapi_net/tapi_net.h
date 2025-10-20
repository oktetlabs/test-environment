/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Network setup library
 *
 * @defgroup tapi_net Network setup library
 * @ingroup te_ts_tapi
 * @{
 *
 * Definition of test API to define and set up test network.
 */

#ifndef __TAPI_NET_H__
#define __TAPI_NET_H__

#include <net/if.h>

#include "te_errno.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_sockaddr.h"
#include "te_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The limit of endpoint number which can be used in test networks. */
#define TAPI_NET_EP_NUM 2

/** Length of network name. */
#define TAPI_NET_NAME_LEN 64

/** Supported interface types. */
typedef enum tapi_net_iface_type {
    /** Unknown interface type */
    TAPI_NET_IFACE_TYPE_UNKNOWN = -1,
    /** Base interface. */
    TAPI_NET_IFACE_TYPE_BASE,
    /** VLAN interface. */
    TAPI_NET_IFACE_TYPE_VLAN,
    /** QinQ interface. */
    TAPI_NET_IFACE_TYPE_QINQ,
    /** GRE tunnel interface. */
    TAPI_NET_IFACE_TYPE_GRE,
} tapi_net_iface_type;

/** VLAN-specific configuration. */
typedef struct tapi_net_vlan {
    int vlan_id; /**< VLAN ID. */
} tapi_net_vlan;

/** QinQ-specific configuration. */
typedef struct tapi_net_qinq {
    int outer_id; /**< Outer VLAN ID (S-tag). */
    int inner_id; /**< Inner VLAN ID (C-tag). */
} tapi_net_qinq;

/**
 * Logical interface definition.
 *
 * Interfaces are organized as a stack, starting from a base
 * and building up through VLAN, QinQ, GRE, or other layers.
 * The stack is represented as a singly linked list, where each
 * interface points to the next one above it.
 *
 * The structure defines the interface type and any
 * type-specific parameters, such as a VLAN ID.
 */
typedef struct tapi_net_iface {
    SLIST_ENTRY(tapi_net_iface) iface_next; /**< Link to the upper
                                                 interface. */
    tapi_net_iface_type type;               /**< Interface type. */
    struct sockaddr *addr;                  /**< Address assigned
                                                 to interface. */
    union {
        tapi_net_vlan vlan;                 /**< VLAN config. */
        tapi_net_qinq qinq;                 /**< QinQ config. */
    } conf;
    char *name;                             /**< Interface name. */
} tapi_net_iface;

/** Singly-linked list of logical interfaces. */
typedef SLIST_HEAD(tapi_net_iface_head, tapi_net_iface) tapi_net_iface_head;

/** Network endpoint of single connection. */
typedef struct tapi_net_endpoint {
    char *ta_name;    /**< Agent name. */
    char *if_name;    /**< Interface name. */
} tapi_net_endpoint;

/** Type of NAT rule. */
typedef enum tapi_net_nat_rule_type {
    /** Unknown NAT type. */
    TAPI_NET_NAT_RULE_TYPE_UNKNOWN = -1,
    /** Destination address translation. */
    TAPI_NET_NAT_RULE_TYPE_DNAT,
    /** Source address translation. */
    TAPI_NET_NAT_RULE_TYPE_SNAT,
} tapi_net_nat_rule_type;

/** Mode of NAT rule. */
typedef enum tapi_net_nat_rule_mode {
    /** Unknown NAT rule mode. */
    TAPI_NET_NAT_RULE_MODE_UNKNOWN = -1,
    /** Address-based NAT rule mode. */
    TAPI_NET_NAT_RULE_MODE_ADDRESS,
    /** Masquerade NAT rule mode (SNAT only). */
    TAPI_NET_NAT_RULE_MODE_MASQUERADE,
} tapi_net_nat_rule_mode;

/** Single NAT rule. */
typedef struct tapi_net_nat_rule {
    tapi_net_nat_rule_type type;    /**< Type of NAT rule. */
    tapi_net_nat_rule_mode mode;    /**< Mode of NAT rule. */
    tapi_net_endpoint from;         /**< Endpoint where the rule
                                         is matched. */
    tapi_net_endpoint to;           /**< Endpoint whose IP is used
                                         for translation. */
} tapi_net_nat_rule;

/** Test agent with a list of logical interfaces. */
typedef struct tapi_net_ta {
    char *ta_name;        /**< Test Agent name. */
    te_vec ifaces;        /**< Vector of logical interfaces built on TA. */
    te_vec nat_rules;     /**< Vector of NAT rules set on TA. */
} tapi_net_ta;

/**
 * Logical network between two endpoints.
 * For the test purposes, networks are modeled as point-to-point connections.
 */
typedef struct tapi_net_link {
    /** Network name. */
    char name[TAPI_NET_NAME_LEN];
    /** Network endpoints. */
    tapi_net_endpoint endpoints[TAPI_NET_EP_NUM];
    /** Address family. */
    int af;
} tapi_net_link;

/**
 * Network configuration context that includes all
 * interface definitions and network topologies.
 */
typedef struct tapi_net_ctx {
    te_vec agents;   /**< Vector holding agent-specific information. */
    te_vec nets;     /**< Vector holding network-specific information. */
} tapi_net_ctx;

/**
 * Initialize network configuration context.
 * This sets up vectors and their destructors.
 */
extern void tapi_net_ctx_init(tapi_net_ctx *net_ctx);

/**
 * Release Network Configuration Context.
 *
 * @param  net_ctx     Network Configuration Context.
 */
extern void tapi_net_ctx_release(tapi_net_ctx *net_ctx);

/**
 * Initialize Test Agent network configuration.
 *
 * @param[in]  ta_name              TA name.
 * @param[out] cfg_net_ta           Resulting TA network configuration.
 */
extern void tapi_net_ta_init(const char *ta_name, tapi_net_ta *cfg_net_ta);

/**
 * Set interfaces in network configuration for specific Test Agent.
 *
 * @param[in]  cfg_net_ta           TA network configuration.
 * @param[in]  if_name_list         NULL-terminated list of physical
 *                                  interface names.
 *
 * @return Status of the initializion.
 */
extern void tapi_net_ta_set_ifaces(tapi_net_ta *net_cfg_ta,
                                   const char **if_name_list);

/**
 * Destroy Test Agent network configuration.
 *
 * @param net_cfg_ta Pointer to TA network configuration.
 */
extern void tapi_net_ta_destroy(tapi_net_ta *cfg_net_ta);

/**
 * Set interface VLAN-specific information.
 *
 * @param iface    Pointer to VLAN interface.
 * @param vlan     VLAN information.
 *
 * @return Status of the setting.
 * @retval TE_EINVAL     Invalid @p vlan_id was passed.
 * @retval 0             Success.
 */
extern te_errno tapi_net_iface_set_vlan_conf(tapi_net_iface *iface,
                                             const tapi_net_vlan *vlan);

/**
 * Set interface QinQ-specific information.
 *
 * @param iface     Pointer to QinQ interface.
 * @param qinq      QinQ information.
 *
 * @return Status of the setting.
 * @retval TE_EINVAL     Invalid @p inner_id or @p outer_id was passed.
 * @retval 0             Success.
 */
extern te_errno tapi_net_iface_set_qinq_conf(tapi_net_iface *iface,
                                             const tapi_net_qinq *qinq);

/**
 * Add new logical interface.
 *
 * @param[in]  iface_type  Iterface type.
 * @param[in]  if_name     Interface name.
 * @param[in]  base_iface  Pointer to the interface to base on.
 * @param[out] iface       Pointer to the added interfaces.
 *
 * @return Status of the adding.
 * @retval TE_EINVAL     Unsupported type of logical interface.
 * @retval 0             Success.
 */
extern te_errno tapi_net_logical_iface_add(tapi_net_iface_type iface_type,
                                           const char *if_name,
                                           tapi_net_iface *base_iface,
                                           tapi_net_iface **iface);

/**
 * Find TA network configuration in network context.
 *
 * @param net_ctx      Network configuration context.
 * @param ta_name      TA name to find.
 *
 * @return Pointer to TA network configuration, or @c NULL in case of error.
 */
extern tapi_net_ta *tapi_net_find_agent_by_name(tapi_net_ctx *net_ctx,
                                                const char *ta_name);
/**
 * Find interface by its name in TA network configuration.
 *
 * @param net_cfg_ta   Pointer TA network configuration.
 * @param if_name      Interface name to find.
 *
 * @return Pointer to interface, or @c NULL in case of error.
 */
extern tapi_net_iface *tapi_net_find_iface_by_name(tapi_net_ta *net_cfg_ta,
                                                   const char *if_name);

/**
 * Get interface type by its string representation.
 *
 * @param if_name  String with Network type
 *
 * @return Interface type.
 */
extern tapi_net_iface_type tapi_net_iface_type_by_name(
                               const char *iface_type_str);

/**
 * Setup interfaces specified in the network context.
 *
 * @param net_ctx    Network context.
 *
 * @return Status code.
 */
extern te_errno tapi_net_setup_ifaces(const tapi_net_ctx *net_ctx);

/**
 * Get interface name of the top-most interface in the stack.
 *
 * @param  iface_head   Head of the interface stack.
 *
 * @return Interface name or @c NULL on error.
 */
extern const char *tapi_net_get_top_iface_name(
                       const tapi_net_iface_head *iface_head);

/**
 * Get address of the top-most interface in the stack.
 *
 * @param[in]  iface_head   Head of the interface stack.
 * @param[out] addr         Address of the interface.
 *
 * @return Status of the adding.
 * @retval TE_ENOENT     No address assigned to requested interface.
 * @retval 0             Success.
 */
extern te_errno tapi_net_get_top_iface_addr(
                    const tapi_net_iface_head *iface_head,
                    const struct sockaddr **addr);

/**
 * Setup network based on network context.
 *
 * @param net_ctx       Network contex to use for setup.
 *
 * @return Status code.
 */
extern te_errno tapi_net_setup(tapi_net_ctx *net_ctx);

/**
 * Initialize NAT rule.
 *
 * @param rule     NAT rule to init.
 */
extern void tapi_net_nat_rule_init(tapi_net_nat_rule *rule);

/**
 * Validate NAT rule.
 *
 * @param rule     NAT rule to validate.
 *
 * @return Status code.
 * @retval TE_EINVAL     Validation failed.
 * @retval 0             Success.
 */
extern te_errno tapi_net_nat_rule_validate(const tapi_net_nat_rule *rule);

/**
 * Check NAT rule duplicates.
 *
 * @param agent   Test agent which owns the NAT rule list.
 * @param rule    Rule to validate.
 *
 * @return Status code.
 * @retval TE_EEXIST     Conflicting rule already exists.
 * @retval 0             Success.
 */
extern te_errno tapi_net_nat_rule_check_dup(const tapi_net_ta *agent,
                                            const tapi_net_nat_rule *rule);

/**
 * Fill in IP addresses for logical interfaces based on netowrks in Configurator.
 *
 * This function set appropriate IP addresses for the logical interface
 * structures mentioned in Configurator to use them after in tests.
 *
 * @param net_ctx       Network contex
 *
 * @return Status code.
 */
extern te_errno tapi_net_addr_fill(tapi_net_ctx *net_ctx);

/**
 * Resolve IP address of specific network endpoint.
 *
 * @param net_ctx       Network contex to use for resolving.
 * @param ep            Network endpoint.
 *
 * @return Resolved IP address.
 */
extern const struct sockaddr *tapi_net_ep_resolve_ip_addr(
                                  tapi_net_ctx *ctx,
                                  const tapi_net_endpoint *ep);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TAPI_NET_H__ */

/**@} <!-- END tapi_net --> */
