/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Auxiliary library to define network in YAML format
 *
 * @defgroup tapi_net_yaml Auxiliary library to define network in YAML format
 * @ingroup tapi_net
 * @{
 *
 * Definition of test API to provide a way to set up test network defined in YAML files.
 */

#ifndef __TAPI_NET_YAML_H__
#define __TAPI_NET_YAML_H__

#include "tapi_net.h"

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mapping between an address family and its string representation
 * used in the YAML configuration file.
 */
static const te_enum_map tapi_net_yaml_af_map[] = {
    {.name = "inet",  .value = AF_INET},
    {.name = "inet6", .value = AF_INET6},
    TE_ENUM_MAP_END
};

/**
 * Parse a YAML network configuration file.
 *
 * The function reads a YAML document describing test agents, their base
 * interfaces, and logical point-to-point networks built over those interfaces.
 * On success it fills @p net_ctx with arrays of agents and networks that
 * can be used to set up network for tests.
 *
 * === Data model (YAML schema, informal) ===
 *
 * Root mapping contains:
 *   - "interfaces": sequence (required)
 *   - "networks":   sequence (required)
 *
 * 1) interfaces: list of base NICs per agent.
 *    Each item is a mapping:
 *      - agent : string (required)     // test agent name
 *      - names : sequence<string>      // non-empty list of base ifaces
 *
 *    Parsed into: tapi_net_ta[] with per-agent lists of
 *    base interfaces as SLIST stacks (base at head). See
 *    tapi_net_ta/tapi_net_iface.
 *
 * 2) networks: list of point-to-point logical networks.
 *    Each item is a mapping with fields:
 *      - iface_type : enum { base, vlan, qinq, gre } (required)
 *      - af         : enum { inet, inet6 }           (required)
 *      - vlan_id    : int >= 0                       (VLAN only)
 *      - outer_id   : int >= 0                       (QinQ only, S-tag)
 *      - inner_id   : int >= 0                       (QinQ only, C-tag)
 *      - endpoints  : sequence of exactly 2 mappings (required):
 *          * agent      : string (required)
 *          * base_iface : string (required)
 *          * iface      : string (optional; defaults to base_iface for 'base')
 *
 *    Behavior:
 *      - iface_type == base:
 *          No new logical interfaces are created; existing base
 *          interfaces are used as is.
 *      - iface_type in { vlan, qinq, gre }:
 *          Logical interfaces are created on top of the specified base
 *          interfaces and pushed into the per-base SLIST stack (e.g.,
 *          GRE sub-interface can be based on VLAN sub-interface with vlan_id,
 *          which in turn can be based on one of the interfaces defined in
 *          the root 'interfaces' section).
 *          The endpoint "iface" field names the resulting logical
 *          interface.
 *
 *    Parsed into: tapi_net_link[]; each net has name "test_net_<idx>",
 *    address family, and two endpoints (agent/iface).
 *
 * 3) nat: list of NAT rules per agent (optional).
 *    Each item is a mapping:
 *      - agent : string (required)
 *      - rules : sequence<rule> (required)
 *
 *    rule mapping:
 *      - type : enum { snat, dnat } (required)
 *      - mode : enum { address, masquerade } (required)
 *      - from : { agent: string, iface: string } (required)
 *      - to   : { agent: string, iface: string } (conditional)
 *
 *    Semantics:
 *      - For mode = "address":
 *          Both @c from and @c to endpoint objects are required.
 *          Static translation occurs between these two interfaces.
 *
 *      - For mode = "masquerade":
 *          Supported only for @c type = "snat".
 *          The @c to field should be omitted.
 *          The configurator applies standard source NAT
 *          masquerading rules.
 *
 *    Parsed into: tapi_net_nat_rule[] associated with each
 *    @c tapi_net_ta in the network context.
 *
 * === Variable expansion ===
 *
 * Scalar values in the YAML (strings and integers written as scalars) are
 * passed through environment expansion.
 *
 * @param[in]  filename             Path to YAML configuration file.
 * @param[out] net_ctx              Network configuration context.
 *
 * @return Status code.
 */
extern te_errno tapi_net_yaml_parse(const char *filename, tapi_net_ctx *net_ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TAPI_NET_YAML_H__ */

/**@} <!-- END tapi_net_yaml --> */
