/** @file
 * @brief RPC for RTE flow
 *
 * RPC for RTE flow functions
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Roman Zhukov <Roman.Zhukov@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC rte_flow"

#include <netinet/in.h>
#include <linux/if_ether.h>

#include "te_config.h"
#include "te_alloc.h"

#include "rte_config.h"
#include "rte_flow.h"
#include "rte_ethdev.h"

#include "asn_usr.h"
#include "asn_impl.h"
#include "ndn_rte_flow.h"
#include "ndn_gre.h"

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
#define RTE_FLOW_INT24_FIELD_LEN 3
#define RTE_FLOW_FIELD_NAME_MAX_LEN 128
#define RTE_FLOW_BIT_FIELD_LEN 1
#define RTE_FLOW_VXLAN_VNI_VALID_OFFSET 3
#define RTE_FLOW_GENEVE_CRITICAL_OFFSET 6
#define RTE_FLOW_GENEVE_OAM_OFFSET 7
#define RTE_FLOW_GENEVE_OPT_LEN_OFFSET 8
#define RTE_FLOW_GENEVE_OPT_LEN_SIZE 6
#define RTE_FLOW_GENEVE_VER_OFFSET 14
#define RTE_FLOW_GENEVE_VER_SIZE 2
#define RTE_FLOW_GRE_CKSUM_OFFSET 15
#define RTE_FLOW_GRE_KEY_OFFSET 13
#define RTE_FLOW_GRE_SEQN_OFFSET 12
#define RTE_FLOW_GRE_VER_LEN 3


static te_errno
rte_flow_attr_from_ndn(const asn_value *ndn_flow,
                       struct rte_flow_attr **attr_out)
{
    int                     rc;
    struct rte_flow_attr   *attr = NULL;
    asn_value              *ndn_attr;
    const char             *attr_label;

    if (attr_out == NULL)
        return TE_EINVAL;

    /* ndn_flow is could be ASN.1 representation of flow rule or attributes */
    attr_label = (asn_get_type(ndn_flow) == ndn_rte_flow_rule) ? "attr" : "";

    rc = asn_get_subvalue(ndn_flow, &ndn_attr, attr_label);
    if (rc != 0)
        goto out;

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
        if (rc != 0 && rc != TE_EASNINCOMPLVAL && rc != TE_EASNOTHERCHOICE) \
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
        if (rc != 0 && rc != TE_EASNINCOMPLVAL && rc != TE_EASNOTHERCHOICE) \
            goto out;                                                       \
    } while (0)

/*
 * Get values of spec, mask and last of requested field with specified name
 * based on size of the value (in bits) and offset of the value (in bits)
 */
static te_errno
asn_read_int_field_with_offset(const asn_value *pdu, const char *name,
                               size_t size, uint32_t offset, uint32_t *spec_p,
                               uint32_t *mask_p, uint32_t *last_p)
{
    uint32_t val;
    uint32_t spec_val = 0;
    uint32_t mask_val = 0;
    uint32_t last_val = 0;
    char buf[RTE_FLOW_FIELD_NAME_MAX_LEN];
    unsigned int i;
    int rc;

    snprintf(buf, RTE_FLOW_FIELD_NAME_MAX_LEN, "%s.#plain", name);
    rc = asn_read_uint32(pdu, &val, buf);
    if (rc == 0)
    {
        spec_val |= val << offset;
        for (i = 0; i < size; i++)
            mask_val |= 1U << (offset + i);
    }
    else if (rc == TE_EASNOTHERCHOICE)
    {
        snprintf(buf, RTE_FLOW_FIELD_NAME_MAX_LEN, "%s.#range.first", name);
        rc = asn_read_uint32(pdu, &val, buf);
        if (rc == 0)
            spec_val |= val << offset;
        if (rc == 0 || rc == TE_EASNINCOMPLVAL)
        {
            snprintf(buf, RTE_FLOW_FIELD_NAME_MAX_LEN, "%s.#range.last", name);
            rc = asn_read_uint32(pdu, &val, buf);
        }
        if (rc == 0)
            last_val |= val << offset;
        if (rc == 0 || rc == TE_EASNINCOMPLVAL)
        {
            snprintf(buf, RTE_FLOW_FIELD_NAME_MAX_LEN, "%s.#range.mask", name);
            rc = asn_read_uint32(pdu, &val, buf);
        }
        if (rc == 0)
            mask_val |= val << offset;
    }
    if (rc != 0 && rc != TE_EASNINCOMPLVAL && rc != TE_EASNOTHERCHOICE)
        return rc;

    *spec_p |= spec_val;
    *mask_p |= mask_val;
    *last_p |= last_val;

    return 0;
}

static void
convert_int24_to_array(uint8_t *array, uint32_t val)
{
    array[0] = val >> (CHAR_BIT << 1);
    array[1] = val >> CHAR_BIT;
    array[2] = val;
}

static te_errno
asn_read_int24_field(const asn_value *pdu, char *name, uint8_t *spec_val,
                     uint8_t *mask_val, uint8_t *last_val)
{
    uint32_t val;
    size_t size = sizeof(val);
    char buf[RTE_FLOW_FIELD_NAME_MAX_LEN];
    int rc;

    snprintf(buf, RTE_FLOW_FIELD_NAME_MAX_LEN, "%s.#plain", name);
    rc = asn_read_value_field(pdu, &val, &size, buf);
    if (rc == 0)
    {
        convert_int24_to_array(spec_val, val);
        memset(mask_val, 0xff, RTE_FLOW_INT24_FIELD_LEN);
    }
    else if (rc == TE_EASNOTHERCHOICE)
    {
        snprintf(buf, RTE_FLOW_FIELD_NAME_MAX_LEN, "%s.#range.first", name);
        rc = asn_read_value_field(pdu, &val, &size, buf);
        if (rc == 0)
            convert_int24_to_array(spec_val, val);
        if (rc == 0 || rc == TE_EASNINCOMPLVAL)
        {
            snprintf(buf, RTE_FLOW_FIELD_NAME_MAX_LEN, "%s.#range.last", name);
            rc = asn_read_value_field(pdu, &val, &size, buf);
        }
        if (rc == 0)
            convert_int24_to_array(last_val, val);
        if (rc == 0 || rc == TE_EASNINCOMPLVAL)
        {
            snprintf(buf, RTE_FLOW_FIELD_NAME_MAX_LEN, "%s.#range.mask", name);
            rc = asn_read_value_field(pdu, &val, &size, buf);
        }
        if (rc == 0)
            convert_int24_to_array(mask_val, val);
    }
    if (rc != 0 && rc != TE_EASNINCOMPLVAL && rc != TE_EASNOTHERCHOICE)
        return rc;

    return 0;
}

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
    uint32_t spec_tci = 0;
    uint32_t mask_tci = 0;
    uint32_t last_tci = 0;
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

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_vlan));
    if (rc != 0)
        return rc;

    if (is_double_tagged)
    {
        rc = asn_read_int_field_with_offset(vlan_pdu, "vid",
                                            RTE_FLOW_VLAN_VID_FILED_LEN, 0,
                                            &spec_tci, &mask_tci, &last_tci);
        if (rc == 0)
            rc = asn_read_int_field_with_offset(vlan_pdu, "dei",
                                                RTE_FLOW_VLAN_DEI_FILED_LEN,
                                                RTE_FLOW_VLAN_VID_FILED_LEN,
                                                &spec_tci, &mask_tci, &last_tci);
        if (rc == 0)
            rc = asn_read_int_field_with_offset(vlan_pdu, "pcp",
                                                RTE_FLOW_VLAN_PCP_FILED_LEN,
                                                RTE_FLOW_VLAN_VID_FILED_LEN +
                                                RTE_FLOW_VLAN_DEI_FILED_LEN,
                                                &spec_tci, &mask_tci, &last_tci);
        if (rc != 0)
            goto out;
    }
    else
    {
        rc = asn_read_int_field_with_offset(vlan_pdu, "vlan-id",
                                            RTE_FLOW_VLAN_VID_FILED_LEN, 0,
                                            &spec_tci, &mask_tci, &last_tci);
        if (rc == 0)
            rc = asn_read_int_field_with_offset(vlan_pdu, "cfi",
                                                RTE_FLOW_VLAN_DEI_FILED_LEN,
                                                RTE_FLOW_VLAN_VID_FILED_LEN,
                                                &spec_tci, &mask_tci, &last_tci);
        if (rc == 0)
            rc = asn_read_int_field_with_offset(vlan_pdu, "priority",
                                                RTE_FLOW_VLAN_PCP_FILED_LEN,
                                                RTE_FLOW_VLAN_VID_FILED_LEN +
                                                RTE_FLOW_VLAN_DEI_FILED_LEN,
                                                &spec_tci, &mask_tci, &last_tci);
        if (rc != 0)
            goto out;
    }

    spec->tci = rte_cpu_to_be_16(spec_tci);
    mask->tci = rte_cpu_to_be_16(mask_tci);
    last->tci = rte_cpu_to_be_16(last_tci);

#ifdef HAVE_STRUCT_RTE_FLOW_ITEM_VLAN_TPID

    if (is_double_tagged)
        ASN_READ_INT_RANGE_FIELD(vlan_pdu, tpid, tpid, sizeof(spec->tpid));

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

#else /* !HAVE_STRUCT_RTE_FLOW_ITEM_VLAN_TPID */

    /*
     * Since the NDN representation of VLAN does not have field for
     * 'inner_type', then in the flow rule pattern move the EtherType
     * from ETH to the last VLAN item
     */
    if (pattern[item_nb - 1].type == RTE_FLOW_ITEM_TYPE_VLAN)
    {
        struct rte_flow_item_vlan *prev_mask =
            (struct rte_flow_item_vlan *)pattern[item_nb - 1].mask;
        struct rte_flow_item_vlan *prev_spec =
            (struct rte_flow_item_vlan *)pattern[item_nb - 1].spec;

        if (prev_mask != NULL && prev_mask->inner_type != 0)
        {
            mask->inner_type = prev_mask->inner_type;
            spec->inner_type = prev_spec->inner_type;
            prev_mask->inner_type = 0;
            prev_spec->inner_type = 0;
        }
    }
    else if (pattern[item_nb - 1].type == RTE_FLOW_ITEM_TYPE_ETH)
    {
        struct rte_flow_item_eth *prev_mask =
            (struct rte_flow_item_eth *)pattern[item_nb - 1].mask;
        struct rte_flow_item_eth *prev_spec =
            (struct rte_flow_item_eth *)pattern[item_nb - 1].spec;

        if (prev_mask != NULL && prev_mask->type != 0)
        {
            mask->inner_type = prev_mask->type;
            spec->inner_type = prev_spec->type;
            prev_mask->type = 0;
            prev_spec->type = 0;
        }
    }
    else
    {
        rc = TE_EINVAL;
        goto out;
    }

#define FILL_FLOW_ITEM_VLAN(_field) \
    do {                                            \
        if (_field->inner_type != 0 ||              \
            _field->tci != 0)                       \
            pattern[item_nb]._field = _field;       \
        else                                        \
            free(_field);                           \
    } while(0)

    FILL_FLOW_ITEM_VLAN(spec);
    FILL_FLOW_ITEM_VLAN(mask);
    FILL_FLOW_ITEM_VLAN(last);
#undef FILL_FLOW_ITEM_VLAN

#endif /* HAVE_STRUCT_RTE_FLOW_ITEM_VLAN_TPID */

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
        if (rc != 0 && rc != TE_EASNINCOMPLVAL && rc != TE_EASNOTHERCHOICE) \
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
rte_flow_item_vxlan_from_pdu(const asn_value *vxlan_pdu,
                             struct rte_flow_item *item)
{
    struct rte_flow_item_vxlan *spec = NULL;
    struct rte_flow_item_vxlan *mask = NULL;
    struct rte_flow_item_vxlan *last = NULL;
    uint32_t spec_vni_valid = 0;
    uint32_t mask_vni_valid = 0;
    uint32_t last_vni_valid = 0;
    int rc;

    if (item == NULL)
        return TE_EINVAL;

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_vxlan));
    if (rc != 0)
        return rc;

    rc = asn_read_int_field_with_offset(vxlan_pdu, "vni-valid",
                                        RTE_FLOW_BIT_FIELD_LEN,
                                        RTE_FLOW_VXLAN_VNI_VALID_OFFSET,
                                        &spec_vni_valid, &mask_vni_valid,
                                        &last_vni_valid);
    if (rc != 0)
        goto out;

    spec->flags = spec_vni_valid;
    mask->flags = mask_vni_valid;
    last->flags = last_vni_valid;

    rc = asn_read_int24_field(vxlan_pdu, "vni", spec->vni, mask->vni, last->vni);
    if (rc != 0)
        goto out;

#define FILL_FLOW_ITEM_VXLAN(_field) \
    do {                                                                        \
        if (!rte_flow_is_zero_addr(_field->vni, RTE_FLOW_INT24_FIELD_LEN) ||    \
            _field->flags != 0)                                                 \
            item->_field = _field;                                              \
        else                                                                    \
            free(_field);                                                       \
    } while(0)

    item->type = RTE_FLOW_ITEM_TYPE_VXLAN;
    FILL_FLOW_ITEM_VXLAN(spec);
    FILL_FLOW_ITEM_VXLAN(mask);
    FILL_FLOW_ITEM_VXLAN(last);
#undef FILL_FLOW_ITEM_VXLAN

    return 0;
out:
    free(spec);
    free(mask);
    free(last);
    return rc;
}

static te_errno
rte_flow_item_geneve_from_pdu(const asn_value *geneve_pdu,
                              struct rte_flow_item *item)
{
#ifdef HAVE_RTE_FLOW_GENEVE
    struct rte_flow_item_geneve *spec = NULL;
    struct rte_flow_item_geneve *mask = NULL;
    struct rte_flow_item_geneve *last = NULL;
    uint32_t spec_fields = 0;
    uint32_t mask_fields = 0;
    uint32_t last_fields = 0;
    int rc;

    if (item == NULL)
        return TE_EINVAL;

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_geneve));
    if (rc != 0)
        return rc;

    rc = asn_read_int_field_with_offset(geneve_pdu, "critical",
                                        RTE_FLOW_BIT_FIELD_LEN,
                                        RTE_FLOW_GENEVE_CRITICAL_OFFSET,
                                        &spec_fields, &mask_fields,
                                        &last_fields);
    if (rc == 0)
        rc = asn_read_int_field_with_offset(geneve_pdu, "oam",
                                            RTE_FLOW_BIT_FIELD_LEN,
                                            RTE_FLOW_GENEVE_OAM_OFFSET,
                                            &spec_fields, &mask_fields,
                                            &last_fields);
    if (rc == 0)
        rc = asn_read_int_field_with_offset(geneve_pdu, "options-length",
                                            RTE_FLOW_GENEVE_OPT_LEN_SIZE,
                                            RTE_FLOW_GENEVE_OPT_LEN_OFFSET,
                                            &spec_fields, &mask_fields,
                                            &last_fields);
    if (rc == 0)
        rc = asn_read_int_field_with_offset(geneve_pdu, "version",
                                            RTE_FLOW_GENEVE_VER_SIZE,
                                            RTE_FLOW_GENEVE_VER_OFFSET,
                                            &spec_fields, &mask_fields,
                                            &last_fields);
    if (rc != 0)
        goto out;

    spec->ver_opt_len_o_c_rsvd0 = rte_cpu_to_be_16(spec_fields);
    mask->ver_opt_len_o_c_rsvd0 = rte_cpu_to_be_16(mask_fields);
    last->ver_opt_len_o_c_rsvd0 = rte_cpu_to_be_16(last_fields);

    ASN_READ_INT_RANGE_FIELD(geneve_pdu, protocol, protocol, sizeof(spec->protocol));
    rc = asn_read_int24_field(geneve_pdu, "vni", spec->vni, mask->vni, last->vni);
    if (rc != 0)
        goto out;

#define FILL_FLOW_ITEM_GENEVE(_field) \
    do {                                                                        \
        if (!rte_flow_is_zero_addr(_field->vni, RTE_FLOW_INT24_FIELD_LEN) ||    \
            _field->protocol != 0 ||                                            \
            _field->ver_opt_len_o_c_rsvd0 != 0)                                 \
            item->_field = _field;                                              \
        else                                                                    \
            free(_field);                                                       \
    } while(0)

    item->type = RTE_FLOW_ITEM_TYPE_GENEVE;
    FILL_FLOW_ITEM_GENEVE(spec);
    FILL_FLOW_ITEM_GENEVE(mask);
    FILL_FLOW_ITEM_GENEVE(last);
#undef FILL_FLOW_ITEM_GENEVE

    return 0;
out:
    free(spec);
    free(mask);
    free(last);
    return rc;
#else /* !HAVE_RTE_FLOW_GENEVE */
    if (item == NULL || geneve_pdu == NULL)
        return TE_EINVAL;

    return TE_EPROTONOSUPPORT;
#endif /* HAVE_RTE_FLOW_GENEVE */
}

static te_errno
rte_flow_item_gre_from_pdu(const asn_value *gre_pdu,
                           struct rte_flow_item *item)
{
    struct rte_flow_item_gre *spec = NULL;
    struct rte_flow_item_gre *mask = NULL;
    struct rte_flow_item_gre *last = NULL;
    uint32_t spec_fields = 0;
    uint32_t mask_fields = 0;
    uint32_t last_fields = 0;
    int rc;

    if (item == NULL)
        return TE_EINVAL;

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_gre));
    if (rc != 0)
        return rc;

    rc = asn_read_int_field_with_offset(gre_pdu, "cksum-present",
                                        RTE_FLOW_BIT_FIELD_LEN,
                                        RTE_FLOW_GRE_CKSUM_OFFSET,
                                        &spec_fields, &mask_fields,
                                        &last_fields);
    if (rc == 0)
        rc = asn_read_int_field_with_offset(gre_pdu, "version",
                                            RTE_FLOW_GRE_VER_LEN, 0,
                                            &spec_fields, &mask_fields,
                                            &last_fields);
    if (rc != 0)
        goto out;

    spec->c_rsvd0_ver = rte_cpu_to_be_16(spec_fields);
    mask->c_rsvd0_ver = rte_cpu_to_be_16(mask_fields);
    last->c_rsvd0_ver = rte_cpu_to_be_16(last_fields);

    ASN_READ_INT_RANGE_FIELD(gre_pdu, protocol, protocol, sizeof(spec->protocol));

#define FILL_FLOW_ITEM_GRE(_field) \
    do {                                    \
        if (_field->c_rsvd0_ver != 0 ||     \
            _field->protocol != 0)          \
            item->_field = _field;          \
        else                                \
            free(_field);                   \
    } while(0)

    item->type = RTE_FLOW_ITEM_TYPE_GRE;
    FILL_FLOW_ITEM_GRE(spec);
    FILL_FLOW_ITEM_GRE(mask);
    FILL_FLOW_ITEM_GRE(last);
#undef FILL_FLOW_ITEM_GRE

    return 0;
out:
    free(spec);
    free(mask);
    free(last);
    return rc;
}

static te_errno
rte_flow_item_nvgre_from_pdu(const asn_value *gre_pdu,
                             asn_value *nvgre_pdu,
                             struct rte_flow_item *item)
{
    struct rte_flow_item_nvgre *spec = NULL;
    struct rte_flow_item_nvgre *mask = NULL;
    struct rte_flow_item_nvgre *last = NULL;
    uint32_t spec_fields = 0;
    uint32_t mask_fields = 0;
    uint32_t last_fields = 0;
    int rc;

    if (item == NULL)
        return TE_EINVAL;

    rc = rte_alloc_mem_for_flow_item((void **)&spec,
                                     (void **)&mask,
                                     (void **)&last,
                                     sizeof(struct rte_flow_item_nvgre));
    if (rc != 0)
        return rc;

    rc = asn_read_int_field_with_offset(gre_pdu, "cksum-present",
                                        RTE_FLOW_BIT_FIELD_LEN,
                                        RTE_FLOW_GRE_CKSUM_OFFSET,
                                        &spec_fields, &mask_fields,
                                        &last_fields);
    if (rc == 0)
        rc = asn_read_int_field_with_offset(gre_pdu, "key-present",
                                            RTE_FLOW_BIT_FIELD_LEN,
                                            RTE_FLOW_GRE_KEY_OFFSET,
                                            &spec_fields, &mask_fields,
                                            &last_fields);
    if (rc == 0)
        rc = asn_read_int_field_with_offset(gre_pdu, "seqn-present",
                                            RTE_FLOW_BIT_FIELD_LEN,
                                            RTE_FLOW_GRE_SEQN_OFFSET,
                                            &spec_fields, &mask_fields,
                                            &last_fields);
    if (rc == 0)
        rc = asn_read_int_field_with_offset(gre_pdu, "version",
                                            RTE_FLOW_GRE_VER_LEN, 0,
                                            &spec_fields, &mask_fields,
                                            &last_fields);
    if (rc != 0)
        goto out;

    spec->c_k_s_rsvd0_ver = rte_cpu_to_be_16(spec_fields);
    mask->c_k_s_rsvd0_ver = rte_cpu_to_be_16(mask_fields);
    last->c_k_s_rsvd0_ver = rte_cpu_to_be_16(last_fields);

    rc = asn_read_int24_field(nvgre_pdu, "vsid", spec->tni, mask->tni, last->tni);
    if (rc != 0)
        goto out;

    ASN_READ_INT_RANGE_FIELD(gre_pdu, protocol, protocol, sizeof(spec->protocol));
    ASN_READ_INT_RANGE_FIELD(nvgre_pdu, flowid, flow_id, sizeof(spec->flow_id));

#define FILL_FLOW_ITEM_NVGRE(_field) \
    do {                                                                        \
        if (_field->c_k_s_rsvd0_ver != 0 ||                                     \
            _field->protocol != 0 ||                                            \
            !rte_flow_is_zero_addr(_field->tni, RTE_FLOW_INT24_FIELD_LEN) ||    \
            _field->flow_id != 0)                                               \
            item->_field = _field;                                              \
        else                                                                    \
            free(_field);                                                       \
    } while(0)

    item->type = RTE_FLOW_ITEM_TYPE_NVGRE;
    FILL_FLOW_ITEM_NVGRE(spec);
    FILL_FLOW_ITEM_NVGRE(mask);
    FILL_FLOW_ITEM_NVGRE(last);
#undef FILL_FLOW_ITEM_NVGRE

    return 0;
out:
    free(spec);
    free(mask);
    free(last);
    return rc;
}

static te_errno
rte_flow_item_gre_and_nvgre_from_pdu(const asn_value *gre_pdu,
                                     struct rte_flow_item *item)
{
    asn_value *opt_key_pdu;
    asn_value *nvgre_pdu;
    asn_tag_value opt_key_tag;
    int rc;

    rc = asn_get_subvalue(gre_pdu, &opt_key_pdu, "opt-key");
    if (rc == 0)
    {
        rc = asn_get_choice_value(opt_key_pdu, &nvgre_pdu, NULL, &opt_key_tag);
        if (rc != 0)
            return rc;

        if (opt_key_tag == NDN_TAG_GRE_OPT_KEY_NVGRE)
        {
            rc = rte_flow_item_nvgre_from_pdu(gre_pdu, nvgre_pdu, item);
            if (rc != 0)
                return rc;
        }
        else
        {
            return TE_EINVAL;
        }
    }
    else if (rc == TE_EASNINCOMPLVAL)
    {
        rc = rte_flow_item_gre_from_pdu(gre_pdu, item);
        if (rc != 0)
            return rc;
    }
    else if (rc != 0)
        return rc;

    return 0;
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

static te_errno
rte_flow_check_item(struct rte_flow_item *item, size_t size)
{
    void *spec = NULL;
    void *mask = NULL;
    void *last = NULL;
    int rc;

    if (item->spec == NULL)
    {
        rc = rte_alloc_mem_for_flow_item((void **)&spec, (void **)&mask,
                                         (void **)&last, size);
        if (rc != 0)
            return rc;

        item->spec = spec;
        item->mask = mask;
        free(last);
    }

    return 0;
}

static te_errno
rte_flow_check_test_items(asn_tag_value tag,
                          struct rte_flow_item *pattern,
                          unsigned int pattern_len)
{
    struct rte_flow_item *eth = NULL;
    struct rte_flow_item_eth *eth_spec;
    struct rte_flow_item_eth *eth_mask;
    struct rte_flow_item *ip4 = NULL;
    struct rte_flow_item_ipv4 *ip4_spec;
    struct rte_flow_item_ipv4 *ip4_mask;
    unsigned int i;
    uint16_t ethertype = 0;
    uint8_t ip_proto = 0;
    int rc;

    for (i = 0; i < pattern_len; i++)
    {
        switch (pattern[i].type)
        {
            case RTE_FLOW_ITEM_TYPE_ETH:
                eth = &pattern[i];
                break;

            case RTE_FLOW_ITEM_TYPE_IPV4:
                ip4 = &pattern[i];
                break;

            case RTE_FLOW_ITEM_TYPE_END:
                break;

            default:
                continue;
        }
    }

    switch (tag)
    {
        case TE_PROTO_ARP:
            rc = rte_flow_check_item(eth, sizeof(struct rte_flow_item_eth));
            if (rc != 0)
                return rc;
            ethertype = rte_cpu_to_be_16(ETHER_TYPE_ARP);
            break;

        case TE_PROTO_PPPOE:
            if (eth == NULL)
                return TE_EINVAL;
            eth_spec = (struct rte_flow_item_eth *)eth->spec;
            if (eth_spec == NULL ||
                (eth_spec->type != rte_cpu_to_be_16(ETH_P_PPP_DISC) &&
                 eth_spec->type != rte_cpu_to_be_16(ETH_P_PPP_SES)))
                return TE_EINVAL;
            break;

        case TE_PROTO_ICMP4:
            rc = rte_flow_check_item(ip4, sizeof(struct rte_flow_item_ipv4));
            if (rc != 0)
                return rc;
            ip_proto = IPPROTO_ICMP;
            break;
    }

    if (ethertype != 0)
    {
        eth_spec = (struct rte_flow_item_eth *)eth->spec;
        eth_mask = (struct rte_flow_item_eth *)eth->mask;

        eth_spec->type = ethertype;
        eth_mask->type = UINT16_MAX;
    }

    if (ip_proto != 0)
    {
        ip4_spec = (struct rte_flow_item_ipv4 *)ip4->spec;
        ip4_mask = (struct rte_flow_item_ipv4 *)ip4->mask;

        ip4_spec->hdr.next_proto_id = ip_proto;
        ip4_mask->hdr.next_proto_id = UINT8_MAX;
    }

    return 0;
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
    { TE_PROTO_VXLAN,   rte_flow_item_vxlan_from_pdu },
    { TE_PROTO_GENEVE,  rte_flow_item_geneve_from_pdu },
    { TE_PROTO_GRE,     rte_flow_item_gre_and_nvgre_from_pdu },
    { TE_PROTO_ICMP4,   rte_flow_item_void },
    { TE_PROTO_ARP,     rte_flow_item_void },
    { TE_PROTO_PPPOE,   rte_flow_item_void },
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
    const char             *pattern_label;

    if (pattern_out == NULL || pattern_len == NULL)
        return TE_EINVAL;

    /* ndn_flow is could be ASN.1 representation of flow rule or pattern */
    pattern_label = (asn_get_type(ndn_flow) == ndn_rte_flow_rule) ?
                    "pattern" : "";

    ndn_len = (unsigned int)asn_get_length(ndn_flow, pattern_label);
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
        rc = asn_get_indexed(ndn_flow, &gen_pdu, i, pattern_label);
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

        rc = rte_flow_check_test_items(item_tag, pattern, item_nb);
        if (rc != 0)
            goto out;
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

    if (action == NULL || conf_pdu == NULL)
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

/* Mapping between ASN.1 representation of RSS HF and RTE flags */
static const struct asn2rte_rss_hf_map {
    asn_tag_value asn_tag;
    uint64_t      rte_flag;
} asn2rte_rss_hf_map[] = {
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV4,
      ETH_RSS_IPV4 },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_FRAG_IPV4,
      ETH_RSS_FRAG_IPV4 },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_TCP,
      ETH_RSS_NONFRAG_IPV4_TCP },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_UDP,
      ETH_RSS_NONFRAG_IPV4_UDP },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_SCTP,
      ETH_RSS_NONFRAG_IPV4_SCTP },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV4_OTHER,
      ETH_RSS_NONFRAG_IPV4_OTHER },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6,
      ETH_RSS_IPV6 },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_TCP,
      ETH_RSS_NONFRAG_IPV6_TCP },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_UDP,
      ETH_RSS_NONFRAG_IPV6_UDP },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_SCTP,
      ETH_RSS_NONFRAG_IPV6_SCTP },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NONFRAG_IPV6_OTHER,
      ETH_RSS_NONFRAG_IPV6_OTHER },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_L2_PAYLOAD,
      ETH_RSS_L2_PAYLOAD },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6_EX,
      ETH_RSS_IPV6_EX },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6_TCP_EX,
      ETH_RSS_IPV6_TCP_EX },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IPV6_UDP_EX,
      ETH_RSS_IPV6_UDP_EX },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_PORT,
      ETH_RSS_PORT },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_VXLAN,
      ETH_RSS_VXLAN },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_GENEVE,
      ETH_RSS_GENEVE },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_NVGRE,
      ETH_RSS_NVGRE },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_IP,
      ETH_RSS_IP },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_TCP,
      ETH_RSS_TCP },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_UDP,
      ETH_RSS_UDP },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_SCTP,
      ETH_RSS_SCTP },
    { NDN_FLOW_ACTION_CONF_RSS_OPT_HF_TUNNEL,
      ETH_RSS_TUNNEL },
};

static te_errno
rte_flow_action_rss_opt_hf_from_pdu(const asn_value *rss_conf,
                                   uint64_t        *rss_hf_rte_out)
{
    const asn_value *rss_hf;
    uint64_t         rss_hf_rte = 0;
    te_errno         rc;
    unsigned int     i;

    rc = asn_get_subvalue(rss_conf, (asn_value **)&rss_hf, "rss-hf");
    if (rc == TE_EASNINCOMPLVAL)
        return 0;
    else if (rc != 0)
        return rc;

    for (i = 0; i < TE_ARRAY_LEN(asn2rte_rss_hf_map); ++i)
    {
        const asn_value *hf;

        rc = asn_get_child_value(rss_hf, &hf,
                                 PRIVATE, asn2rte_rss_hf_map[i].asn_tag);
        if (rc == 0)
            rss_hf_rte |= asn2rte_rss_hf_map[i].rte_flag;
        else if (rc != TE_EASNINCOMPLVAL)
            return rc;
    }

    *rss_hf_rte_out = rss_hf_rte;

    return 0;
}

static te_errno
rte_flow_action_rss_opt_from_pdu(const asn_value            *conf_pdu_choice,
                                struct rte_flow_action_rss *conf)
{
    asn_value               *rss_conf;
    int                      rss_key_len;
    struct rte_eth_rss_conf *opt = NULL;
    size_t                   d_len;
    te_errno                 rc;

    rc = asn_get_subvalue(conf_pdu_choice, &rss_conf, "rss-conf");
    if (rc == TE_EASNINCOMPLVAL)
        return 0;
    else if (rc != 0)
        return rc;

    opt = TE_ALLOC(sizeof(*opt));
    if (opt == NULL)
        return TE_ENOMEM;

    rss_key_len = asn_get_length(rss_conf, "rss-key");
    if (rss_key_len > 0)
    {
        uint8_t *rss_key = NULL;

        rss_key = TE_ALLOC(rss_key_len);
        if (rss_key == NULL)
        {
            rc = TE_ENOMEM;
            goto fail;
        }

        d_len = rss_key_len;
        rc = asn_read_value_field(rss_conf, rss_key, &d_len, "rss-key");
        if (rc != 0)
        {
            free(rss_key);
            goto fail;
        }

        opt->rss_key = rss_key;
        opt->rss_key_len = rss_key_len;
    }

    rc = rte_flow_action_rss_opt_hf_from_pdu(rss_conf, &opt->rss_hf);
    if (rc != 0)
        goto fail;

#ifdef HAVE_STRUCT_RTE_FLOW_ACTION_RSS_RSS_CONF
    conf->rss_conf = opt;
#else /* !HAVE_STRUCT_RTE_FLOW_ACTION_RSS_RSS_CONF */
    conf->types = opt->rss_hf;

    if (rss_key_len > 0)
    {
        uint8_t *rss_key = NULL;

        rss_key = TE_ALLOC(rss_key_len);
        if (rss_key == NULL)
        {
            rc = TE_ENOMEM;
            goto fail;
        }

        memcpy(rss_key, opt->rss_key, rss_key_len);

        conf->key_len = rss_key_len;
        conf->key = rss_key;
    }

    free(opt);

#endif /* HAVE_STRUCT_RTE_FLOW_ACTION_RSS_RSS_CONF */

    return 0;

fail:
    free(opt);

    return rc;
}

static te_errno
rte_flow_action_rss_from_pdu(const asn_value        *conf_pdu,
                            struct rte_flow_action *action)
{
    struct rte_flow_action_rss *conf = NULL;
    uint16_t                   *queue = NULL;
    asn_value                  *conf_pdu_choice;
    asn_tag_value               conf_pdu_choice_tag;
    asn_value                  *queue_list;
    int                         nb_entries;
    te_errno                    rc;
    int                         i;

    if (action == NULL || conf_pdu == NULL)
        return TE_EINVAL;

    rc = asn_get_choice_value(conf_pdu, &conf_pdu_choice,
                              NULL, &conf_pdu_choice_tag);
    if (rc != 0)
        return rc;
    else if (conf_pdu_choice_tag != NDN_FLOW_ACTION_CONF_RSS)
        return TE_EINVAL;

    rc = asn_get_subvalue(conf_pdu_choice, &queue_list, "queue");
    if (rc == TE_EASNINCOMPLVAL)
    {
        nb_entries = 0;
    }
    else if (rc != 0)
    {
        return rc;
    }
    else
    {
        nb_entries = asn_get_length(queue_list, "");
        if (nb_entries < 0)
            return TE_EINVAL;
    }

    conf = TE_ALLOC(sizeof(*conf));
    if (conf == NULL)
        return TE_ENOMEM;

    queue = TE_ALLOC(nb_entries * sizeof(*queue));
    if (queue == NULL)
        goto fail_alloc_queue;

    for (i = 0; i < nb_entries; ++i)
    {
        asn_value *queue_index;
        size_t     d_len = sizeof(*queue);

        rc = asn_get_indexed(queue_list, &queue_index, i, "");
        if (rc != 0)
            goto fail;

        rc = asn_read_value_field(queue_index, &queue[i], &d_len, "");
        if (rc != 0)
            goto fail;
    }

#ifdef HAVE_STRUCT_RTE_FLOW_ACTION_RSS_NUM
    conf->num = nb_entries;
    conf = realloc(conf, sizeof(*conf) + (nb_entries * sizeof(conf->queue[0])));
    if (conf == NULL)
    {
        rc = TE_ENOMEM;
        goto fail;
    }

    memcpy(conf->queue, queue, nb_entries * sizeof(conf->queue[0]));
    free(queue);
#else /* !HAVE_STRUCT_RTE_FLOW_ACTION_RSS_NUM */
    conf->queue_num = nb_entries;
    conf->queue = queue;
#endif /* HAVE_STRUCT_RTE_FLOW_ACTION_RSS_NUM */

    rc = rte_flow_action_rss_opt_from_pdu(conf_pdu_choice, conf);
    if (rc != 0)
        goto fail;

    action->type = RTE_FLOW_ACTION_TYPE_RSS;
    action->conf = conf;

    return 0;

fail:
    free(queue);
fail_alloc_queue:
    free(conf);

    return rc;
}

static te_errno
rte_flow_action_drop_from_pdu(__rte_unused const asn_value *conf_pdu,
                              struct rte_flow_action *action)
{
    if (action == NULL)
        return TE_EINVAL;

    action->type = RTE_FLOW_ACTION_TYPE_DROP;
    return 0;
}

static te_errno
rte_flow_action_flag_from_pdu(__rte_unused const asn_value *conf_pdu,
                              struct rte_flow_action *action)
{
    if (action == NULL)
        return TE_EINVAL;

    action->type = RTE_FLOW_ACTION_TYPE_FLAG;
    return 0;
}

static te_errno
rte_flow_action_mark_from_pdu(const asn_value *conf_pdu,
                              struct rte_flow_action *action)
{
    struct rte_flow_action_mark    *conf = NULL;
    asn_tag_value                   tag;
    size_t                          len;
    int                             rc;
    uint32_t                        val;

    if (action == NULL)
        return TE_EINVAL;

    action->type = RTE_FLOW_ACTION_TYPE_MARK;

    if (conf_pdu == NULL)
        return 0;

    rc = asn_get_choice_value(conf_pdu, NULL, NULL, &tag);
    if (rc != 0)
        return rc;
    else if (tag != NDN_FLOW_ACTION_MARK_ID)
        return TE_EINVAL;

    conf = calloc(1, sizeof(*conf));
    if (conf == NULL)
        return TE_ENOMEM;

    len = sizeof(conf->id);
    rc = asn_read_value_field(conf_pdu, &val, &len, "#id");
    if (rc != 0)
    {
        free(conf);
        return rc;
    }

    conf->id = val;
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
    { NDN_FLOW_ACTION_TYPE_RSS,     rte_flow_action_rss_from_pdu },
    { NDN_FLOW_ACTION_TYPE_QUEUE,   rte_flow_action_queue_from_pdu },
    { NDN_FLOW_ACTION_TYPE_VOID,    rte_flow_action_void },
    { NDN_FLOW_ACTION_TYPE_DROP,    rte_flow_action_drop_from_pdu },
    { NDN_FLOW_ACTION_TYPE_FLAG,    rte_flow_action_flag_from_pdu },
    { NDN_FLOW_ACTION_TYPE_MARK,    rte_flow_action_mark_from_pdu },
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
    const char                 *actions_label;

    if (actions_out == NULL)
        return TE_EINVAL;

    /* ndn_flow is could be ASN.1 representation of flow rule or actions */
    actions_label = (asn_get_type(ndn_flow) == ndn_rte_flow_rule) ?
                    "actions" : "";

    ndn_len = (unsigned int)asn_get_length(ndn_flow, actions_label);
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
        rc = asn_get_indexed(ndn_flow, &action, i, actions_label);
        if (rc != 0)
            goto out;

        size = sizeof(type);
        rc = asn_read_value_field(action, &type, &size, "type");
        if (rc != 0)
            goto out;

        rc = asn_get_subvalue(action, &conf, "conf");
        if (rc == TE_EASNINCOMPLVAL)
            conf = NULL;
        else if (rc != 0)
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
rte_flow_components_from_ndn(const asn_value *ndn_flow_components,
                             uint8_t component_flags,
                             struct rte_flow_attr **attr_out,
                             struct rte_flow_item **pattern_out,
                             struct rte_flow_action **actions_out)
{
    unsigned int    pattern_len;
    int             rc;

    if (component_flags & TARPC_RTE_FLOW_ATTR_FLAG)
    {
        rc = rte_flow_attr_from_ndn(ndn_flow_components, attr_out);
        if (rc != 0)
            goto out;
    }

    if (component_flags & TARPC_RTE_FLOW_PATTERN_FLAG)
    {
        rc = rte_flow_pattern_from_ndn(ndn_flow_components, pattern_out,
                                       &pattern_len);
        if (rc != 0)
            goto out_pattern;
    }

    if (component_flags & TARPC_RTE_FLOW_ACTIONS_FLAG)
    {
        rc = rte_flow_actions_from_ndn(ndn_flow_components, actions_out);
        if (rc != 0)
            goto out_actions;
    }

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

static const asn_type *
tarpc_rte_flow_rule_component_flags2type(uint8_t component_flags)
{
    switch (component_flags)
    {
        case TARPC_RTE_FLOW_ATTR_FLAG:
            return ndn_rte_flow_attr;

        case TARPC_RTE_FLOW_PATTERN_FLAG:
            return ndn_rte_flow_pattern;

        case TARPC_RTE_FLOW_ACTIONS_FLAG:
            return ndn_rte_flow_actions;

        case TARPC_RTE_FLOW_RULE_FLAGS:
            return ndn_rte_flow_rule;

        default:
            return NULL;
    }
}

TARPC_FUNC_STANDALONE(rte_mk_flow_rule_components, {},
{
    te_errno                        rc = 0;
    int                             num_symbols_parsed;
    asn_value                      *flow_rule_components = NULL;
    struct rte_flow_attr           *attr = NULL;
    struct rte_flow_item           *pattern = NULL;
    struct rte_flow_action         *actions = NULL;
    const asn_type                 *type;

    type = tarpc_rte_flow_rule_component_flags2type(in->component_flags);
    if (type == NULL)
    {
        rc = TE_EINVAL;
        goto out;
    }

    rc = asn_parse_value_text(in->flow_rule_components, type,
                              &flow_rule_components, &num_symbols_parsed);
    if (rc != 0)
        goto out;

    rc = rte_flow_components_from_ndn(flow_rule_components, in->component_flags,
                                      &attr, &pattern, &actions);
    if (rc != 0)
        goto out;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_FLOW, {
        if (attr != NULL)
            out->attr = RCF_PCH_MEM_INDEX_ALLOC(attr, ns);
        if (pattern != NULL)
            out->pattern = RCF_PCH_MEM_INDEX_ALLOC(pattern, ns);
        if (actions != NULL)
            out->actions = RCF_PCH_MEM_INDEX_ALLOC(actions, ns);
        rc = 0;
    });

out:
    out->retval = -TE_RC(TE_RPCS, rc);
    asn_free_value(flow_rule_components);
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

TARPC_FUNC_STANDALONE(rte_flow_isolate, {},
{
    struct rte_flow_error error;

    memset(&error, 0, sizeof(error));

#ifdef HAVE_STRUCT_RTE_FLOW_OPS_ISOLATE
    MAKE_CALL(out->retval = rte_flow_isolate(in->port_id, in->set, &error));
#else /* !HAVE_STRUCT_RTE_FLOW_OPS_ISOLATE */
    out->retval = -ENOTSUP;
#endif /* HAVE_STRUCT_RTE_FLOW_OPS_ISOLATE */

    neg_errno_h2rpc(&out->retval);
    tarpc_rte_error2tarpc(&out->error, &error);
})
