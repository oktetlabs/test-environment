/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Energy Efficient Ethernet
 *
 * Unix TA Network Interface Energy Efficient Ethernet settings
 */

#define TE_LGR_USER     "Conf EEE"

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

#ifdef ETHTOOL_GEEE

/** Fields in ethtool_eee structure */
typedef enum eee_field {
    /** eee_active field */
    FIELD_EEE_ACTIVE,
    /** eee_enabled field */
    FIELD_EEE_ENABLED,
    /** tx_lpi_enabled field */
    FIELD_TX_LPI_ENABLED,
    /** tx_lpi_timer field */
    FIELD_TX_LPI_TIMER,
} eee_field;

/* Get pointer to field of ethtool_eee structure. */
 static uint32_t *
get_field_ptr(struct ethtool_eee *eptr, eee_field field)
{
    switch (field)
    {
        case FIELD_EEE_ACTIVE:
            return &eptr->eee_active;
        case FIELD_EEE_ENABLED:
            return &eptr->eee_enabled;
        case FIELD_TX_LPI_ENABLED:
            return &eptr->tx_lpi_enabled;
        case FIELD_TX_LPI_TIMER:
            return &eptr->tx_lpi_timer;
    }

    return NULL;
}

/* Common code for getting field value */
static te_errno
common_field_get(unsigned int gid,
                 const char *oid,
                 char *value,
                 const char *if_name,
                 eee_field field)
{
    struct ethtool_eee *eptr;
    uint32_t *field_ptr;
    te_errno rc;

    UNUSED(oid);

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_EEE,
                           (void **)&eptr);
    if (rc != 0)
    {
        if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
        {
            /*
             * Avoid Configurator errors if EEE is not supported.
             */
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        return rc;
    }

    field_ptr = get_field_ptr(eptr, field);
    if (field_ptr == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_snprintf(value, RCF_MAX_VAL, "%u", (unsigned int)(*field_ptr));
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed, %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

/* Common code for setting field value */
static te_errno
common_param_set(unsigned int gid,
                 const char *oid,
                 const char *value,
                 const char *if_name,
                 eee_field field)
{
    struct ethtool_eee *eptr;
    uint32_t *field_ptr;
    unsigned long int parsed_val;
    te_errno rc;

    UNUSED(oid);

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

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_EEE,
                           (void **)&eptr);
    if (rc != 0)
        return rc;

    field_ptr = get_field_ptr(eptr, field);
    if (field_ptr == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *field_ptr = parsed_val;
    return 0;
}

/* Get eee_active field */
static te_errno
eee_active_get(unsigned int gid, const char *oid, char *value,
               const char *if_name)
{
    return common_field_get(gid, oid, value, if_name, FIELD_EEE_ACTIVE);
}

/* Get eee_enabled field */
static te_errno
eee_enabled_get(unsigned int gid, const char *oid, char *value,
                const char *if_name)
{
    return common_field_get(gid, oid, value, if_name, FIELD_EEE_ENABLED);
}

/* Get tx_lpi_enabled field */
static te_errno
tx_lpi_enabled_get(unsigned int gid, const char *oid, char *value,
                   const char *if_name)
{
    return common_field_get(gid, oid, value, if_name, FIELD_TX_LPI_ENABLED);
}

/* Get tx_lpi_timer field */
static te_errno
tx_lpi_timer_get(unsigned int gid, const char *oid, char *value,
                 const char *if_name)
{
    return common_field_get(gid, oid, value, if_name, FIELD_TX_LPI_TIMER);
}

/* Set eee_enabled field */
static te_errno
eee_enabled_set(unsigned int gid, const char *oid, const char *value,
                const char *if_name)
{
    return common_param_set(gid, oid, value, if_name, FIELD_EEE_ENABLED);
}

/* Set tx_lpi_enabled field */
static te_errno
tx_lpi_enabled_set(unsigned int gid, const char *oid, const char *value,
                   const char *if_name)
{
    return common_param_set(gid, oid, value, if_name, FIELD_TX_LPI_ENABLED);
}

/* Set tx_lpi_timer field */
static te_errno
tx_lpi_timer_set(unsigned int gid, const char *oid, const char *value,
                 const char *if_name)
{
    return common_param_set(gid, oid, value, if_name, FIELD_TX_LPI_TIMER);
}

/* Commit changes to EEE configuration */
static te_errno
eee_commit(unsigned int gid, const cfg_oid *p_oid)
{
    char *if_name;

    UNUSED(gid);
    if_name = CFG_OID_GET_INST_NAME(p_oid, 2);

    return commit_ethtool_value(if_name, gid, TA_ETHTOOL_EEE);
}

static rcf_pch_cfg_object node_eee;

RCF_PCH_CFG_NODE_RWC(node_tx_lpi_timer, "tx_lpi_timer", NULL,
                     NULL, tx_lpi_timer_get, tx_lpi_timer_set,
                     &node_eee);

RCF_PCH_CFG_NODE_RWC(node_tx_lpi_enabled, "tx_lpi_enabled", NULL,
                     &node_tx_lpi_timer, tx_lpi_enabled_get, tx_lpi_enabled_set,
                     &node_eee);

RCF_PCH_CFG_NODE_RWC(node_eee_enabled, "eee_enabled", NULL,
                     &node_tx_lpi_enabled, eee_enabled_get, eee_enabled_set,
                     &node_eee);

RCF_PCH_CFG_NODE_RO(node_eee_active, "eee_active", NULL,
                    &node_eee_enabled, eee_active_get);

RCF_PCH_CFG_NODE_NA_COMMIT(node_eee, "eee", &node_eee_active, NULL,
                           eee_commit);

/**
 * Add "eee" node to interface in configuration tree.
 *
 * @return Status code.
 */
extern te_errno
ta_unix_conf_if_eee_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_eee);
}

#else

/* See description above */
extern te_errno
ta_unix_conf_if_eee_init(void)
{
    WARN("Interface Energy Efficient Ethernet settings are not supported");
    return 0;
}
#endif
