/** @file
 * @brief RPC client API for RTE flow
 *
 * RPC client API for RTE flow functions
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Roman Zhukov <Roman.Zhukov@oktetlabs.ru>
 */

#ifndef __TE_RTE_FLOW_NDN_H__
#define __TE_RTE_FLOW_NDN_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"
#include "ndn_rte_flow.h"
#include "asn_usr.h"


#ifdef __cplusplus
extern "C" {
#endif

#define TEST_GET_NDN_RTE_FLOW_RULE_GENERIC(_var_name, _rte_flow_ndn_type) \
    do {                                                        \
        const char *_str_val;                                   \
        int _parsed;                                            \
                                                                \
        _str_val = test_get_param(argc, argv, #_var_name);      \
        if (_str_val == NULL)                                   \
            TEST_STOP;                                          \
                                                                \
        CHECK_RC(asn_parse_value_text(_str_val,                 \
                                      _rte_flow_ndn_type,       \
                                      &(_var_name),             \
                                      &_parsed));               \
        if (_str_val[_parsed] != '\0')                          \
            TEST_FAIL("Trailing symbols after rte "             \
                      "flow rule components '%s'",              \
                      &_str_val[_parsed]);                      \
    } while (0)

/**
 * Get ndn_rte_flow_rule ASN.1 type test parameter.
 *
 * @param _var_name  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_NDN_RTE_FLOW_RULE(_var_name) \
    TEST_GET_NDN_RTE_FLOW_RULE_GENERIC(_var_name, ndn_rte_flow_rule)

/**
 * Get ndn_rte_flow_attr ASN.1 type test parameter.
 *
 * @param _var_name  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_NDN_RTE_FLOW_ATTR(_var_name) \
    TEST_GET_NDN_RTE_FLOW_RULE_GENERIC(_var_name, ndn_rte_flow_attr)

/**
 * Get ndn_rte_flow_pattern ASN.1 type test parameter.
 *
 * @param _var_name  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_NDN_RTE_FLOW_PATTERN(_var_name) \
    TEST_GET_NDN_RTE_FLOW_RULE_GENERIC(_var_name, ndn_rte_flow_pattern)

/**
 * Get ndn_rte_flow_actions ASN.1 type test parameter.
 *
 * @param _var_name  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_NDN_RTE_FLOW_ACTIONS(_var_name) \
    TEST_GET_NDN_RTE_FLOW_RULE_GENERIC(_var_name, ndn_rte_flow_actions)

/**
 * Make RTE flow components from ASN.1 representation. In one function call,
 * attributes, pattern and actions can be made all together from ASN.1 flow
 * rule, or only one of them from corresponding ASN.1 representation.
 *
 * @param[in]  flow_rule_components ASN.1 flow rule components
 * @param[out] attr                 RTE flow attr pointer
 * @param[out] pattern              RTE flow item pointer to the array of items
 * @param[out] actions              RTE flow action pointer to the array of
 *                                  actions
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern int rpc_rte_mk_flow_rule_components(rcf_rpc_server *rpcs,
                                           const asn_value *flow_rule_components,
                                           rpc_rte_flow_attr_p *attr,
                                           rpc_rte_flow_item_p *pattern,
                                           rpc_rte_flow_action_p *actions);

/**
 * Free RTE flow structures with attributes, pattern and actions.
 *
 * @param[in]  attr        RTE flow attr pointer
 * @param[in]  pattern     RTE flow item pointer to the array of items
 * @param[in]  actions     RTE flow action pointer to the array of actions
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern void rpc_rte_free_flow_rule(rcf_rpc_server *rpcs,
                                   rpc_rte_flow_attr_p attr,
                                   rpc_rte_flow_item_p pattern,
                                   rpc_rte_flow_action_p actions);

/**
 * Insert RTE flow rule items provided in ASN.1 representaton into
 * RTE flow rule pattern starting at specified index.
 *
 * @param[inout] pattern    RTE flow item pointer to the array of items,
 *                          on success contains pointer to the new array
 * @param[in]    items      ASN.1 representation of flow rule items to insert
 * @param[in]    index      Index starting at which to insert items, negative
 *                          means insert to the end of the pattern
 *
 * @return Status code; jumps out in case of failure
 */
extern int rpc_rte_insert_flow_rule_items(rcf_rpc_server *rpcs,
                                          rpc_rte_flow_item_p *pattern,
                                          const asn_value *items,
                                          int index);

/**
 * Concatenate the given flow action arrays in a newly allocated one
 *
 * @param[in]   flow_actions       RPC pointer to an END-terminated action array
 * @param[in]   opaque_actions     RPC pointer to an array of opaque actions
 * @param[in]   nb_opaque_actions  The number of the opaque actions
 * @param[out]  united_actions     Location for the result
 *
 * @return Status code; jumps out on error (negative value)
 */
extern int rpc_rte_flow_prepend_opaque_actions(
                                      rcf_rpc_server         *rpcs,
                                      rpc_rte_flow_action_p   flow_actions,
                                      rpc_rte_flow_action_p   opaque_actions,
                                      unsigned int            nb_opaque_actions,
                                      rpc_rte_flow_action_p  *united_actions);

/** Cleanup counterpart for @p rpc_rte_flow_prepend_opaque_actions() */
extern void rpc_rte_flow_release_united_actions(
                                        rcf_rpc_server         *rpcs,
                                        rpc_rte_flow_action_p   united_actions);

/**
 * Concatenate the given flow item arrays in a newly allocated one
 *
 * @param[in]   flow_items       RPC pointer to an END-terminated item array
 * @param[in]   opaque_items     RPC pointer to an array of opaque items
 * @param[in]   nb_opaque_items  The number of the opaque items
 * @param[out]  united_items     Location for the result
 *
 * @return Status code; jumps out on error (negative value)
 */
extern int rpc_rte_flow_prepend_opaque_items(
                                          rcf_rpc_server       *rpcs,
                                          rpc_rte_flow_item_p   flow_items,
                                          rpc_rte_flow_item_p   opaque_items,
                                          unsigned int          nb_opaque_items,
                                          rpc_rte_flow_item_p  *united_items);

/** Cleanup counterpart for @p rpc_rte_flow_prepend_opaque_items() */
extern void rpc_rte_flow_release_united_items(
                                            rcf_rpc_server       *rpcs,
                                            rpc_rte_flow_item_p   united_items);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RTE_FLOW_NDN_H__ */
