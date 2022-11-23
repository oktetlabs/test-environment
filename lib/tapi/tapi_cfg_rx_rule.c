/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Rx rules configuration TAPI
 *
 * Implementation of test API for Network Interface Rx classification
 * rules (doc/cm/cm_rx_rules.xml).
 */

#define TE_LGR_USER "Config Rx Rules"

#include "te_config.h"
#include "te_defs.h"
#include "conf_api.h"
#include "logger_api.h"
#include "te_string.h"
#include "tapi_cfg_rx_rule.h"

/*
 * Convert rule location to string representation used in
 * configuration tree.
 */
static te_errno
rx_rule_loc2str(int64_t location, te_string *str)
{
    if (location < 0)
    {
        const char *loc_str = NULL;

        switch (location)
        {
            case TAPI_CFG_RX_RULE_ANY:
                loc_str = "any";
                break;

            case TAPI_CFG_RX_RULE_FIRST:
                loc_str = "first";
                break;

            case TAPI_CFG_RX_RULE_LAST:
                loc_str = "last";
                break;

            default:
                ERROR("%s(): cannot parse rule location %ld",
                      __FUNCTION__, location);
                return TE_RC(TE_TAPI, TE_EINVAL);
        }

        return te_string_append_chk(str, "%s", loc_str);
    }

    return te_string_append_chk(str, "%ld", location);
}

/* Fill OID of Rx classification rule */
static te_errno
fill_rule_oid(te_string *str, const char *ta, const char *if_name,
              int64_t location)
{
    te_errno rc;

    rc = te_string_append_chk(
                          str,
                          "/agent:%s/interface:%s/rx_rules:/rule:",
                          ta, if_name);
    if (rc != 0)
        return rc;

    return rx_rule_loc2str(location, str);
}

/* Common code of Rx rule accessors */
#define RULE_ACCESSOR_START \
    te_string str_rule_oid = TE_STRING_INIT_STATIC(CFG_OID_MAX);      \
    const char *rule_oid = te_string_value(&str_rule_oid);            \
    te_errno rc;                                                      \
                                                                      \
    rc = fill_rule_oid(&str_rule_oid, ta, if_name, location);         \
    if (rc != 0)                                                      \
        return rc

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_add(const char *ta, const char *if_name,
                     int64_t location, tapi_cfg_rx_rule_flow flow_type)
{
    RULE_ACCESSOR_START;

    rc = cfg_add_instance_local_str(rule_oid, NULL, CVT_NONE, NULL);
    if (rc != 0)
        return rc;

    if (flow_type != TAPI_CFG_RX_RULE_FLOW_UNKNOWN)
    {
        rc = tapi_cfg_rx_rule_flow_type_set(ta, if_name, location,
                                            flow_type);
    }

    return rc;
}

/* See description in tapi_cfg_rx_rule.h */
tapi_cfg_rx_rule_flow
tapi_cfg_rx_rule_flow_by_socket(int af, rpc_socket_type sock_type)
{
    if (af == AF_INET)
    {
        if (sock_type == RPC_SOCK_STREAM)
            return TAPI_CFG_RX_RULE_FLOW_TCP_V4;
        else if (sock_type == RPC_SOCK_DGRAM)
            return TAPI_CFG_RX_RULE_FLOW_UDP_V4;
    }
    else if (af == AF_INET6)
    {
        if (sock_type == RPC_SOCK_STREAM)
            return TAPI_CFG_RX_RULE_FLOW_TCP_V6;
        else if (sock_type == RPC_SOCK_DGRAM)
            return TAPI_CFG_RX_RULE_FLOW_UDP_V6;
    }

    return TAPI_CFG_RX_RULE_FLOW_UNKNOWN;
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_flow_type_set(const char *ta,
                               const char *if_name,
                               int64_t location,
                               tapi_cfg_rx_rule_flow flow_type)
{
    RULE_ACCESSOR_START;

    const char *ft_str;

    switch (flow_type)
    {
        case TAPI_CFG_RX_RULE_FLOW_TCP_V4:
            ft_str = "tcp_v4";
            break;

        case TAPI_CFG_RX_RULE_FLOW_UDP_V4:
            ft_str = "udp_v4";
            break;

        case TAPI_CFG_RX_RULE_FLOW_SCTP_V4:
            ft_str = "sctp_v4";
            break;

        case TAPI_CFG_RX_RULE_FLOW_AH_V4:
            ft_str = "ah_v4";
            break;

        case TAPI_CFG_RX_RULE_FLOW_ESP_V4:
            ft_str = "esp_v4";
            break;

        case TAPI_CFG_RX_RULE_FLOW_IPV4_USER:
            ft_str = "ipv4_user";
            break;

        case TAPI_CFG_RX_RULE_FLOW_TCP_V6:
            ft_str = "tcp_v6";
            break;

        case TAPI_CFG_RX_RULE_FLOW_UDP_V6:
            ft_str = "udp_v6";
            break;

        case TAPI_CFG_RX_RULE_FLOW_SCTP_V6:
            ft_str = "sctp_v6";
            break;

        case TAPI_CFG_RX_RULE_FLOW_AH_V6:
            ft_str = "ah_v6";
            break;

        case TAPI_CFG_RX_RULE_FLOW_ESP_V6:
            ft_str = "esp_v6";
            break;

        case TAPI_CFG_RX_RULE_FLOW_IPV6_USER:
            ft_str = "ipv6_user";
            break;

        case TAPI_CFG_RX_RULE_FLOW_ETHER:
            ft_str = "ether";
            break;

        default:
            ERROR("%s(): unknown flow type %d", __FUNCTION__, flow_type);
            return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return cfg_set_instance_local_fmt(
                        CVT_STRING, ft_str,
                        "%s/flow_spec:", rule_oid);
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_rx_queue_set(const char *ta,
                              const char *if_name,
                              int64_t location,
                              int64_t rxq)
{
    RULE_ACCESSOR_START;

    return cfg_set_instance_local_fmt(
                        CFG_VAL(INT64, rxq),
                        "%s/rx_queue:", rule_oid);
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_rss_context_set(const char *ta,
                                 const char *if_name,
                                 int64_t location,
                                 int64_t context_id)
{
    RULE_ACCESSOR_START;

    return cfg_set_instance_local_fmt(
                        CFG_VAL(INT64, context_id),
                        "%s/rss_context:", rule_oid);
}

/* Set value or mask for a flow specification field */
te_errno
set_field_value_or_mask(const char *ta, const char *if_name,
                        int64_t location, const char *field_name,
                        te_bool mask, cfg_val_type type, const void *value)
{
    te_string str_oid = TE_STRING_INIT_STATIC(CFG_OID_MAX);
    const char *oid = te_string_value(&str_oid);

    RULE_ACCESSOR_START;

    rc = te_string_append_chk(&str_oid, "%s/flow_spec:/%s:%s",
                              rule_oid, field_name,
                              (mask ? "/mask:" : ""));
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);

    return cfg_set_instance_local_fmt(type, value, "%s", oid);
}

/* Get value of a flow specification field */
static te_errno
get_field(const char *ta, const char *if_name, int64_t location,
          const char *field_name, te_bool mask,
          cfg_val_type type, void *value)
{
    char *last_id = (mask ? "/mask:" : "");

    RULE_ACCESSOR_START;

    return cfg_get_instance_fmt(&type, value, "%s/flow_spec:/%s:%s",
                                rule_oid, field_name, last_id);
}

/*
 * Implementation of value/mask setters/getters for flow specification
 * fields.
 */
#define FIELD_ACCESSORS(field_, type_, cfg_type_) \
    te_errno                                                              \
    tapi_cfg_rx_rule_ ## field_ ## _setv(                                 \
                              const char *ta,                             \
                              const char *if_name,                        \
                              int64_t location,                           \
                              const type_ value)                          \
    {                                                                     \
        return set_field_value_or_mask(ta, if_name, location, #field_,    \
                                       FALSE, CFG_VAL(cfg_type_, value)); \
    }                                                                     \
                                                                          \
    te_errno                                                              \
    tapi_cfg_rx_rule_ ## field_ ## _setm(                                 \
                              const char *ta,                             \
                              const char *if_name,                        \
                              int64_t location,                           \
                              const type_ value)                          \
    {                                                                     \
        return set_field_value_or_mask(ta, if_name, location, #field_,    \
                                       TRUE, CFG_VAL(cfg_type_, value));  \
    }                                                                     \
                                                                          \
    te_errno                                                              \
    tapi_cfg_rx_rule_ ## field_ ## _getv(                                 \
                              const char *ta,                             \
                              const char *if_name,                        \
                              int64_t location,                           \
                              type_ *value)                               \
    {                                                                     \
        return get_field(ta, if_name, location, #field_, FALSE,           \
                         CVT_ ## cfg_type_, value);                       \
    }                                                                     \
                                                                          \
    te_errno                                                              \
    tapi_cfg_rx_rule_ ## field_ ## _getm(                                 \
                              const char *ta,                             \
                              const char *if_name,                        \
                              int64_t location,                           \
                              type_ *value)                               \
    {                                                                     \
        return get_field(ta, if_name, location, #field_, TRUE,            \
                         CVT_ ## cfg_type_, value);                       \
    }                                                                     \

FIELD_ACCESSORS(src_mac, struct sockaddr *, ADDRESS)
FIELD_ACCESSORS(dst_mac, struct sockaddr *, ADDRESS)
FIELD_ACCESSORS(ether_type, uint16_t, UINT16)
FIELD_ACCESSORS(vlan_tpid, uint16_t, UINT16)
FIELD_ACCESSORS(vlan_tci, uint16_t, UINT16)
FIELD_ACCESSORS(data0, uint32_t, UINT32)
FIELD_ACCESSORS(data1, uint32_t, UINT32)
FIELD_ACCESSORS(src_l3_addr, struct sockaddr *, ADDRESS)
FIELD_ACCESSORS(dst_l3_addr, struct sockaddr *, ADDRESS)
FIELD_ACCESSORS(src_port, uint16_t, UINT16)
FIELD_ACCESSORS(dst_port, uint16_t, UINT16)
FIELD_ACCESSORS(tos_or_tclass, uint8_t, UINT8)
FIELD_ACCESSORS(spi, uint32_t, UINT32)
FIELD_ACCESSORS(l4_4_bytes, uint32_t, UINT32)
FIELD_ACCESSORS(l4_proto, uint8_t, UINT8)

#undef FIELD_ACCESSORS

/* Fill fields storing address and port */
te_errno
fill_addr_port(const char *ta, const char *if_name, int64_t location,
               int af, const struct sockaddr *addr, te_bool default_zero,
               const char *addr_name, const char *port_name,
               te_bool mask)
{
    struct sockaddr_storage addr_st;
    struct sockaddr *addr_aux = NULL;
    unsigned int port = 0;
    te_errno rc;

    if (addr == NULL)
    {
        memset(&addr_st, 0, sizeof(addr_st));
        addr_aux = SA(&addr_st);

        addr_aux->sa_family = af;

        if (!default_zero)
        {
            uint8_t *p;
            size_t size;
            size_t i;

            port = 0xffff;

            p = (uint8_t *)te_sockaddr_get_netaddr(addr_aux);
            size = te_netaddr_get_size(addr_aux->sa_family);

            for (i = 0; i < size; i++)
                p[i] = 0xff;
        }

        addr = addr_aux;
    }
    else
    {
        port = ntohs(te_sockaddr_get_port(addr));
    }

    rc = set_field_value_or_mask(ta, if_name, location,
                                 port_name, mask,
                                 CFG_VAL(UINT16, port));
    if (rc != 0)
        return rc;

    return set_field_value_or_mask(ta, if_name, location,
                                   addr_name, mask,
                                   CFG_VAL(ADDRESS, addr));
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_fill_ip_addrs_ports(const char *ta, const char *if_name,
                                     int64_t location, int af,
                                     const struct sockaddr *src,
                                     const struct sockaddr *src_mask,
                                     const struct sockaddr *dst,
                                     const struct sockaddr *dst_mask)
{
    te_errno rc;

    rc = fill_addr_port(ta, if_name, location, af, src,
                        TRUE, "src_l3_addr", "src_port", FALSE);
    if (rc != 0)
        return rc;

    rc = fill_addr_port(ta, if_name, location, af, src_mask,
                        src == NULL ? TRUE : FALSE, "src_l3_addr",
                        "src_port", TRUE);
    if (rc != 0)
        return rc;

    rc = fill_addr_port(ta, if_name, location, af, dst,
                        TRUE, "dst_l3_addr", "dst_port", FALSE);
    if (rc != 0)
        return rc;

    return fill_addr_port(ta, if_name, location, af, dst_mask,
                          dst == NULL ? TRUE : FALSE, "dst_l3_addr",
                          "dst_port", TRUE);
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_commit(const char *ta,
                        const char *if_name,
                        int64_t location)
{
    RULE_ACCESSOR_START;

    rc = cfg_commit(rule_oid);
    if (rc == 0 && location < 0)
    {
        /*
         * Special insertion location was used - added rule will appear
         * under its real location after synchronization.
         */
        rc = cfg_synchronize_fmt(
                              TRUE, "/agent:%s/interface:%s/rx_rules:",
                              ta, if_name);
    }

    return rc;
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_get_last_added(const char *ta, const char *if_name,
                                int64_t *location)
{
    return cfg_get_int64(location,
                         "/agent:%s/interface:%s/rx_rules:/last_added:",
                         ta, if_name);
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_del(const char *ta, const char *if_name,
                     int64_t location)
{
    RULE_ACCESSOR_START;

    return cfg_del_instance_fmt(FALSE, "%s", rule_oid);
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_flow_type_get(const char *ta,
                               const char *if_name,
                               int64_t location,
                               tapi_cfg_rx_rule_flow *flow_type)
{
    RULE_ACCESSOR_START;

    char *ft_str = NULL;

    rc = cfg_get_instance_string_fmt(
                        &ft_str, "%s/flow_spec:", rule_oid);
    if (rc != 0)
        return rc;

    if (strcmp(ft_str, "tcp_v4") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_TCP_V4;
    else if (strcmp(ft_str, "udp_v4") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_UDP_V4;
    else if (strcmp(ft_str, "sctp_v4") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_SCTP_V4;
    else if (strcmp(ft_str, "ah_v4") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_AH_V4;
    else if (strcmp(ft_str, "esp_v4") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_ESP_V4;
    else if (strcmp(ft_str, "ipv4_user") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_IPV4_USER;
    else if (strcmp(ft_str, "tcp_v6") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_TCP_V6;
    else if (strcmp(ft_str, "udp_v6") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_UDP_V6;
    else if (strcmp(ft_str, "sctp_v6") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_SCTP_V6;
    else if (strcmp(ft_str, "ah_v6") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_AH_V6;
    else if (strcmp(ft_str, "esp_v6") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_ESP_V6;
    else if (strcmp(ft_str, "ipv6_user") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_IPV6_USER;
    else if (strcmp(ft_str, "ether") == 0)
        *flow_type = TAPI_CFG_RX_RULE_FLOW_ETHER;
    else
        *flow_type = TAPI_CFG_RX_RULE_FLOW_UNKNOWN;

    free(ft_str);

    return 0;
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_rx_queue_get(const char *ta,
                              const char *if_name,
                              int64_t location,
                              int64_t *rxq)
{
    RULE_ACCESSOR_START;

    return cfg_get_int64(rxq, "%s/rx_queue:", rule_oid);
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_rss_context_get(const char *ta,
                                 const char *if_name,
                                 int64_t location,
                                 int64_t *context_id)
{
    RULE_ACCESSOR_START;

    return cfg_get_int64(context_id, "%s/rss_context:", rule_oid);
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_spec_loc_get(const char *ta, const char *if_name,
                              te_bool *value)
{
    return cfg_get_bool(value,
                        "/agent:%s/interface:%s/rx_rules:/spec_loc:",
                        ta, if_name);
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_table_size_get(const char *ta, const char *if_name,
                                uint32_t *size)
{
    return cfg_get_uint32(size,
                          "/agent:%s/interface:%s/rx_rules:/table_size:",
                          ta, if_name);
}

/* See description in tapi_cfg_rx_rule.h */
te_errno
tapi_cfg_rx_rule_find_location(const char *ta, const char *if_name,
                               unsigned int start, unsigned int end,
                               int64_t *location)
{
    te_errno rc;
    uint32_t table_size;
    unsigned int i;
    cfg_handle rule_handle;

    if (end == 0)
    {
        rc = tapi_cfg_rx_rule_table_size_get(ta, if_name, &table_size);
        if (rc != 0)
            return rc;

        end = table_size;
    }

    for (i = start; i < end; i++)
    {
        rc = cfg_find_fmt(&rule_handle,
                          "/agent:%s/interface:%s/rx_rules:/rule:%u",
                          ta, if_name, i);
        if (rc == TE_RC(TE_CS, TE_ENOENT))
        {
            *location = i;
            return 0;
        }
        else if (rc != 0)
        {
            return rc;
        }
    }

    return TE_RC(TE_TAPI, TE_ENOSPC);
}
