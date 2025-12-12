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

/**
 * Get list of supported interrupt coalescing parameters.
 *
 * @param gid             Group ID (unused)
 * @param oid             Object instance identifier (unused)
 * @param sub_id          Name of the object to be listed (unused)
 * @param list_out        Pointer to the list will be stored here
 * @param if_name         Interface name
 * @param coalesce_name   Not used
 * @param global_name     Not used
 *
 * @return Status code.
 */
static te_errno
coalesce_param_list(unsigned int gid,
                    const char *oid,
                    const char *sub_id,
                    char **list_out,
                    const char *if_name,
                    const char *coalesce_name,
                    const char *global_name)
{
    struct ethtool_coalesce ecmd;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(coalesce_name);
    UNUSED(global_name);

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GCOALESCE, &ecmd);
    if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
    {
        *list_out = NULL;
        return 0;
    }
    else if (rc != 0)
    {
        return rc;
    }

    *list_out = strdup(
        "rx_coalesce_usecs "
        "rx_max_coalesced_frames "
        "rx_coalesce_usecs_irq "
        "rx_max_coalesced_frames_irq "
        "tx_coalesce_usecs "
        "tx_max_coalesced_frames "
        "tx_coalesce_usecs_irq "
        "tx_max_coalesced_frames_irq "
        "stats_block_coalesce_usecs "
        "use_adaptive_rx_coalesce "
        "use_adaptive_tx_coalesce "
        "pkt_rate_low "
        "rx_coalesce_usecs_low "
        "rx_max_coalesced_frames_low "
        "tx_coalesce_usecs_low "
        "tx_max_coalesced_frames_low "
        "pkt_rate_high "
        "rx_coalesce_usecs_high "
        "rx_max_coalesced_frames_high "
        "tx_coalesce_usecs_high "
        "tx_max_coalesced_frames_high "
        "rate_sample_interval");

    if (*list_out == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

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
 * @param value         Buffer for obtained value.
 *
 * @return Status code.
 */
static te_errno
common_param_get(struct ethtool_coalesce *eptr,
                 const char *param_name,
                 char *value)
{
    unsigned int param_val;
    te_errno rc;

    rc = process_coalesce_param(eptr, param_name, &param_val,
                                false);
    if (rc != 0)
        return rc;

    rc = te_snprintf(value, RCF_MAX_VAL, "%u", param_val);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

/**
 * Get value of interrupt coalescing parameter.
 *
 * @param gid             Group ID
 * @param oid             Object instance identifier (unused)
 * @param value           Where to save the value
 * @param if_name         Interface name
 * @param coalesce_name   Unused
 * @param global_name     Unused
 * @param param_name      Name of the parameter
 *
 * @return Status code.
 */
static te_errno
coalesce_param_get(unsigned int gid,
                   const char *oid,
                   char *value,
                   const char *if_name,
                   const char *coalesce_name,
                   const char *global_name,
                   const char *param_name)
{
    struct ethtool_coalesce *eptr;
    te_errno rc;

    UNUSED(oid);
    UNUSED(coalesce_name);
    UNUSED(global_name);

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_COALESCE,
                           (void **)&eptr);
    if (rc != 0)
        return rc;

    return common_param_get(eptr, param_name, value);
}

/**
 * Common code for setting interrupt coalescing parameter value.
 *
 * @param eptr          Pointer to ethtool_coalesce structure.
 * @param param_name    Parameter name.
 * @param value         Parameter value.
 *
 * @return Status code.
 */
static te_errno
common_param_set(struct ethtool_coalesce *eptr,
                 const char *param_name,
                 const char *value)
{
    unsigned long int parsed_val;
    unsigned int param_val;
    te_errno rc;

    rc = te_strtoul(value, 10, &parsed_val);
    if (rc != 0)
    {
        ERROR("%s(): invalid value '%s'", __FUNCTION__, value);
        return rc;
    }
    else if (parsed_val > UINT_MAX)
    {
        ERROR("%s(): too big value '%s'", __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    param_val = parsed_val;
    return process_coalesce_param(eptr, param_name, &param_val,
                                  true);
}

/**
 * Set value of interrupt coalescing parameter.
 *
 * @param gid             Group ID
 * @param oid             Object instance identifier (unused)
 * @param value           Value to set
 * @param if_name         Interface name
 * @param coalesce_name   Unused
 * @param global_name     Unused
 * @param param_name      Name of the parameter
 *
 * @return Status code.
 */
static te_errno
coalesce_param_set(unsigned int gid,
                   const char *oid,
                   char *value,
                   const char *if_name,
                   const char *coalesce_name,
                   const char *global_name,
                   const char *param_name)
{
    struct ethtool_coalesce *eptr;
    te_errno rc;

    UNUSED(oid);
    UNUSED(coalesce_name);
    UNUSED(global_name);

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_COALESCE,
                           (void **)&eptr);
    if (rc != 0)
        return rc;

    return common_param_set(eptr, param_name, value);
}

/**
 * Commit changes to interrupt coalescing settings.
 *
 * @param gid     Group ID (unused)
 * @param p_oid   Pointer to Object Instance Identitier
 *
 * @return Status code.
 */
static te_errno
if_coalesce_commit(unsigned int gid, const cfg_oid *p_oid)
{
    char *if_name;

    UNUSED(gid);
    if_name = CFG_OID_GET_INST_NAME(p_oid, 2);

    return commit_ethtool_value(if_name, gid, TA_ETHTOOL_COALESCE);
}

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
 * @param gid             Group ID.
 * @param oid             Object instance identifier (unused).
 * @param value           Where to save the value.
 * @param if_name         Interface name.
 * @param coalesce_name   Unused.
 * @param queues_name     Unused.
 * @param queue_name      Queue name.
 * @param param_name      Name of the parameter.
 *
 * @return Status code.
 */
static te_errno
queue_param_get(unsigned int gid,
                const char *oid,
                char *value,
                const char *if_name,
                const char *coalesce_name,
                const char *queues_name,
                const char *queue_name,
                const char *param_name)
{

    struct ethtool_coalesce *eptr;
    te_errno rc;

    UNUSED(oid);
    UNUSED(coalesce_name);
    UNUSED(queues_name);

    rc = get_per_queue_data(gid, if_name, queue_name, &eptr);
    if (rc != 0)
        return rc;

    return common_param_get(eptr, param_name, value);
}

/**
 * Set value of interrupt coalescing parameter for a specific queue.
 *
 * @param gid             Group ID.
 * @param oid             Object instance identifier (unused).
 * @param value           Value to set.
 * @param if_name         Interface name.
 * @param coalesce_name   Unused
 * @param queues_name     Unused.
 * @param queue_name      Queue name.
 * @param param_name      Name of the parameter.
 *
 * @return Status code.
 */
static te_errno
queue_param_set(unsigned int gid,
                const char *oid,
                char *value,
                const char *if_name,
                const char *coalesce_name,
                const char *queues_name,
                const char *queue_name,
                const char *param_name)
{
    struct ethtool_coalesce *eptr;
    te_errno rc;

    UNUSED(oid);
    UNUSED(coalesce_name);
    UNUSED(queues_name);

    rc = get_per_queue_data(gid, if_name, queue_name, &eptr);
    if (rc != 0)
        return rc;

    return common_param_set(eptr, param_name, value);
}

/**
 * Get list of interface queues.
 *
 * @param gid             Group ID (unused).
 * @param oid             Object instance identifier (unused).
 * @param sub_id          Name of the object to be listed (unused).
 * @param list_out        Pointer to the list will be stored here.
 * @param if_name         Interface name.
 * @param coalesce_name   Not used.
 *
 * @return Status code.
 */
static te_errno
queue_list(unsigned int gid,
           const char *oid,
           const char *sub_id,
           char **list_out,
           const char *if_name,
           const char *coalesce_name)
{
    ta_ethtool_pq_coalesce *pq;
    te_errno rc;
    te_string str = TE_STRING_INIT;
    unsigned int i;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(coalesce_name);

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_PQ_COALESCE,
                           (void **)&pq);
    if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
    {
        *list_out = NULL;
        return 0;
    }
    else if (rc != 0)
    {
        return rc;
    }

    for (i = 0; i < pq->queues_num; i++)
    {
        if (i > 0)
            te_string_append(&str, " ");

        te_string_append(&str, "%u", i);
    }

    *list_out = str.ptr;
    return 0;
}

/**
 * Commit changes to interrupt coalescing settings for interface queues.
 *
 * @param gid     Group ID (unused).
 * @param p_oid   Pointer to Object Instance Identitier.
 *
 * @return Status code.
 */
static te_errno
queues_commit(unsigned int gid, const cfg_oid *p_oid)
{
    char *if_name;

    UNUSED(gid);
    if_name = CFG_OID_GET_INST_NAME(p_oid, 2);

    return commit_ethtool_value(if_name, gid, TA_ETHTOOL_PQ_COALESCE);
}

/* Predeclaration */
static rcf_pch_cfg_object node_queues;

/** Queue coalescing parameter node */
static rcf_pch_cfg_object node_queue_param = {
    "param", 0, NULL, NULL,
    (rcf_ch_cfg_get)queue_param_get,
    (rcf_ch_cfg_set)queue_param_set,
    NULL, NULL, (rcf_ch_cfg_list)coalesce_param_list,
    NULL, &node_queues
};

/** Interface queue node */
static rcf_pch_cfg_object node_queue = {
    "queue", 0, &node_queue_param, NULL,
    NULL, NULL, NULL, NULL,
    (rcf_ch_cfg_list)queue_list,
    NULL, &node_queues
};

/** Interface queues node */
static rcf_pch_cfg_object node_queues = {
    "queues", 0, &node_queue, NULL,
    NULL, NULL, NULL, NULL, NULL,
    (rcf_ch_cfg_commit)queues_commit, NULL
};

#endif /* ETHTOOL_PERQUEUE */

/* Predeclaration */
static rcf_pch_cfg_object node_global;

/** Coalescing parameter node */
static rcf_pch_cfg_object node_param = {
    "param", 0, NULL, NULL,
    (rcf_ch_cfg_get)coalesce_param_get,
    (rcf_ch_cfg_set)coalesce_param_set,
    NULL, NULL, (rcf_ch_cfg_list)coalesce_param_list,
    NULL, &node_global
};

/** Interrupt coalescing settings for whole interface */
RCF_PCH_CFG_NODE_NA_COMMIT(node_global, "global", &node_param, NULL,
                           &if_coalesce_commit);

/** Common node for interrupt coalescing settings */
RCF_PCH_CFG_NODE_NA(node_net_if_coalesce, "coalesce", &node_global, NULL);

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

    rc = rcf_pch_add_node("/agent/interface", &node_net_if_coalesce);
    if (rc != 0)
        return rc;

#ifdef ETHTOOL_PERQUEUE
    rc = rcf_pch_add_node("/agent/interface/coalesce", &node_queues);
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
