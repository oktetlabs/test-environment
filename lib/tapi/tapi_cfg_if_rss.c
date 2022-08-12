/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for Network Interface RSS settings
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_LGR_USER "Config RSS"

#include "te_config.h"
#include "te_defs.h"
#include "conf_api.h"
#include "tapi_cfg_if_rss.h"
#include "te_alloc.h"
#include "te_str.h"

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_rx_queues_get(const char *ta,
                              const char *if_name,
                              int *rx_queues)
{
    return cfg_get_instance_int_fmt(
                  rx_queues, "/agent:%s/interface:%s/rss:/rx_queues:",
                  ta, if_name);
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_hash_key_get(const char *ta,
                             const char *if_name,
                             unsigned int rss_context,
                             uint8_t **buf, size_t *len)
{
    char *val = NULL;
    uint8_t *result = NULL;
    size_t req_len;
    te_errno rc;

    rc = cfg_get_instance_string_fmt(
            &val,
            "/agent:%s/interface:%s/rss:/context:%u/hash_indir:/hash_key:",
            ta, if_name, rss_context);
    if (rc != 0)
        return rc;

    req_len = (strlen(val) + 1) / 3;
    result = TE_ALLOC(req_len);
    if (result == NULL)
    {
        free(val);
        return TE_ENOMEM;
    }

    rc = te_str_hex_str2raw(val, result, req_len);
    free(val);

    if (rc == 0)
    {
        *buf = result;
        *len = req_len;
    }
    else
    {
        free(buf);
    }

    return rc;
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_hash_key_set_local(
                             const char *ta,
                             const char *if_name,
                             unsigned int rss_context,
                             const uint8_t *buf, size_t len)
{
    te_string str = TE_STRING_INIT;
    te_errno rc;

    rc = te_str_hex_raw2str(buf, len, &str);
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_local_fmt(
            CFG_VAL(STRING, te_string_value(&str)),
            "/agent:%s/interface:%s/rss:/context:%u/hash_indir:/hash_key:",
            ta, if_name, rss_context);
    te_string_free(&str);
    return rc;
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_indir_table_size(const char *ta,
                                 const char *if_name,
                                 unsigned int rss_context,
                                 unsigned int *size)
{
    cfg_handle *set = NULL;
    te_errno rc;

    rc = cfg_find_pattern_fmt(
            size, &set,
            "/agent:%s/interface:%s/rss:/context:%u/hash_indir:/indir:*",
            ta, if_name, rss_context);
    free(set);
    return rc;
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_indir_get(const char *ta,
                          const char *if_name,
                          unsigned int rss_context,
                          unsigned int idx,
                          int *val)
{
    return cfg_get_instance_int_fmt(
            val,
            "/agent:%s/interface:%s/rss:/context:%u/hash_indir:/indir:%u",
            ta, if_name, rss_context, idx);
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_indir_set_local(const char *ta,
                                const char *if_name,
                                unsigned int rss_context,
                                unsigned int idx,
                                int val)
{
    return cfg_set_instance_local_fmt(
            CFG_VAL(INTEGER, val),
            "/agent:%s/interface:%s/rss:/context:%u/hash_indir:/indir:%u",
            ta, if_name, rss_context, idx);
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_hash_indir_commit(const char *ta,
                                  const char *if_name,
                                  unsigned int rss_context)
{
    return cfg_commit_fmt(
                "/agent:%s/interface:%s/rss:/context:%u/hash_indir:",
                ta, if_name, rss_context);
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_fill_indir_table(const char *ta,
                                 const char *if_name,
                                 unsigned int rss_context,
                                 unsigned int queue_from,
                                 unsigned int queue_to)
{
    unsigned int table_size;
    unsigned int i;
    int queue;
    int incr;
    te_errno rc;

    rc = tapi_cfg_if_rss_indir_table_size(ta, if_name, rss_context,
                                          &table_size);
    if (rc != 0)
        return rc;

    incr = (queue_from <= queue_to ? 1 : -1);

    for (i = 0, queue = queue_from; i < table_size; i++)
    {
        rc = tapi_cfg_if_rss_indir_set_local(ta, if_name, rss_context,
                                             i, queue);
        if (rc != 0)
            return rc;

        if (queue == queue_to)
            queue = queue_from;
        else
            queue += incr;
    }

    return 0;
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_hfuncs_get(const char *ta,
                           const char *if_name,
                           unsigned int rss_context,
                           tapi_cfg_if_rss_hfunc **hfuncs,
                           unsigned int *hfuncs_num)
{
    cfg_val_type type;
    tapi_cfg_if_rss_hfunc *result = NULL;
    cfg_handle *funcs_nodes = NULL;
    unsigned int funcs_num;
    unsigned int i;
    char *name;
    int enabled;
    te_errno rc = 0;

    rc = cfg_find_pattern_fmt(
          &funcs_num, &funcs_nodes,
          "/agent:%s/interface:%s/rss:/context:%u/hash_indir:/hash_func:*",
          ta, if_name, rss_context);
    if (rc != 0)
        goto cleanup;

    if (funcs_num == 0)
    {
        *hfuncs = NULL;
        *hfuncs_num = 0;
        return 0;
    }

    result = TE_ALLOC(sizeof(tapi_cfg_if_rss_hfunc) * funcs_num);
    if (result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    for (i = 0; i < funcs_num; i++)
    {
        type = CVT_INTEGER;
        rc = cfg_get_instance(funcs_nodes[i], &type, &enabled);
        if (rc != 0)
            goto cleanup;

        result[i].enabled = enabled;

        rc = cfg_get_inst_name(funcs_nodes[i], &name);
        if (rc != 0)
            goto cleanup;

        rc = te_snprintf(result[i].name,
                         TAPI_CFG_IF_RSS_HFUNC_NAME_LEN,
                         "%s", name);
        free(name);
        if (rc != 0)
            goto cleanup;
    }

    *hfuncs = result;
    *hfuncs_num = funcs_num;

cleanup:

    free(funcs_nodes);

    if (rc != 0)
        free(result);

    return rc;
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_hfunc_set_local(const char *ta,
                                const char *if_name,
                                unsigned int rss_context,
                                const char *func_name,
                                int state)
{
    return cfg_set_instance_local_fmt(
          CFG_VAL(INTEGER, state),
          "/agent:%s/interface:%s/rss:/context:%u/hash_indir:/hash_func:%s",
          ta, if_name, rss_context, func_name);
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_hfunc_set_single_local(const char *ta,
                                       const char *if_name,
                                       unsigned int rss_context,
                                       const char *func_name)
{
    tapi_cfg_if_rss_hfunc *hfuncs = NULL;
    unsigned int hfuncs_count;
    unsigned int i;
    te_errno rc = 0;
    te_bool found = FALSE;

    rc = tapi_cfg_if_rss_hfuncs_get(ta, if_name, rss_context,
                                    &hfuncs, &hfuncs_count);
    if (rc != 0)
        return rc;

    for (i = 0; i < hfuncs_count; i++)
    {
        if (strcmp(hfuncs[i].name, func_name) == 0)
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        rc = TE_RC(TE_TAPI, TE_ENOENT);
        goto cleanup;
    }

    for (i = 0; i < hfuncs_count; i++)
    {
        if (strcmp(hfuncs[i].name, func_name) == 0)
        {
            /*
             * Do at least this set even if there is no need,
             * so that if user calls commit() next, it does not
             * fail.
             */
            rc = tapi_cfg_if_rss_hfunc_set_local(ta, if_name, rss_context,
                                                 hfuncs[i].name, 1);
            if (rc != 0)
                goto cleanup;
        }
        else
        {
            if (hfuncs[i].enabled)
            {
                rc = tapi_cfg_if_rss_hfunc_set_local(ta, if_name,
                                                     rss_context,
                                                     hfuncs[i].name, 0);
                if (rc != 0)
                    goto cleanup;
            }
        }
    }

cleanup:

    free(hfuncs);

    return rc;
}
