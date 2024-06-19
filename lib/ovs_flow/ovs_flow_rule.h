/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief OVS Flow Rule Processing Library
 *
 * Definition of the flow rule descriptor structure and the associated
 * operations.
 */

#ifndef __TE_OVS_FLOW_RULE_H__
#define __TE_OVS_FLOW_RULE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ovs_flow_rule Open vSwitch flow rule handling
 * @ingroup ovs_flow
 * @{
 */

/** Open vSwitch flow rule descriptor structure */
typedef struct ovs_flow_rule {
    uint64_t cookie;
    uint64_t table;
    uint64_t n_packets;
    uint64_t n_bytes;
    uint64_t n_offload_packets;
    uint64_t n_offload_bytes;

    char *body;
} ovs_flow_rule;

/**
 * Parse the given string into an Open vSwitch flow rule.
 *
 * @param[in]  rule_str Flow rule string representation.
 * @param[out] rule     Flow rule descriptor structure.
 *
 * @return Status code.
 */
extern te_errno ovs_flow_rule_parse(const char *rule_str,
                                    ovs_flow_rule *rule);

/**
 * Free the additional memory used by the flow rule descriptor structure.
 *
 * @param[in]  rule     Flow rule descriptor structure.
 *
 * @return Nothing.
 */
extern void ovs_flow_rule_fini(ovs_flow_rule *rule);

/**
 * Format the flow rule including all metadata.
 * The function allocates a memory buffer for the resulting string, it is the
 * user's responsibility to free it.
 *
 * @param[in]  rule     Flow rule descriptor structure.
 * @param[out] rule_str Where to store the resulting string.
 *
 * @return Status code.
 */
extern te_errno ovs_flow_rule_to_string(ovs_flow_rule *rule, char **rule_str);

/**
 * Format the flow rule according to Open vSwitch's ovs-ofctl expectations.
 * The function allocates a memory buffer for the resulting string, it is the
 * user's responsibility to free it.
 *
 * @param[in]  rule     Flow rule descriptor structure.
 * @param[out] rule_str Where to store the resulting string.
 *
 * @return Status code.
 */
extern te_errno ovs_flow_rule_to_ofctl(ovs_flow_rule *rule, char **rule_str);

/**
 * Given a string representation of some flow rule, extract the value of the
 * field with user-provided name.
 * The function does not allocate anything, the value pointer is guaranteed to
 * be within the same string buffer or NULL.
 * If present, the mask will be included in the result.
 *
 * @param[in]  rule         Flow rule in string form.
 * @param[in]  field        Field whose value needs to be extracted.
 * @param[out] value        Value start address.
 * @param[out] value_len    Length of the value.
 *
 * @return Status code.
 */
extern te_errno ovs_flow_rule_str_get_field(const char *rule, const char *field,
                                        const char **value, size_t *value_len);

/**@} <!-- END ovs_flow_rule --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_OVS_FLOW_RULE_H__ */
