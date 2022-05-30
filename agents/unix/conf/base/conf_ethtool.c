/** @file
 * @brief Unix Test Agent
 *
 * Implementation of common API for SIOCETHTOOL usage in Unix TA
 * configuration
 *
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#include "te_config.h"
#include "config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"

#ifdef HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

#include "unix_internal.h"
#include "conf_ethtool.h"

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H

/*
 * Information about link mode: its name, new and old native constants
 */
typedef struct link_mode_info {
    const char *name;
    int new_bit_index;
    int old_supp_flag;
    int old_adv_flag;
} link_mode_info;

/*
 * Information about link mode supported by both
 * ETHTOOL_GSET/ETHTOOL_SSET and
 * ETHTOOL_GLINKSETTINGS/ETHTOOL_SLINKSETTINGS.
 */
#define XSET_MODEINFO(_name) \
    [ TA_ETHTOOL_LINK_MODE_ ## _name ] =                          \
        { .name = #_name,                                         \
          .new_bit_index = ETHTOOL_LINK_MODE_ ## _name ## _BIT,   \
          .old_supp_flag = SUPPORTED_ ## _name,                   \
          .old_adv_flag = ADVERTISED_ ## _name }                  \

/*
 * Information about link mode supported only by
 * ETHTOOL_GLINKSETTINGS/ETHTOOL_SLINKSETTINGS.
 */
#define XLINKSETTINGS_MODEINFO(_name) \
    [ TA_ETHTOOL_LINK_MODE_ ## _name ] =                          \
        { .name = #_name,                                         \
          .new_bit_index = ETHTOOL_LINK_MODE_ ## _name ## _BIT,   \
          .old_supp_flag = TA_ETHTOOL_LINK_MODE_UNDEF,            \
          .old_adv_flag = TA_ETHTOOL_LINK_MODE_UNDEF }            \

/* Information about all known link modes */
static const link_mode_info modes_info[] = {
    XSET_MODEINFO(10baseT_Half),
    XSET_MODEINFO(10baseT_Full),
    XSET_MODEINFO(100baseT_Half),
    XSET_MODEINFO(100baseT_Full),
    XSET_MODEINFO(1000baseT_Half),
    XSET_MODEINFO(1000baseT_Full),
    XSET_MODEINFO(Autoneg),
    XSET_MODEINFO(TP),
    XSET_MODEINFO(AUI),
    XSET_MODEINFO(MII),
    XSET_MODEINFO(FIBRE),
    XSET_MODEINFO(BNC),
    XSET_MODEINFO(10000baseT_Full),
    XSET_MODEINFO(Pause),
    XSET_MODEINFO(Asym_Pause),
    XSET_MODEINFO(2500baseX_Full),
    XSET_MODEINFO(Backplane),
    XSET_MODEINFO(1000baseKX_Full),
    XSET_MODEINFO(10000baseKX4_Full),
    XSET_MODEINFO(10000baseKR_Full),
    XSET_MODEINFO(10000baseR_FEC),
    XSET_MODEINFO(20000baseMLD2_Full),
    XSET_MODEINFO(20000baseKR2_Full),
    XSET_MODEINFO(40000baseKR4_Full),
    XSET_MODEINFO(40000baseCR4_Full),
    XSET_MODEINFO(40000baseSR4_Full),
    XSET_MODEINFO(40000baseLR4_Full),
    XSET_MODEINFO(56000baseKR4_Full),
    XSET_MODEINFO(56000baseCR4_Full),
    XSET_MODEINFO(56000baseSR4_Full),
    XSET_MODEINFO(56000baseLR4_Full),

    XLINKSETTINGS_MODEINFO(25000baseCR_Full),
    XLINKSETTINGS_MODEINFO(25000baseKR_Full),
    XLINKSETTINGS_MODEINFO(25000baseSR_Full),
    XLINKSETTINGS_MODEINFO(50000baseCR2_Full),
    XLINKSETTINGS_MODEINFO(50000baseKR2_Full),
    XLINKSETTINGS_MODEINFO(100000baseKR4_Full),
    XLINKSETTINGS_MODEINFO(100000baseSR4_Full),
    XLINKSETTINGS_MODEINFO(100000baseCR4_Full),
    XLINKSETTINGS_MODEINFO(100000baseLR4_ER4_Full),
    XLINKSETTINGS_MODEINFO(50000baseSR2_Full),
    XLINKSETTINGS_MODEINFO(1000baseX_Full),
    XLINKSETTINGS_MODEINFO(10000baseCR_Full),
    XLINKSETTINGS_MODEINFO(10000baseSR_Full),
    XLINKSETTINGS_MODEINFO(10000baseLR_Full),
    XLINKSETTINGS_MODEINFO(10000baseLRM_Full),
    XLINKSETTINGS_MODEINFO(10000baseER_Full),
    XLINKSETTINGS_MODEINFO(2500baseT_Full),
    XLINKSETTINGS_MODEINFO(5000baseT_Full),
    XLINKSETTINGS_MODEINFO(FEC_NONE),
    XLINKSETTINGS_MODEINFO(FEC_RS),
    XLINKSETTINGS_MODEINFO(FEC_BASER),
    XLINKSETTINGS_MODEINFO(50000baseKR_Full),
    XLINKSETTINGS_MODEINFO(50000baseSR_Full),
    XLINKSETTINGS_MODEINFO(50000baseCR_Full),
    XLINKSETTINGS_MODEINFO(50000baseLR_ER_FR_Full),
    XLINKSETTINGS_MODEINFO(50000baseDR_Full),
    XLINKSETTINGS_MODEINFO(100000baseKR2_Full),
    XLINKSETTINGS_MODEINFO(100000baseSR2_Full),
    XLINKSETTINGS_MODEINFO(100000baseCR2_Full),
    XLINKSETTINGS_MODEINFO(100000baseLR2_ER2_FR2_Full),
    XLINKSETTINGS_MODEINFO(100000baseDR2_Full),
    XLINKSETTINGS_MODEINFO(200000baseKR4_Full),
    XLINKSETTINGS_MODEINFO(200000baseSR4_Full),
    XLINKSETTINGS_MODEINFO(200000baseLR4_ER4_FR4_Full),
    XLINKSETTINGS_MODEINFO(200000baseDR4_Full),
    XLINKSETTINGS_MODEINFO(200000baseCR4_Full),
    XLINKSETTINGS_MODEINFO(100baseT1_Full),
    XLINKSETTINGS_MODEINFO(1000baseT1_Full),
    XLINKSETTINGS_MODEINFO(400000baseKR8_Full),
    XLINKSETTINGS_MODEINFO(400000baseSR8_Full),
    XLINKSETTINGS_MODEINFO(400000baseLR8_ER8_FR8_Full),
    XLINKSETTINGS_MODEINFO(400000baseDR8_Full),
    XLINKSETTINGS_MODEINFO(400000baseCR8_Full),
    XLINKSETTINGS_MODEINFO(FEC_LLRS),
    XLINKSETTINGS_MODEINFO(100000baseKR_Full),
    XLINKSETTINGS_MODEINFO(100000baseSR_Full),
    XLINKSETTINGS_MODEINFO(100000baseLR_ER_FR_Full),
    XLINKSETTINGS_MODEINFO(100000baseCR_Full),
    XLINKSETTINGS_MODEINFO(100000baseDR_Full),
    XLINKSETTINGS_MODEINFO(200000baseKR2_Full),
    XLINKSETTINGS_MODEINFO(200000baseSR2_Full),
    XLINKSETTINGS_MODEINFO(200000baseLR2_ER2_FR2_Full),
    XLINKSETTINGS_MODEINFO(200000baseDR2_Full),
    XLINKSETTINGS_MODEINFO(200000baseCR2_Full),
    XLINKSETTINGS_MODEINFO(400000baseKR4_Full),
    XLINKSETTINGS_MODEINFO(400000baseSR4_Full),
    XLINKSETTINGS_MODEINFO(400000baseLR4_ER4_FR4_Full),
    XLINKSETTINGS_MODEINFO(400000baseDR4_Full),
    XLINKSETTINGS_MODEINFO(400000baseCR4_Full),
    XLINKSETTINGS_MODEINFO(100baseFX_Half),
    XLINKSETTINGS_MODEINFO(100baseFX_Full)
};

/* Get information about a specific link mode */
static const link_mode_info *
get_mode_info(ta_ethtool_link_mode mode)
{
    const link_mode_info *result;

    if (mode < 0 || mode >= TE_ARRAY_LEN(modes_info))
        return NULL;

    result = &modes_info[mode];
    if (result->name == NULL)
    {
        /* The element in array was not initialized */
        return NULL;
    }

    return result;
}

/**
 * Get string representation of ethtool command.
 *
 * @param cmd     Ethtool command number
 *
 * @return String representation.
 */
static const char *
ethtool_cmd2str(int cmd)
{
#define CASE_CMD(_cmd) \
    case _cmd:               \
        return #_cmd

    switch (cmd)
    {
        CASE_CMD(ETHTOOL_GCOALESCE);
        CASE_CMD(ETHTOOL_SCOALESCE);
        CASE_CMD(ETHTOOL_GPAUSEPARAM);
        CASE_CMD(ETHTOOL_SPAUSEPARAM);
        CASE_CMD(ETHTOOL_GSET);
        CASE_CMD(ETHTOOL_SSET);
        CASE_CMD(ETHTOOL_GLINKSETTINGS);
        CASE_CMD(ETHTOOL_SLINKSETTINGS);
    }

    return "<UNKNOWN>";
#undef CASE_CMD
}

/* Initialize ifreq structure, fill interface name */
static te_errno
prepare_ifr(struct ifreq *ifr, const char *if_name)
{
    te_errno rc;

    memset(ifr, 0, sizeof(*ifr));

    rc = te_snprintf(ifr->ifr_name, sizeof(ifr->ifr_name),
                     "%s", if_name);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

/* Call SIOCETHTOOL ioctl() */
static te_errno
call_ioctl(struct ifreq *ifr, const char *if_name, int cmd)
{
    int rc;

    rc = ioctl(cfg_socket, SIOCETHTOOL, ifr);
    if (rc < 0)
    {
        /*
         * Avoid extra logs if this command is simply not supported
         * for a given interface.
         */
        if (errno != EOPNOTSUPP)
        {
            ERROR("%s(if_name=%s, cmd=%s): ioctl() failed", __FUNCTION__,
                  if_name, ethtool_cmd2str(cmd));
        }
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/* See description in conf_ethtool.h */
te_errno
call_ethtool_ioctl(const char *if_name, int cmd, void *value)
{
    struct ifreq ifr;
    te_errno rc;

    rc = prepare_ifr(&ifr, if_name);
    if (rc != 0)
        return rc;

    *(uint32_t *)value = cmd;
    ifr.ifr_data = (caddr_t)value;

    return call_ioctl(&ifr, if_name, cmd);
}

/*
 * Try to fill link settings structure with ETHTOOL_GLINKSETTINGS.
 * If that command is not defined or fails, use ETHTOOL_GSET instead.
 */
static te_errno
get_ethtool_lsets(const char *if_name, ta_ethtool_lsets *lsets)
{
    struct ifreq ifr;
    te_errno rc;

    struct ethtool_cmd *native_cmd;

#ifdef ETHTOOL_GLINKSETTINGS
    struct ethtool_link_settings *native_lsets;
    int nwords;
#endif

    rc = prepare_ifr(&ifr, if_name);
    if (rc != 0)
        return rc;

#ifdef ETHTOOL_GLINKSETTINGS
    do {
        memset(lsets, 0, sizeof(*lsets));
        native_lsets = &lsets->sets.xlinksettings.fields;
        native_lsets->cmd = ETHTOOL_GLINKSETTINGS;
        ifr.ifr_data = (caddr_t)native_lsets;

        rc = call_ioctl(&ifr, if_name, ETHTOOL_GLINKSETTINGS);
        if (rc != 0)
            break;

        if (native_lsets->link_mode_masks_nwords >= 0)
        {
            ERROR("%s(): initial ETHTOOL_GLINKSETTINGS succeeds but "
                  "link_mode_masks_nwords is %d instead of a negative "
                  "value telling the required mask size", __FUNCTION__,
                  native_lsets->link_mode_masks_nwords);
            return TE_OS_RC(TE_TA_UNIX, TE_EINVAL);
        }

        nwords = -native_lsets->link_mode_masks_nwords;
        if (nwords > TA_ETHTOOL_LMODE_MASK_WORDS_MAX)
        {
            ERROR("%s(): ETHTOOL_GLINKSETTINGS reports too big "
                  "link_mode_masks_nwords %d", __FUNCTION__, nwords);
            return TE_OS_RC(TE_TA_UNIX, TE_EOVERFLOW);
        }

        native_lsets->link_mode_masks_nwords = nwords;

        rc = call_ioctl(&ifr, if_name, ETHTOOL_GLINKSETTINGS);
        if (rc != 0)
            return rc;

        lsets->use_xlinksettings = TRUE;
        return 0;
    } while (FALSE);
#endif

    memset(lsets, 0, sizeof(*lsets));
    lsets->use_xlinksettings = FALSE;
    native_cmd = &lsets->sets.xset;
    native_cmd->cmd = ETHTOOL_GSET;
    ifr.ifr_data = (caddr_t)native_cmd;
    return call_ioctl(&ifr, if_name, ETHTOOL_GSET);
}

/*
 * Set link settings using a previously obtained link settings
 * structure. If the structure was obtained with ETHTOOL_GLINKSETTINGS,
 * then ETHTOOL_SLINKSETTINGS is used, otherwise - ETHTOOL_SSET.
 */
static te_errno
set_ethtool_lsets(const char *if_name, ta_ethtool_lsets *lsets)
{
    void *cmd_data;
    int native_cmd;

    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_SLINKSETTINGS
        native_cmd = ETHTOOL_SLINKSETTINGS;
        cmd_data = &lsets->sets.xlinksettings.fields;
#else
        ERROR("%s(): ETHTOOL_SLINKSETTINGS should be used but "
              "it is not defined", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
#endif
    }
    else
    {
        native_cmd = ETHTOOL_SSET;
        cmd_data = &lsets->sets.xset;
    }

    return call_ethtool_ioctl(if_name, native_cmd, cmd_data);
}

/* See description in conf_ethtool.h */
te_errno
get_ethtool_value(const char *if_name, unsigned int gid,
                  ta_ethtool_cmd cmd, void *val_local, void **ptr_out,
                  te_bool do_set)
{
    ta_cfg_obj_t *obj;
    te_errno rc;

    const char *obj_type;
    size_t val_size;
    int native_cmd = 0;

    switch (cmd)
    {
        case TA_ETHTOOL_COALESCE:
            obj_type = TA_OBJ_TYPE_IF_COALESCE;
            val_size = sizeof(struct ethtool_coalesce);
            native_cmd = ETHTOOL_GCOALESCE;
            break;

        case TA_ETHTOOL_PAUSEPARAM:
            obj_type = TA_OBJ_TYPE_IF_PAUSE;
            val_size = sizeof(struct ethtool_pauseparam);
            native_cmd = ETHTOOL_GPAUSEPARAM;
            break;

        case TA_ETHTOOL_LINKSETTINGS:
            obj_type = TA_OBJ_TYPE_IF_LINK_SETS;
            val_size = sizeof(ta_ethtool_lsets);
            break;

        default:
            ERROR("%s(): unknown ethtool command %d", __FUNCTION__, cmd);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    obj = ta_obj_find(obj_type, if_name, gid);
    if (obj != NULL)
    {
        *ptr_out = obj->user_data;
    }
    else
    {
        memset(val_local, 0, val_size);

        if (cmd == TA_ETHTOOL_LINKSETTINGS)
            rc = get_ethtool_lsets(if_name, val_local);
        else
            rc = call_ethtool_ioctl(if_name, native_cmd, val_local);

        if (rc != 0)
            return rc;

        if (do_set)
        {
            *ptr_out = calloc(val_size, 1);
            if (*ptr_out == NULL)
            {
                ERROR("%s(): not enough memory", __FUNCTION__);
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            }

            rc = ta_obj_add(obj_type, if_name, "", gid,
                            *ptr_out, &free, NULL);
            if (rc != 0)
            {
                ERROR("%s(): failed to add a new object", __FUNCTION__);
                free(*ptr_out);
                *ptr_out = NULL;
                return TE_RC(TE_TA_UNIX, rc);
            }

            memcpy(*ptr_out, val_local, val_size);
        }
        else
        {
            *ptr_out = val_local;
        }
    }

    return 0;
}

/* See description in conf_ethtool.h */
te_errno
commit_ethtool_value(const char *if_name, unsigned int gid,
                     ta_ethtool_cmd cmd)
{
    const char *obj_type;
    ta_cfg_obj_t *obj;
    te_errno rc;
    int native_cmd;

    switch (cmd)
    {
        case TA_ETHTOOL_COALESCE:
            obj_type = TA_OBJ_TYPE_IF_COALESCE;
            native_cmd = ETHTOOL_SCOALESCE;
            break;

        case TA_ETHTOOL_PAUSEPARAM:
            obj_type = TA_OBJ_TYPE_IF_PAUSE;
            native_cmd = ETHTOOL_SPAUSEPARAM;
            break;

        case TA_ETHTOOL_LINKSETTINGS:
            obj_type = TA_OBJ_TYPE_IF_LINK_SETS;
            break;

        default:
            ERROR("%s(): unknown ethtool command %d", __FUNCTION__, cmd);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    obj = ta_obj_find(obj_type, if_name, gid);
    if (obj == NULL)
    {
        ERROR("%s(): object of type '%s' was not found for "
              "interface '%s'", __FUNCTION__, obj_type, if_name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (cmd == TA_ETHTOOL_LINKSETTINGS)
        rc = set_ethtool_lsets(if_name, obj->user_data);
    else
        rc = call_ethtool_ioctl(if_name, native_cmd, obj->user_data);

    ta_obj_free(obj);

    return rc;
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lsets_field_get(ta_ethtool_lsets *lsets,
                           ta_ethtool_lsets_field field,
                           unsigned int *value)
{
    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_GLINKSETTINGS
        switch (field)
        {
            case TA_ETHTOOL_LSETS_AUTONEG:
                *value = lsets->sets.xlinksettings.fields.autoneg;
                break;

            case TA_ETHTOOL_LSETS_SPEED:
                *value = lsets->sets.xlinksettings.fields.speed;
                break;

            case TA_ETHTOOL_LSETS_DUPLEX:
                *value = lsets->sets.xlinksettings.fields.duplex;
                break;
        }
#else
        ERROR("%s(): ETHTOOL_GLINKSETTINGS is not defined but "
              "use_xlinksettings is set to TRUE", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    }
    else
    {
        switch (field)
        {
            case TA_ETHTOOL_LSETS_AUTONEG:
                *value = lsets->sets.xset.autoneg;
                break;

            case TA_ETHTOOL_LSETS_SPEED:
                *value = lsets->sets.xset.speed;
                break;

            case TA_ETHTOOL_LSETS_DUPLEX:
                *value = lsets->sets.xset.duplex;
                break;
        }
    }

    return 0;
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lsets_field_set(ta_ethtool_lsets *lsets,
                           ta_ethtool_lsets_field field,
                           unsigned int value)
{
    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_GLINKSETTINGS
        switch (field)
        {
            case TA_ETHTOOL_LSETS_AUTONEG:
                lsets->sets.xlinksettings.fields.autoneg = value;
                break;

            case TA_ETHTOOL_LSETS_SPEED:
                lsets->sets.xlinksettings.fields.speed = value;
                break;

            case TA_ETHTOOL_LSETS_DUPLEX:
                lsets->sets.xlinksettings.fields.duplex = value;
                break;
        }
#else
        ERROR("%s(): ETHTOOL_GLINKSETTINGS is not defined but "
              "use_xlinksettings is set to TRUE", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    }
    else
    {
        switch (field)
        {
            case TA_ETHTOOL_LSETS_AUTONEG:
                lsets->sets.xset.autoneg = value;
                break;

            case TA_ETHTOOL_LSETS_SPEED:
                lsets->sets.xset.speed = value;
                break;

            case TA_ETHTOOL_LSETS_DUPLEX:
                lsets->sets.xset.duplex = value;
                break;
        }
    }

    return 0;
}

/* See description in conf_ethtool.h */
const char *
ta_ethtool_lmode_name(ta_ethtool_link_mode mode)
{
    const link_mode_info *info;

    info = get_mode_info(mode);
    if (info == NULL)
        return "<UNKNOWN>";

    return info->name;
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_parse(const char *name,
                       ta_ethtool_link_mode *mode)
{
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(modes_info); i++)
    {
        if (modes_info[i].name != NULL &&
            strcmp(modes_info[i].name, name) == 0)
        {
            *mode = i;
            return 0;
        }
    }

    ERROR("%s(): link mode '%s' is not known", __FUNCTION__, name);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/*
 * Link modes masks. Numeric values here should correspond
 * to the index of the mask in a list of masks after
 * ethtool_link_settings structure.
 */
/* Mask of supported link modes */
#define SUPPORTED_MASK_ID 0
/* Mask of advertised link modes */
#define ADVERTISING_MASK_ID 1
/* Mask of link modes advertised by link partner */
#define LP_ADVERTISING_MASK_ID 2

/*
 * Get native link mode flag for deprecated ETHTOOL_GSET/ETHTOOL_SSET.
 */
static unsigned int
get_old_mode(ta_ethtool_link_mode mode,
             unsigned int mask_id)
{
    const link_mode_info *info;

    info = get_mode_info(mode);
    if (info != NULL)
    {
        if (mask_id == SUPPORTED_MASK_ID)
            return info->old_supp_flag;
        else
            return info->old_adv_flag;
    }

    return TA_ETHTOOL_LINK_MODE_UNDEF;
}

/*
 * Perform link mode get or set operation on link settings structure
 * filled with ETHTOOL_GSET.
 */
static te_errno
old_lmode_op(ta_ethtool_lsets *lsets,
             unsigned int mask_id,
             ta_ethtool_link_mode mode,
             te_bool do_set,
             te_bool *value)
{
    uint32_t *mask;
    unsigned int native_mode;
    te_errno rc;

    native_mode = get_old_mode(mode, mask_id);
    if (native_mode == TA_ETHTOOL_LINK_MODE_UNDEF)
    {
        if (!do_set)
        {
            /*
             * Ignore not defined native link modes for get operation.
             */
            *value = FALSE;
            rc = 0;
        }
        else
        {
            ERROR("%s(): link mode %u (%s) is not defined", __FUNCTION__,
                  mode, ta_ethtool_lmode_name(mode));
            rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        return rc;
    }

    switch (mask_id)
    {
        case SUPPORTED_MASK_ID:
            mask = &lsets->sets.xset.supported;
            break;

        case ADVERTISING_MASK_ID:
            mask = &lsets->sets.xset.advertising;
            break;

        case LP_ADVERTISING_MASK_ID:
            mask = &lsets->sets.xset.lp_advertising;
            break;

        default:
            ERROR("%s(): unknown mask id %d", __FUNCTION__, mask_id);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (do_set)
    {
        if (*value)
            *mask = *mask | native_mode;
        else
            *mask = *mask & ~native_mode;
    }
    else
    {
        if (*mask & native_mode)
            *value = TRUE;
        else
            *value = FALSE;
    }

    return 0;
}

#ifdef ETHTOOL_GLINKSETTINGS
/*
 * Get native link mode bit index for ETHTOOL_GLINKSETTINGS and
 * ETHTOOL_SLINKSETTINGS.
 */
static unsigned int
get_new_mode(ta_ethtool_link_mode mode)
{
    const link_mode_info *info;

    info = get_mode_info(mode);
    if (info != NULL)
        return info->new_bit_index;

    return TA_ETHTOOL_LINK_MODE_UNDEF;
}

/*
 * Perform link mode get or set operation on link settings structure
 * filled with ETHTOOL_GLINKSETTINGS.
 */
static te_errno
new_lmode_op(ta_ethtool_lsets *lsets,
             unsigned int mask_id,
             ta_ethtool_link_mode mode,
             te_bool do_set,
             te_bool *value)
{
    unsigned int native_mode;
    unsigned int nword;
    unsigned int flag;
    unsigned int *word;
    te_errno rc;

    unsigned int masks_nwords;

    native_mode = get_new_mode(mode);
    if (native_mode == TA_ETHTOOL_LINK_MODE_UNDEF)
    {
        if (!do_set)
        {
            /*
             * Ignore not defined native link modes for get operation.
             */
            *value = FALSE;
            rc = 0;
        }
        else
        {
            ERROR("%s(): link mode %u (%s) is not defined", __FUNCTION__,
                  mode, ta_ethtool_lmode_name(mode));
            rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        return rc;
    }

    masks_nwords = lsets->sets.xlinksettings.fields.link_mode_masks_nwords;

    nword = native_mode / 32;
    if (nword >= masks_nwords)
    {
        ERROR("%s(): link mode %s cannot fit into mask",
              __FUNCTION__, ta_ethtool_lmode_name(mode));
    }

    flag = native_mode % 32;

    word = lsets->sets.xlinksettings.fields.link_mode_masks +
           mask_id * masks_nwords + nword;

    if (do_set)
    {
        if (*value)
            *word = *word | (1 << flag);
        else
            *word = *word & ~(1 << flag);
    }
    else
    {
        if (*word & (1 << flag))
            *value = TRUE;
        else
            *value = FALSE;
    }

    return 0;
}

#endif

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_supported(ta_ethtool_lsets *lsets,
                           ta_ethtool_link_mode mode,
                           te_bool *supported)
{
    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_GLINKSETTINGS
        return new_lmode_op(lsets, SUPPORTED_MASK_ID,
                            mode, FALSE, supported);
#else
        ERROR("%s(): ETHTOOL_GLINKSETTINGS is not defined but "
              "use_xlinksettings is set to TRUE", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    }
    else
    {
        return old_lmode_op(lsets, SUPPORTED_MASK_ID,
                            mode, FALSE, supported);
    }
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_advertised(ta_ethtool_lsets *lsets,
                            ta_ethtool_link_mode mode,
                            te_bool *advertised)
{
    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_GLINKSETTINGS
        return new_lmode_op(lsets, ADVERTISING_MASK_ID,
                            mode, FALSE, advertised);
#else
        ERROR("%s(): ETHTOOL_GLINKSETTINGS is not defined but "
              "use_xlinksettings is set to TRUE", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    }
    else
    {
        return old_lmode_op(lsets, ADVERTISING_MASK_ID,
                            mode, FALSE, advertised);
    }
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_lp_advertised(ta_ethtool_lsets *lsets,
                               ta_ethtool_link_mode mode,
                               te_bool *lp_advertised)
{
    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_GLINKSETTINGS
        return new_lmode_op(lsets, LP_ADVERTISING_MASK_ID,
                            mode, FALSE, lp_advertised);
#else
        ERROR("%s(): ETHTOOL_GLINKSETTINGS is not defined but "
              "use_xlinksettings is set to TRUE", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    }
    else
    {
        return old_lmode_op(lsets, LP_ADVERTISING_MASK_ID,
                            mode, FALSE, lp_advertised);
    }
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_advertise(ta_ethtool_lsets *lsets,
                           ta_ethtool_link_mode mode,
                           te_bool enable)
{
    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_GLINKSETTINGS
        return new_lmode_op(lsets, ADVERTISING_MASK_ID,
                            mode, TRUE, &enable);
#else
        ERROR("%s(): ETHTOOL_GLINKSETTINGS is not defined but "
              "use_xlinksettings is set to TRUE", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    }
    else
    {
        return old_lmode_op(lsets, ADVERTISING_MASK_ID,
                            mode, TRUE, &enable);
    }
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_list_names(ta_ethtool_lsets *lsets,
                            te_bool link_partner,
                            te_string *list_str)
{
    size_t i;
    te_bool enabled;
    te_errno rc;

    for (i = 0; i < TE_ARRAY_LEN(modes_info); i++)
    {
        if (modes_info[i].name == NULL)
            continue;

        if (link_partner)
            rc = ta_ethtool_lmode_lp_advertised(lsets, i, &enabled);
        else
            rc = ta_ethtool_lmode_supported(lsets, i, &enabled);

        if (rc != 0)
            return rc;

        if (enabled)
        {
            rc = te_string_append(list_str, "%s ", modes_info[i].name);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

#endif
