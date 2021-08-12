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

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H

/**
 * Call @c SIOCETHTOOL ioctl() to get or set interrupt coalescing
 * parameters.
 *
 * @param if_name     Name of the interface
 * @param ecmd        Pointer to ethtool_coalesce structure to pass
 *                    to ioctl()
 * @param do_set      If @c TRUE, use @c ETHTOOL_SCOALESCE command to set
 *                    parameters, otherwise - @c ETHTOOL_GCOALESCE to get
 *                    them
 *
 * @return Status code.
 */
static te_errno
ioctl_ethtool_coalesce(const char *if_name, struct ethtool_coalesce *ecmd,
                       te_bool do_set)
{
    struct ifreq ifr;
    int rc;
    te_errno te_rc;

    memset(&ifr, 0, sizeof(ifr));

    te_rc = te_snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", if_name);
    if (te_rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, te_rc);
    }

    ecmd->cmd = (do_set ? ETHTOOL_SCOALESCE : ETHTOOL_GCOALESCE);
    ifr.ifr_data = (caddr_t)ecmd;

    rc = ioctl(cfg_socket, SIOCETHTOOL, &ifr);
    if (rc < 0)
    {
        ERROR("%s(do_set=%s): ioctl() failed", __FUNCTION__,
              (do_set ? "TRUE" : "FALSE"));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/**
 * Get a pointer to ethtool_coalesce structure to work with.
 * It can be either pointer to locally declared structure
 * (if this is a simple get request), or a pointer to a structure
 * stored in a TA configuration object (if a set request is in
 * progress which is going to be committed later, perhaps together
 * with other set requests which should all operate on the same
 * ethtool_coalesce structure).
 * In the latter case, if there is no such object for the given
 * interface yet, this function creates it and fills the structure
 * stored in it with @c ETHTOOL_GCOALESCE command.
 *
 * @param if_name         Interface name
 * @param gid             Group ID
 * @param ecmd_local      Pointer to locally declared
 *                        ethtool_coalesce structure
 * @param ecmd_out        Here requested pointer will be saved
 * @param do_set          If @c TRUE, it is a set request
 *
 * @return Status code.
 */
static te_errno
get_ethtool_coalesce(const char *if_name, unsigned int gid,
                     struct ethtool_coalesce *ecmd_local,
                     struct ethtool_coalesce **ecmd_out,
                     te_bool do_set)
{
    ta_cfg_obj_t *obj;
    te_errno rc;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_COALESCE,
                      if_name);
    if (obj != NULL)
    {
        *ecmd_out = (struct ethtool_coalesce *)(obj->user_data);
    }
    else
    {
        memset(ecmd_local, 0, sizeof(*ecmd_local));
        rc = ioctl_ethtool_coalesce(if_name, ecmd_local, FALSE);
        if (rc != 0)
            return rc;

        if (do_set)
        {
            *ecmd_out = calloc(sizeof(*ecmd_local), 1);
            if (*ecmd_out == NULL)
            {
                ERROR("%s(): not enough memory", __FUNCTION__);
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            }

            rc = ta_obj_add(TA_OBJ_TYPE_IF_COALESCE, if_name, "",
                            gid, *ecmd_out, NULL);
            if (rc != 0)
            {
                ERROR("%s(): failed to add a new object", __FUNCTION__);
                free(*ecmd_out);
                *ecmd_out = NULL;
                return TE_RC(TE_TA_UNIX, rc);
            }

            memcpy(*ecmd_out, ecmd_local, sizeof(*ecmd_local));
        }
        else
        {
            *ecmd_out = ecmd_local;
        }
    }

    return 0;
}

/**
 * Get list of supported interrupt coalescing parameters.
 *
 * @param gid       Group ID (unused)
 * @param oid       Object instance identifier (unused)
 * @param sub_id    Name of the object to be listed (unused)
 * @param list_out  Pointer to the list will be stored here
 *
 * @return Status code.
 */
static te_errno
coalesce_param_list(unsigned int gid,
                    const char *oid,
                    const char *sub_id,
                    char **list_out)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

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
    struct ethtool_coalesce ecmd;
    struct ethtool_coalesce *eptr;
    unsigned int param_val;
    te_errno rc;

    UNUSED(oid);
    UNUSED(coalesce_name);

    rc = get_ethtool_coalesce(if_name, gid, &ecmd, &eptr,
                              FALSE);
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
    struct ethtool_coalesce ecmd;
    struct ethtool_coalesce *eptr;
    unsigned long int parsed_val;
    unsigned int param_val;
    te_errno rc;

    UNUSED(oid);
    UNUSED(coalesce_name);

    rc = get_ethtool_coalesce(if_name, gid, &ecmd, &eptr,
                              TRUE);
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
    struct ethtool_coalesce *eptr;
    char *if_name;
    ta_cfg_obj_t *obj;
    te_errno rc;

    UNUSED(gid);
    if_name = CFG_OID_GET_INST_NAME(p_oid, 2);

    obj = ta_obj_find(TA_OBJ_TYPE_IF_COALESCE,
                      if_name);
    if (obj == NULL)
    {
        ERROR("%s(): coalescing settings object was not found for "
              "interface '%s'", __FUNCTION__, if_name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    eptr = (struct ethtool_coalesce *)(obj->user_data);
    rc = ioctl_ethtool_coalesce(if_name, eptr, TRUE);

    free(eptr);
    ta_obj_free(obj);

    return rc;
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
