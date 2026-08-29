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
                                indir_default_set))));

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
