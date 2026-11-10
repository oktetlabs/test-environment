/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for Network Interface RSS settings
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "Config RSS"

#include "te_config.h"
#include "te_defs.h"
#include "conf_api.h"
#include "tapi_cfg_if_rss.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_enum.h"
#include "logger_api.h"

/* Format of OID of the hash fields of a flow type */
#define FLOW_HASH_FMT \
    "/agent:%s/interface:%s/rss:/context:%u/flow_hash:%s"

/* Names of flow types in the configuration tree */
static const te_enum_map flow_hash_names[] = {
    { .name = "tcp4", .value = TAPI_CFG_IF_RSS_FLOW_TCP4 },
    { .name = "udp4", .value = TAPI_CFG_IF_RSS_FLOW_UDP4 },
    { .name = "sctp4", .value = TAPI_CFG_IF_RSS_FLOW_SCTP4 },
    { .name = "ah_esp4", .value = TAPI_CFG_IF_RSS_FLOW_AH_ESP4 },
    { .name = "tcp6", .value = TAPI_CFG_IF_RSS_FLOW_TCP6 },
    { .name = "udp6", .value = TAPI_CFG_IF_RSS_FLOW_UDP6 },
    { .name = "sctp6", .value = TAPI_CFG_IF_RSS_FLOW_SCTP6 },
    { .name = "ah_esp6", .value = TAPI_CFG_IF_RSS_FLOW_AH_ESP6 },
    TE_ENUM_MAP_END
};

/* Names of hash fields in the configuration tree */
static const te_enum_map flow_hash_field_names[] = {
    { .name = "l2da", .value = TAPI_CFG_IF_RSS_FIELD_BIT_L2DA },
    { .name = "vlan", .value = TAPI_CFG_IF_RSS_FIELD_BIT_VLAN },
    { .name = "l3_proto", .value = TAPI_CFG_IF_RSS_FIELD_BIT_L3_PROTO },
    { .name = "ip_src", .value = TAPI_CFG_IF_RSS_FIELD_BIT_IP_SRC },
    { .name = "ip_dst", .value = TAPI_CFG_IF_RSS_FIELD_BIT_IP_DST },
    { .name = "l4_b_0_1", .value = TAPI_CFG_IF_RSS_FIELD_BIT_L4_SRC },
    { .name = "l4_b_2_3", .value = TAPI_CFG_IF_RSS_FIELD_BIT_L4_DST },
    { .name = "gtp_teid", .value = TAPI_CFG_IF_RSS_FIELD_BIT_GTP_TEID },
    { .name = "ip6_fl", .value = TAPI_CFG_IF_RSS_FIELD_BIT_IP6_FL },
    { .name = "discard", .value = TAPI_CFG_IF_RSS_FIELD_BIT_DISCARD },
    TE_ENUM_MAP_END
};

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

/* Common code for hash key setting */
static te_errno
hash_key_set_common(bool local,
                    const char *ta,
                    const char *if_name,
                    unsigned int rss_context,
                    const uint8_t *buf, size_t len)
{
    te_string str = TE_STRING_INIT;
    te_errno rc;
    const char *path_fmt;

    path_fmt = "/agent:%s/interface:%s/rss:/context:%u/hash_indir:/hash_key:";

    rc = te_str_hex_raw2str(buf, len, &str);
    if (rc != 0)
        return rc;

    if (local)
    {
        rc = cfg_set_instance_local_fmt(
                CFG_VAL(STRING, te_string_value(&str)), path_fmt,
                ta, if_name, rss_context);
    }
    else
    {
        rc = cfg_set_instance_fmt(
                CFG_VAL(STRING, te_string_value(&str)), path_fmt,
                ta, if_name, rss_context);
    }

    te_string_free(&str);
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
    return hash_key_set_common(true, ta, if_name, rss_context, buf, len);
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_hash_key_set(
                             const char *ta,
                             const char *if_name,
                             unsigned int rss_context,
                             const uint8_t *buf, size_t len)
{
    return hash_key_set_common(false, ta, if_name, rss_context, buf, len);
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
            CFG_VAL(INT32, val),
            "/agent:%s/interface:%s/rss:/context:%u/hash_indir:/indir:%u",
            ta, if_name, rss_context, idx);
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_indir_default_set_local(const char *ta,
                                        const char *if_name)
{
    return cfg_set_instance_local_fmt(
            CFG_VAL(BOOL, 1),
            "/agent:%s/interface:%s/rss:/context:0/hash_indir:/indir_default:",
            ta, if_name);
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_indir_default_set(const char *ta, const char *if_name)
{
    return cfg_set_instance_fmt(
            CFG_VAL(BOOL, 1),
            "/agent:%s/interface:%s/rss:/context:0/hash_indir:/indir_default:",
            ta, if_name);
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_hash_indir_commit(const char *ta,
                                  const char *if_name,
                                  unsigned int rss_context)
{
    te_errno rc;

    rc = cfg_commit_fmt(
                "/agent:%s/interface:%s/rss:/context:%u/hash_indir:",
                ta, if_name, rss_context);
    if (rc != 0)
        return rc;

    /*
     * Configurator does not read values back after a commit: get what
     * the driver has applied, so that the getters do not return what
     * was requested. The commit applies the hash key, the hash functions
     * and the indirection table together, and a driver may change any
     * of them, so synchronize the whole committed object.
     */
    return cfg_synchronize_fmt(
                true, "/agent:%s/interface:%s/rss:/context:%u/hash_indir:",
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
tapi_cfg_if_rss_print_indir_table(const char *ta,
                                  const char *if_name,
                                  unsigned int rss_context)
{
    te_errno rc = 0;
    unsigned int size;
    unsigned int i;
    te_string str = TE_STRING_INIT;
    int val;

    rc = tapi_cfg_if_rss_indir_table_size(ta, if_name, rss_context, &size);
    if (rc != 0)
        return rc;

    te_string_append(&str, "RSS indirection table for interface "
                     "%s on agent %s, RSS context %u\ncontains %u entries.",
                     if_name, ta, rss_context, size);

    if (size > 0)
    {
        te_string_append(&str, "\n\nEntry ---> Rx queue\n");

        for (i = 0; i < size; i++)
        {
            rc = tapi_cfg_if_rss_indir_get(ta, if_name, rss_context,
                                           i, &val);
            if (rc != 0)
                goto finish;

            te_string_append(&str, "%5u ---> %d\n", i, val);
        }
    }

    RING("%s", te_string_value(&str));

finish:

    te_string_free(&str);
    return rc;
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

    for (i = 0; i < funcs_num; i++)
    {
        type = CVT_INT32;
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
          CFG_VAL(INT32, state),
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
    bool found = false;

    rc = tapi_cfg_if_rss_hfuncs_get(ta, if_name, rss_context,
                                    &hfuncs, &hfuncs_count);
    if (rc != 0)
        return rc;

    for (i = 0; i < hfuncs_count; i++)
    {
        if (strcmp(hfuncs[i].name, func_name) == 0)
        {
            found = true;
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

/* Get the name of a flow type in the configuration tree */
static te_errno
flow_hash_name(tapi_cfg_if_rss_flow flow, const char **flow_name)
{
    *flow_name = te_enum_map_from_any_value(flow_hash_names, flow, NULL);
    if (*flow_name == NULL)
    {
        ERROR("%s(): unknown flow type %d", __func__, (int)flow);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
}

/*
 * Find the field instances of a flow type and get the flags of
 * the fields (zero for a field that has no flag).
 * The arrays should be released by the caller.
 */
static te_errno
flow_hash_fields_find(const char *ta, const char *if_name,
                      unsigned int rss_context, const char *flow_name,
                      unsigned int *num, cfg_handle **handles,
                      unsigned int **flags)
{
    cfg_handle flow_handle;
    cfg_handle *set = NULL;
    unsigned int *result = NULL;
    unsigned int count;
    unsigned int i;
    char *name;
    te_errno rc;

    /* There is no instance if the driver does not support the type. */
    rc = cfg_find_fmt(&flow_handle, FLOW_HASH_FMT, ta, if_name,
                      rss_context, flow_name);
    if (rc != 0)
        return rc;

    rc = cfg_find_pattern_fmt(&count, &set, FLOW_HASH_FMT "/field:*",
                              ta, if_name, rss_context, flow_name);
    if (rc != 0)
        return rc;

    result = TE_ALLOC(sizeof(*result) * (count + 1));

    for (i = 0; i < count; i++)
    {
        rc = cfg_get_inst_name(set[i], &name);
        if (rc != 0)
        {
            free(set);
            free(result);
            return rc;
        }

        result[i] = te_enum_map_from_str(flow_hash_field_names, name, 0);
        free(name);
    }

    *num = count;
    *handles = set;
    *flags = result;

    return 0;
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_flow_hash_get(const char *ta,
                              const char *if_name,
                              unsigned int rss_context,
                              tapi_cfg_if_rss_flow flow,
                              unsigned int *fields)
{
    const char *flow_name;
    cfg_handle *handles = NULL;
    unsigned int *flags = NULL;
    unsigned int result = 0;
    unsigned int num;
    unsigned int i;
    te_errno rc;

    if (ta == NULL || if_name == NULL || fields == NULL)
    {
        ERROR("%s(): invalid arguments", __func__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = flow_hash_name(flow, &flow_name);
    if (rc != 0)
        return rc;

    /*
     * Configurator does not read values back after a commit, and
     * the fields may be changed outside of it: get what the driver
     * reports.
     */
    rc = cfg_synchronize_fmt(true, FLOW_HASH_FMT, ta, if_name,
                             rss_context, flow_name);
    if (rc != 0)
        return rc;

    rc = flow_hash_fields_find(ta, if_name, rss_context, flow_name,
                               &num, &handles, &flags);
    if (rc != 0)
        return rc;

    for (i = 0; i < num; i++)
    {
        cfg_val_type type = CVT_BOOL;
        bool enabled;

        rc = cfg_get_instance(handles[i], &type, &enabled);
        if (rc != 0)
            goto cleanup;

        if (enabled)
            result |= flags[i];
    }

    *fields = result;

cleanup:

    free(handles);
    free(flags);

    return rc;
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_flow_hash_set_local(const char *ta,
                                    const char *if_name,
                                    unsigned int rss_context,
                                    tapi_cfg_if_rss_flow flow,
                                    unsigned int fields)
{
    const char *flow_name;
    cfg_handle *handles = NULL;
    unsigned int *flags = NULL;
    unsigned int known = 0;
    unsigned int num;
    unsigned int i;
    te_errno rc;

    rc = flow_hash_name(flow, &flow_name);
    if (rc != 0)
        return rc;

    rc = flow_hash_fields_find(ta, if_name, rss_context, flow_name,
                               &num, &handles, &flags);
    if (rc != 0)
        return rc;

    for (i = 0; i < num; i++)
        known |= flags[i];

    if ((fields & ~known) != 0)
    {
        ERROR("%s(): hash fields 0x%x are not known to TA %s",
              __func__, fields & ~known, ta);
        rc = TE_RC(TE_TAPI, TE_EOPNOTSUPP);
        goto cleanup;
    }

    /*
     * A field that has no flag here cannot be requested, and
     * the fields that are not requested are excluded.
     */
    for (i = 0; i < num; i++)
    {
        unsigned int enabled = ((fields & flags[i]) != 0 ? 1 : 0);

        rc = cfg_set_instance_local(handles[i], CVT_BOOL, enabled);
        if (rc != 0)
            goto cleanup;
    }

cleanup:

    free(handles);
    free(flags);

    return rc;
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_flow_hash_commit(const char *ta,
                                 const char *if_name,
                                 unsigned int rss_context,
                                 tapi_cfg_if_rss_flow flow)
{
    const char *flow_name;
    te_errno rc;

    rc = flow_hash_name(flow, &flow_name);
    if (rc != 0)
        return rc;

    rc = cfg_commit_fmt(FLOW_HASH_FMT, ta, if_name, rss_context,
                        flow_name);
    if (rc != 0)
        return rc;

    /*
     * Configurator does not read values back after a commit: get what
     * the driver has applied, so that the getter does not return what
     * was requested.
     */
    return cfg_synchronize_fmt(true, FLOW_HASH_FMT, ta, if_name,
                               rss_context, flow_name);
}

/* See description in tapi_cfg_if_rss.h */
te_errno
tapi_cfg_if_rss_flow_hash_set(const char *ta,
                              const char *if_name,
                              unsigned int rss_context,
                              tapi_cfg_if_rss_flow flow,
                              unsigned int fields)
{
    te_errno rc;

    rc = tapi_cfg_if_rss_flow_hash_set_local(ta, if_name, rss_context,
                                             flow, fields);
    if (rc != 0)
        return rc;

    return tapi_cfg_if_rss_flow_hash_commit(ta, if_name, rss_context,
                                            flow);
}
