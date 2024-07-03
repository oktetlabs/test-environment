/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Implementation of common API for SIOCETHTOOL usage in Unix TA
 * configuration
 *
 *
 * Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#include "config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "te_alloc.h"
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
#include "conf_common.h"

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H

/* The last failed Ethtool command number */
static int TE_THREAD_LOCAL failed_ethtool_cmd = -1;

/* Maximum length of object name */
#define MAX_OBJ_NAME_LEN 1024

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

/* See description in conf_ethtool.h */
int
ta_ethtool_failed_cmd(void)
{
    return failed_ethtool_cmd;
}

/* See description in conf_ethtool.h */
void
ta_ethtool_reset_failed_cmd(void)
{
    failed_ethtool_cmd = -1;
}

/* See description in conf_ethtool.h */
const char *
ta_ethtool_cmd2str(int cmd)
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
        CASE_CMD(ETHTOOL_GSSET_INFO);
        CASE_CMD(ETHTOOL_GSTRINGS);
#ifdef ETHTOOL_GRSSH
        CASE_CMD(ETHTOOL_GRSSH);
#endif
#ifdef ETHTOOL_SRSSH
        CASE_CMD(ETHTOOL_SRSSH);
#endif
        CASE_CMD(ETHTOOL_GPFLAGS);
        CASE_CMD(ETHTOOL_SPFLAGS);
#ifdef ETHTOOL_GRXCLSRLCNT
        CASE_CMD(ETHTOOL_GRXCLSRLCNT);
#endif
#ifdef ETHTOOL_GRXCLSRLALL
        CASE_CMD(ETHTOOL_GRXCLSRLALL);
#endif
#ifdef ETHTOOL_GRXCLSRULE
        CASE_CMD(ETHTOOL_GRXCLSRULE);
#endif
#ifdef ETHTOOL_SRXCLSRLINS
        CASE_CMD(ETHTOOL_SRXCLSRLINS);
#endif
#ifdef ETHTOOL_SRXCLSRLDEL
        CASE_CMD(ETHTOOL_SRXCLSRLDEL);
#endif
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
    te_errno te_err;

    rc = ioctl(cfg_socket, SIOCETHTOOL, ifr);
    if (rc < 0)
    {
        failed_ethtool_cmd = cmd;
        te_err = te_rc_os2te(errno);
        return TE_RC(TE_TA_UNIX, te_err);
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

#ifdef ETHTOOL_SLINKSETTINGS
/*
 * Check whether ETHTOOL_SLINKSETTINGS is supported for
 * an interface.
 */
static void
check_slinksettings_support(const char *if_name,
                            ta_ethtool_lsets *lsets)
{
    struct ifreq ifr;
    struct ethtool_link_settings native_lsets;
    int rc;
    te_errno te_rc;

    /*
     * This produces incorrectly filled settings which
     * will be rejected by Linux kernel. Interface should
     * not be affected in any way. If driver does not
     * provide set_link_ksettings() callback, EOPNOTSUPP
     * is obtained; otherwise - EINVAL.
     */
    memset(&native_lsets, 0, sizeof(native_lsets));
    native_lsets.cmd = ETHTOOL_SLINKSETTINGS;

    te_rc = prepare_ifr(&ifr, if_name);
    if (te_rc != 0)
        return;

    ifr.ifr_data = (caddr_t)&native_lsets;

    rc = ioctl(cfg_socket, SIOCETHTOOL, &ifr);
    if (rc < 0 && errno == EOPNOTSUPP)
    {
        lsets->set_supported = false;
    }
    else if (rc >= 0)
    {
        ERROR("%s(if_name=%s): ioctl() succeeded with incorrect "
              "request", __FUNCTION__, if_name);
    }
}
#endif

/*
 * Check whether ETHTOOL_SSET is supported for an interface.
 */
static void
check_sset_support(const char *if_name, ta_ethtool_lsets *lsets)
{
    te_errno rc;
    char value[RCF_MAX_VAL] = "";
    static const char *known_kinds[] = { "vlan", "bond", "team",
                                         "ipvlan", "macvlan" };
    unsigned int i;

    /*
     * There is no good way to check whether deprecated ETHTOOL_SSET
     * works without potentially affecting network interface.
     * So just check for known interface kinds here.
     */

    rc = get_interface_kind(if_name, value);
    if (rc == 0)
    {
        for (i = 0; i < TE_ARRAY_LEN(known_kinds); i++)
        {
            if (strcmp(value, known_kinds[i]) == 0)
            {
                lsets->set_supported = false;
                break;
            }
        }
    }
}

/*
 * Check whether changing link settings is supported by the driver.
 */
static void
lsets_check_set_support(const char *if_name,
                        ta_ethtool_lsets *lsets)
{
    lsets->set_supported = true;

    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_SLINKSETTINGS
        check_slinksettings_support(if_name, lsets);
#else
        ERROR("%s(if_name=%s): ETHTOOL_SLINKSETTINGS should be used but "
              "it is not defined", __FUNCTION__, if_name);
#endif
    }
    else
    {
        check_sset_support(if_name, lsets);
    }
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

        lsets->use_xlinksettings = true;
        return 0;
    } while (false);
#endif

    memset(lsets, 0, sizeof(*lsets));
    lsets->use_xlinksettings = false;
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
                  ta_ethtool_cmd cmd, void **ptr_out)
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

        case TA_ETHTOOL_PFLAGS:
            obj_type = TA_OBJ_TYPE_IF_PFLAGS;
            val_size = sizeof(struct ethtool_value);
            native_cmd = ETHTOOL_GPFLAGS;
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
        *ptr_out = calloc(val_size, 1);
        if (*ptr_out == NULL)
        {
            ERROR("%s(): not enough memory", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }

        if (cmd == TA_ETHTOOL_LINKSETTINGS)
        {
            rc = get_ethtool_lsets(if_name, *ptr_out);
            if (rc == 0)
                lsets_check_set_support(if_name, *ptr_out);
        }
        else
            rc = call_ethtool_ioctl(if_name, native_cmd, *ptr_out);

        if (rc != 0)
        {
            free(*ptr_out);
            *ptr_out = NULL;
            return rc;
        }

        rc = ta_obj_add(obj_type, if_name, "", gid,
                        *ptr_out, &free, &obj);
        if (rc != 0)
        {
            ERROR("%s(): failed to add a new object", __FUNCTION__);
            free(*ptr_out);
            *ptr_out = NULL;
            return TE_RC(TE_TA_UNIX, rc);
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

        case TA_ETHTOOL_PFLAGS:
            obj_type = TA_OBJ_TYPE_IF_PFLAGS;
            native_cmd = ETHTOOL_SPFLAGS;
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

te_errno
ta_ethtool_get_strings(unsigned int gid, const char *if_name,
                       unsigned int set_id, const ta_ethtool_strings **strs)
{
    te_errno rc;
    te_string obj_name = TE_STRING_INIT_STATIC(MAX_OBJ_NAME_LEN);
    size_t req_size;
    size_t strs_num;
    struct ethtool_gstrings *strings = NULL;
    ta_ethtool_strings *result;
    ta_cfg_obj_t *obj;
    unsigned int i;

    /*
     * FIXME: this comment was moved from conf_eth.c, have no idea
     * what it means.
     * The data buffer definition in the structure below follows the
     * same approach as one used in Ethtool application, although that
     * approach seems to be unreliable under any standard except the GNU C
     */
    struct {
        struct ethtool_sset_info hdr;
        uint32_t buf[1];
    } sset_info;

    rc = te_string_append(&obj_name, "%s.%u",
                          if_name, set_id);
    if (rc != 0)
        return rc;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_STRINGS, te_string_value(&obj_name),
                      gid);
    if (obj != NULL)
    {
        *strs = obj->user_data;
        return 0;
    }

    memset(&sset_info, 0, sizeof(sset_info));
    sset_info.hdr.cmd = ETHTOOL_GSSET_INFO;
    sset_info.hdr.sset_mask = 1ULL << set_id;

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GSSET_INFO, &sset_info);
    if (rc != 0)
        return rc;

    strs_num = sset_info.hdr.data[0];
    req_size = sizeof(struct ethtool_gstrings) +
                            ETH_GSTRING_LEN * strs_num;

    strings = TE_ALLOC(req_size);

    strings->cmd = ETHTOOL_GSTRINGS;
    strings->string_set = set_id;
    strings->len = strs_num;

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GSTRINGS, strings);
    if (rc != 0)
    {
        free(strings);
        return rc;
    }

    req_size = sizeof(ta_ethtool_strings) +
               (ETH_GSTRING_LEN + 1) * strs_num;

    result = TE_ALLOC(req_size);

    result->num = strs_num;
    for (i = 0; i < strs_num; i++)
    {
        strncpy(result->strings[i],
                (char *)(strings->data + i * ETH_GSTRING_LEN),
                ETH_GSTRING_LEN);
        result->strings[i][ETH_GSTRING_LEN] = '\0';
    }
    free(strings);

    rc = ta_obj_add(TA_OBJ_TYPE_IF_STRINGS, te_string_value(&obj_name),
                    "", gid, result, &free, &obj);
    if (rc != 0)
    {
        free(result);
        return rc;
    }

    *strs = result;
    return rc;
}

/* see description in conf_ethtool.h */
te_errno
ta_ethtool_get_strings_list(unsigned int gid, const char *if_name,
                            unsigned int set_id, char **list_out)
{
    const struct ta_ethtool_strings *sset = NULL;
    unsigned int i;
    te_errno rc;
    te_string str = TE_STRING_INIT;

    rc = ta_ethtool_get_strings(gid, if_name, set_id, &sset);
    if (rc != 0)
        return rc;

    for (i = 0; i < sset->num; i++)
    {
        rc = te_string_append_chk(&str, "%s ", sset->strings[i]);
        if (rc != 0)
        {
            te_string_free(&str);
            return rc;
        }
    }

    *list_out = str.ptr;
    return 0;
}

/* see description in conf_ethtool.h */
te_errno
ta_ethtool_get_string_idx(unsigned int gid, const char *if_name,
                          unsigned int set_id, const char *target,
                          unsigned int *idx)
{
    const ta_ethtool_strings *sset = NULL;
    unsigned int i;
    te_errno rc;

    rc = ta_ethtool_get_strings(gid, if_name, set_id, &sset);
    if (rc != 0)
        return rc;

    for (i = 0; i < sset->num; i++)
    {
        if (strcmp(target, sset->strings[i]) == 0)
        {
            *idx = i;
            return 0;
        }
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

#ifdef ETHTOOL_GRSSH

static te_errno
get_ethtool_rssh(const char *if_name, unsigned int rss_context,
                 struct ethtool_rxfh **data)
{
    struct ethtool_rxfh sizes;
    struct ethtool_rxfh *full_data;
    te_errno rc = 0;
    size_t new_size;

    memset(&sizes, 0, sizeof(sizes));

    sizes.rss_context = rss_context;

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GRSSH, &sizes);
    if (rc != 0)
        return rc;

    new_size = sizeof(struct ethtool_rxfh) +
               sizes.indir_size * sizeof(uint32_t) + sizes.key_size;
    full_data = TE_ALLOC(new_size);

    full_data->rss_context = rss_context;
    full_data->indir_size = sizes.indir_size;
    full_data->key_size = sizes.key_size;

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GRSSH, full_data);
    if (rc != 0)
        return rc;

    *data = full_data;
    return 0;
}

/* Fill object name for ETHTOOL_GRSSH/ETHTOOL_SRSSH */
static te_errno
rssh_object_name(te_string *str, const char *if_name,
                 unsigned int rss_context)
{
    return te_string_append(str, "%s.%u",
                            if_name, rss_context);
}

/* Release memory allocated for ta_ethtool_rxfh structure */
static void
free_ta_ethtool_rxfh(void *data)
{
    ta_ethtool_rxfh *ta_rxfh = data;

    free(ta_rxfh->rxfh);
    free(ta_rxfh);
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_get_rssh(unsigned int gid, const char *if_name,
                    unsigned int rss_context,
                    ta_ethtool_rxfh **rxfh)
{
    te_errno rc;
    te_string obj_name = TE_STRING_INIT_STATIC(MAX_OBJ_NAME_LEN);
    ta_ethtool_rxfh *result = NULL;
    struct ethtool_rxfh *got_rxfh = NULL;
    ta_cfg_obj_t *obj;

    rc = rssh_object_name(&obj_name, if_name, rss_context);
    if (rc != 0)
        return rc;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_RSSH, te_string_value(&obj_name),
                      gid);
    if (obj != NULL)
    {
        *rxfh = obj->user_data;
        return 0;
    }

    rc = get_ethtool_rssh(if_name, rss_context, &got_rxfh);
    if (rc != 0)
    {
        free(got_rxfh);
        return rc;
    }

    result = TE_ALLOC(sizeof(*result));
    result->rxfh = got_rxfh;
    result->indir_change = false;
    result->indir_reset = false;

    rc = ta_obj_add(TA_OBJ_TYPE_IF_RSSH, te_string_value(&obj_name),
                    "", gid, result, &free_ta_ethtool_rxfh, &obj);
    if (rc != 0)
    {
        free_ta_ethtool_rxfh(result);
        return rc;
    }

    *rxfh = result;
    return 0;
}

#ifdef ETHTOOL_SRSSH
te_errno
ta_ethtool_commit_rssh(unsigned int gid, const char *if_name,
                       unsigned int rss_context)
{
    te_errno rc;
    te_string obj_name = TE_STRING_INIT_STATIC(MAX_OBJ_NAME_LEN);
    ta_cfg_obj_t *obj;
    ta_ethtool_rxfh *ta_rxfh;
    bool remove_indir_data = false;
    uint32_t orig_indir_size;

    rc = rssh_object_name(&obj_name, if_name, rss_context);
    if (rc != 0)
        return rc;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_RSSH, te_string_value(&obj_name),
                      gid);
    if (obj == NULL)
    {
        ERROR("%s(): failed to find object %s",
              __FUNCTION__, te_string_value(&obj_name));
        return TE_ENOENT;
    }

    ta_rxfh = (ta_ethtool_rxfh *)(obj->user_data);
    orig_indir_size = ta_rxfh->rxfh->indir_size;

#ifdef ETH_RXFH_INDIR_NO_CHANGE
    if (!ta_rxfh->indir_change)
    {
        ta_rxfh->rxfh->indir_size = ETH_RXFH_INDIR_NO_CHANGE;
        remove_indir_data = true;
    }
#endif
    if (ta_rxfh->indir_reset)
    {
        ta_rxfh->rxfh->indir_size = 0;
        remove_indir_data = true;
    }

    if (remove_indir_data)
    {
        unsigned int i;
        uint8_t *p = (uint8_t *)(ta_rxfh->rxfh->rss_config);
        unsigned int offs = orig_indir_size * sizeof(uint32_t);

        /*
         * If RSS indirection table data should not be passed in set
         * request (because the table is reset to default or is not
         * changed), we should move hash key data to the beginning of
         * rss_config.
         */
        if (offs > 0)
        {
            for (i = 0; i < ta_rxfh->rxfh->key_size; i++)
                p[i] = p[i + offs];
        }
    }

    rc = call_ethtool_ioctl(if_name, ETHTOOL_SRSSH, ta_rxfh->rxfh);
    ta_obj_free(obj);
    return rc;
}
#endif

#endif

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
             bool do_set,
             bool *value)
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
            *value = false;
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
            *value = true;
        else
            *value = false;
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
             bool do_set,
             bool *value)
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
            *value = false;
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
        if (do_set)
        {
            ERROR("%s(): link mode %s cannot fit into mask",
                  __FUNCTION__, ta_ethtool_lmode_name(mode));
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        else
        {
            *value = false;
            return 0;
        }
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
            *value = true;
        else
            *value = false;
    }

    return 0;
}

#endif

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_supported(ta_ethtool_lsets *lsets,
                           ta_ethtool_link_mode mode,
                           bool *supported)
{
    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_GLINKSETTINGS
        return new_lmode_op(lsets, SUPPORTED_MASK_ID,
                            mode, false, supported);
#else
        ERROR("%s(): ETHTOOL_GLINKSETTINGS is not defined but "
              "use_xlinksettings is set to TRUE", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    }
    else
    {
        return old_lmode_op(lsets, SUPPORTED_MASK_ID,
                            mode, false, supported);
    }
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_advertised(ta_ethtool_lsets *lsets,
                            ta_ethtool_link_mode mode,
                            bool *advertised)
{
    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_GLINKSETTINGS
        return new_lmode_op(lsets, ADVERTISING_MASK_ID,
                            mode, false, advertised);
#else
        ERROR("%s(): ETHTOOL_GLINKSETTINGS is not defined but "
              "use_xlinksettings is set to TRUE", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    }
    else
    {
        return old_lmode_op(lsets, ADVERTISING_MASK_ID,
                            mode, false, advertised);
    }
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_lp_advertised(ta_ethtool_lsets *lsets,
                               ta_ethtool_link_mode mode,
                               bool *lp_advertised)
{
    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_GLINKSETTINGS
        return new_lmode_op(lsets, LP_ADVERTISING_MASK_ID,
                            mode, false, lp_advertised);
#else
        ERROR("%s(): ETHTOOL_GLINKSETTINGS is not defined but "
              "use_xlinksettings is set to TRUE", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    }
    else
    {
        return old_lmode_op(lsets, LP_ADVERTISING_MASK_ID,
                            mode, false, lp_advertised);
    }
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_advertise(ta_ethtool_lsets *lsets,
                           ta_ethtool_link_mode mode,
                           bool enable)
{
    if (lsets->use_xlinksettings)
    {
#ifdef ETHTOOL_GLINKSETTINGS
        return new_lmode_op(lsets, ADVERTISING_MASK_ID,
                            mode, true, &enable);
#else
        ERROR("%s(): ETHTOOL_GLINKSETTINGS is not defined but "
              "use_xlinksettings is set to TRUE", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    }
    else
    {
        return old_lmode_op(lsets, ADVERTISING_MASK_ID,
                            mode, true, &enable);
    }
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_lmode_list_names(ta_ethtool_lsets *lsets,
                            bool link_partner,
                            te_string *list_str)
{
    size_t i;
    bool enabled;
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

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_get_max_speed(ta_ethtool_lsets *lsets, unsigned int *speed,
                         unsigned int *duplex)
{
    unsigned int last_speed;
    unsigned int last_duplex;
    unsigned int mode_speed;
    unsigned int mode_duplex;
    unsigned int i;
    bool update_speed;
    bool enabled;
    te_errno rc;

    last_speed = SPEED_UNKNOWN;
    last_duplex = DUPLEX_UNKNOWN;

    for (i = 0; i < TE_ARRAY_LEN(modes_info); i++)
    {
        update_speed = false;

        switch (i)
        {
            case TA_ETHTOOL_LINK_MODE_10baseT_Half:
                mode_speed = 10;
                mode_duplex = DUPLEX_HALF;
                break;

            case TA_ETHTOOL_LINK_MODE_10baseT_Full:
                mode_speed = 10;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_100baseT_Half:
            case TA_ETHTOOL_LINK_MODE_100baseFX_Half:
                mode_speed = 100;
                mode_duplex = DUPLEX_HALF;
                break;

            case TA_ETHTOOL_LINK_MODE_100baseT_Full:
            case TA_ETHTOOL_LINK_MODE_100baseT1_Full:
            case TA_ETHTOOL_LINK_MODE_100baseFX_Full:
                mode_speed = 100;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_1000baseT_Half:
                mode_speed = 1000;
                mode_duplex = DUPLEX_HALF;
                break;

            case TA_ETHTOOL_LINK_MODE_1000baseT_Full:
            case TA_ETHTOOL_LINK_MODE_1000baseKX_Full:
            case TA_ETHTOOL_LINK_MODE_1000baseX_Full:
            case TA_ETHTOOL_LINK_MODE_1000baseT1_Full:
                mode_speed = 1000;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_2500baseX_Full:
            case TA_ETHTOOL_LINK_MODE_2500baseT_Full:
                mode_speed = 2500;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_5000baseT_Full:
                mode_speed = 5000;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_10000baseT_Full:
            case TA_ETHTOOL_LINK_MODE_10000baseKX4_Full:
            case TA_ETHTOOL_LINK_MODE_10000baseKR_Full:
            case TA_ETHTOOL_LINK_MODE_10000baseR_FEC:
            case TA_ETHTOOL_LINK_MODE_10000baseCR_Full:
            case TA_ETHTOOL_LINK_MODE_10000baseSR_Full:
            case TA_ETHTOOL_LINK_MODE_10000baseLR_Full:
            case TA_ETHTOOL_LINK_MODE_10000baseLRM_Full:
            case TA_ETHTOOL_LINK_MODE_10000baseER_Full:
                mode_speed = 10000;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_20000baseMLD2_Full:
            case TA_ETHTOOL_LINK_MODE_20000baseKR2_Full:
                mode_speed = 20000;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_25000baseCR_Full:
            case TA_ETHTOOL_LINK_MODE_25000baseKR_Full:
            case TA_ETHTOOL_LINK_MODE_25000baseSR_Full:
                mode_speed = 25000;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_40000baseKR4_Full:
            case TA_ETHTOOL_LINK_MODE_40000baseCR4_Full:
            case TA_ETHTOOL_LINK_MODE_40000baseSR4_Full:
            case TA_ETHTOOL_LINK_MODE_40000baseLR4_Full:
                mode_speed = 40000;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_50000baseCR2_Full:
            case TA_ETHTOOL_LINK_MODE_50000baseKR2_Full:
            case TA_ETHTOOL_LINK_MODE_50000baseSR2_Full:
            case TA_ETHTOOL_LINK_MODE_50000baseKR_Full:
            case TA_ETHTOOL_LINK_MODE_50000baseSR_Full:
            case TA_ETHTOOL_LINK_MODE_50000baseCR_Full:
            case TA_ETHTOOL_LINK_MODE_50000baseLR_ER_FR_Full:
            case TA_ETHTOOL_LINK_MODE_50000baseDR_Full:
                mode_speed = 50000;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_56000baseKR4_Full:
            case TA_ETHTOOL_LINK_MODE_56000baseCR4_Full:
            case TA_ETHTOOL_LINK_MODE_56000baseSR4_Full:
            case TA_ETHTOOL_LINK_MODE_56000baseLR4_Full:
                mode_speed = 56000;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_100000baseKR4_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseSR4_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseCR4_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseLR4_ER4_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseKR2_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseSR2_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseCR2_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseLR2_ER2_FR2_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseDR2_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseKR_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseSR_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseLR_ER_FR_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseCR_Full:
            case TA_ETHTOOL_LINK_MODE_100000baseDR_Full:
                mode_speed = 100000;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_200000baseKR4_Full:
            case TA_ETHTOOL_LINK_MODE_200000baseSR4_Full:
            case TA_ETHTOOL_LINK_MODE_200000baseLR4_ER4_FR4_Full:
            case TA_ETHTOOL_LINK_MODE_200000baseDR4_Full:
            case TA_ETHTOOL_LINK_MODE_200000baseCR4_Full:
            case TA_ETHTOOL_LINK_MODE_200000baseKR2_Full:
            case TA_ETHTOOL_LINK_MODE_200000baseSR2_Full:
            case TA_ETHTOOL_LINK_MODE_200000baseLR2_ER2_FR2_Full:
            case TA_ETHTOOL_LINK_MODE_200000baseDR2_Full:
            case TA_ETHTOOL_LINK_MODE_200000baseCR2_Full:
                mode_speed = 200000;
                mode_duplex = DUPLEX_FULL;
                break;

            case TA_ETHTOOL_LINK_MODE_400000baseKR8_Full:
            case TA_ETHTOOL_LINK_MODE_400000baseSR8_Full:
            case TA_ETHTOOL_LINK_MODE_400000baseLR8_ER8_FR8_Full:
            case TA_ETHTOOL_LINK_MODE_400000baseDR8_Full:
            case TA_ETHTOOL_LINK_MODE_400000baseCR8_Full:
            case TA_ETHTOOL_LINK_MODE_400000baseKR4_Full:
            case TA_ETHTOOL_LINK_MODE_400000baseSR4_Full:
            case TA_ETHTOOL_LINK_MODE_400000baseLR4_ER4_FR4_Full:
            case TA_ETHTOOL_LINK_MODE_400000baseDR4_Full:
            case TA_ETHTOOL_LINK_MODE_400000baseCR4_Full:
                mode_speed = 400000;
                mode_duplex = DUPLEX_FULL;
                break;

            default:
                continue;
        }

        if (last_speed == SPEED_UNKNOWN || mode_speed > last_speed)
        {
            update_speed = true;
        }
        else if (mode_speed == last_speed)
        {
            if (last_duplex == DUPLEX_UNKNOWN ||
                (last_duplex == DUPLEX_HALF && mode_duplex == DUPLEX_FULL))
                update_speed = true;
        }

        if (update_speed)
        {
            rc = ta_ethtool_lmode_supported(lsets, i, &enabled);
            if (rc != 0)
                return rc;

            if (enabled)
            {
                last_speed = mode_speed;
                last_duplex = mode_duplex;
            }
        }
    }

    *speed = last_speed;
    *duplex = last_duplex;

    return 0;
}

#ifdef ETHTOOL_GRXCLSRLALL

/* Release memory allocated for ta_ethtool_rx_cls_rules */
static void
free_rx_cls_rules(void *arg)
{
    ta_ethtool_rx_cls_rules *rules = arg;

    if (rules == NULL)
        return;

    free(rules->locs);
    free(rules);
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_get_rx_cls_rules(unsigned int gid, const char *if_name,
                            ta_ethtool_rx_cls_rules **rules_data)
{
    struct ethtool_rxnfc rules_count;
    struct ethtool_rxnfc *rules = NULL;
    size_t req_size;
    ta_ethtool_rx_cls_rules *result = NULL;
    ta_cfg_obj_t *obj;
    unsigned int i;
    te_errno rc;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_RX_CLS_RULES, if_name, gid);
    if (obj != NULL)
    {
        *rules_data = obj->user_data;
        return 0;
    }

    result = TE_ALLOC(sizeof(*result));

    memset(&rules_count, 0, sizeof(rules_count));

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GRXCLSRLCNT, &rules_count);
    if (rc != 0)
        goto cleanup;

#ifdef RX_CLS_LOC_SPECIAL
    result->spec_loc_flag = !!(rules_count.data & RX_CLS_LOC_SPECIAL);
#else
    result->spec_loc_flag = false;
#endif

    result->rule_cnt = rules_count.rule_cnt;
    if (result->rule_cnt > 0)
        result->locs = TE_ALLOC(sizeof(unsigned int) * result->rule_cnt);

    req_size = sizeof(*rules) + sizeof(uint32_t) * result->rule_cnt;
    rules = TE_ALLOC(req_size);
    rules->rule_cnt = result->rule_cnt;

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GRXCLSRLALL, rules);
    if (rc != 0)
        goto cleanup;

    result->table_size = rules->data;

    /*
     * May be some rule was removed between two SIOCETHTOOL calls.
     */
    if (rules->rule_cnt < result->rule_cnt)
        result->rule_cnt = rules->rule_cnt;

    for (i = 0; i < result->rule_cnt; i++)
    {
        result->locs[i] = rules->rule_locs[i];
    }

    rc = ta_obj_add(TA_OBJ_TYPE_IF_RX_CLS_RULES, if_name,
                    "", gid, result, &free_rx_cls_rules, &obj);

cleanup:

    if (rc == 0)
        *rules_data = result;
    else
        free_rx_cls_rules(result);

    free(rules);

    return rc;
}

/* Define missing flow type flags to zero to simplify code. */

#ifndef FLOW_EXT
#define FLOW_EXT 0
#endif
#ifndef FLOW_MAC_EXT
#define FLOW_MAC_EXT 0
#endif
#ifndef FLOW_RSS
#define FLOW_RSS 0
#endif

/* All the known network flow flags */
#define FLOW_TYPE_FLAGS (FLOW_EXT | FLOW_MAC_EXT | FLOW_RSS)

/*
 * Fill fields in TA Rx rule structure with data stored in
 * ethtool_flow_union.
 */

static void
ether_to_ta(const struct ethhdr *spec,
            ta_ethtool_rx_cls_rule_fields *ta_fields)
{
    memcpy(&ta_fields->src_mac, &spec->h_source,
           sizeof(spec->h_source));
    memcpy(&ta_fields->dst_mac, &spec->h_dest,
           sizeof(spec->h_dest));

    ta_fields->ether_type = ntohs(spec->h_proto);
}

#ifdef HAVE_STRUCT_ETHTOOL_USRIP6_SPEC
static void
usrip6_to_ta(const struct ethtool_usrip6_spec *spec,
             ta_ethtool_rx_cls_rule_fields *ta_fields)
{
    memcpy(&ta_fields->src_l3_addr, &spec->ip6src,
           sizeof(spec->ip6src));
    memcpy(&ta_fields->dst_l3_addr, &spec->ip6dst,
           sizeof(spec->ip6dst));

    ta_fields->l4_4_bytes = spec->l4_4_bytes;
    ta_fields->tos_or_tclass = spec->tclass;
    ta_fields->l4_proto = spec->l4_proto;
}
#endif

#ifdef HAVE_STRUCT_ETHTOOL_AH_ESPIP6_SPEC
static void
ah_espip6_to_ta(const struct ethtool_ah_espip6_spec *spec,
                ta_ethtool_rx_cls_rule_fields *ta_fields)
{
    memcpy(&ta_fields->src_l3_addr, &spec->ip6src,
           sizeof(spec->ip6src));
    memcpy(&ta_fields->dst_l3_addr, &spec->ip6dst,
           sizeof(spec->ip6dst));

    ta_fields->spi = ntohl(spec->spi);
    ta_fields->tos_or_tclass = spec->tclass;
}
#endif

#ifdef HAVE_STRUCT_ETHTOOL_TCPIP6_SPEC
static void
tcpip6_to_ta(const struct ethtool_tcpip6_spec *spec,
             ta_ethtool_rx_cls_rule_fields *ta_fields)
{
    memcpy(&ta_fields->src_l3_addr, &spec->ip6src,
           sizeof(spec->ip6src));
    memcpy(&ta_fields->dst_l3_addr, &spec->ip6dst,
           sizeof(spec->ip6dst));

    ta_fields->src_port = ntohs(spec->psrc);
    ta_fields->dst_port = ntohs(spec->pdst);
    ta_fields->tos_or_tclass = spec->tclass;
}
#endif

#ifdef HAVE_STRUCT_ETHTOOL_USRIP4_SPEC
static void
usrip4_to_ta(const struct ethtool_usrip4_spec *spec,
             ta_ethtool_rx_cls_rule_fields *ta_fields)
{
    memcpy(&ta_fields->src_l3_addr, &spec->ip4src,
           sizeof(spec->ip4src));
    memcpy(&ta_fields->dst_l3_addr, &spec->ip4dst,
           sizeof(spec->ip4dst));

    ta_fields->l4_4_bytes = spec->l4_4_bytes;
    ta_fields->tos_or_tclass = spec->tos;
    ta_fields->l4_proto = spec->proto;
}
#endif

#ifdef HAVE_STRUCT_ETHTOOL_AH_ESPIP4_SPEC
static void
ah_espip4_to_ta(const struct ethtool_ah_espip4_spec *spec,
                ta_ethtool_rx_cls_rule_fields *ta_fields)
{
    memcpy(&ta_fields->src_l3_addr, &spec->ip4src,
           sizeof(spec->ip4src));
    memcpy(&ta_fields->dst_l3_addr, &spec->ip4dst,
           sizeof(spec->ip4dst));

    ta_fields->spi = ntohl(spec->spi);
    ta_fields->tos_or_tclass = spec->tos;
}
#endif

#ifdef HAVE_STRUCT_ETHTOOL_TCPIP4_SPEC
static void
tcpip4_to_ta(const struct ethtool_tcpip4_spec *spec,
             ta_ethtool_rx_cls_rule_fields *ta_fields)
{
    memcpy(&ta_fields->src_l3_addr, &spec->ip4src,
           sizeof(spec->ip4src));
    memcpy(&ta_fields->dst_l3_addr, &spec->ip4dst,
           sizeof(spec->ip4dst));

    ta_fields->src_port = ntohs(spec->psrc);
    ta_fields->dst_port = ntohs(spec->pdst);
    ta_fields->tos_or_tclass = spec->tos;
}
#endif

static te_errno
rule_fields_h2ta(uint32_t flow_type,
                 const union ethtool_flow_union *h_fields,
                 ta_ethtool_rx_cls_rule_fields *ta_fields)
{
    switch (flow_type)
    {
#ifdef HAVE_STRUCT_ETHTOOL_TCPIP4_SPEC
        case TCP_V4_FLOW:
        case UDP_V4_FLOW:
        case SCTP_V4_FLOW:
            tcpip4_to_ta((struct ethtool_tcpip4_spec *)h_fields,
                         ta_fields);
            break;
#endif

#ifdef HAVE_STRUCT_ETHTOOL_AH_ESPIP4_SPEC
        case AH_V4_FLOW:
        case ESP_V4_FLOW:
            ah_espip4_to_ta((struct ethtool_ah_espip4_spec *)h_fields,
                            ta_fields);
            break;
#endif

#ifdef HAVE_STRUCT_ETHTOOL_USRIP4_SPEC
        case IPV4_USER_FLOW:
            usrip4_to_ta((struct ethtool_usrip4_spec *)h_fields, ta_fields);
            break;
#endif

#ifdef HAVE_STRUCT_ETHTOOL_TCPIP6_SPEC
        case TCP_V6_FLOW:
        case UDP_V6_FLOW:
        case SCTP_V6_FLOW:
            tcpip6_to_ta((struct ethtool_tcpip6_spec *)h_fields,
                         ta_fields);
            break;
#endif

#ifdef HAVE_STRUCT_ETHTOOL_AH_ESPIP6_SPEC
        case AH_V6_FLOW:
        case ESP_V6_FLOW:
            ah_espip6_to_ta((struct ethtool_ah_espip6_spec *)h_fields,
                            ta_fields);
            break;
#endif

#ifdef HAVE_STRUCT_ETHTOOL_USRIP6_SPEC
        case IPV6_USER_FLOW:
            usrip6_to_ta((struct ethtool_usrip6_spec *)h_fields, ta_fields);
            break;
#endif

        case ETHER_FLOW:
            ether_to_ta((struct ethhdr *)h_fields, ta_fields);
            break;

        default:
            ERROR("%s(): not supported flow type 0x%x", __FUNCTION__,
                  flow_type);
            return TE_EINVAL;
    }

    return 0;
}

/*
 * Fill fields in TA Rx rule structure with data stored in ethtool_flow_ext
 * structure.
 */
static void
rule_ext_fields_h2ta(uint32_t flow_flags,
                     const struct ethtool_flow_ext *h_fields,
                     ta_ethtool_rx_cls_rule_fields *ta_fields)
{
#if FLOW_MAC_EXT != 0
    if (flow_flags & FLOW_MAC_EXT)
    {
        memcpy(&ta_fields->dst_mac, &h_fields->h_dest,
               sizeof(h_fields->h_dest));
    }
#endif

    if (flow_flags & FLOW_EXT)
    {
        ta_fields->vlan_tpid = ntohs(h_fields->vlan_etype);
        ta_fields->vlan_tci = ntohs(h_fields->vlan_tci);
        ta_fields->data0 = ntohl(h_fields->data[0]);
        ta_fields->data1 = ntohl(h_fields->data[1]);
    }
}

/*
 * Convert native representation of Rx classification rule to
 * TA representation.
 */
static te_errno
rule_h2ta(const struct ethtool_rxnfc *h_rule,
          ta_ethtool_rx_cls_rule *ta_rule)
{
    te_errno rc;

    memset(ta_rule, 0, sizeof(*ta_rule));

    ta_rule->location = h_rule->fs.location;

    ta_rule->flow_type = h_rule->fs.flow_type & ~FLOW_TYPE_FLAGS;

    ta_rule->rx_queue = h_rule->fs.ring_cookie;

#ifdef HAVE_STRUCT_ETHTOOL_RXNFC_RSS_CONTEXT
    if (h_rule->fs.flow_type & FLOW_RSS)
        ta_rule->rss_context = h_rule->rss_context;
    else
#endif
        ta_rule->rss_context = -1;

    rc = rule_fields_h2ta(ta_rule->flow_type, &h_rule->fs.h_u,
                          &ta_rule->field_values);
    if (rc != 0)
        return rc;

    rc = rule_fields_h2ta(ta_rule->flow_type, &h_rule->fs.m_u,
                          &ta_rule->field_masks);
    if (rc != 0)
        return rc;

    rule_ext_fields_h2ta(h_rule->fs.flow_type, &h_rule->fs.h_ext,
                         &ta_rule->field_values);
    rule_ext_fields_h2ta(h_rule->fs.flow_type, &h_rule->fs.m_ext,
                         &ta_rule->field_masks);

    return 0;
}

/*
 * Fill ethtool_flow_union with data stored in TA Rx rule structure.
 */

static void
ether_from_ta(const ta_ethtool_rx_cls_rule_fields *ta_fields,
              struct ethhdr *spec)
{
    memcpy(&spec->h_source, &ta_fields->src_mac,
           sizeof(spec->h_source));
    memcpy(&spec->h_dest, &ta_fields->dst_mac,
           sizeof(spec->h_dest));

    spec->h_proto = htons(ta_fields->ether_type);
}

#ifdef HAVE_STRUCT_ETHTOOL_USRIP6_SPEC
static void
usrip6_from_ta(const ta_ethtool_rx_cls_rule_fields *ta_fields,
               struct ethtool_usrip6_spec *spec)
{
    memcpy(&spec->ip6src, &ta_fields->src_l3_addr,
           sizeof(spec->ip6src));
    memcpy(&spec->ip6dst, &ta_fields->dst_l3_addr,
           sizeof(spec->ip6dst));

    spec->l4_4_bytes = ta_fields->l4_4_bytes;
    spec->tclass = ta_fields->tos_or_tclass;
    spec->l4_proto = ta_fields->l4_proto;
}
#endif

#ifdef HAVE_STRUCT_ETHTOOL_AH_ESPIP6_SPEC
static void
ah_espip6_from_ta(const ta_ethtool_rx_cls_rule_fields *ta_fields,
                  struct ethtool_ah_espip6_spec *spec)
{
    memcpy(&spec->ip6src, &ta_fields->src_l3_addr,
           sizeof(spec->ip6src));
    memcpy(&spec->ip6dst, &ta_fields->dst_l3_addr,
           sizeof(spec->ip6dst));

    spec->spi = htonl(ta_fields->spi);
    spec->tclass = ta_fields->tos_or_tclass;
}
#endif

#ifdef HAVE_STRUCT_ETHTOOL_TCPIP6_SPEC
static void
tcpip6_from_ta(const ta_ethtool_rx_cls_rule_fields *ta_fields,
               struct ethtool_tcpip6_spec *spec)
{
    memcpy(&spec->ip6src, &ta_fields->src_l3_addr,
           sizeof(spec->ip6src));
    memcpy(&spec->ip6dst, &ta_fields->dst_l3_addr,
           sizeof(spec->ip6dst));

    spec->psrc = htons(ta_fields->src_port);
    spec->pdst = htons(ta_fields->dst_port);
    spec->tclass = ta_fields->tos_or_tclass;
}
#endif

#ifdef HAVE_STRUCT_ETHTOOL_USRIP4_SPEC
static void
usrip4_from_ta(const ta_ethtool_rx_cls_rule_fields *ta_fields,
               bool mask, struct ethtool_usrip4_spec *spec)
{
    memcpy(&spec->ip4src, &ta_fields->src_l3_addr,
           sizeof(spec->ip4src));
    memcpy(&spec->ip4dst, &ta_fields->dst_l3_addr,
           sizeof(spec->ip4dst));

    spec->l4_4_bytes = ta_fields->l4_4_bytes;
    spec->tos = ta_fields->tos_or_tclass;

    if (mask)
    {
        /* Comments in ethtool.h say mask must be 0 for these fields */
        spec->ip_ver = 0;
        spec->proto = 0;
    }
    else
    {
        spec->ip_ver = ETH_RX_NFC_IP4;
        spec->proto = ta_fields->l4_proto;
    }
}
#endif

#ifdef HAVE_STRUCT_ETHTOOL_AH_ESPIP4_SPEC
static void
ah_espip4_from_ta(const ta_ethtool_rx_cls_rule_fields *ta_fields,
                  struct ethtool_ah_espip4_spec *spec)
{
    memcpy(&spec->ip4src, &ta_fields->src_l3_addr,
           sizeof(spec->ip4src));
    memcpy(&spec->ip4dst, &ta_fields->dst_l3_addr,
           sizeof(spec->ip4dst));

    spec->spi = htonl(ta_fields->spi);
    spec->tos = ta_fields->tos_or_tclass;
}
#endif

#ifdef HAVE_STRUCT_ETHTOOL_TCPIP4_SPEC
static void
tcpip4_from_ta(const ta_ethtool_rx_cls_rule_fields *ta_fields,
               struct ethtool_tcpip4_spec *spec)
{
    memcpy(&spec->ip4src, &ta_fields->src_l3_addr,
           sizeof(spec->ip4src));
    memcpy(&spec->ip4dst, &ta_fields->dst_l3_addr,
           sizeof(spec->ip4dst));

    spec->psrc = htons(ta_fields->src_port);
    spec->pdst = htons(ta_fields->dst_port);
    spec->tos = ta_fields->tos_or_tclass;
}
#endif

static te_errno
rule_fields_ta2h(uint32_t flow_type, bool mask,
                 const ta_ethtool_rx_cls_rule_fields *ta_fields,
                 union ethtool_flow_union *h_fields)
{
    UNUSED(mask);

    switch (flow_type)
    {
#ifdef HAVE_STRUCT_ETHTOOL_TCPIP4_SPEC
        case TCP_V4_FLOW:
        case UDP_V4_FLOW:
        case SCTP_V4_FLOW:
            tcpip4_from_ta(ta_fields,
                           (struct ethtool_tcpip4_spec *)h_fields);
            break;
#endif

#ifdef HAVE_STRUCT_ETHTOOL_AH_ESPIP4_SPEC
        case AH_V4_FLOW:
        case ESP_V4_FLOW:
            ah_espip4_from_ta(ta_fields,
                              (struct ethtool_ah_espip4_spec *)h_fields);
            break;
#endif

#ifdef HAVE_STRUCT_ETHTOOL_USRIP4_SPEC
        case IPV4_USER_FLOW:
            usrip4_from_ta(ta_fields, mask,
                           (struct ethtool_usrip4_spec *)h_fields);
            break;
#endif

#ifdef HAVE_STRUCT_ETHTOOL_TCPIP6_SPEC
        case TCP_V6_FLOW:
        case UDP_V6_FLOW:
        case SCTP_V6_FLOW:
            tcpip6_from_ta(ta_fields,
                           (struct ethtool_tcpip6_spec *)h_fields);
            break;
#endif

#ifdef HAVE_STRUCT_ETHTOOL_AH_ESPIP6_SPEC
        case AH_V6_FLOW:
        case ESP_V6_FLOW:
            ah_espip6_from_ta(ta_fields,
                              (struct ethtool_ah_espip6_spec *)h_fields);
            break;
#endif

#ifdef HAVE_STRUCT_ETHTOOL_USRIP6_SPEC
        case IPV6_USER_FLOW:
            usrip6_from_ta(ta_fields,
                           (struct ethtool_usrip6_spec *)h_fields);
            break;
#endif

        case ETHER_FLOW:
            ether_from_ta(ta_fields, (struct ethhdr *)h_fields);
            break;

        default:
            ERROR("%s(): not supported flow type 0x%x", __FUNCTION__,
                  flow_type);
            return TE_EINVAL;
    }

    return 0;
}

/* Check whether given buffer contains only zeroes */

static bool
data_is_zero(const void *data, size_t len)
{
    const uint8_t *p = (uint8_t *)data;
    size_t i;

    for (i = 0; i < len; i++)
    {
        if (p[i] != 0)
            return false;
    }

    return true;
}

#define DATA_IS_ZERO(data_) data_is_zero(&data_, sizeof(data_))

/* Fill ethtool_flow_ext structure from data stored in TA structure */
static void
rule_ext_fields_ta2h(uint32_t flow_type,
                     const ta_ethtool_rx_cls_rule_fields *ta_fields,
                     struct ethtool_rx_flow_spec *h_spec,
                     struct ethtool_flow_ext *h_fields)
{
#if FLOW_MAC_EXT != 0
    if (!DATA_IS_ZERO(ta_fields->dst_mac) && flow_type != ETHER_FLOW)
    {
        h_spec->flow_type |= FLOW_MAC_EXT;
        memcpy(h_fields->h_dest, ta_fields->dst_mac,
               sizeof(h_fields->h_dest));
    }
#endif

    if (!DATA_IS_ZERO(ta_fields->vlan_tpid) ||
        !DATA_IS_ZERO(ta_fields->vlan_tci) ||
        !DATA_IS_ZERO(ta_fields->data0) ||
        !DATA_IS_ZERO(ta_fields->data1))
    {
        h_spec->flow_type |= FLOW_EXT;
        h_fields->vlan_etype = htons(ta_fields->vlan_tpid);
        h_fields->vlan_tci = htons(ta_fields->vlan_tci);
        h_fields->data[0] = htonl(ta_fields->data0);
        h_fields->data[1] = htonl(ta_fields->data1);
    }
}

/*
 * Fill native Rx classification rule structure with data stored
 * in TA structure.
 */
static te_errno
rule_ta2h(const ta_ethtool_rx_cls_rule *ta_rule,
          struct ethtool_rxnfc *h_rule)
{
    te_errno rc;

    memset(h_rule, 0, sizeof(*h_rule));

    h_rule->fs.location = ta_rule->location;

#ifdef RX_CLS_LOC_SPECIAL
    if (ta_rule->location == RX_CLS_LOC_ANY ||
        ta_rule->location == RX_CLS_LOC_FIRST ||
        ta_rule->location == RX_CLS_LOC_LAST)
    {
        h_rule->fs.location |= RX_CLS_LOC_SPECIAL;
    }
#endif

    h_rule->fs.flow_type = ta_rule->flow_type;

    h_rule->fs.ring_cookie = ta_rule->rx_queue;

    if (ta_rule->rss_context >= 0)
    {
#ifdef HAVE_STRUCT_ETHTOOL_RXNFC_RSS_CONTEXT
        h_rule->fs.flow_type |= FLOW_RSS;
        h_rule->rss_context = ta_rule->rss_context;
#else
        ERROR("%s(): setting RSS context for Rx rules is not supported",
              __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
    }

    rc = rule_fields_ta2h(ta_rule->flow_type, false, &ta_rule->field_values,
                          &h_rule->fs.h_u);
    if (rc != 0)
        return rc;

    rc = rule_fields_ta2h(ta_rule->flow_type, true, &ta_rule->field_masks,
                          &h_rule->fs.m_u);
    if (rc != 0)
        return rc;

    rule_ext_fields_ta2h(ta_rule->flow_type, &ta_rule->field_values,
                         &h_rule->fs, &h_rule->fs.h_ext);
    rule_ext_fields_ta2h(ta_rule->flow_type, &ta_rule->field_masks,
                         &h_rule->fs, &h_rule->fs.m_ext);

    return 0;
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_get_rx_cls_rule(unsigned int gid, const char *if_name,
                           unsigned int location,
                           ta_ethtool_rx_cls_rule **rule_out)
{
    te_string obj_name = TE_STRING_INIT_STATIC(MAX_OBJ_NAME_LEN);
    struct ethtool_rxnfc rule;
    ta_ethtool_rx_cls_rule *ta_rule;
    ta_cfg_obj_t *obj;
    te_errno rc = 0;

    rc = te_string_append_chk(&obj_name, "%s.%u", if_name, location);
    if (rc != 0)
        goto cleanup;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_RX_CLS_RULE,
                      te_string_value(&obj_name),
                      gid);
    if (obj != NULL)
    {
        *rule_out = obj->user_data;
        return 0;
    }

    ta_rule = TE_ALLOC(sizeof(*ta_rule));

    memset(&rule, 0, sizeof(rule));
    rule.fs.location = location;

    rc = call_ethtool_ioctl(if_name, ETHTOOL_GRXCLSRULE, &rule);
    if (rc != 0)
        goto cleanup;

    rc = rule_h2ta(&rule, ta_rule);
    if (rc != 0)
        goto cleanup;

    rc = ta_obj_add(TA_OBJ_TYPE_IF_RX_CLS_RULE, te_string_value(&obj_name),
                    "", gid, ta_rule, &free, &obj);

cleanup:

    if (rc == 0)
        *rule_out = ta_rule;
    else
        free(ta_rule);

    return rc;
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_add_rx_cls_rule(unsigned int gid, const char *if_name,
                           unsigned int location,
                           ta_ethtool_rx_cls_rule **rule_out)
{
    te_string obj_name = TE_STRING_INIT_STATIC(MAX_OBJ_NAME_LEN);
    ta_ethtool_rx_cls_rule *ta_rule = NULL;
    ta_cfg_obj_t *obj;
    te_errno rc = 0;

    rc = te_string_append_chk(&obj_name, "%s.%u", if_name, location);
    if (rc != 0)
        goto cleanup;

    ta_rule = TE_ALLOC(sizeof(*ta_rule));

    ta_rule->location = location;
    ta_rule->rss_context = -1;

    rc = ta_obj_add(TA_OBJ_TYPE_IF_RX_CLS_RULE, te_string_value(&obj_name),
                    "", gid, ta_rule, &free, &obj);

cleanup:

    if (rc == 0)
    {
        if (rule_out != NULL)
            *rule_out = ta_rule;
    }
    else
    {
        free(ta_rule);
    }

    return rc;
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_commit_rx_cls_rule(unsigned int gid, const char *if_name,
                              unsigned int location,
                              unsigned int *ret_location)
{
    te_errno rc;
    te_string obj_name = TE_STRING_INIT_STATIC(MAX_OBJ_NAME_LEN);
    ta_cfg_obj_t *obj;
    struct ethtool_rxnfc rule;
    ta_ethtool_rx_cls_rule *ta_rule = NULL;

    rc = te_string_append_chk(&obj_name, "%s.%u", if_name, location);
    if (rc != 0)
        return rc;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_RX_CLS_RULE,
                      te_string_value(&obj_name),
                      gid);
    if (obj == NULL)
    {
        /*
         * Nothing to commit. This is normal; commit is called
         * in case of delete operation even though there is
         * no need in that.
         */
        return 0;
    }

    ta_rule = obj->user_data;
    rc = rule_ta2h(ta_rule, &rule);
    if (rc != 0)
    {
        ta_obj_free(obj);
        return rc;
    }

    rc = call_ethtool_ioctl(if_name, ETHTOOL_SRXCLSRLINS, &rule);
    if (rc == 0)
        *ret_location = rule.fs.location;

    ta_obj_free(obj);
    return rc;
}

/* See description in conf_ethtool.h */
te_errno
ta_ethtool_del_rx_cls_rule(const char *if_name,
                           unsigned int location)
{
    struct ethtool_rxnfc rule;

    memset(&rule, 0, sizeof(rule));
    rule.fs.location = location;

    return call_ethtool_ioctl(if_name, ETHTOOL_SRXCLSRLDEL, &rule);
}

#endif /* ifdef ETHTOOL_GRXCLSRLALL */

#endif
