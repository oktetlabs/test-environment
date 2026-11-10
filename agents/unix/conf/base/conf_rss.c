/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RSS
 *
 * Unix TA Network Interface Receive Side Scaling settings
 *
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */


#define TE_LGR_USER     "Conf RSS"

#include "te_config.h"
#include "config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "te_str.h"
#include "te_alloc.h"
#include "te_vector.h"
#include "te_enum.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "rcf_pch_tree.h"
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

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H

/* Get position of RSS hash key in answer to ETHTOOL_GRSSH */
#define RSS_HASH_KEY(_rxfh) \
    (uint8_t *)((_rxfh)->rss_config + (_rxfh)->indir_size)

#ifdef ETHTOOL_GRXFH

/** Description of a field that may be included in RSS hash. */
typedef struct rss_flow_hash_field {
    /** Name of the field in the configuration tree. */
    const char *name;
    /** Flag of the field in ETHTOOL_GRXFH/ETHTOOL_SRXFH data. */
    uint64_t flag;
} rss_flow_hash_field;

/** Flow types whose RSS hash fields may be configured. */
static const te_enum_map rss_flow_hash_types[] = {
    { .name = "tcp4", .value = TCP_V4_FLOW },
    { .name = "udp4", .value = UDP_V4_FLOW },
    { .name = "sctp4", .value = SCTP_V4_FLOW },
    { .name = "ah_esp4", .value = AH_ESP_V4_FLOW },
    { .name = "tcp6", .value = TCP_V6_FLOW },
    { .name = "udp6", .value = UDP_V6_FLOW },
    { .name = "sctp6", .value = SCTP_V6_FLOW },
    { .name = "ah_esp6", .value = AH_ESP_V6_FLOW },
    TE_ENUM_MAP_END
};

/**
 * Fields that may be included in RSS hash of a flow type.
 * @c RXH_DISCARD is the sign bit of an int in the kernel header, hence
 * the casts: the other bits of the 64-bit data must stay clear.
 */
static const rss_flow_hash_field rss_flow_hash_fields[] = {
    { .name = "l2da", .flag = (uint32_t)RXH_L2DA },
    { .name = "vlan", .flag = (uint32_t)RXH_VLAN },
    { .name = "l3_proto", .flag = (uint32_t)RXH_L3_PROTO },
    { .name = "ip_src", .flag = (uint32_t)RXH_IP_SRC },
    { .name = "ip_dst", .flag = (uint32_t)RXH_IP_DST },
    { .name = "l4_b_0_1", .flag = (uint32_t)RXH_L4_B_0_1 },
    { .name = "l4_b_2_3", .flag = (uint32_t)RXH_L4_B_2_3 },
#ifdef RXH_GTP_TEID
    { .name = "gtp_teid", .flag = (uint32_t)RXH_GTP_TEID },
#endif
#ifdef RXH_IP6_FL
    { .name = "ip6_fl", .flag = (uint32_t)RXH_IP6_FL },
#endif
    { .name = "discard", .flag = (uint32_t)RXH_DISCARD },
};

#endif /* ETHTOOL_GRXFH */

/* Get number of RX queues */
static te_errno
rx_queues_get(const ta_conf_ctx *ctx, int32_t *val)
{
#ifndef ETHTOOL_GRXRINGS
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_rxnfc rxnfc;
    te_errno rc;

    memset(&rxnfc, 0, sizeof(rxnfc));

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GRXRINGS, &rxnfc);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);

        return rc;
    }

    *val = rxnfc.data;
    return 0;
#endif
}

/* Get RSS hash key */
static te_errno
hash_key_get(const ta_conf_ctx *ctx, te_string *val)
{
#ifndef ETHTOOL_GRSSH
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    ta_ethtool_rxfh *rxfh;
    te_errno rc;

    rc = ta_ethtool_get_rssh(ta_conf_ctx_gid(ctx), if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    return te_str_hex_raw2str(RSS_HASH_KEY(rxfh->rxfh),
                              rxfh->rxfh->key_size, val);
#endif
}

/* Set RSS hash key */
static te_errno
hash_key_set(const ta_conf_ctx *ctx, const char *val)
{
#ifndef ETHTOOL_SRSSH
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    ta_ethtool_rxfh *rxfh;
    te_errno rc;

    rc = ta_ethtool_get_rssh(ta_conf_ctx_gid(ctx), if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    rxfh->hash_key_change = true;
    return te_str_hex_str2raw(val, RSS_HASH_KEY(rxfh->rxfh),
                              rxfh->rxfh->key_size);
#endif
}

/* List of supported hash functions */
static te_errno
hash_func_list(const ta_conf_ctx *ctx, te_vec *names)
{
#if !HAVE_DECL_ETH_SS_RSS_HASH_FUNCS || !defined(ETHTOOL_GRSSH)
    UNUSED(ctx);
    UNUSED(names);

    return 0;
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const struct ta_ethtool_strings *func_names = NULL;
    unsigned int i;
    te_errno rc;

    rc = ta_ethtool_get_strings(ta_conf_ctx_gid(ctx), if_name,
                                ETH_SS_RSS_HASH_FUNCS, &func_names);
    if (rc != 0)
        return rc;

    for (i = 0; i < func_names->num; i++)
    {
        char *name = TE_STRDUP(func_names->strings[i]);

        TE_VEC_APPEND(names, name);
    }

    return 0;
#endif
}

/* Get state of a specific hash function (is it enabled?) */
static te_errno
hash_func_get(const ta_conf_ctx *ctx, bool *val)
{
#ifndef ETHTOOL_GRSSH
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *func_name = ta_conf_ctx_inst(ctx, "hash_func");
    ta_ethtool_rxfh *rxfh;
    const ta_ethtool_strings *func_names;
    unsigned int i;
    te_errno rc;

    rc = ta_ethtool_get_rssh(ta_conf_ctx_gid(ctx), if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_get_strings(ta_conf_ctx_gid(ctx), if_name,
                                ETH_SS_RSS_HASH_FUNCS, &func_names);
    if (rc != 0)
        return rc;

    *val = false;
    for (i = 0; i < func_names->num; i++)
    {
        if (strcmp(func_names->strings[i], func_name) == 0)
        {
            *val = ((rxfh->rxfh->hfunc & (1 << i)) != 0);
            break;
        }
    }

    return 0;
#endif
}

/* Set state of a specific hash function (is it enabled?) */
static te_errno
hash_func_set(const ta_conf_ctx *ctx, bool val)
{
#ifndef ETHTOOL_SRSSH
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *func_name = ta_conf_ctx_inst(ctx, "hash_func");
    ta_ethtool_rxfh *rxfh;
    const ta_ethtool_strings *func_names;
    unsigned int i;
    unsigned int flag = 0;
    te_errno rc;

    rc = ta_ethtool_get_rssh(ta_conf_ctx_gid(ctx), if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_get_strings(ta_conf_ctx_gid(ctx), if_name,
                                ETH_SS_RSS_HASH_FUNCS, &func_names);
    if (rc != 0)
        return rc;

    for (i = 0; i < func_names->num; i++)
    {
        if (strcmp(func_names->strings[i], func_name) == 0)
        {
            flag = (1 << i);
            break;
        }
    }

    if (flag == 0)
    {
        ERROR("%s(): unknown hash function %s", __FUNCTION__, func_name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (val)
        rxfh->rxfh->hfunc |= flag;
    else
        rxfh->rxfh->hfunc &= ~flag;

    return 0;
#endif
}

/* List entries of indirection table */
static te_errno
indir_list(const ta_conf_ctx *ctx, te_vec *names)
{
#ifndef ETHTOOL_GRSSH
    UNUSED(ctx);
    UNUSED(names);

    return 0;
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    ta_ethtool_rxfh *rxfh = NULL;
    unsigned int i;
    te_errno rc;

    rc = ta_ethtool_get_rssh(ta_conf_ctx_gid(ctx), if_name, 0, &rxfh);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP)
            return 0;

        return rc;
    }

    for (i = 0; i < rxfh->rxfh->indir_size; i++)
    {
        char buf[sizeof("4294967295")];
        char *name;

        te_snprintf(buf, sizeof(buf), "%u", i);
        name = TE_STRDUP(buf);
        TE_VEC_APPEND(names, name);
    }

    return 0;
#endif
}

/* Get value of an indirection table entry */
static te_errno
indir_get(const ta_conf_ctx *ctx, int32_t *val)
{
#ifndef ETHTOOL_GRSSH
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *indir_name = ta_conf_ctx_inst(ctx, "indir");
    ta_ethtool_rxfh *rxfh;
    unsigned int idx;
    te_errno rc;

    rc = te_strtoui(indir_name, 0, &idx);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_get_rssh(ta_conf_ctx_gid(ctx), if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    if (idx >= rxfh->rxfh->indir_size)
    {
        ERROR("%s(): too big index '%s'", __FUNCTION__, indir_name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    *val = rxfh->rxfh->rss_config[idx];
    return 0;
#endif
}

/* Set value of an indirection table entry */
static te_errno
indir_set(const ta_conf_ctx *ctx, int32_t val)
{
#ifndef ETHTOOL_SRSSH
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *indir_name = ta_conf_ctx_inst(ctx, "indir");
    ta_ethtool_rxfh *rxfh;
    unsigned int idx;
    te_errno rc;

    rc = te_strtoui(indir_name, 0, &idx);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_get_rssh(ta_conf_ctx_gid(ctx), if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    if (idx >= rxfh->rxfh->indir_size)
    {
        ERROR("%s(): too big index '%s'", __FUNCTION__, indir_name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (val != rxfh->rxfh->rss_config[idx])
    {
        rxfh->rxfh->rss_config[idx] = val;
        rxfh->indir_change = true;
    }

    return 0;
#endif
}

/* Get value of indir_default node */
static te_errno
indir_default_get(const ta_conf_ctx *ctx, bool *val)
{
#ifndef ETHTOOL_SRSSH
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *rss_ctx = ta_conf_ctx_inst(ctx, "context");
    ta_ethtool_rxfh *rxfh;
    unsigned int rss_ctx_id;
    te_errno rc;

    rc = te_strtoui(rss_ctx, 0, &rss_ctx_id);
    if (rc != 0)
        return rc;

    /* Resetting works only for default context */
    if (rss_ctx_id != 0)
        return TE_RC(TE_TAPI, TE_ENOENT);

    rc = ta_ethtool_get_rssh(ta_conf_ctx_gid(ctx), if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    *val = 0;
    return 0;
#endif
}

/* Set value of indir_default node */
static te_errno
indir_default_set(const ta_conf_ctx *ctx, bool val)
{
#ifndef ETHTOOL_SRSSH
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *rss_ctx = ta_conf_ctx_inst(ctx, "context");
    ta_ethtool_rxfh *rxfh;
    unsigned int rss_ctx_id;
    te_errno rc;

    rc = te_strtoui(rss_ctx, 0, &rss_ctx_id);
    if (rc != 0)
        return rc;

    if (rss_ctx_id != 0)
    {
        ERROR("%s(): indirection table can be reset to default only for "
              "default context", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (val == 0)
        return 0;

    rc = ta_ethtool_get_rssh(ta_conf_ctx_gid(ctx), if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    rxfh->indir_reset = true;
    return 0;
#endif
}

/* Commit all changes to hash_indir object (via ETHTOOL_SRSSH) */
static te_errno
hash_indir_commit(const ta_conf_ctx *ctx)
{
#ifndef ETHTOOL_SRSSH
    UNUSED(ctx);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");

    return ta_ethtool_commit_rssh(ta_conf_ctx_gid(ctx), if_name, 0);
#endif
}

/* List known RSS contexts */
static te_errno
rss_ctx_list(const ta_conf_ctx *ctx, te_vec *names)
{
#ifndef ETHTOOL_GRSSH
    UNUSED(ctx);
    UNUSED(names);

    return 0;
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    ta_ethtool_rxfh *rxfh = NULL;
    char *name;
    te_errno rc;

    /*
     * Check whether ETHTOOL_GRSSH is supported.
     */
    rc = ta_ethtool_get_rssh(ta_conf_ctx_gid(ctx), if_name, 0, &rxfh);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP ||
            TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            return 0;
        }

        return rc;
    }

    /* TODO: support not default RSS contexts. */
    name = TE_STRDUP("0");
    TE_VEC_APPEND(names, name);

    return 0;
#endif
}

#ifdef ETHTOOL_GRXFH

/* Get ID of RSS context from the name of its instance */
static te_errno
rss_ctx_id_get(const ta_conf_ctx *ctx, unsigned int *rss_ctx_id)
{
    return te_strtoui(ta_conf_ctx_inst(ctx, "context"), 0, rss_ctx_id);
}

/*
 * Get the cached hash fields of the flow type a request is about,
 * and the flag of the field if the request is about a field.
 */
static te_errno
flow_hash_fields_get(const ta_conf_ctx *ctx, bool for_field,
                     uint64_t **fields, uint64_t *flag)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *flow_name = ta_conf_ctx_inst(ctx, "flow_hash");
    unsigned int rss_ctx_id;
    unsigned int i;
    int flow_type;
    te_errno rc;

    rc = rss_ctx_id_get(ctx, &rss_ctx_id);
    if (rc != 0)
        return rc;

    flow_type = te_enum_map_from_str(rss_flow_hash_types, flow_name, -1);
    if (flow_type < 0)
    {
        ERROR("%s(): unknown flow type '%s'", __FUNCTION__, flow_name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (for_field)
    {
        const char *field_name = ta_conf_ctx_inst(ctx, "field");

        for (i = 0; i < TE_ARRAY_LEN(rss_flow_hash_fields); i++)
        {
            if (strcmp(rss_flow_hash_fields[i].name, field_name) == 0)
                break;
        }

        if (i == TE_ARRAY_LEN(rss_flow_hash_fields))
        {
            ERROR("%s(): unknown hash field '%s'", __FUNCTION__,
                  field_name);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        *flag = rss_flow_hash_fields[i].flag;
    }

    return ta_ethtool_get_rx_flow_hash(ta_conf_ctx_gid(ctx), if_name,
                                       rss_ctx_id, flow_type, fields);
}

#endif /* ETHTOOL_GRXFH */

/* List flow types whose RSS hash fields can be obtained */
static te_errno
flow_hash_list(const ta_conf_ctx *ctx, te_vec *names)
{
#ifndef ETHTOOL_GRXFH
    UNUSED(ctx);
    UNUSED(names);

    return 0;
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const te_enum_map *type;
    unsigned int rss_ctx_id;
    uint64_t *fields;
    te_errno rc;

    rc = rss_ctx_id_get(ctx, &rss_ctx_id);
    if (rc != 0)
        return rc;

    for (type = rss_flow_hash_types; type->name != NULL; type++)
    {
        char *name;

        rc = ta_ethtool_get_rx_flow_hash(ta_conf_ctx_gid(ctx), if_name,
                                         rss_ctx_id, type->value, &fields);
        if (rc != 0)
        {
            /*
             * Drivers answer with one of these errors when they do not
             * support the request at all or for the flow type.
             */
            if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP ||
                TE_RC_GET_ERROR(rc) == TE_EINVAL)
            {
                continue;
            }

            return rc;
        }

        name = TE_STRDUP(type->name);
        TE_VEC_APPEND(names, name);
    }

    return 0;
#endif
}

/* List fields that may be included in RSS hash of a flow type */
static te_errno
flow_hash_field_list(const ta_conf_ctx *ctx, te_vec *names)
{
#ifndef ETHTOOL_GRXFH
    UNUSED(ctx);
    UNUSED(names);

    return 0;
#else
    unsigned int i;

    UNUSED(ctx);

    for (i = 0; i < TE_ARRAY_LEN(rss_flow_hash_fields); i++)
    {
        char *name = TE_STRDUP(rss_flow_hash_fields[i].name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
#endif
}

/* Get state of a field of a flow type (is it included in RSS hash?) */
static te_errno
flow_hash_field_get(const ta_conf_ctx *ctx, bool *val)
{
#ifndef ETHTOOL_GRXFH
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    uint64_t *fields;
    uint64_t flag;
    te_errno rc;

    rc = flow_hash_fields_get(ctx, true, &fields, &flag);
    if (rc != 0)
        return rc;

    *val = ((*fields & flag) != 0);
    return 0;
#endif
}

/* Set state of a field of a flow type (is it included in RSS hash?) */
static te_errno
flow_hash_field_set(const ta_conf_ctx *ctx, bool val)
{
#if !defined(ETHTOOL_GRXFH) || !defined(ETHTOOL_SRXFH)
    UNUSED(ctx);
    UNUSED(val);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    uint64_t *fields;
    uint64_t flag;
    te_errno rc;

    rc = flow_hash_fields_get(ctx, true, &fields, &flag);
    if (rc != 0)
        return rc;

    if (val)
        *fields |= flag;
    else
        *fields &= ~flag;

    return 0;
#endif
}

/* Commit all changes to flow_hash object (via ETHTOOL_SRXFH) */
static te_errno
flow_hash_commit(const ta_conf_ctx *ctx)
{
#if !defined(ETHTOOL_GRXFH) || !defined(ETHTOOL_SRXFH)
    UNUSED(ctx);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *flow_name = ta_conf_ctx_inst(ctx, "flow_hash");
    unsigned int rss_ctx_id;
    int flow_type;
    te_errno rc;

    rc = rss_ctx_id_get(ctx, &rss_ctx_id);
    if (rc != 0)
        return rc;

    flow_type = te_enum_map_from_str(rss_flow_hash_types, flow_name, -1);
    if (flow_type < 0)
    {
        ERROR("%s(): unknown flow type '%s'", __FUNCTION__, flow_name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return ta_ethtool_commit_rx_flow_hash(ta_conf_ctx_gid(ctx), if_name,
                                          rss_ctx_id, flow_type);
#endif
}

/** Common node for RSS settings */
static const ta_conf_node *const node_rss =
    TA_CONF_NA("rss",
        TA_CONF_RO_INT32("rx_queues", rx_queues_get),
        TA_CONF_LIST("context", rss_ctx_list,
            TA_CONF_NA_COMMIT("hash_indir", hash_indir_commit,
                TA_CONF_RW_STR("hash_key", hash_key_get, hash_key_set),
                TA_CONF_RW_COLL_BOOL("hash_func", hash_func_get,
                                     hash_func_set, hash_func_list),
                TA_CONF_RW_COLL_INT32("indir", indir_get, indir_set,
                                     indir_list),
                TA_CONF_RW_BOOL("indir_default", indir_default_get,
                                indir_default_set)),
            TA_CONF_NODE((.name = "flow_hash", .list = flow_hash_list,
                          .commit = flow_hash_commit),
                TA_CONF_RW_COLL_BOOL("field", flow_hash_field_get,
                                     flow_hash_field_set,
                                     flow_hash_field_list))));

/**
 * Add a child node for RSS settings to the interface object.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_rss_init(void)
{
    return ta_conf_register("/agent/interface", node_rss);
}

#else

/* See description above */
extern te_errno
ta_unix_conf_if_rss_init(void)
{
    WARN("RSS settings are not supported");
    return 0;
}
#endif
