/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Interrupt Coalescing
 *
 * Unix TA Network Interface Interrupt Coalescing settings
 *
 *
 * Copyright (C) 2021-2025 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Intr Coalesce"

#include "te_config.h"
#include "config.h"

#include <limits.h>

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "te_str.h"
#include "te_alloc.h"
#include "te_vector.h"
#include "rcf_pch_tree.h"

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

/** Names of fields in ethtool_coalesce structure exposed as parameters */
static const char *const coalesce_param_names[] = {
    "rx_coalesce_usecs",
    "rx_max_coalesced_frames",
    "rx_coalesce_usecs_irq",
    "rx_max_coalesced_frames_irq",
    "tx_coalesce_usecs",
    "tx_max_coalesced_frames",
    "tx_coalesce_usecs_irq",
    "tx_max_coalesced_frames_irq",
    "stats_block_coalesce_usecs",
    "use_adaptive_rx_coalesce",
    "use_adaptive_tx_coalesce",
    "pkt_rate_low",
    "rx_coalesce_usecs_low",
    "rx_max_coalesced_frames_low",
    "tx_coalesce_usecs_low",
    "tx_max_coalesced_frames_low",
    "pkt_rate_high",
    "rx_coalesce_usecs_high",
    "rx_max_coalesced_frames_high",
    "tx_coalesce_usecs_high",
    "tx_max_coalesced_frames_high",
    "rate_sample_interval",
};

/**
 * Get list of supported interrupt coalescing parameters.
 *
 * @param ctx             Request context (parent instance OID)
 * @param names           Vector of heap-allocated names to append to
 *
 * @return Status code.
 */
static te_errno
coalesce_param_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_coalesce ecmd;
    te_errno rc;
    size_t i;

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GCOALESCE, &ecmd);
    if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
        return 0;
    else if (rc != 0)
        return rc;

    for (i = 0; i < TE_ARRAY_LEN(coalesce_param_names); i++)
    {
        char *name = TE_STRDUP(coalesce_param_names[i]);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

/**
 * if the provided name matches the name of the field in
 * ethtool_coalesce structure, process parameter setting or
 * getting and return success.
 *
 * @param _name     Parameter name
 * @param _field    Field name in ethtool_coalesce structure
 * @param _ecmd     Pointer to ethtool_coalesce structure
 * @param _val      Pointer to value which is to be either
 *                  get or set
 * @param _set      If @c true, value in the structure should be
 *                  set to @p _val, otherwise - vice versa
 */
#define PROCESS_PARAM(_name, _field, _ecmd, _val, _set) \
    do {                                                \
        if (strcmp(_name, #_field) == 0)                \
        {                                               \
            if (_set)                                   \
                _ecmd->_field = *_val;                  \
            else                                        \
                *_val = _ecmd->_field;                  \
                                                        \
            return 0;                                   \
        }                                               \
    } while (0)

/**
 * Process interrupt coalescing parameter operation.
 *
 * @param ecmd      Pointer to ethtool_coalesce structure
 * @param name      Name of the parameter (structure field)
 * @param val       Pointer to the value which should be either
 *                  copied to @p ecmd or updated from it
 * @param do_set    If @c true, the structure field should be
 *                  updated, otherwise field value from the
 *                  structure should be obtained
 *
 * @return Status code.
 */
static te_errno
process_coalesce_param(struct ethtool_coalesce *ecmd,
                       const char *name,
                       unsigned int *val,
                       bool do_set)
{
    PROCESS_PARAM(name, rx_coalesce_usecs, ecmd, val, do_set);
    PROCESS_PARAM(name, rx_max_coalesced_frames, ecmd, val, do_set);
    PROCESS_PARAM(name, rx_coalesce_usecs_irq, ecmd, val, do_set);
    PROCESS_PARAM(name, rx_max_coalesced_frames_irq, ecmd, val, do_set);
    PROCESS_PARAM(name, tx_coalesce_usecs, ecmd, val, do_set);
    PROCESS_PARAM(name, tx_max_coalesced_frames, ecmd, val, do_set);
    PROCESS_PARAM(name, tx_coalesce_usecs_irq, ecmd, val, do_set);
    PROCESS_PARAM(name, tx_max_coalesced_frames_irq, ecmd, val, do_set);
    PROCESS_PARAM(name, stats_block_coalesce_usecs, ecmd, val, do_set);
    PROCESS_PARAM(name, use_adaptive_rx_coalesce, ecmd, val, do_set);
    PROCESS_PARAM(name, use_adaptive_tx_coalesce, ecmd, val, do_set);
    PROCESS_PARAM(name, pkt_rate_low, ecmd, val, do_set);
    PROCESS_PARAM(name, rx_coalesce_usecs_low, ecmd, val, do_set);
    PROCESS_PARAM(name, rx_max_coalesced_frames_low, ecmd, val, do_set);
    PROCESS_PARAM(name, tx_coalesce_usecs_low, ecmd, val, do_set);
    PROCESS_PARAM(name, tx_max_coalesced_frames_low, ecmd, val, do_set);
    PROCESS_PARAM(name, pkt_rate_high, ecmd, val, do_set);
    PROCESS_PARAM(name, rx_coalesce_usecs_high, ecmd, val, do_set);
    PROCESS_PARAM(name, rx_max_coalesced_frames_high, ecmd, val, do_set);
    PROCESS_PARAM(name, tx_coalesce_usecs_high, ecmd, val, do_set);
    PROCESS_PARAM(name, tx_max_coalesced_frames_high, ecmd, val, do_set);
    PROCESS_PARAM(name, rate_sample_interval, ecmd, val, do_set);

    ERROR("%s(): unknown coalescing parameter '%s'", __FUNCTION__, name);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/**
 * Common code for obtaining interrupt coalescing parameter value.
 *
 * @param eptr          Pointer to ethtool_coalesce structure.
 * @param param_name    Parameter name.
 * @param val           Where to save obtained value.
 *
 * @return Status code.
 */
static te_errno
common_param_get(struct ethtool_coalesce *eptr,
                 const char *param_name,
                 uint64_t *val)
{
    unsigned int param_val;
    te_errno rc;

    rc = process_coalesce_param(eptr, param_name, &param_val, false);
    if (rc != 0)
        return rc;

    *val = param_val;
    return 0;
}

/**
 * Get value of interrupt coalescing parameter.
 *
 * @param ctx             Request context
 * @param val             Where to save the value
 *
 * @return Status code.
 */
static te_errno
coalesce_param_get(const ta_conf_ctx *ctx, uint64_t *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *param_name = ta_conf_ctx_inst(ctx, "param");
    struct ethtool_coalesce *eptr;
    te_errno rc;

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx), TA_ETHTOOL_COALESCE,
                           (void **)&eptr);
    if (rc != 0)
        return rc;

    return common_param_get(eptr, param_name, val);
}

/**
 * Common code for setting interrupt coalescing parameter value.
 *
 * @param eptr          Pointer to ethtool_coalesce structure.
 * @param param_name    Parameter name.
 * @param val           Parameter value.
 *
 * @return Status code.
 */
static te_errno
common_param_set(struct ethtool_coalesce *eptr,
                 const char *param_name,
                 uint64_t val)
{
    unsigned int param_val;

    if (val > UINT_MAX)
    {
        ERROR("%s(): too big value '%" PRIu64 "' for '%s'", __FUNCTION__, val,
              param_name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    param_val = val;
    return process_coalesce_param(eptr, param_name, &param_val, true);
}

/**
 * Set value of interrupt coalescing parameter.
 *
 * @param ctx             Request context
 * @param val             Value to set
 *
 * @return Status code.
 */
static te_errno
coalesce_param_set(const ta_conf_ctx *ctx, uint64_t val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *param_name = ta_conf_ctx_inst(ctx, "param");
    struct ethtool_coalesce *eptr;
    te_errno rc;

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx), TA_ETHTOOL_COALESCE,
                           (void **)&eptr);
    if (rc != 0)
        return rc;

    return common_param_set(eptr, param_name, val);
}

/**
 * Commit changes to interrupt coalescing settings.
 *
 * @param ctx     Request context
 *
 * @return Status code.
 */
static te_errno
if_coalesce_commit(const ta_conf_ctx *ctx)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");

    return commit_ethtool_value(if_name, ta_conf_ctx_gid(ctx),
                                TA_ETHTOOL_COALESCE);
}

/** Common node for interrupt coalescing settings */
static const ta_conf_node *const node_net_if_coalesce =
    TA_CONF_NA("coalesce",
        TA_CONF_NA_COMMIT("global", if_coalesce_commit,
            TA_CONF_RW_COLL_UINT64("param", coalesce_param_get,
                                 coalesce_param_set,
                                 coalesce_param_list)));

#ifdef ETHTOOL_PERQUEUE

/**
 * Get interrupt coalescing data for a specific queue.
 *
 * @param gid             Group ID.
 * @param if_name         Interface name.
 * @param queue_name      Queue name.
 * @param eptr_out        Output location for pointer to ethtool_coalesce
 *                        structure for a specific queue.
 *
 * @return Status code.
 */
static te_errno
get_per_queue_data(unsigned int gid, const char *if_name,
                   const char *queue_name,
                   struct ethtool_coalesce **eptr_out)
{
    ta_ethtool_pq_coalesce *pq;
    unsigned long int parsed_queue;
    te_errno rc;

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_PQ_COALESCE,
                           (void **)&pq);
    if (rc != 0)
        return rc;

    rc = te_strtoul(queue_name, 10, &parsed_queue);
    if (rc != 0)
    {
        ERROR("%s(): invalid queue '%s'", __FUNCTION__, queue_name);
        return rc;
    }
    else if (parsed_queue > pq->queues_num)
    {
        ERROR("%s(): too big queue number '%s'", __FUNCTION__, queue_name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (eptr_out != NULL)
    {
        *eptr_out = (struct ethtool_coalesce *)(pq->pq_op.data);
        *eptr_out += parsed_queue;
    }

    return 0;
}

/**
 * Get value of interrupt coalescing parameter for a specific queue.
 *
 * @param ctx             Request context
 * @param val             Where to save the value
 *
 * @return Status code.
 */
static te_errno
queue_param_get(const ta_conf_ctx *ctx, uint64_t *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *queue_name = ta_conf_ctx_inst(ctx, "queue");
    const char *param_name = ta_conf_ctx_inst(ctx, "param");
    struct ethtool_coalesce *eptr;
    te_errno rc;

    rc = get_per_queue_data(ta_conf_ctx_gid(ctx), if_name, queue_name, &eptr);
    if (rc != 0)
        return rc;

    return common_param_get(eptr, param_name, val);
}

/**
 * Set value of interrupt coalescing parameter for a specific queue.
 *
 * @param ctx             Request context
 * @param val             Value to set
 *
 * @return Status code.
 */
static te_errno
queue_param_set(const ta_conf_ctx *ctx, uint64_t val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *queue_name = ta_conf_ctx_inst(ctx, "queue");
    const char *param_name = ta_conf_ctx_inst(ctx, "param");
    struct ethtool_coalesce *eptr;
    te_errno rc;

    rc = get_per_queue_data(ta_conf_ctx_gid(ctx), if_name, queue_name, &eptr);
    if (rc != 0)
        return rc;

    return common_param_set(eptr, param_name, val);
}

/**
 * Get list of interface queues.
 *
 * @param ctx             Request context (parent instance OID)
 * @param names           Vector of heap-allocated names to append to
 *
 * @return Status code.
 */
static te_errno
queue_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    ta_ethtool_pq_coalesce *pq;
    te_errno rc;
    unsigned int i;

    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx), TA_ETHTOOL_PQ_COALESCE,
                           (void **)&pq);
    if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
        return 0;
    else if (rc != 0)
        return rc;

    for (i = 0; i < pq->queues_num; i++)
    {
        char *name = te_string_fmt("%u", i);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

/**
 * Commit changes to interrupt coalescing settings for interface queues.
 *
 * @param ctx     Request context
 *
 * @return Status code.
 */
static te_errno
queues_commit(const ta_conf_ctx *ctx)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");

    return commit_ethtool_value(if_name, ta_conf_ctx_gid(ctx),
                                TA_ETHTOOL_PQ_COALESCE);
}

static const ta_conf_node *const node_queues =
    TA_CONF_NA_COMMIT("queues", queues_commit,
        TA_CONF_LIST("queue", queue_list,
            TA_CONF_RW_COLL_UINT64("param", queue_param_get,
                                 queue_param_set,
                                 coalesce_param_list)));

#endif /* ETHTOOL_PERQUEUE */

/**
 * Add a child node for interrupt coalescing settings to the interface
 * object.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_coalesce_init(void)
{
    te_errno rc;

    rc = ta_conf_register("/agent/interface", node_net_if_coalesce);
    if (rc != 0)
        return rc;

#ifdef ETHTOOL_PERQUEUE
    rc = ta_conf_register("/agent/interface/coalesce", node_queues);
#endif

    return rc;
}

#else

/* See description above */
extern te_errno
ta_unix_conf_if_coalesce_init(void)
{
    WARN("Interface interrupt coalescing settings are not supported");
    return 0;
}
#endif
