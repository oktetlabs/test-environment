/** @file
 * @brief RSS
 *
 * Unix TA Network Interface Receive Side Scaling settings
 *
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */


#define TE_LGR_USER     "Conf RSS"

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

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H

/* Get position of RSS hash key in answer to ETHTOOL_GRSSH */
#define RSS_HASH_KEY(_rxfh) \
    (uint8_t *)((_rxfh)->rss_config + (_rxfh)->indir_size)

/* Get number of RX queues */
static te_errno
rx_queues_get(unsigned int gid, const char *oid,
              char *value, const char *if_name)
{
#ifndef ETHTOOL_GRXRINGS
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(if_name);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);

    struct ethtool_rxnfc rxnfc;
    te_errno rc;

    UNUSED(oid);

    memset(&rxnfc, 0, sizeof(rxnfc));

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GRXRINGS, &rxnfc);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);

        return rc;
    }

    return te_string_append(&str_val, "%u", (unsigned int)(rxnfc.data));
#endif
}

/* Get RSS hash key */
static te_errno
hash_key_get(unsigned int gid, const char *oid,
             char *value, const char *if_name,
             const char *unused, const char *rss_ctx)
{
#ifndef ETHTOOL_GRSSH
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(if_name);
    UNUSED(unused);
    UNUSED(rss_ctx);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    struct ethtool_rxfh *rxfh;
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);
    te_errno rc;

    UNUSED(oid);
    UNUSED(unused);
    UNUSED(rss_ctx);

    rc = ta_ethtool_get_rssh(gid, if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    return te_str_hex_raw2str(RSS_HASH_KEY(rxfh), rxfh->key_size, &str_val);
#endif
}

/* Set RSS hash key */
static te_errno
hash_key_set(unsigned int gid, const char *oid,
             char *value, const char *if_name,
             const char *unused, const char *rss_ctx)
{
#ifndef ETHTOOL_SRSSH
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(if_name);
    UNUSED(unused);
    UNUSED(rss_ctx);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    struct ethtool_rxfh *rxfh;
    te_errno rc;

    UNUSED(oid);
    UNUSED(unused);
    UNUSED(rss_ctx);

    rc = ta_ethtool_get_rssh(gid, if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    return te_str_hex_str2raw(value, RSS_HASH_KEY(rxfh), rxfh->key_size);
#endif
}

/* Get list of supported hash functions */
static te_errno
hash_func_list(unsigned int gid,
               const char *oid,
               const char *sub_id,
               char **list_out,
               const char *if_name)
{
#if !HAVE_DECL_ETH_SS_RSS_HASH_FUNCS || !defined(ETHTOOL_GRSSH)
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(list_out);
    UNUSED(if_name);

    *list_out = NULL;
    return 0;
#else
    const struct ta_ethtool_strings *func_names = NULL;
    unsigned int i;
    te_string str = TE_STRING_INIT;
    te_errno rc;

    rc = ta_ethtool_get_strings(gid, if_name, ETH_SS_RSS_HASH_FUNCS,
                                &func_names);
    if (rc != 0)
        return rc;

    for (i = 0; i < func_names->num; i++)
    {
        rc = te_string_append(&str, "%s ", func_names->strings[i]);
        if (rc != 0)
        {
            te_string_free(&str);
            return rc;
        }
    }

    *list_out = str.ptr;
    return 0;
#endif
}

/* Get state of a specific hash function (is it enabled?) */
static te_errno
hash_func_get(unsigned int gid, const char *oid,
              char *value, const char *if_name,
              const char *unused1, const char *rss_ctx,
              const char *unused2, const char *func_name)
{
#ifndef ETHTOOL_GRSSH
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(if_name);
    UNUSED(unused1);
    UNUSED(rss_ctx);
    UNUSED(unused2);
    UNUSED(func_name);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    struct ethtool_rxfh *rxfh;
    const ta_ethtool_strings *func_names;
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);
    unsigned int i;
    unsigned int result = 0;
    te_errno rc;

    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(rss_ctx);
    UNUSED(unused2);

    rc = ta_ethtool_get_rssh(gid, if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_get_strings(gid, if_name, ETH_SS_RSS_HASH_FUNCS,
                                &func_names);
    if (rc != 0)
        return rc;

    for (i = 0; i < func_names->num; i++)
    {
        if (strcmp(func_names->strings[i], func_name) == 0)
        {
            result = ((rxfh->hfunc & (1 << i)) ? 1 : 0);
            break;
        }
    }

    return te_string_append(&str_val, "%u", result);
#endif
}

/* Set state of a specific hash function (is it enabled?) */
static te_errno
hash_func_set(unsigned int gid, const char *oid,
              char *value, const char *if_name,
              const char *unused1, const char *rss_ctx,
              const char *unused2, const char *func_name)
{
#ifndef ETHTOOL_SRSSH
    unused(gid);
    unused(oid);
    unused(value);
    unused(if_name);
    unused(unused1);
    unused(rss_ctx);
    unused(unused2);
    unused(func_name);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    struct ethtool_rxfh *rxfh;
    const ta_ethtool_strings *func_names;
    unsigned int i;
    unsigned int flag = 0;
    int parsed_val;
    te_errno rc;

    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(rss_ctx);
    UNUSED(unused2);

    rc = te_strtoi(value, 0, &parsed_val);
    if (rc != 0 || (parsed_val != 0 && parsed_val != 1))
    {
        ERROR("%s(): incorrect value '%s'", __FUNCTION__, value);
        return TE_EINVAL;
    }

    rc = ta_ethtool_get_rssh(gid, if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_get_strings(gid, if_name, ETH_SS_RSS_HASH_FUNCS,
                                &func_names);
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

    if (parsed_val == 1)
        rxfh->hfunc |= flag;
    else
        rxfh->hfunc &= ~flag;

    return 0;
#endif
}

/* List entries of indirection table */
static te_errno
indir_list(unsigned int gid, const char *oid,
           const char *sub_id, char **list_out,
           const char *if_name, const char *unused1,
           const char *rss_ctx, const char *unused2)
{
#ifndef ETHTOOL_GRSSH
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(if_name);
    UNUSED(unused1);
    UNUSED(rss_ctx);
    UNUSED(unused2);

    *list_out = NULL;
    return 0;
#else
    struct ethtool_rxfh *rxfh = NULL;
    te_string str = TE_STRING_INIT;
    unsigned int i;
    te_errno rc;

    UNUSED(unused1);
    UNUSED(rss_ctx);
    UNUSED(unused2);

    rc = ta_ethtool_get_rssh(gid, if_name, 0, &rxfh);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP)
        {
            *list_out = NULL;
            return 0;
        }

        return rc;
    }

    for (i = 0; i < rxfh->indir_size; i++)
    {
        rc = te_string_append(&str, "%u ", i);
        if (rc != 0)
        {
            te_string_free(&str);
            return rc;
        }
    }

    *list_out = str.ptr;
    return 0;
#endif
}

/* Get value of an indirection table entry */
static te_errno
indir_get(unsigned int gid, const char *oid,
          char *value, const char *if_name,
          const char *unused1, const char *rss_ctx,
          const char *unused2, const char *indir_name)
{
#ifndef ETHTOOL_GRSSH
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(if_name);
    UNUSED(unused1);
    UNUSED(rss_ctx);
    UNUSED(unused2);
    UNUSED(indir_name);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    te_string str_val = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);
    struct ethtool_rxfh *rxfh;
    unsigned int idx;
    te_errno rc;

    UNUSED(unused1);
    UNUSED(rss_ctx);
    UNUSED(unused2);

    rc = te_strtoui(indir_name, 0, &idx);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_get_rssh(gid, if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    if (idx >= rxfh->indir_size)
    {
        ERROR("%s(): too big index '%s'", __FUNCTION__, indir_name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return te_string_append(&str_val, "%u", rxfh->rss_config[idx]);
#endif
}

/* Set value of an indirection table entry */
static te_errno
indir_set(unsigned int gid, const char *oid,
          char *value, const char *if_name,
          const char *unused1, const char *rss_ctx,
          const char *unused2, const char *indir_name)
{
#ifndef ETHTOOL_SRSSH
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(if_name);
    UNUSED(unused1);
    UNUSED(rss_ctx);
    UNUSED(unused2);
    UNUSED(indir_name);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    struct ethtool_rxfh *rxfh;
    unsigned int idx;
    unsigned int value_int;
    te_errno rc;

    UNUSED(unused1);
    UNUSED(rss_ctx);
    UNUSED(unused2);

    rc = te_strtoui(indir_name, 0, &idx);
    if (rc != 0)
        return rc;

    rc = te_strtoui(value, 0, &value_int);
    if (rc != 0)
        return rc;

    rc = ta_ethtool_get_rssh(gid, if_name, 0, &rxfh);
    if (rc != 0)
        return rc;

    if (idx >= rxfh->indir_size)
    {
        ERROR("%s(): too big index '%s'", __FUNCTION__, indir_name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rxfh->rss_config[idx] = value_int;
    return 0;
#endif
}

/* Commit all changes to hash_indir object (via ETHTOOL_SRSSH) */
static te_errno
hash_indir_commit(unsigned int gid, const cfg_oid *p_oid)
{
#ifndef ETHTOOL_SRSSH
    UNUSED(gid);
    UNUSED(p_oid);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#else
    char *if_name;

    UNUSED(gid);

    if_name = CFG_OID_GET_INST_NAME(p_oid, 2);

    return ta_ethtool_commit_rssh(gid, if_name, 0);
#endif
}

/* List known RSS contexts */
static te_errno
rss_ctx_list(unsigned int gid,
             const char *oid,
             const char *sub_id,
             char **list_out,
             const char *if_name)
{
#ifndef ETHTOOL_GRSSH
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(if_name);

    *list_out = NULL;
    return 0;
#else
    te_string str = TE_STRING_INIT;
    struct ethtool_rxfh *rxfh = NULL;
    te_errno rc;

    UNUSED(oid);
    UNUSED(sub_id);

    /*
     * Check whether ETHTOOL_GRSSH is supported.
     */
    rc = ta_ethtool_get_rssh(gid, if_name, 0, &rxfh);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP ||
            TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            *list_out = NULL;
            return 0;
        }

        return rc;
    }

    /* TODO: support not default RSS contexts. */
    rc = te_string_append(&str, "0");
    if (rc != 0)
        return rc;

    *list_out = str.ptr;
    return 0;
#endif
}


static rcf_pch_cfg_object node_hash_indir;

static rcf_pch_cfg_object node_indir = {
    .sub_id = "indir",
    .get = (rcf_ch_cfg_get)indir_get,
    .set = (rcf_ch_cfg_set)indir_set,
    .list = (rcf_ch_cfg_list)indir_list,
    .commit_parent = &node_hash_indir
};

static rcf_pch_cfg_object node_hash_func = {
    .sub_id = "hash_func",
    .brother = &node_indir,
    .get = (rcf_ch_cfg_get)hash_func_get,
    .set = (rcf_ch_cfg_set)hash_func_set,
    .list = (rcf_ch_cfg_list)hash_func_list,
    .commit_parent = &node_hash_indir
};

static rcf_pch_cfg_object node_hash_key = {
    .sub_id = "hash_key",
    .brother = &node_hash_func,
    .get = (rcf_ch_cfg_get)hash_key_get,
    .set = (rcf_ch_cfg_set)hash_key_set,
    .commit_parent = &node_hash_indir
};

RCF_PCH_CFG_NODE_NA_COMMIT(node_hash_indir, "hash_indir", &node_hash_key,
                           NULL, &hash_indir_commit);

RCF_PCH_CFG_NODE_COLLECTION(node_rss_ctx, "context", &node_hash_indir,
                            NULL, NULL, NULL, &rss_ctx_list, NULL);

RCF_PCH_CFG_NODE_RO(node_rx_queues, "rx_queues", NULL, &node_rss_ctx,
                    &rx_queues_get);

RCF_PCH_CFG_NODE_NA(node_rss, "rss", &node_rx_queues, NULL);

/**
 * Add a child node for RSS settings to the interface object.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_rss_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_rss);
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
