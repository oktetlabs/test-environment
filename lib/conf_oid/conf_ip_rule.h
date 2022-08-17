/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief ip rule configuratior objects processing
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_LIB_CONF_OID_CONF_IP_RULE_H_
#define __TE_LIB_CONF_OID_CONF_IP_RULE_H_

#include "te_config.h"

#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "conf_ip.h"
#include "conf_object.h"
#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Context of ip rule */
typedef struct te_conf_ip_rule {
    uint8_t         family;             /**< Address family of rule */
    te_ip_type      type;               /**< Type of rule entry */
    te_bool         invert;             /**< Inversion flag (works as
                                             ip rule not ... ). */
    uint8_t         srclen;             /**< Prefix length of source */
    uint8_t         dstlen;             /**< Prefix length of destination */

    struct sockaddr_storage   src;      /**< Source address */
    struct sockaddr_storage   dst;      /**< Destination address */

    char            iifname[IFNAMSIZ];  /**< Incoming device */
    char            oifname[IFNAMSIZ];  /**< Outgoing device */
    uint32_t        goto_index;         /**< Index of rule for jumping */
    uint8_t         tos;                /**< Type Of Service key */
    uint32_t        fwmark;             /**< fwmark value */
    uint32_t        fwmask;             /**< fwmask value */
    uint32_t        priority;           /**< Rule priority */
    te_ip_table_id  table;              /**< Routing table id */
    uint32_t        flow;               /**< Realms FROM/TO */

    uint32_t        mask;               /**< Mask of set fields for
                                             searching or deleting */
} te_conf_ip_rule;

/** @name Flags to determine fields in structure @b te_conf_ip_rule */
#define TE_IP_RULE_FLAG_NONE        0
#define TE_IP_RULE_FLAG_FAMILY      (1u <<  0)
#define TE_IP_RULE_FLAG_TYPE        (1u <<  1)
#define TE_IP_RULE_FLAG_SRCLEN      (1u <<  2)
#define TE_IP_RULE_FLAG_DSTLEN      (1u <<  3)
#define TE_IP_RULE_FLAG_SRC         (1u <<  4)
#define TE_IP_RULE_FLAG_DST         (1u <<  5)
#define TE_IP_RULE_FLAG_IIFNAME     (1u <<  6)
#define TE_IP_RULE_FLAG_OIFNAME     (1u <<  7)
#define TE_IP_RULE_FLAG_GOTO        (1u <<  8)
#define TE_IP_RULE_FLAG_TOS         (1u <<  9)
#define TE_IP_RULE_FLAG_FWMARK      (1u << 10)
#define TE_IP_RULE_FLAG_FWMASK      (1u << 11)
#define TE_IP_RULE_FLAG_PRIORITY    (1u << 12)
#define TE_IP_RULE_FLAG_TABLE       (1u << 13)
#define TE_IP_RULE_FLAG_FLOW        (1u << 14)
#define TE_IP_RULE_FLAG_INVERT      (1u << 15)
/** @} */

/** Macro to make field name shorter */
#define TE_IP_RULE_FLAG(_flag) \
    TE_CONCAT(TE_IP_RULE_FLAG_, _flag)

/**
 * Initialize structure @b te_conf_ip_rule
 *
 * @param rule      Structure to initialize
 */
extern void te_conf_ip_rule_init(te_conf_ip_rule *rule);

/**
 * Transform structure @b te_conf_ip_rule to @b char*
 *
 * @param [in]  rule    Source structure
 * @param [out] str     Pointer to result string
 *
 * @return              Status code
 */
extern te_errno te_conf_ip_rule_to_str(
        const te_conf_ip_rule *rule, char **str);

/**
 * Transform @b char * to structure @b te_conf_ip_rule and @p required
 * fields
 *
 * @param [in]  str         Source string
 * @param [out] required    Final required fields
 * @param [out] rule        Final structure
 *
 * @return                  Status code
 */
extern te_errno te_conf_ip_rule_from_str(
        const char *str, uint32_t *required, te_conf_ip_rule *rule);

/**
 * Compare two structures @b te_conf_ip_rule with specified @p required
 * fields
 *
 * @param [in] required     Mask required fields
 * @param [in] object_a     First structure
 * @param [in] object_b     Second structure
 *
 * @return                  Result of the comparison
 */
extern te_conf_obj_compare_result te_conf_ip_rule_compare(
        uint32_t required, const te_conf_ip_rule *object_a,
        const te_conf_ip_rule *object_b);

/**
 * Set inversion flag for IP rule.
 *
 * @param rule    IP rule structure.
 * @param invert  If TRUE, set the flag; otherwise clear it.
 */
extern void te_conf_ip_rule_set_invert(te_conf_ip_rule *rule,
                                       te_bool invert);

/**
 * Get state of inversion flag for IP rule.
 *
 * @param rule    IP rule structure.
 *
 * @return TRUE if flag is set, FALSE otherwise.
 */
extern te_bool te_conf_ip_rule_get_invert(const te_conf_ip_rule *rule);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_LIB_CONF_OID_CONF_IP_RULE_H_ */
