/** @file
 * @brief RPC for RTE flow
 *
 * RPC for RTE flow functions
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

#define TE_LGR_USER     "RPC rte_flow"

#include "te_config.h"
#ifdef HAVE_PACKAGE_H
#include "package.h"
#endif

#include "rte_config.h"
#include "rte_flow.h"

#include "asn_usr.h"
#include "asn_impl.h"
#include "ndn_rte_flow.h"

#include "logger_api.h"

#include "rpc_server.h"
#include "rpcs_dpdk.h"
#include "te_errno.h"
#include "te_defs.h"
#include "tad_common.h"

#define RTE_FLOW_BOOL_FILEDS_LEN 1
#define RTE_FLOW_VLAN_VID_FILED_LEN 12
#define RTE_FLOW_VLAN_PCP_FILED_LEN 3
#define RTE_FLOW_VLAN_DEI_FILED_LEN 1

static te_errno
rte_flow_attr_from_ndn(const asn_value *ndn_attr,
                       struct rte_flow_attr **attr_out)
{
    int                     rc;
    struct rte_flow_attr   *attr;

    if (attr_out == NULL)
        return TE_EINVAL;

#define ASN_READ_ATTR_FIELD(_asn_val, _name, _data, _size) \
    do {                                                            \
        size_t __size = _size;                                      \
        unsigned int __val;                                         \
                                                                    \
        rc = asn_read_value_field(_asn_val, &__val,                 \
                                  &__size, #_name);                 \
        if (rc == 0)                                                \
            _data = __val;                                          \
        else if (rc != TE_EASNINCOMPLVAL)                           \
            goto out;                                               \
    } while (0)

    attr = calloc(1, sizeof(*attr));
    if (attr == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    ASN_READ_ATTR_FIELD(ndn_attr, group, attr->group, sizeof(attr->group));
    ASN_READ_ATTR_FIELD(ndn_attr, priority, attr->priority, sizeof(attr->priority));
    ASN_READ_ATTR_FIELD(ndn_attr, ingress, attr->ingress, RTE_FLOW_BOOL_FILEDS_LEN);
    ASN_READ_ATTR_FIELD(ndn_attr, egress, attr->egress, RTE_FLOW_BOOL_FILEDS_LEN);
#undef ASN_READ_ATTR_FIELD

    *attr_out = attr;
    return 0;
out:
    free(attr);
    return rc;
}

static te_errno
rte_int_hton(uint32_t val, void *data, size_t size)
{
    switch (size)
    {
        case sizeof(uint8_t):
            *(uint8_t *)data = val;
            break;

        case sizeof(uint16_t):
            *(uint16_t *)data = rte_cpu_to_be_16(val);
            break;

        case sizeof(uint32_t):
            *(uint32_t *)data = rte_cpu_to_be_32(val);
            break;

        default:
            return TE_EINVAL;
    }

    return 0;
}

#define ASN_READ_INT_RANGE_FIELD(_asn_val, _name, _field, _size) \
    do {                                                                    \
        size_t __size = _size;                                              \
        uint32_t __val;                                                     \
                                                                            \
        rc = asn_read_value_field(_asn_val, &__val,                         \
                                  &__size, #_name ".#plain");               \
        if (rc == 0)                                                        \
        {                                                                   \
            rc = rte_int_hton(__val, &spec->_field, _size);                 \
            if (rc == 0)                                                    \
                rc = rte_int_hton(UINT32_MAX, &mask->_field, _size);        \
        }                                                                   \
        else if (rc == TE_EASNOTHERCHOICE)                                  \
        {                                                                   \
            rc = asn_read_value_field(_asn_val, &__val,                     \
                                      &__size, #_name ".#range.first");     \
            if (rc == 0)                                                    \
                rc = rte_int_hton(__val, &spec->_field, _size);             \
            if (rc == 0 || rc == TE_EASNINCOMPLVAL)                         \
                rc = asn_read_value_field(_asn_val, &__val,                 \
                                          &__size, #_name ".#range.last");  \
            if (rc == 0)                                                    \
                rc = rte_int_hton(__val, &last->_field, _size);             \
            if (rc == 0 || rc == TE_EASNINCOMPLVAL)                         \
                rc = asn_read_value_field(_asn_val, &__val,                 \
                                          &__size, #_name ".#range.mask");  \
            if (rc == 0)                                                    \
                rc = rte_int_hton(__val, &mask->_field, _size);             \
        }                                                                   \
        if (rc != 0 && rc != TE_EASNINCOMPLVAL)                             \
            goto out;                                                       \
    } while (0)

#define ASN_READ_ADDR_RANGE_FIELD(_asn_val, _name, _field, _size) \
    do {                                                                    \
        size_t __size = _size;                                              \
        uint8_t __addr[_size];                                              \
                                                                            \
        rc = asn_read_value_field(_asn_val, __addr, &__size,                \
                                  #_name ".#plain");                        \
        if (rc == 0)                                                        \
        {                                                                   \
            memcpy(spec->_field, __addr, __size);                           \
            memset(mask->_field, 0xff, __size);                             \
        }                                                                   \
        else if (rc == TE_EASNOTHERCHOICE)                                  \
        {                                                                   \
            rc = asn_read_value_field(_asn_val, __addr, &__size,            \
                                      #_name ".#range.first");              \
            if (rc == 0)                                                    \
                memcpy(spec->_field, __addr, __size);                       \
            rc = asn_read_value_field(_asn_val, __addr, &__size,            \
                                      #_name ".#range.last");               \
            if (rc == 0)                                                    \
                memcpy(last->_field, __addr, __size);                       \
            rc = asn_read_value_field(_asn_val, __addr, &__size,            \
                                      #_name ".#range.mask");               \
            if (rc == 0)                                                    \
                memcpy(mask->_field, __addr, __size);                       \
        }                                                                   \
        if (rc != 0 && rc != TE_EASNINCOMPLVAL)                             \
            goto out;                                                       \
    } while (0)

static te_errno
rte_alloc_mem_for_flow_item(void **spec_out, void **mask_out,
                            void **last_out, size_t size)
{
    uint8_t    *spec = NULL;
    uint8_t    *mask = NULL;
    uint8_t    *last = NULL;

    if (spec_out == NULL || mask_out == NULL || last_out == NULL)
        return TE_EINVAL;

    spec = calloc(1, size);
    if (spec == NULL)
        goto out_spec;

    mask = calloc(1, size);
    if (mask == NULL)
        goto out_mask;

    last = calloc(1, size);
    if (last == NULL)
        goto out_last;

    *spec_out = spec;
    *mask_out = mask;
    *last_out = last;

    return 0;

out_last:
    free(mask);
out_mask:
    free(spec);
out_spec:
    return TE_ENOMEM;
}

static te_errno
rte_flow_item_void(__rte_unused const asn_value *void_pdu,
                   struct rte_flow_item *item)
{
    if (item == NULL)
        return TE_EINVAL;

    item->type = RTE_FLOW_ITEM_TYPE_VOID;
    return 0;
}

static te_errno
rte_flow_item_eth_from_pdu(const asn_value *eth_pdu,
                           struct rte_flow_item *item)
{
    struct rte_flow_item_eth *spec = NULL;
    struct rte_flow_item_eth *mask = NULL;
    struct rte_flow_item_eth *last = NULL;
    int rc;

    if (item == NULL)
        return TE_EINVAL;

/*
 * If spec/mask/last is zero then the appropriate
 * item's field remains NULL
 */
#define FILL_FLOW_ITEM_ETH(_field) \
    do {                                            \
        if (!is_zero_ether_addr(&_field->dst) ||    \
            !is_zero_ether_addr(&_field->src) ||    \
            _field->type != 0)                      \
            item->_field = _field;                  \
        else                                        \
            free(_field);                           \
    } while(0)

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_eth));
    if (rc != 0)
        return rc;

    ASN_READ_ADDR_RANGE_FIELD(eth_pdu, src-addr, src.addr_bytes, ETHER_ADDR_LEN);
    ASN_READ_ADDR_RANGE_FIELD(eth_pdu, dst-addr, dst.addr_bytes, ETHER_ADDR_LEN);
    ASN_READ_INT_RANGE_FIELD(eth_pdu, length-type, type, sizeof(spec->type));

    item->type = RTE_FLOW_ITEM_TYPE_ETH;
    FILL_FLOW_ITEM_ETH(spec);
    FILL_FLOW_ITEM_ETH(mask);
    FILL_FLOW_ITEM_ETH(last);
#undef FILL_FLOW_ITEM_ETH

    return 0;
out:
    free(spec);
    free(mask);
    free(last);
    return rc;
}

static te_errno
rte_flow_add_item_to_pattern(struct rte_flow_item **pattern_out,
                             unsigned int *pattern_len_out)
{
    struct rte_flow_item *pattern = *pattern_out;
    unsigned int pattern_len = *pattern_len_out;

    pattern_len++;

    pattern = realloc(pattern, pattern_len * sizeof(*pattern));
    if (pattern == NULL)
        return TE_ENOMEM;

    memset(&pattern[pattern_len - 1], 0, sizeof(*pattern));

    *pattern_len_out = pattern_len;
    *pattern_out = pattern;
    return 0;
}

static te_errno
rte_flow_item_vlan_from_tagged_pdu(asn_value *tagged_pdu,
                                   const char *label,
                                   struct rte_flow_item **pattern_out,
                                   unsigned int *pattern_len_out,
                                   unsigned int *item_nb_out)
{
    struct rte_flow_item_vlan *spec = NULL;
    struct rte_flow_item_vlan *mask = NULL;
    struct rte_flow_item_vlan *last = NULL;
    struct rte_flow_item *pattern;
    unsigned int item_nb = *item_nb_out;
    asn_value *vlan_pdu = tagged_pdu;
    uint16_t spec_tci = 0;
    uint16_t mask_tci = 0;
    uint16_t last_tci = 0;
    te_bool is_empty_outer = FALSE;
    te_bool is_double_tagged = FALSE;
    int rc;

    if (strcmp(label, "outer") == 0 || strcmp(label, "inner") == 0)
    {
        is_double_tagged = TRUE;

        rc = asn_get_subvalue(tagged_pdu, &vlan_pdu, label);
        if (rc == TE_EASNINCOMPLVAL)
        {
            /*
             * If outer or inner are not set, only for
             * outer will be created VLAN item
             */
            if (strcmp(label, "inner") == 0)
                return 0;
            else
                is_empty_outer = TRUE;
        }
        else if (rc != 0)
        {
            goto out;
        }
    }

    rc = rte_flow_add_item_to_pattern(pattern_out, pattern_len_out);
    if (rc != 0)
        goto out;

    item_nb++;
    pattern = *pattern_out;

    pattern[item_nb].type = RTE_FLOW_ITEM_TYPE_VLAN;
    if (is_empty_outer)
        return 0;

#define ASN_READ_VLAN_TCI_RANGE_FIELD(_asn_val, _name, _size, _offset) \
    do {                                                                    \
        size_t __size = _size;                                              \
        uint16_t __val;                                                     \
        uint16_t __offset = _offset;                                        \
        unsigned int i;                                                     \
                                                                            \
        rc = asn_read_value_field(_asn_val, &__val,                         \
                                  &__size, #_name ".#plain");               \
        if (rc == 0)                                                        \
        {                                                                   \
            spec_tci |= __val << __offset;                                  \
            for (i = 0; i < _size; i++)                                     \
                mask_tci |= 1 << (__offset + i);                            \
        }                                                                   \
        else if (rc == TE_EASNOTHERCHOICE)                                  \
        {                                                                   \
            rc = asn_read_value_field(_asn_val, &__val,                     \
                                      &__size, #_name ".#range.first");     \
            if (rc == 0)                                                    \
                spec_tci |= __val << __offset;                              \
            if (rc == 0 || rc == TE_EASNINCOMPLVAL)                         \
                rc = asn_read_value_field(_asn_val, &__val,                 \
                                          &__size, #_name ".#range.last");  \
            if (rc == 0)                                                    \
                last_tci |= __val << __offset;                              \
            if (rc == 0 || rc == TE_EASNINCOMPLVAL)                         \
                rc = asn_read_value_field(_asn_val, &__val,                 \
                                          &__size, #_name ".#range.mask");  \
            if (rc == 0)                                                    \
                mask_tci |= __val << __offset;                              \
        }                                                                   \
        if (rc != 0 && rc != TE_EASNINCOMPLVAL)                             \
            goto out;                                                       \
    } while (0)

    if (is_double_tagged)
    {
        ASN_READ_INT_RANGE_FIELD(vlan_pdu, tpid, tpid, sizeof(spec->tpid));
        ASN_READ_VLAN_TCI_RANGE_FIELD(vlan_pdu, vid,
                                      RTE_FLOW_VLAN_VID_FILED_LEN, 0);
        ASN_READ_VLAN_TCI_RANGE_FIELD(vlan_pdu, dei,
                                      RTE_FLOW_VLAN_DEI_FILED_LEN,
                                      RTE_FLOW_VLAN_VID_FILED_LEN);
        ASN_READ_VLAN_TCI_RANGE_FIELD(vlan_pdu, pcp,
                                      RTE_FLOW_VLAN_PCP_FILED_LEN,
                                      RTE_FLOW_VLAN_VID_FILED_LEN +
                                      RTE_FLOW_VLAN_DEI_FILED_LEN);
    }
    else
    {
        ASN_READ_VLAN_TCI_RANGE_FIELD(vlan_pdu, vlan-id,
                                      RTE_FLOW_VLAN_VID_FILED_LEN, 0);
        ASN_READ_VLAN_TCI_RANGE_FIELD(vlan_pdu, cfi,
                                      RTE_FLOW_VLAN_DEI_FILED_LEN,
                                      RTE_FLOW_VLAN_VID_FILED_LEN);
        ASN_READ_VLAN_TCI_RANGE_FIELD(vlan_pdu, priority,
                                      RTE_FLOW_VLAN_PCP_FILED_LEN,
                                      RTE_FLOW_VLAN_VID_FILED_LEN +
                                      RTE_FLOW_VLAN_DEI_FILED_LEN);
    }
#undef ASN_READ_VLAN_TCI_RANGE_FIELD

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_vlan));
    if (rc != 0)
        return rc;

    spec->tci = rte_cpu_to_be_16(spec_tci);
    mask->tci = rte_cpu_to_be_16(mask_tci);
    last->tci = rte_cpu_to_be_16(last_tci);

#define FILL_FLOW_ITEM_VLAN(_field) \
    do {                                            \
        if (_field->tpid != 0 ||                    \
            _field->tci != 0)                       \
            pattern[item_nb]._field = _field;       \
        else                                        \
            free(_field);                           \
    } while(0)

    FILL_FLOW_ITEM_VLAN(spec);
    FILL_FLOW_ITEM_VLAN(mask);
    FILL_FLOW_ITEM_VLAN(last);
#undef FILL_FLOW_ITEM_VLAN

    *item_nb_out = item_nb;
    *pattern_out = pattern;
    return 0;

out:
    free(spec);
    free(mask);
    free(last);
    return rc;
}

static te_errno
rte_flow_item_vlan_from_eth_pdu(const asn_value *eth_pdu,
                                struct rte_flow_item **pattern_out,
                                unsigned int *pattern_len,
                                unsigned int *item_nb)
{
    asn_value *vlan_pdu;
    int rc = 0;

    if (pattern_out == NULL || pattern_len == NULL ||
        item_nb == NULL || *item_nb >= *pattern_len)
        return TE_EINVAL;

    rc = asn_get_subvalue(eth_pdu, &vlan_pdu, "tagged");
    if (rc ==  TE_EASNINCOMPLVAL)
        return 0;
    else if (rc != 0)
        goto out;

    rc = asn_get_choice_value(vlan_pdu, &vlan_pdu, NULL, NULL);
    if (rc != 0)
        goto out;

    if (strcmp(asn_get_name(vlan_pdu), "tagged") == 0)
    {
        rc = rte_flow_item_vlan_from_tagged_pdu(vlan_pdu, "tagged",
                                                pattern_out, pattern_len, item_nb);
        if (rc != 0)
            goto out;
    }
    else if (strcmp(asn_get_name(vlan_pdu), "double-tagged") == 0)
    {
        /*
         * In flow API there is no difference between outer and inner VLANs,
         * to match the flow rule: the first VLAN is outer and the second is
         * inner.
         * If only the inner is set, the empty outer will be created anyway.
         */
        rc = rte_flow_item_vlan_from_tagged_pdu(vlan_pdu, "outer",
                                                pattern_out, pattern_len, item_nb);
        if (rc != 0)
            goto out;

        rc = rte_flow_item_vlan_from_tagged_pdu(vlan_pdu, "inner",
                                                pattern_out, pattern_len, item_nb);
        if (rc != 0)
            goto out;
    }
    else if (strcmp(asn_get_name(vlan_pdu), "untagged") != 0)
    {
        rc = TE_EINVAL;
    }

out:
    return rc;
}

static te_errno
rte_flow_item_ipv4_from_pdu(const asn_value *ipv4_pdu,
                            struct rte_flow_item *item)
{
    struct rte_flow_item_ipv4 *spec = NULL;
    struct rte_flow_item_ipv4 *mask = NULL;
    struct rte_flow_item_ipv4 *last = NULL;
    int rc;

    if (item == NULL)
        return TE_EINVAL;

#define FILL_FLOW_ITEM_IPV4(_field) \
    do {                                    \
        if (_field->hdr.src_addr != 0 ||    \
            _field->hdr.dst_addr != 0 ||    \
            _field->hdr.next_proto_id != 0) \
            item->_field = _field;          \
        else                                \
            free(_field);                   \
    } while(0)

#define ASN_READ_IPV4_ADDR_RANGE_FIELD(_asn_val, _name, _field) \
    do {                                                                    \
        size_t __size = sizeof(struct in_addr);                             \
                                                                            \
        rc = asn_read_value_field(_asn_val, &spec->_field,                  \
                                  &__size, #_name ".#plain");               \
        if (rc == 0)                                                        \
        {                                                                   \
            mask->_field = UINT32_MAX;                                      \
        }                                                                   \
        else if (rc == TE_EASNOTHERCHOICE)                                  \
        {                                                                   \
            rc = asn_read_value_field(_asn_val, &spec->_field,              \
                                      &__size, #_name ".#range.first");     \
            if (rc == 0 || rc == TE_EASNINCOMPLVAL)                         \
                rc = asn_read_value_field(_asn_val, &last->_field,          \
                                          &__size, #_name ".#range.last");  \
            if (rc == 0 || rc == TE_EASNINCOMPLVAL)                         \
                rc = asn_read_value_field(_asn_val, &mask->_field,          \
                                          &__size, #_name ".#range.mask");  \
        }                                                                   \
        if (rc != 0 && rc != TE_EASNINCOMPLVAL)                             \
            goto out;                                                       \
    } while (0)

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_ipv4));
    if (rc != 0)
        return rc;

    ASN_READ_INT_RANGE_FIELD(ipv4_pdu, protocol, hdr.next_proto_id,
                             sizeof(spec->hdr.next_proto_id));
    ASN_READ_IPV4_ADDR_RANGE_FIELD(ipv4_pdu, src-addr, hdr.src_addr);
    ASN_READ_IPV4_ADDR_RANGE_FIELD(ipv4_pdu, dst-addr, hdr.dst_addr);
#undef ASN_READ_IPV4_ADDR_RANGE_FIELD

    item->type = RTE_FLOW_ITEM_TYPE_IPV4;
    FILL_FLOW_ITEM_IPV4(spec);
    FILL_FLOW_ITEM_IPV4(mask);
    FILL_FLOW_ITEM_IPV4(last);
#undef FILL_FLOW_ITEM_IPV4

    return 0;
out:
    free(spec);
    free(mask);
    free(last);
    return rc;
}

static te_bool
rte_flow_is_zero_addr(const uint8_t *addr, unsigned int size)
{
    uint8_t sum = 0;
    unsigned int i;

    for (i = 0; i < size; i++)
        sum |= addr[i];

    return (sum == 0) ? TRUE : FALSE;
}

static te_errno
rte_flow_item_ipv6_from_pdu(const asn_value *ipv6_pdu,
                            struct rte_flow_item *item)
{
    struct rte_flow_item_ipv6 *spec = NULL;
    struct rte_flow_item_ipv6 *mask = NULL;
    struct rte_flow_item_ipv6 *last = NULL;
    int rc;

    if (item == NULL)
        return TE_EINVAL;

#define FILL_FLOW_ITEM_IPV6(_field) \
    do {                                                                                \
        if (!rte_flow_is_zero_addr(_field->hdr.src_addr, sizeof(struct in6_addr)) ||    \
            !rte_flow_is_zero_addr(_field->hdr.dst_addr, sizeof(struct in6_addr)) ||    \
            _field->hdr.proto != 0)                                                     \
            item->_field = _field;                                                      \
        else                                                                            \
            free(_field);                                                               \
    } while(0)

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_ipv6));
    if (rc != 0)
        return rc;

    ASN_READ_INT_RANGE_FIELD(ipv6_pdu, next-header, hdr.proto, sizeof(spec->hdr.proto));
    ASN_READ_ADDR_RANGE_FIELD(ipv6_pdu, src-addr, hdr.src_addr, sizeof(struct in6_addr));
    ASN_READ_ADDR_RANGE_FIELD(ipv6_pdu, dst-addr, hdr.dst_addr, sizeof(struct in6_addr));

    item->type = RTE_FLOW_ITEM_TYPE_IPV6;
    FILL_FLOW_ITEM_IPV6(spec);
    FILL_FLOW_ITEM_IPV6(mask);
    FILL_FLOW_ITEM_IPV6(last);
#undef FILL_FLOW_ITEM_IPV6

    return 0;
out:
    free(spec);
    free(mask);
    free(last);
    return rc;
}

#define FILL_FLOW_ITEM_TCP_UDP(_field) \
    do {                                    \
        if (_field->hdr.src_port != 0 ||    \
            _field->hdr.dst_port != 0)      \
            item->_field = _field;          \
        else                                \
            free(_field);                   \
    } while(0)

static te_errno
rte_flow_item_tcp_from_pdu(const asn_value *tcp_pdu,
                           struct rte_flow_item *item)
{
    struct rte_flow_item_tcp *spec = NULL;
    struct rte_flow_item_tcp *mask = NULL;
    struct rte_flow_item_tcp *last = NULL;
    int rc;

    if (item == NULL)
        return TE_EINVAL;

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_tcp));
    if (rc != 0)
        return rc;

    ASN_READ_INT_RANGE_FIELD(tcp_pdu, src-port, hdr.src_port, sizeof(spec->hdr.src_port));
    ASN_READ_INT_RANGE_FIELD(tcp_pdu, dst-port, hdr.dst_port, sizeof(spec->hdr.dst_port));

    item->type = RTE_FLOW_ITEM_TYPE_TCP;
    FILL_FLOW_ITEM_TCP_UDP(spec);
    FILL_FLOW_ITEM_TCP_UDP(mask);
    FILL_FLOW_ITEM_TCP_UDP(last);

    return 0;
out:
    free(spec);
    free(mask);
    free(last);
    return rc;
}

static te_errno
rte_flow_item_udp_from_pdu(const asn_value *udp_pdu,
                           struct rte_flow_item *item)
{
    struct rte_flow_item_udp *spec = NULL;
    struct rte_flow_item_udp *mask = NULL;
    struct rte_flow_item_udp *last = NULL;
    int rc;

    if (item == NULL)
        return TE_EINVAL;

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_udp));
    if (rc != 0)
        return rc;

    ASN_READ_INT_RANGE_FIELD(udp_pdu, src-port, hdr.src_port, sizeof(spec->hdr.src_port));
    ASN_READ_INT_RANGE_FIELD(udp_pdu, dst-port, hdr.dst_port, sizeof(spec->hdr.dst_port));

    item->type = RTE_FLOW_ITEM_TYPE_UDP;
    FILL_FLOW_ITEM_TCP_UDP(spec);
    FILL_FLOW_ITEM_TCP_UDP(mask);
    FILL_FLOW_ITEM_TCP_UDP(last);

    return 0;
out:
    free(spec);
    free(mask);
    free(last);
    return rc;
}

static te_errno
rte_flow_item_end(struct rte_flow_item *item)
{
    if (item == NULL)
        return TE_EINVAL;

    item->type = RTE_FLOW_ITEM_TYPE_END;
    return 0;
}

static void
rte_flow_free_pattern(struct rte_flow_item *pattern,
                      unsigned int len)
{
    unsigned int i;

    if (pattern != NULL)
    {
        for (i = 0; i < len; i++)
        {
            free((void *)pattern[i].spec);
            free((void *)pattern[i].mask);
            free((void *)pattern[i].last);
        }
        free(pattern);
    }
}

/*
 * Convert item/action ASN value to rte_flow structures and add
 * to the appropriate list using the map of conversion functions
 */
#define ASN_VAL_CONVERT(_asn_val, _tag, _map, _list, _item_nb) \
    do {                                                            \
        unsigned int i;                                             \
                                                                    \
        for (i = 0; i < TE_ARRAY_LEN(_map); i++)                    \
        {                                                           \
            if (_tag == _map[i].tag)                                \
            {                                                       \
                rc = _map[i].convert(_asn_val, &_list[_item_nb]);   \
                if (rc != 0)                                        \
                    goto out;                                       \
                                                                    \
                break;                                              \
            }                                                       \
        }                                                           \
        if (i == TE_ARRAY_LEN(_map))                                \
        {                                                           \
            rc = TE_EINVAL;                                         \
            goto out;                                               \
        }                                                           \
    } while(0)

/* The mapping list of protocols tags and conversion functions */
static const struct rte_flow_item_tags_mapping {
    asn_tag_value   tag;
    te_errno      (*convert)(const asn_value *item_pdu,
                             struct rte_flow_item *item);
} rte_flow_item_tags_map[] = {
    { TE_PROTO_ETH,     rte_flow_item_eth_from_pdu },
    { TE_PROTO_IP4,     rte_flow_item_ipv4_from_pdu },
    { TE_PROTO_IP6,     rte_flow_item_ipv6_from_pdu },
    { TE_PROTO_TCP,     rte_flow_item_tcp_from_pdu },
    { TE_PROTO_UDP,     rte_flow_item_udp_from_pdu },
    { 0,                rte_flow_item_void },
};

static te_errno
rte_flow_pattern_from_ndn(const asn_value *ndn_flow,
                          struct rte_flow_item **pattern_out,
                          unsigned int *pattern_len)
{
    unsigned int            i;
    int                     rc;
    asn_value              *gen_pdu;
    asn_value              *item_pdu;
    asn_tag_value           item_tag;
    struct rte_flow_item   *pattern = NULL;
    unsigned int            item_nb;
    unsigned int            ndn_len;

    if (pattern_out == NULL || pattern_len == NULL)
        return TE_EINVAL;

    ndn_len = (unsigned int)asn_get_length(ndn_flow, "pattern");
    /*
     * Item END is not specified in the pattern NDN
     * and it should be the last item
     */
    *pattern_len = ndn_len + 1;

    pattern = calloc(*pattern_len, sizeof(*pattern));
    if (pattern == NULL)
        return TE_ENOMEM;

    for (i = 0, item_nb = 0; i < ndn_len; i++, item_nb++)
    {
        rc = asn_get_indexed(ndn_flow, &gen_pdu, i, "pattern");
        if (rc != 0)
            goto out;

        rc = asn_get_choice_value(gen_pdu, &item_pdu, NULL, &item_tag);
        if (rc != 0)
            goto out;

        ASN_VAL_CONVERT(item_pdu, item_tag, rte_flow_item_tags_map,
                        pattern, item_nb);

        if (item_tag == TE_PROTO_ETH)
        {
            rc = rte_flow_item_vlan_from_eth_pdu(item_pdu, &pattern,
                                                 pattern_len, &item_nb);
            if (rc != 0)
                goto out;
        }
    }
    rte_flow_item_end(&pattern[item_nb]);

    *pattern_out = pattern;
    return 0;
out:
    rte_flow_free_pattern(pattern, *pattern_len);
    return rc;
}

static te_errno
rte_flow_action_void(__rte_unused const asn_value *conf_pdu,
                     struct rte_flow_action *action)
{
    if (action == NULL)
        return TE_EINVAL;

    action->type = RTE_FLOW_ACTION_TYPE_VOID;
    return 0;
}

static te_errno
rte_flow_action_queue_from_pdu(const asn_value *conf_pdu,
                               struct rte_flow_action *action)
{
    struct rte_flow_action_queue   *conf = NULL;
    asn_tag_value                   tag;
    size_t                          len;
    int                             rc;
    unsigned int                    val;

    if (action == NULL)
        return TE_EINVAL;

    rc = asn_get_choice_value(conf_pdu, NULL, NULL, &tag);
    if (rc != 0)
        return rc;
    else if (tag != NDN_FLOW_ACTION_QID)
        return TE_EINVAL;

    conf = calloc(1, sizeof(*conf));
    if (conf == NULL)
        return TE_ENOMEM;

    len = sizeof(conf->index);
    rc = asn_read_value_field(conf_pdu, &val, &len, "#index");
    if (rc != 0)
    {
        free(conf);
        return rc;
    }

    conf->index = val;

    action->type = RTE_FLOW_ACTION_TYPE_QUEUE;
    action->conf = conf;

    return 0;
}

static te_errno
rte_flow_action_end(struct rte_flow_action *action)
{
    if (action == NULL)
        return TE_EINVAL;

    action->type = RTE_FLOW_ACTION_TYPE_END;
    return 0;
}

static void
rte_flow_free_actions(struct rte_flow_action *actions,
                      unsigned int len)
{
    unsigned int i;

    if (actions != NULL)
    {
        for (i = 0; i < len; i++)
            free((void *)actions[i].conf);

        free(actions);
    }
}

/* The mapping list of action types and conversion functions */
static const struct rte_flow_action_types_mapping {
    uint8_t         tag;
    te_errno      (*convert)(const asn_value *conf_pdu,
                             struct rte_flow_action *action);
} rte_flow_action_types_map[] = {
    { NDN_FLOW_ACTION_TYPE_QUEUE,   rte_flow_action_queue_from_pdu },
    { NDN_FLOW_ACTION_TYPE_VOID,    rte_flow_action_void },
};

static te_errno
rte_flow_actions_from_ndn(const asn_value *ndn_flow,
                          struct rte_flow_action **actions_out)
{
    unsigned int                i;
    size_t                      size;
    int                         rc;
    asn_value                  *action;
    asn_value                  *conf;
    uint8_t                     type;
    struct rte_flow_action     *actions;
    unsigned int                action_nb;
    unsigned int                actions_len;
    unsigned int                ndn_len;

    if (actions_out == NULL)
        return TE_EINVAL;

    ndn_len = (unsigned int)asn_get_length(ndn_flow, "actions");
    /*
     * Action END is not specified in the actions NDN
     * and it should be the last action
     */
    actions_len = ndn_len + 1;

    actions = calloc(actions_len, sizeof(*actions));
    if (actions == NULL)
        return TE_ENOMEM;

    for (i = 0, action_nb = 0; i < ndn_len; i++, action_nb++)
    {
        rc = asn_get_indexed(ndn_flow, &action, i, "actions");
        if (rc != 0)
            goto out;

        size = sizeof(type);
        rc = asn_read_value_field(action, &type, &size, "type");
        if (rc != 0)
            goto out;

        rc = asn_get_subvalue(action, &conf, "conf");
        if (rc != 0)
            goto out;

        ASN_VAL_CONVERT(conf, type, rte_flow_action_types_map,
                        actions, action_nb);
    }
    rte_flow_action_end(&actions[action_nb]);

    *actions_out = actions;
    return 0;
out:
    rte_flow_free_actions(actions, actions_len);
    return rc;
}

static te_errno
rte_flow_from_ndn(const asn_value *ndn_flow,
                  struct rte_flow_attr **attr_out,
                  struct rte_flow_item **pattern_out,
                  struct rte_flow_action **actions_out)
{
    const asn_type *type;
    asn_value      *ndn_attr;
    unsigned int    pattern_len;
    int             rc;

    type = asn_get_type(ndn_flow);
    if (type != ndn_rte_flow_rule)
    {
        rc = TE_EINVAL;
        goto out;
    }

    rc = asn_get_subvalue(ndn_flow, &ndn_attr, "attr");
    if (rc != 0)
        goto out;

    rc = rte_flow_attr_from_ndn(ndn_attr, attr_out);
    if (rc != 0)
        goto out;

    rc = rte_flow_pattern_from_ndn(ndn_flow, pattern_out, &pattern_len);
    if (rc != 0)
        goto out_pattern;

    rc = rte_flow_actions_from_ndn(ndn_flow, actions_out);
    if (rc != 0)
        goto out_actions;

    return 0;

out_actions:
    rte_flow_free_pattern(*pattern_out, pattern_len);
out_pattern:
    free(*attr_out);
out:
    return rc;
}

static int
tarpc_rte_error_type2tarpc(const enum rte_flow_error_type rte,
                           enum tarpc_rte_flow_error_type *rpc)
{
#define CASE_RTE2TARPC(_rte) \
    case (_rte): *rpc = TARPC_##_rte; break

    switch (rte)
    {
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_NONE);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_UNSPECIFIED);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_HANDLE);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_ATTR_GROUP);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_ATTR_PRIORITY);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_ATTR_INGRESS);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_ATTR_EGRESS);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_ATTR);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_ITEM_NUM);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_ITEM);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_ACTION_NUM);
        CASE_RTE2TARPC(RTE_FLOW_ERROR_TYPE_ACTION);
#undef CASE_RTE2TARPC

        default:
            return -1;
    }
    return 0;
}

static int
tarpc_rte_error2tarpc(struct tarpc_rte_flow_error *tarpc_error,
                      struct rte_flow_error *error)
{
    enum tarpc_rte_flow_error_type  tarpc_error_type;

    if (tarpc_rte_error_type2tarpc(error->type, &tarpc_error_type) != 0)
        return -1;

    tarpc_error->type = tarpc_error_type;

    if (error->message != NULL)
        tarpc_error->message = strdup(error->message);
    else
        tarpc_error->message = strdup("");

    return 0;
}

static void
rte_free_flow_rule(struct rte_flow_attr *attr,
                   struct rte_flow_item *pattern,
                   struct rte_flow_action *actions)
{
    unsigned int i;

    free(attr);

    if (pattern != NULL)
    {
        for (i = 0; pattern[i].type != RTE_FLOW_ITEM_TYPE_END; i++)
        {
            free((void *)pattern[i].spec);
            free((void *)pattern[i].mask);
            free((void *)pattern[i].last);
        }
        free(pattern);
    }

    if (actions != NULL)
    {
        for (i = 0; actions[i].type != RTE_FLOW_ACTION_TYPE_END; i++)
            free((void *)actions[i].conf);
        free(actions);
    }
}

TARPC_FUNC_STATIC(rte_free_flow_rule, {},
{
    struct rte_flow_attr           *attr = NULL;
    struct rte_flow_item           *pattern = NULL;
    struct rte_flow_action         *actions = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_FLOW, {
        attr = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->attr, ns);
        pattern = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->pattern, ns);
        actions = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->actions, ns);
    });

    MAKE_CALL(func(attr, pattern, actions));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_FLOW, {
        RCF_PCH_MEM_INDEX_FREE(in->attr, ns);
        RCF_PCH_MEM_INDEX_FREE(in->pattern, ns);
        RCF_PCH_MEM_INDEX_FREE(in->actions, ns);
    });
})

TARPC_FUNC_STANDALONE(rte_mk_flow_rule_from_str, {},
{
    te_errno                        rc = 0;
    int                             num_symbols_parsed;
    asn_value                      *flow_rule = NULL;
    struct rte_flow_attr           *attr = NULL;
    struct rte_flow_item           *pattern = NULL;
    struct rte_flow_action         *actions = NULL;

    rc = asn_parse_value_text(in->flow_rule, ndn_rte_flow_rule,
                              &flow_rule, &num_symbols_parsed);
    if (rc != 0)
        goto out;

    rc = rte_flow_from_ndn(flow_rule, &attr, &pattern, &actions);
    if (rc != 0)
        goto out;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_FLOW, {
        out->attr = RCF_PCH_MEM_INDEX_ALLOC(attr, ns);
        out->pattern = RCF_PCH_MEM_INDEX_ALLOC(pattern, ns);
        out->actions = RCF_PCH_MEM_INDEX_ALLOC(actions, ns);
        rc = 0;
    });

out:
    out->retval = -TE_RC(TE_RPCS, rc);
    asn_free_value(flow_rule);
})

TARPC_FUNC(rte_flow_validate, {},
{
    struct rte_flow_attr           *attr = NULL;
    struct rte_flow_item           *pattern = NULL;
    struct rte_flow_action         *actions = NULL;
    struct rte_flow_error           error;

    memset(&error, 0, sizeof(error));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_FLOW, {
        attr = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->attr, ns);
        pattern = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->pattern, ns);
        actions = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->actions, ns);
    });

    MAKE_CALL(out->retval = func(in->port_id, attr, pattern, actions, &error));
    neg_errno_h2rpc(&out->retval);

    if (tarpc_rte_error2tarpc(&out->error, &error) != 0)
        out->retval = -TE_RC(TE_RPCS, TE_EINVAL);
})

TARPC_FUNC(rte_flow_create, {},
{
    struct rte_flow_attr           *attr = NULL;
    struct rte_flow_item           *pattern = NULL;
    struct rte_flow_action         *actions = NULL;
    struct rte_flow                *flow = NULL;
    struct rte_flow_error           error;

    memset(&error, 0, sizeof(error));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_FLOW, {
        attr = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->attr, ns);
        pattern = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->pattern, ns);
        actions = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->actions, ns);
    });

    MAKE_CALL(flow = func(in->port_id, attr, pattern, actions, &error));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_FLOW, {
        out->flow = RCF_PCH_MEM_INDEX_ALLOC(flow, ns);
    });

    tarpc_rte_error2tarpc(&out->error, &error);
})

TARPC_FUNC(rte_flow_destroy, {},
{
    struct rte_flow                *flow = NULL;
    struct rte_flow_error           error;

    memset(&error, 0, sizeof(error));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_FLOW, {
        flow = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->flow, ns);
    });

    MAKE_CALL(out->retval = func(in->port_id, flow, &error));
    neg_errno_h2rpc(&out->retval);

    if (out->retval == 0)
    {
        RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_FLOW, {
            RCF_PCH_MEM_INDEX_FREE(in->flow, ns);
        });
    }

    tarpc_rte_error2tarpc(&out->error, &error);
})TARPC_FUNC(rte_flow_flush, {},
{
    struct rte_flow_error error;

    memset(&error, 0, sizeof(error));

    MAKE_CALL(out->retval = func(in->port_id, &error));
    neg_errno_h2rpc(&out->retval);

    tarpc_rte_error2tarpc(&out->error, &error);
})

TARPC_FUNC(rte_flow_isolate, {},
{
    struct rte_flow_error error;

    memset(&error, 0, sizeof(error));

    MAKE_CALL(out->retval = func(in->port_id, in->set, &error));
    neg_errno_h2rpc(&out->retval);

    tarpc_rte_error2tarpc(&out->error, &error);
})
