/** @file
 * @brief Interrupt Coalescing
 *
 * Unix TA Network Interface Interrupt Coalescing settings
 *
 *
 * Copyright (C) 2021 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
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
 *
 * @return Status code.
 */
static te_errno
coalesce_param_list(unsigned int gid,
                    const char *oid,
                    const char *sub_id,
                    char **list_out,
                    const char *if_name,
                    const char *coalesce_name)
{
    struct ethtool_coalesce ecmd;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(coalesce_name);

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
 * @param _set      If @c TRUE, value in the structure should be
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
 * @param do_set    If @c TRUE, the structure field should be
 *                  updated, otherwise field value from the
 *                  structure should be obtained
 *
 * @return Status code.
 */
static te_errno
process_coalesce_param(struct ethtool_coalesce *ecmd,
                       const char *name,
                       unsigned int *val,
                       te_bool do_set)
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
 * Get value of interrupt coalescing parameter.
 *
 * @param gid             Group ID
 * @param oid             Object instance identifier (unused)
 * @param value           Where to save the value
 * @param if_name         Interface name
 * @param coalesce_name   Unused
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
                   const char *param_name)
{
    struct ethtool_coalesce *eptr;
    unsigned int param_val;
    te_errno rc;

    UNUSED(oid);
    UNUSED(coalesce_name);

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_COALESCE,
                           (void **)&eptr);
    if (rc != 0)
        return rc;

    rc = process_coalesce_param(eptr, param_name, &param_val,
                                FALSE);
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
 * Set value of interrupt coalescing parameter.
 *
 * @param gid             Group ID
 * @param oid             Object instance identifier (unused)
 * @param value           Value to set
 * @param if_name         Interface name
 * @param coalesce_name   Unused
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
                   const char *param_name)
{
    struct ethtool_coalesce *eptr;
    unsigned long int parsed_val;
    unsigned int param_val;
    te_errno rc;

    UNUSED(oid);
    UNUSED(coalesce_name);

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_COALESCE,
                           (void **)&eptr);
    if (rc != 0)
        return rc;

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
                                  TRUE);
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

/* Predeclaration */
static rcf_pch_cfg_object node_net_if_coalesce;

/** Coalescing parameter node */
static rcf_pch_cfg_object node_param = {
    "param", 0, NULL, NULL,
    (rcf_ch_cfg_get)coalesce_param_get,
    (rcf_ch_cfg_set)coalesce_param_set,
    NULL, NULL, (rcf_ch_cfg_list)coalesce_param_list,
    NULL, &node_net_if_coalesce
};

/** Common node for interrupt coalescing settings */
RCF_PCH_CFG_NODE_NA_COMMIT(node_net_if_coalesce, "coalesce", &node_param, NULL,
                           &if_coalesce_commit);

/**
 * Add a child node for interrupt coalescing settings to the interface
 * object.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_coalesce_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_net_if_coalesce);
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
