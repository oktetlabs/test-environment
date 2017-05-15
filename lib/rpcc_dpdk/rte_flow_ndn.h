/** @file
 * @brief RPC client API for RTE flow
 *
 * RPC client API for RTE flow functions
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
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

/**
 * Get ndn_rte_flow_rule ASN.1 type test parameter.
 *
 * @param _var_name  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_NDN_RTE_FLOW_RULE(_var_name) \
    do {                                                        \
        const char *_str_val;                                   \
        int _parsed;                                            \
                                                                \
        _str_val = test_get_param(argc, argv, #_var_name);      \
        if (_str_val == NULL)                                   \
            TEST_STOP;                                          \
                                                                \
        CHECK_RC(asn_parse_value_text(_str_val,                 \
                                      ndn_rte_flow_rule,        \
                                      &(_var_name),             \
                                      &_parsed));               \
        if (_str_val[_parsed] != '\0')                          \
            TEST_FAIL("Trailing symbols after rte "             \
                      "flow rule '%s'", &_str_val[_parsed]);    \
    } while (0)

/**
 * Make RTE flow structures with attributes, pattern and actions
 * from ASN.1 representation of flow rule.
 *
 * @param[in]  flow_rule   ASN.1 flow rule
 * @param[out] attr        RTE flow attr pointer
 * @param[out] pattern     RTE flow item pointer to the array of items
 * @param[out] actions     RTE flow action pointer to the array of actions
 *
 * @return @c 0 on success; jumps out in case of failure
 */
extern int rpc_rte_mk_flow_rule_from_str(rcf_rpc_server *rpcs,
                                         const asn_value  *flow_rule,
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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RTE_FLOW_NDN_H__ */
