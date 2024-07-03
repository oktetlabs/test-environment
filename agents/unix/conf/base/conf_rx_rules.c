/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Rx classification rules
 *
 * Rx classification rules configuration on Unix TA
 */

#define TE_LGR_USER     "Conf Rx rules"

#include "te_config.h"
#include "config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "unix_internal.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

#include "conf_ethtool.h"

#ifdef ETHTOOL_GRXCLSRLALL

/* @c true if adding new Rx classification rule is in progress */
static bool rule_add_started = false;
/* Location of Rx rule added the last time */
static long int last_rule_added = -1;
/*
 * Name of the interface on which Rx classification rule was added
 * most recently
 */
static char last_rule_if_name[RCF_MAX_VAL] = "";

/* List existing Rx rules */
static te_errno
rules_list(unsigned int gid,
           const char *oid,
           const char *sub_id,
           char **list_out,
           const char *if_name)
{
    ta_ethtool_rx_cls_rules *rules = NULL;
    unsigned int i;
    te_string str = TE_STRING_INIT;
    te_errno rc;

    UNUSED(oid);
    UNUSED(sub_id);

    rc = ta_ethtool_get_rx_cls_rules(gid, if_name, &rules);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP)
        {
            *list_out = NULL;
            return 0;
        }
        return rc;
    }

    for (i = 0; i < rules->rule_cnt; i++)
    {
        rc = te_string_append_chk(&str, "%u ", rules->locs[i]);
        if (rc != 0)
        {
            te_string_free(&str);
            return rc;
        }
    }

    *list_out = str.ptr;
    return 0;
}

/* Get location of Rx rule added the last time for a given interface */
static te_errno
rules_last_added_get(unsigned int gid, const char *oid,
                     char *value, const char *if_name)
{
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);

    UNUSED(gid);
    UNUSED(oid);

    if (last_rule_added < 0 || strcmp(if_name, last_rule_if_name) != 0)
        return TE_ENOENT;

    return te_string_append_chk(&str_val, "%ld", last_rule_added);
}

/* Get size of Rx classification rules table */
static te_errno
rules_table_size_get(unsigned int gid, const char *oid,
                     char *value, const char *if_name)
{
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);
    ta_ethtool_rx_cls_rules *rules = NULL;
    te_errno rc;

    UNUSED(oid);

    rc = ta_ethtool_get_rx_cls_rules(gid, if_name, &rules);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);

        return rc;
    }

    return te_string_append_chk(&str_val, "%u", rules->table_size);
}

/*
 * Check whether special insert locations are supported for Rx
 * classification rules
 */
static te_errno
rules_spec_loc_get(unsigned int gid, const char *oid,
                   char *value, const char *if_name)
{
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);
    ta_ethtool_rx_cls_rules *rules = NULL;
    te_errno rc;

    UNUSED(oid);

    rc = ta_ethtool_get_rx_cls_rules(gid, if_name, &rules);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);

        return rc;
    }

    return te_string_append_chk(&str_val, "%u",
                                (rules->spec_loc_flag ? 1 : 0));
}

/* Get string name of network flow type */
static te_errno
flow_type2str(unsigned int flow_type, te_string *str)
{
    switch (flow_type)
    {
        case TCP_V4_FLOW:
            return te_string_append_chk(str, "tcp_v4");

        case UDP_V4_FLOW:
            return te_string_append_chk(str, "udp_v4");

        case SCTP_V4_FLOW:
            return te_string_append_chk(str, "sctp_v4");

#ifdef AH_V4_FLOW
        case AH_V4_FLOW:
            return te_string_append_chk(str, "ah_v4");
#endif

#ifdef ESP_V4_FLOW
        case ESP_V4_FLOW:
            return te_string_append_chk(str, "esp_v4");
#endif

#ifdef IPV4_USER_FLOW
        case IPV4_USER_FLOW:
            return te_string_append_chk(str, "ipv4_user");
#endif

        case TCP_V6_FLOW:
            return te_string_append_chk(str, "tcp_v6");

        case UDP_V6_FLOW:
            return te_string_append_chk(str, "udp_v6");

        case SCTP_V6_FLOW:
            return te_string_append_chk(str, "sctp_v6");

#ifdef AH_V6_FLOW
        case AH_V6_FLOW:
            return te_string_append_chk(str, "ah_v6");
#endif

#ifdef ESP_V6_FLOW
        case ESP_V6_FLOW:
            return te_string_append_chk(str, "esp_v6");
#endif

#ifdef IPV6_USER_FLOW
        case IPV6_USER_FLOW:
            return te_string_append_chk(str, "ipv6_user");
#endif

#ifdef ETHER_FLOW
        case ETHER_FLOW:
            return te_string_append_chk(str, "ether");
#endif
    }

    ERROR("%s(): flow type 0x%x is not supported",
          __FUNCTION__, flow_type);
    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

/* Parse string name of network flow type */
static te_errno
str2flow_type(const char *value, unsigned int *flow_type)
{
    unsigned int type;

    if (strcmp(value, "tcp_v4") == 0)
    {
        type = TCP_V4_FLOW;
    }
    else if (strcmp(value, "udp_v4") == 0)
    {
        type = UDP_V4_FLOW;
    }
    else if (strcmp(value, "sctp_v4") == 0)
    {
        type = SCTP_V4_FLOW;
    }
#ifdef AH_V4_FLOW
    else if (strcmp(value, "ah_v4") == 0)
    {
        type = AH_V4_FLOW;
    }
#endif
#ifdef ESP_V4_FLOW
    else if (strcmp(value, "esp_v4") == 0)
    {
        type = ESP_V4_FLOW;
    }
#endif
#ifdef IPV4_USER_FLOW
    else if (strcmp(value, "ipv4_user") == 0)
    {
        type = IPV4_USER_FLOW;
    }
#endif
    else if (strcmp(value, "tcp_v6") == 0)
    {
        type = TCP_V6_FLOW;
    }
    else if (strcmp(value, "udp_v6") == 0)
    {
        type = UDP_V6_FLOW;
    }
    else if (strcmp(value, "sctp_v6") == 0)
    {
        type = SCTP_V6_FLOW;
    }
#ifdef AH_V6_FLOW
    else if (strcmp(value, "ah_v6") == 0)
    {
        type = AH_V6_FLOW;
    }
#endif
#ifdef ESP_V6_FLOW
    else if (strcmp(value, "esp_v6") == 0)
    {
        type = ESP_V6_FLOW;
    }
#endif
#ifdef IPV6_USER_FLOW
    else if (strcmp(value, "ipv6_user") == 0)
    {
        type = IPV6_USER_FLOW;
    }
#endif
#ifdef ETHER_FLOW
    else if (strcmp(value, "ether") == 0)
    {
        type = ETHER_FLOW;
    }
#endif
    else
    {
        ERROR("%s(): flow type '%s' is not supported",
              __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    *flow_type = type;
    return 0;
}

/* Parse string containing Rx rule location */
static te_errno
parse_rule_location(const char *loc_str, unsigned int *loc_out)
{
    unsigned int location;
    te_errno rc;

    if (loc_str == NULL)
        return TE_EINVAL;

#ifdef RX_CLS_LOC_SPECIAL
    if (strcmp(loc_str, "any") == 0)
    {
        location = RX_CLS_LOC_ANY;
    }
    else if (strcmp(loc_str, "first") == 0)
    {
        location = RX_CLS_LOC_FIRST;
    }
    else if (strcmp(loc_str, "last") == 0)
    {
        location = RX_CLS_LOC_LAST;
    }
    else
#endif
    {
        rc = te_strtoui(loc_str, 0, &location);
        if (rc != 0)
            return rc;
    }

    *loc_out = location;

    return 0;
}

/*
 * Get information about Rx rule with a given location in
 * rules table
 */
static te_errno
get_rule(unsigned int gid, const char *if_name, const char *loc_str,
         ta_ethtool_rx_cls_rule **rule)
{
    unsigned int location;
    te_errno rc;

    rc = parse_rule_location(loc_str, &location);
    if (rc != 0)
        return rc;

    return ta_ethtool_get_rx_cls_rule(gid, if_name, location, rule);
}

/* Start adding a new Rx rule (it will have to be committed) */
static te_errno
rule_add(unsigned int gid, const char *oid,
         const char *value, const char *if_name,
         const char *rules_name, const char *loc_str)
{
    unsigned int location;
    te_errno rc;

    UNUSED(oid);
    UNUSED(value);
    UNUSED(rules_name);

    if (rule_add_started)
    {
        ERROR("%s(): adding only one rule at a time is supported",
              __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINPROGRESS);
    }

    rc = parse_rule_location(loc_str, &location);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_add_rx_cls_rule(gid, if_name, location, NULL);
    if (rc == 0)
    {
        rule_add_started = true;
        TE_SPRINTF(last_rule_if_name, "%s", if_name);
        last_rule_added = -1;
    }

    return rc;
}

/* Remove existing Rx rule */
static te_errno
rule_del(unsigned int gid, const char *oid,
         const char *if_name, const char *rules_name,
         const char *loc_str)
{
    unsigned int location;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(rules_name);

    rc = te_strtoui(loc_str, 10, &location);
    if (rc != 0)
        return rc;

    return ta_ethtool_del_rx_cls_rule(if_name, location);
}

/* Commit changes to Rx rule */
static te_errno
rule_commit(unsigned int gid, const cfg_oid *p_oid)
{
    char *if_name = NULL;
    char *rule_loc = NULL;
    unsigned int location;
    unsigned int ret_location;
    te_errno rc;

    if (p_oid->len <= 4)
    {
        ERROR("%s(): committed OID is too short", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if_name = CFG_OID_GET_INST_NAME(p_oid, 2);
    rule_loc = CFG_OID_GET_INST_NAME(p_oid, 4);

    rc = parse_rule_location(rule_loc, &location);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_commit_rx_cls_rule(gid, if_name, location,
                                       &ret_location);
    if (rc == 0 && rule_add_started)
        last_rule_added = ret_location;

    rule_add_started = false;

    return rc;
}

/* Get RSS context of Rx rule */
static te_errno
rule_rss_context_get(unsigned int gid, const char *oid,
                     char *value, const char *if_name,
                     const char *rules_name,
                     const char *loc_str)
{
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);

    ta_ethtool_rx_cls_rule *rule = NULL;
    te_errno rc;

    UNUSED(oid);
    UNUSED(rules_name);

    rc = get_rule(gid, if_name, loc_str, &rule);
    if (rc != 0)
        return rc;

    return te_string_append_chk(&str_val, "%jd", rule->rss_context);
}

/* Set RSS context of Rx rule */
static te_errno
rule_rss_context_set(unsigned int gid, const char *oid,
                     const char *value, const char *if_name,
                     const char *rules_name, const char *loc_str)
{
    ta_ethtool_rx_cls_rule *rule = NULL;
    intmax_t rss_ctx = 0;
    te_errno rc;

    UNUSED(oid);
    UNUSED(rules_name);

    rc = get_rule(gid, if_name, loc_str, &rule);
    if (rc != 0)
        return rc;

    rc = te_strtoimax(value, 10, &rss_ctx);
    if (rc != 0)
        return rc;

    rule->rss_context = rss_ctx;
    return 0;
}

/* Get RSS queue Id of Rx rule */
static te_errno
rule_rx_queue_get(unsigned int gid, const char *oid,
                  char *value, const char *if_name,
                  const char *rules_name, const char *loc_str)
{
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);

    ta_ethtool_rx_cls_rule *rule = NULL;
    te_errno rc;

    UNUSED(oid);
    UNUSED(rules_name);

    rc = get_rule(gid, if_name, loc_str, &rule);
    if (rc != 0)
        return rc;

    if (rule->rx_queue == RX_CLS_FLOW_DISC)
    {
        return te_string_append_chk(&str_val, "%d", -1);
    }
    else
    {
        return te_string_append_chk(
                        &str_val, "%ju", rule->rx_queue);
    }
}

/* Set RSS queue Id of Rx rule */
static te_errno
rule_rx_queue_set(unsigned int gid, const char *oid,
                  const char *value, const char *if_name,
                  const char *rules_name, const char *loc_str)
{
    ta_ethtool_rx_cls_rule *rule = NULL;
    intmax_t rx_queue = 0;
    te_errno rc;

    UNUSED(oid);
    UNUSED(rules_name);

    rc = get_rule(gid, if_name, loc_str, &rule);
    if (rc != 0)
        return rc;

    rc = te_strtoimax(value, 10, &rx_queue);
    if (rc != 0)
        return rc;

    if (rx_queue == -1)
        rx_queue = RX_CLS_FLOW_DISC;

    rule->rx_queue = rx_queue;
    return 0;
}

/*
 * Get value stored in flow_spec configuration node (flow type of Rx rule)
 */
static te_errno
rule_flow_spec_get(unsigned int gid, const char *oid,
                   char *value, const char *if_name,
                   const char *rules_name, const char *loc_str)
{
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);

    ta_ethtool_rx_cls_rule *rule = NULL;
    te_errno rc;

    UNUSED(oid);
    UNUSED(rules_name);

    rc = get_rule(gid, if_name, loc_str, &rule);
    if (rc != 0)
        return rc;

    return flow_type2str(rule->flow_type, &str_val);
}

/* Set value stored in flow_spec node (flow type of Rx rule) */
static te_errno
rule_flow_spec_set(unsigned int gid, const char *oid,
                   const char *value, const char *if_name,
                   const char *rules_name, const char *loc_str)
{
    ta_ethtool_rx_cls_rule *rule = NULL;
    unsigned int type;
    te_errno rc;

    UNUSED(oid);
    UNUSED(rules_name);

    rc = get_rule(gid, if_name, loc_str, &rule);
    if (rc != 0)
        return rc;

    rc = str2flow_type(value, &type);
    if (rc != 0)
        return rc;

    rule->flow_type = type;
    return 0;
}

/*
 * Parse OID of Rx rule flow specification field to determine
 * where to look for it
 */
static te_errno
rule_field_from_oid(const char *oid, te_string *field_name, bool *mask)
{
    cfg_oid *parsed_oid;
    const char *name;
    te_errno rc;

    parsed_oid = cfg_convert_oid_str(oid);
    if (parsed_oid == NULL)
    {
        ERROR("%s(): failed to parse '%s'", __FUNCTION__, oid);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }
    if (parsed_oid->len < 2)
    {
        ERROR("%s(): OID '%s' is too short", __FUNCTION__, oid);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
    name = cfg_oid_inst_subid(parsed_oid, parsed_oid->len - 1);
    if (name == NULL)
        goto finish;
    if (strcmp(name, "mask") == 0)
    {
        *mask = true;

        name = cfg_oid_inst_subid(parsed_oid, parsed_oid->len - 2);
        if (name == NULL)
            goto finish;
    }
    else
    {
        *mask = false;
    }

    rc = te_string_append_chk(field_name, "%s", name);
    if (rc != 0)
        goto finish;

    rc = 0;

finish:

    cfg_free_oid(parsed_oid);

    return rc;
}

/* Print MAC address to string */
static te_errno
print_mac_addr(uint8_t *addr, te_string *str)
{
    return te_string_append_chk(str, TE_PRINTF_MAC_FMT,
                                TE_PRINTF_MAC_VAL(addr));
}

/* Print IP address to string */
static te_errno
print_ip_addr(ta_ethtool_rx_cls_rule *rule, uint8_t *addr, char *value)
{
    const char *result;
    int af;

    switch (rule->flow_type)
    {
        case TCP_V6_FLOW:
        case UDP_V6_FLOW:
        case SCTP_V6_FLOW:
#ifdef AH_V6_FLOW
        case AH_V6_FLOW:
#endif
#ifdef ESP_V6_FLOW
        case ESP_V6_FLOW:
#endif
#ifdef IPV6_USER_FLOW
        case IPV6_USER_FLOW:
#endif
            af = AF_INET6;
            break;

        default:
            af = AF_INET;
    }

    result = inet_ntop(af, addr, value, RCF_MAX_VAL);
    if (result == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* Common method for getting field value and mask */
static te_errno
rule_field_get(unsigned int gid, const char *oid,
               char *value, const char *if_name,
               const char *rules_name, const char *loc_str)
{
    ta_ethtool_rx_cls_rule *rule = NULL;
    ta_ethtool_rx_cls_rule_fields *fields = NULL;
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);
    te_string field_name_str = TE_STRING_INIT_STATIC(CFG_SUBID_MAX);
    const char *field_name = te_string_value(&field_name_str);
    bool mask;
    te_errno rc;

    UNUSED(rules_name);

    rc = get_rule(gid, if_name, loc_str, &rule);
    if (rc != 0)
        return rc;

    rc = rule_field_from_oid(oid, &field_name_str, &mask);
    if (rc != 0)
        return rc;

    if (mask)
        fields = &rule->field_masks;
    else
        fields = &rule->field_values;

#define CHECK_PRINT_UINT(_field) \
    do {                                                                  \
        if (strcmp(field_name, #_field) == 0)                             \
        {                                                                 \
            return te_string_append_chk(&str_val, "%u", fields->_field);  \
        }                                                                 \
    } while (0)

    CHECK_PRINT_UINT(ether_type);
    CHECK_PRINT_UINT(vlan_tpid);
    CHECK_PRINT_UINT(vlan_tci);
    CHECK_PRINT_UINT(data0);
    CHECK_PRINT_UINT(data1);
    CHECK_PRINT_UINT(src_port);
    CHECK_PRINT_UINT(dst_port);
    CHECK_PRINT_UINT(tos_or_tclass);
    CHECK_PRINT_UINT(spi);
    CHECK_PRINT_UINT(l4_4_bytes);
    CHECK_PRINT_UINT(l4_proto);

#undef CHECK_PRINT_UINT

    if (strcmp(field_name, "src_mac") == 0)
    {
        return print_mac_addr(fields->src_mac, &str_val);
    }
    else if (strcmp(field_name, "dst_mac") == 0)
    {
        return print_mac_addr(fields->dst_mac, &str_val);
    }
    else if (strcmp(field_name, "src_l3_addr") == 0)
    {
        return print_ip_addr(rule, fields->src_l3_addr, value);
    }
    else if (strcmp(field_name, "dst_l3_addr") == 0)
    {
        return print_ip_addr(rule, fields->dst_l3_addr, value);
    }

    ERROR("%s(): unknown field '%s'", __FUNCTION__, field_name);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/* Parse MAC address */
static te_errno
parse_mac_addr(const char *value, uint8_t *addr)
{
    int rc;

    rc = sscanf(value, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]);
    if (rc != 6)
    {
        ERROR("%s(): failed to parse MAC address '%s'",
              __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return 0;
}

/* Parse IP address */
static te_errno
parse_ip_addr(const char *value, uint8_t *addr)
{
    int af;
    int rc;

    if (strchr(value, ':') == NULL)
        af = AF_INET;
    else
        af = AF_INET6;

    rc = inet_pton(af, value, addr);
    if (rc < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

/* Common method for setting field value and mask */
static te_errno
rule_field_set(unsigned int gid, const char *oid,
               const char *value, const char *if_name,
               const char *rules_name, const char *loc_str)
{
    ta_ethtool_rx_cls_rule *rule = NULL;
    ta_ethtool_rx_cls_rule_fields *fields = NULL;
    te_string field_name_str = TE_STRING_INIT_STATIC(CFG_SUBID_MAX);
    const char *field_name = te_string_value(&field_name_str);
    bool mask;
    te_errno rc;

    UNUSED(rules_name);

    rc = get_rule(gid, if_name, loc_str, &rule);
    if (rc != 0)
        return rc;

    rc = rule_field_from_oid(oid, &field_name_str, &mask);
    if (rc != 0)
        return rc;

    if (mask)
        fields = &rule->field_masks;
    else
        fields = &rule->field_values;

#define CHECK_PARSE_UINT(_field) \
    do {                                                        \
        if (strcmp(field_name, #_field) == 0)                   \
        {                                                       \
            return te_strtou_size(value, 10, &fields->_field,   \
                                  sizeof(fields->_field));      \
        }                                                       \
    } while (0)

    CHECK_PARSE_UINT(ether_type);
    CHECK_PARSE_UINT(vlan_tpid);
    CHECK_PARSE_UINT(vlan_tci);
    CHECK_PARSE_UINT(data0);
    CHECK_PARSE_UINT(data1);
    CHECK_PARSE_UINT(src_port);
    CHECK_PARSE_UINT(dst_port);
    CHECK_PARSE_UINT(tos_or_tclass);
    CHECK_PARSE_UINT(spi);
    CHECK_PARSE_UINT(l4_4_bytes);
    CHECK_PARSE_UINT(l4_proto);

#undef CHECK_PARSE_UINT

    if (strcmp(field_name, "src_mac") == 0)
    {
       return parse_mac_addr(value, fields->src_mac);
    }
    else if (strcmp(field_name, "dst_mac") == 0)
    {
       return parse_mac_addr(value, fields->dst_mac);
    }
    else if (strcmp(field_name, "src_l3_addr") == 0)
    {
       return parse_ip_addr(value, fields->src_l3_addr);
    }
    else if (strcmp(field_name, "dst_l3_addr") == 0)
    {
       return parse_ip_addr(value, fields->dst_l3_addr);
    }

    ERROR("%s(): unknown field '%s'", __FUNCTION__, field_name);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

static rcf_pch_cfg_object node_rule;

/* Define objects for rule field and its mask */
#define RULE_FIELD(name_) \
{                                                             \
    .sub_id = #name_,                                         \
    .get = (rcf_ch_cfg_get)rule_field_get,                    \
    .set = (rcf_ch_cfg_set)rule_field_set,                    \
    .commit_parent = &node_rule                               \
},                                                            \
{                                                             \
    .sub_id = "mask",                                         \
    .get = (rcf_ch_cfg_get)rule_field_get,                    \
    .set = (rcf_ch_cfg_set)rule_field_set,                    \
    .commit_parent = &node_rule                               \
}

/* Flow specification fields for Rx rule */
static rcf_pch_cfg_object rule_fields[] = {
    RULE_FIELD(src_mac),
    RULE_FIELD(dst_mac),
    RULE_FIELD(ether_type),
    RULE_FIELD(vlan_tpid),
    RULE_FIELD(vlan_tci),
    RULE_FIELD(data0),
    RULE_FIELD(data1),
    RULE_FIELD(src_l3_addr),
    RULE_FIELD(dst_l3_addr),
    RULE_FIELD(src_port),
    RULE_FIELD(dst_port),
    RULE_FIELD(tos_or_tclass),
    RULE_FIELD(spi),
    RULE_FIELD(l4_4_bytes),
    RULE_FIELD(l4_proto),
};

static rcf_pch_cfg_object node_rule_flow_spec = {
    .sub_id = "flow_spec",
    .get = (rcf_ch_cfg_get)rule_flow_spec_get,
    .set = (rcf_ch_cfg_set)rule_flow_spec_set,
    .commit_parent = &node_rule
};

static rcf_pch_cfg_object node_rule_rss_context = {
    .sub_id = "rss_context",
    .brother = &node_rule_flow_spec,
    .get = (rcf_ch_cfg_get)rule_rss_context_get,
    .set = (rcf_ch_cfg_set)rule_rss_context_set,
    .commit_parent = &node_rule
};

static rcf_pch_cfg_object node_rule_rx_queue = {
    .sub_id = "rx_queue",
    .brother = &node_rule_rss_context,
    .get = (rcf_ch_cfg_get)rule_rx_queue_get,
    .set = (rcf_ch_cfg_set)rule_rx_queue_set,
    .commit_parent = &node_rule
};

static rcf_pch_cfg_object node_rule = {
    .sub_id = "rule",
    .son = &node_rule_rx_queue,
    .list = (rcf_ch_cfg_list)rules_list,
    .add = (rcf_ch_cfg_add)rule_add,
    .del = (rcf_ch_cfg_del)rule_del,
    .commit = (rcf_ch_cfg_commit)rule_commit,
};

static rcf_pch_cfg_object node_rules_last_added = {
    .sub_id = "last_added",
    .brother = &node_rule,
    .get = (rcf_ch_cfg_get)rules_last_added_get,
};

static rcf_pch_cfg_object node_rules_spec_loc = {
    .sub_id = "spec_loc",
    .brother = &node_rules_last_added,
    .get = (rcf_ch_cfg_get)rules_spec_loc_get,
};

static rcf_pch_cfg_object node_rules_table_size = {
    .sub_id = "table_size",
    .brother = &node_rules_spec_loc,
    .get = (rcf_ch_cfg_get)rules_table_size_get,
};

static rcf_pch_cfg_object node_rules = {
    .sub_id = "rx_rules",
    .son = &node_rules_table_size,
};

/**
 * Add a child node for Rx classification rules to the interface object.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_rx_rules_init(void)
{
    int i;
    te_errno rc;

    rc = rcf_pch_add_node("/agent/interface/", &node_rules);
    if (rc != 0)
        return rc;

    for (i = TE_ARRAY_LEN(rule_fields) / 2; i > 0; i--)
    {
        rule_fields[i * 2 - 2].son = &rule_fields[i * 2 - 1];
        rc = rcf_pch_add_node("/agent/interface/rx_rules/rule/flow_spec",
                              &rule_fields[i * 2 - 2]);
        if (rc != 0)
            return rc;
    }

    return 0;
}

#else /* ETHTOOL_GRXCLSRLALL is not defined */

/* See description above */
extern te_errno
ta_unix_conf_if_rx_rules_init(void)
{
    WARN("Rx classification rules configuration is not supported");
    return 0;
}

#endif
