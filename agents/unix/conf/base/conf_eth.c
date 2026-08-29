/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Extra ethernet interface configurations
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Extra eth Conf"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "logger_api.h"
#include "unix_internal.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "te_vector.h"
#include "te_queue.h"
#include "rcf_pch_tree.h"

#if HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#include "te_rpc_sys_socket.h"

#include "conf_oid.h"
#include "conf_ethtool.h"

#ifdef HAVE_LINUX_ETHTOOL_H
#define E_BITS_PER_DWORD ((unsigned)(sizeof(uint32_t) * CHAR_BIT))

#define E_FEATURE_BITS_TO_DWORDS(_nb_features) \
    (((_nb_features) + E_BITS_PER_DWORD - 1) / E_BITS_PER_DWORD)

#define E_FEATURE_WORD(_dwords, _index, _field) \
    ((_dwords)[(_index) / E_BITS_PER_DWORD]._field)

#define E_FEATURE_FIELD_FLAG(_index) \
    (1U << (_index) % E_BITS_PER_DWORD)

#define E_FEATURE_BIT_SET(_dwords, _index, _field) \
    (E_FEATURE_WORD(_dwords, _index, _field) |= E_FEATURE_FIELD_FLAG(_index))

#define E_FEATURE_BIT_IS_SET(_dwords, _index, _field) \
    ((E_FEATURE_WORD(_dwords, _index, _field) &       \
      E_FEATURE_FIELD_FLAG(_index)) != 0)

typedef struct eth_feature_entry {
    char        name[ETH_GSTRING_LEN];
    bool enabled;
    bool readonly;
    bool need_update;
} eth_feature_entry;

typedef struct eth_if_context {
    SLIST_ENTRY(eth_if_context) links;
    char                        ifname[IFNAMSIZ];
    struct eth_feature_entry   *features;
    unsigned int                nb_features;
    bool valid;
} eth_if_context;

static SLIST_HEAD(eth_if_contexts, eth_if_context) if_contexts;


/* ioctl call to Ethtool */
static te_errno
eth_feature_ioctl_send(const char *ifname,
                       void       *cmd)
{
    struct ifreq ifr;
    int ret;

    memset(&ifr, 0, sizeof(ifr));
    te_strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_data = cmd;

    ret = ioctl(cfg_socket, SIOCETHTOOL, &ifr);

    return ret < 0 ? te_rc_os2te(errno) : 0;
}

/* Allocate and fill in feature names */
static te_errno
eth_feature_alloc_get_names(struct eth_if_context *if_context)
{
    /*
     * The data buffer definition in the structure below follows the
     * same approach as one used in Ethtool application, although that
     * approach seems to be unreliable under any standard except the GNU C
     */
    struct {
        struct ethtool_sset_info    hdr;
        uint32_t                    buf[1];
    } sset_info;

    te_errno                        rc = 0;
    uint32_t                        nb_features;
    struct ethtool_gstrings        *names = NULL;
    struct eth_feature_entry       *features = NULL;
    unsigned int                    i;

    sset_info.hdr.cmd = ETHTOOL_GSSET_INFO;
    sset_info.hdr.reserved = 0;
    sset_info.hdr.sset_mask = 1ULL << ETH_SS_FEATURES;

    rc = eth_feature_ioctl_send(if_context->ifname, &sset_info);
    if (rc != 0)
        return rc;

    if (!sset_info.hdr.sset_mask)
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);

    nb_features = sset_info.hdr.data[0];
    if (nb_features == 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    names = TE_ALLOC(sizeof(*names) + nb_features * ETH_GSTRING_LEN);

    names->cmd = ETHTOOL_GSTRINGS;
    names->string_set = ETH_SS_FEATURES;
    names->len = nb_features;

    rc = eth_feature_ioctl_send(if_context->ifname, names);
    if (rc != 0)
        goto fail;

    features = TE_ALLOC(nb_features * sizeof(*features));

    for (i = 0; i < nb_features; i++)
    {
        te_strlcpy(features[i].name,
                   (char *)(names->data + (i * ETH_GSTRING_LEN)),
                   ETH_GSTRING_LEN);
    }

    if_context->features = features;
    if_context->nb_features = nb_features;

    free(names);

    return 0;

fail:
    free(names);
    free(features);

    return rc;
}

/* Fill in feature values (On/Off) */
static te_errno
eth_feature_get_values(struct eth_if_context *if_context)
{
    struct ethtool_gfeatures   *e_features;
    unsigned int                i;
    te_errno                    rc = 0;

    e_features = TE_ALLOC(sizeof(*e_features) +
                         E_FEATURE_BITS_TO_DWORDS(if_context->nb_features) *
                         sizeof(e_features->features[0]));

    e_features->cmd = ETHTOOL_GFEATURES;
    e_features->size = E_FEATURE_BITS_TO_DWORDS(if_context->nb_features);
    rc = eth_feature_ioctl_send(if_context->ifname, e_features);
    if (rc != 0)
    {
        free(e_features);
        return rc;
    }

    for (i = 0; i < if_context->nb_features; ++i)
    {
        if_context->features[i].enabled =
            E_FEATURE_BIT_IS_SET(e_features->features, i, active);

        if_context->features[i].readonly =
           (!E_FEATURE_BIT_IS_SET(e_features->features, i, available) ||
            E_FEATURE_BIT_IS_SET(e_features->features, i, never_changed));
    }

    free(e_features);

    return 0;
}

/* Allocate features and get their values */
static te_errno
eth_feature_alloc_get(struct eth_if_context *if_context)
{
    te_errno rc = 0;

    rc = eth_feature_alloc_get_names(if_context);
    if ((rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP)) ||
        (rc == TE_RC(TE_TA_UNIX, TE_ENOENT)))
        return 0;
    else if (rc != 0)
        return rc;

    rc = eth_feature_get_values(if_context);
    if (rc != 0)
    {
        free(if_context->features);
        if_context->features = NULL;
        if_context->nb_features = 0;
        return rc;
    }

    return 0;
}

/* Find an interface context in the list */
static struct eth_if_context *
eth_feature_iface_context_find(const char *ifname)
{
    struct eth_if_context *if_context;

    SLIST_FOREACH(if_context, &if_contexts, links)
    {
        if (strcmp(if_context->ifname, ifname) == 0)
            break;
    }

    return if_context;
}

/* Add an interface context to the list */
static struct eth_if_context *
eth_feature_iface_context_add(const char *ifname)
{
    struct eth_if_context *if_context;
    te_errno                rc;

    if_context = TE_ALLOC(sizeof(*if_context));

    te_strlcpy(if_context->ifname, ifname, IFNAMSIZ);

    rc = eth_feature_alloc_get(if_context);
    if_context->valid = (rc == 0);

    SLIST_INSERT_HEAD(&if_contexts, if_context, links);

    return if_context;
}

/* Get (find or add) an interface context */
static struct eth_if_context *
eth_feature_iface_context(const char *ifname)
{
    struct eth_if_context *if_context;

    if (ifname == NULL)
        return NULL;

    if_context = eth_feature_iface_context_find(ifname);

    return (if_context != NULL) ? if_context :
                                  eth_feature_iface_context_add(ifname);
}

/* 'list' method implementation */
static te_errno
eth_feature_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char             *ifname = ta_conf_ctx_inst(ctx, "interface");
    struct eth_if_context  *if_context;
    unsigned int             i;

    if_context = eth_feature_iface_context(ifname);
    if (if_context == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    for (i = 0; if_context->valid && (i < if_context->nb_features); ++i)
    {
        char *name = TE_STRDUP(if_context->features[i].name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

/* Determine the feature index by its name */
static te_errno
eth_feature_get_index_by_name(struct eth_if_context     *if_context,
                              const char                 *feature_name,
                              unsigned int               *feature_index_out)
{
    unsigned int i;

    for (i = 0; i < if_context->nb_features; ++i)
    {
        if(strcmp(if_context->features[i].name, feature_name) == 0)
        {
            *feature_index_out = i;
            return 0;
        }
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/** Get pointer to feature structure by its name */
static te_errno
eth_feature_get_by_name(const char *ifname,
                        const char *feature_name,
                        eth_feature_entry **feature)
{
    struct eth_if_context *if_context;
    unsigned int feature_index;
    te_errno rc;

    if_context = eth_feature_iface_context(ifname);
    if ((if_context == NULL) || !if_context->valid)
        return TE_ENOENT;

    rc = eth_feature_get_index_by_name(if_context,
                                       feature_name,
                                       &feature_index);
    if (rc != 0)
        return rc;

    *feature = &if_context->features[feature_index];
    return 0;
}


/* 'get' method implementation */
static te_errno
eth_feature_get(const ta_conf_ctx *ctx, bool *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *feature_name = ta_conf_ctx_inst(ctx, "feature");
    eth_feature_entry *feature;
    te_errno rc;

    rc = eth_feature_get_by_name(ifname, feature_name, &feature);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    *val = feature->enabled;
    return 0;
}

/* 'set' method implementation */
static te_errno
eth_feature_set(const ta_conf_ctx *ctx, bool val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *feature_name = ta_conf_ctx_inst(ctx, "feature");
    eth_feature_entry *feature;
    te_errno rc;

    rc = eth_feature_get_by_name(ifname, feature_name, &feature);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (feature->enabled == val)
        return 0;

    if (feature->readonly)
    {
        ERROR("Feature '%s' is read-only on interface '%s' and cannot "
              "be changed", feature_name, ifname);
        return TE_RC(TE_TA_UNIX, TE_EACCES);
    }

    feature->enabled = val;
    feature->need_update = true;

    return 0;
}

/* Set new feature values */
static te_errno
eth_feature_set_values(struct eth_if_context *if_context)
{
    struct ethtool_sfeatures   *e_features;
    te_errno                    rc;
    unsigned int                i;

    e_features = TE_ALLOC(sizeof(*e_features) +
                          E_FEATURE_BITS_TO_DWORDS(if_context->nb_features) *
                          sizeof(e_features->features[0]));

    e_features->cmd = ETHTOOL_SFEATURES;

    e_features->size = E_FEATURE_BITS_TO_DWORDS(if_context->nb_features);
    for (i = 0; i < if_context->nb_features; ++i)
    {
        if (if_context->features[i].readonly)
        {
            continue;
        }

        if (!if_context->features[i].need_update)
            continue;

        E_FEATURE_BIT_SET(e_features->features, i, valid);
        if (if_context->features[i].enabled)
            E_FEATURE_BIT_SET(e_features->features, i, requested);

       if_context->features[i].need_update = false;
    }
    rc = eth_feature_ioctl_send(if_context->ifname, e_features);

    free(e_features);

    return rc;
}

/* 'commit' method implementation */
static te_errno
eth_feature_commit(const ta_conf_ctx *ctx)
{
    const char             *ifname = ta_conf_ctx_inst(ctx, "interface");
    struct eth_if_context  *if_context;
    te_errno                rc;

    if_context = eth_feature_iface_context(ifname);
    if ((if_context == NULL) || !if_context->valid)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = eth_feature_set_values(if_context);

    return TE_RC(TE_TA_UNIX, rc);
}

/** 'get' method implementation for interface/feature/readonly */
static te_errno
eth_feature_readonly_get(const ta_conf_ctx *ctx, bool *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    const char *feature_name = ta_conf_ctx_inst(ctx, "feature");
    eth_feature_entry *feature;
    te_errno rc;

    rc = eth_feature_get_by_name(ifname, feature_name, &feature);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    *val = feature->readonly;
    return 0;
}

/* list all private flags */
static te_errno
eth_priv_flags_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    te_errno rc;
    int failed_ethtool_cmd;
    struct ethtool_value *pflags = NULL;
    char *list_str = NULL;

    /*
     * Check whether private flags values may be obtained.
     * Unfortunately it is possible that list of private flags
     * can be retrieved for an interface but their values cannot
     * be obtained. In such case it's better not to list private
     * flag names in configuration tree to avoid breaking tree
     * synchronization.
     */
    rc = get_ethtool_value(if_name, ta_conf_ctx_gid(ctx), TA_ETHTOOL_PFLAGS,
                           (void **)&pflags);
    if (rc == TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
    {
        return 0;
    }
    else if (rc != 0)
    {
        ERROR("%s(): unexpected error %r occurred when trying to "
              "obtain values of private flags for %s", __FUNCTION__, rc,
              if_name);
        return rc;
    }

    ta_ethtool_reset_failed_cmd();
    rc = ta_ethtool_get_strings_list(ta_conf_ctx_gid(ctx), if_name,
                                     ETH_SS_PRIV_FLAGS, &list_str);
    failed_ethtool_cmd = ta_ethtool_failed_cmd();

    if (rc != 0)
    {
        if (failed_ethtool_cmd < 0)
        {
            ERROR("%s(): unexpected error %r occurred when "
                  "trying to get list of private flags for %s",
                  __FUNCTION__, rc, if_name);
        }
        else if ((rc != TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP) &&
                  rc != TE_RC(TE_TA_UNIX, TE_EINVAL)) ||
                 failed_ethtool_cmd != ETHTOOL_GSTRINGS)
        {
            ERROR("%s(): unexpected error %r occurred when "
                  "trying to get list of private flags for %s; "
                  "failed SIOCETHTOOL command is %d (%s)",
                  __FUNCTION__, rc, if_name,
                  failed_ethtool_cmd,
                  ta_ethtool_cmd2str(failed_ethtool_cmd));
        }
        else
        {
            /*
             * If private flags are not supported, let Configurator
             * think they are not present to avoid error messages.
             */
            rc = 0;
        }

        return rc;
    }

    if (list_str != NULL)
    {
        char *saveptr;
        char *tok;

        for (tok = strtok_r(list_str, " ", &saveptr); tok != NULL;
             tok = strtok_r(NULL, " ", &saveptr))
        {
            char *name = TE_STRDUP(tok);

            TE_VEC_APPEND(names, name);
        }
        free(list_str);
    }

    return 0;
}

/* common code for getting and setting private flag */
static te_errno
eth_priv_flags_common(unsigned int gid, const char *if_name,
                      const char *flag_name, unsigned int *idx,
                      uint32_t **data)
{
    te_errno rc;
    struct ethtool_value *pflags = NULL;

    rc = ta_ethtool_get_string_idx(gid, if_name, ETH_SS_PRIV_FLAGS,
                                   flag_name, idx);
    if (rc != 0)
        return rc;

    rc = get_ethtool_value(if_name, gid, TA_ETHTOOL_PFLAGS,
                           (void **)&pflags);
    if (rc != 0)
        return rc;

    *data = &pflags->data;
    return 0;
}

/* get state of private flag */
static te_errno
eth_priv_flags_get(const ta_conf_ctx *ctx, bool *val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *flag_name = ta_conf_ctx_inst(ctx, "flag");
    te_errno rc;
    unsigned int idx;
    uint32_t *data;

    rc = eth_priv_flags_common(ta_conf_ctx_gid(ctx), if_name, flag_name,
                               &idx, &data);
    if (rc != 0)
        return rc;

    *val = (*data & (1 << idx)) != 0;
    return 0;
}

/* set state of private flag */
static te_errno
eth_priv_flags_set(const ta_conf_ctx *ctx, bool val)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    const char *flag_name = ta_conf_ctx_inst(ctx, "flag");
    te_errno rc;
    unsigned int idx;
    uint32_t *data;

    rc = eth_priv_flags_common(ta_conf_ctx_gid(ctx), if_name, flag_name,
                               &idx, &data);
    if (rc != 0)
        return rc;

    if (idx > sizeof(uint32_t) * 8)
    {
        ERROR("%s(): index of private flag '%s' is too big (%u), cannot "
              "access it", __FUNCTION__, flag_name, idx);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (val)
        *data = *data | (1 << idx);
    else
        *data = *data & ~(1 << idx);

    return 0;
}

/* commit private flags */
static te_errno
eth_priv_flags_commit(const ta_conf_ctx *ctx)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");

    return commit_ethtool_value(if_name, ta_conf_ctx_gid(ctx),
                                TA_ETHTOOL_PFLAGS);
}

/**
 * Reset ethernet interface.
 *
 * @param ctx          Request context
 * @param val          New value
 *
 * @return Status code
 */
static te_errno
eth_reset_set(const ta_conf_ctx *ctx, const char *val)
{
#ifdef ETHTOOL_RESET
    const char              *ifname = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_value    eval = {.cmd = ETHTOOL_RESET,
                                    .data = ETH_RESET_ALL};
    struct ifreq            ifr = {.ifr_data = (void *)&eval};

    if (strcmp(val, "0") == 0)
        return 0;

    te_strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    if (ioctl(cfg_socket, SIOCETHTOOL, &ifr) != 0)
    {
        ERROR("ioctl failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
#else
    UNUSED(ctx);
    UNUSED(val);
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
}

/**
 * Get reset value (dummy)
 *
 * @param ctx          Request context
 * @param val          Location to save value
 *
 * @return Status code
 */
static te_errno
eth_reset_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    UNUSED(val);

    return 0;
}

/* List of device info parameters. */
typedef enum {
    ETH_DRVINFO_DRIVER = 0,
    ETH_DRVINFO_VERSION,
    ETH_DRVINFO_FW_VERSION,
} eth_drvinfo;

/**
 * Get driver info using ioctl(SIOCETHTOOL).
 *
 * @param ifname    Interface name
 * @param parameter Requested parameter
 * @param val       Location to append the parameter value to
 *
 * @return Status code
 */
static te_errno
eth_drvinfo_get(const char *ifname, eth_drvinfo parameter, te_string *val)
{
    struct ethtool_drvinfo  drvinfo = {.cmd = ETHTOOL_GDRVINFO};
    te_errno                rc = 0;

    rc = eth_feature_ioctl_send(ifname, &drvinfo);

    /* EOPNOTSUPP is returned for loopback interface - leave the function
     * keeping the value empty. */
    if (rc == TE_EOPNOTSUPP)
        return 0;
    else if (rc != 0)
        return rc;

    switch (parameter)
    {
        case ETH_DRVINFO_DRIVER:
            return te_string_append_chk(val, "%s", drvinfo.driver);

        case ETH_DRVINFO_VERSION:
            return te_string_append_chk(val, "%s", drvinfo.version);

        case ETH_DRVINFO_FW_VERSION:
            return te_string_append_chk(val, "%s", drvinfo.fw_version);

        default:
            ERROR("Unknown parameter value %d", parameter);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
}

/**
 * Get firmware version using ioctl(SIOCETHTOOL).
 *
 * @param ctx          Request context
 * @param val          Location to save value
 *
 * @return Status code
 */
static te_errno
eth_firmwareversion_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");

    return eth_drvinfo_get(ifname, ETH_DRVINFO_FW_VERSION, val);
}

/**
 * Get driver version using ioctl(SIOCETHTOOL).
 *
 * @param ctx          Request context
 * @param val          Location to save value
 *
 * @return Status code
 */
static te_errno
eth_driverversion_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");

    return eth_drvinfo_get(ifname, ETH_DRVINFO_VERSION, val);
}

/**
 * Get driver name using ioctl(SIOCETHTOOL).
 *
 * @param ctx          Request context
 * @param val          Location to save value
 *
 * @return Status code
 */
static te_errno
eth_drivername_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");

    return eth_drvinfo_get(ifname, ETH_DRVINFO_DRIVER, val);
}

/* Get driver message level */
static te_errno
eth_msglvl_get(const ta_conf_ctx *ctx, uint64_t *val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_value eval;
    te_errno rc;

    memset(&eval, 0, sizeof(eval));
    eval.cmd = ETHTOOL_GMSGLVL;

    rc = eth_feature_ioctl_send(ifname, &eval);
    if (rc != 0)
    {
        /*
         * ENOENT will make Configurator hide this node instead of
         * failing if it is not supported.
         */
        if (rc == TE_EOPNOTSUPP)
            rc = TE_ENOENT;

        return TE_RC(TE_TA_UNIX, rc);
    }

    *val = eval.data;
    return 0;
}

static te_errno
eth_ring_size_get(const ta_conf_ctx  *ctx,
                  bool          is_rx,
                  bool          get_maximum,
                  int64_t      *val)
{
    const char               *ifname = ta_conf_ctx_inst(ctx, "interface");
    struct eth_if_context    *if_context;
    struct ethtool_ringparam  ethtool_ringparam = { .cmd = ETHTOOL_GRINGPARAM };
    te_errno                  rc;

    if_context = eth_feature_iface_context(ifname);
    if (if_context == NULL || !if_context->valid)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = eth_feature_ioctl_send(ifname, &ethtool_ringparam);
    if (rc == TE_EOPNOTSUPP)
    {
        *val = -1;
    }
    else if (rc != 0)
    {
        return TE_RC(TE_TA_UNIX, rc);
    }
    else if (get_maximum)
    {
        *val = is_rx ? ethtool_ringparam.rx_max_pending :
                       ethtool_ringparam.tx_max_pending;
    }
    else
    {
        *val = is_rx ? ethtool_ringparam.rx_pending :
                       ethtool_ringparam.tx_pending;
    }

    return 0;
}

static te_errno
eth_ring_tx_max_get(const ta_conf_ctx *ctx, int64_t *val)
{
    return eth_ring_size_get(ctx, false, true, val);
}

static te_errno
eth_ring_rx_max_get(const ta_conf_ctx *ctx, int64_t *val)
{
    return eth_ring_size_get(ctx, true, true, val);
}

static te_errno
eth_ring_tx_current_get(const ta_conf_ctx *ctx, int64_t *val)
{
    return eth_ring_size_get(ctx, false, false, val);
}

static te_errno
eth_ring_rx_current_get(const ta_conf_ctx *ctx, int64_t *val)
{
    return eth_ring_size_get(ctx, true, false, val);
}

/* Set driver message level */
static te_errno
eth_msglvl_set(const ta_conf_ctx *ctx, uint64_t val)
{
    const char *ifname = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_value eval;
    te_errno rc;

    if (val > UINT_MAX)
    {
        ERROR("%s(): too big value '%" PRIu64 "'", __FUNCTION__, val);
        return TE_RC(TE_TA_UNIX, TE_ERANGE);
    }

    memset(&eval, 0, sizeof(eval));
    eval.cmd = ETHTOOL_SMSGLVL;
    eval.data = val;

    rc = eth_feature_ioctl_send(ifname, &eval);
    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
eth_ring_size_set(const ta_conf_ctx *ctx, bool is_rx, int64_t val)
{
    const char               *ifname = ta_conf_ctx_inst(ctx, "interface");
    struct eth_if_context    *if_context;
    struct ethtool_ringparam  ethtool_ringparam = { .cmd = ETHTOOL_GRINGPARAM };
    te_errno                  rc;

    if_context = eth_feature_iface_context(ifname);
    if (if_context == NULL || !if_context->valid)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (val < 0)
    {
        ERROR("%s(): invalid value '%" PRId64 "'", __FUNCTION__, val);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = eth_feature_ioctl_send(ifname, &ethtool_ringparam);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    ethtool_ringparam.cmd = ETHTOOL_SRINGPARAM;

    if (is_rx)
    {
        if ((uint64_t)val > ethtool_ringparam.rx_max_pending)
            return TE_RC(TE_TA_UNIX, TE_ERANGE);
        ethtool_ringparam.rx_pending = val;
    }
    else
    {
        if ((uint64_t)val > ethtool_ringparam.tx_max_pending)
            return TE_RC(TE_TA_UNIX, TE_ERANGE);
        ethtool_ringparam.tx_pending = val;
    }

    rc = eth_feature_ioctl_send(ifname, &ethtool_ringparam);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return 0;
}

static te_errno
eth_ring_rx_current_set(const ta_conf_ctx *ctx, int64_t val)
{
    return eth_ring_size_set(ctx, true, val);
}

static te_errno
eth_ring_tx_current_set(const ta_conf_ctx *ctx, int64_t val)
{
    return eth_ring_size_set(ctx, false, val);
}

static te_errno
eth_channels_ofst_get(cfg_oid  *oid,
                      size_t   *ofp)
{
    switch ((*cfg_oid_inst_subid(oid, 4) << CHAR_BIT) |
            (*cfg_oid_inst_subid(oid, 5)))
    {
        case (('c' << CHAR_BIT) | 'c'):
            *ofp = offsetof(struct ethtool_channels, combined_count);
            break;

        case (('c' << CHAR_BIT) | 'm'):
            *ofp = offsetof(struct ethtool_channels, max_combined);
            break;

        case (('o' << CHAR_BIT) | 'c'):
            *ofp = offsetof(struct ethtool_channels, other_count);
            break;

        case (('o' << CHAR_BIT) | 'm'):
            *ofp = offsetof(struct ethtool_channels, max_other);
            break;

        case (('r' << CHAR_BIT) | 'c'):
            *ofp = offsetof(struct ethtool_channels, rx_count);
            break;

        case (('r' << CHAR_BIT) | 'm'):
            *ofp = offsetof(struct ethtool_channels, max_rx);
            break;

        case (('t' << CHAR_BIT) | 'c'):
            *ofp = offsetof(struct ethtool_channels, tx_count);
            break;

        case (('t' << CHAR_BIT) | 'm'):
            *ofp = offsetof(struct ethtool_channels, max_tx);
            break;

        default:
            return TE_ENOENT;
    }

    return 0;
}

static te_errno
eth_channels_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char                *iface = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_channels    channels = { .cmd = ETHTOOL_GCHANNELS };
    struct eth_if_context     *iface_ctx;
    uint32_t                  *vp = NULL;
    te_errno                   rc;

    iface_ctx = eth_feature_iface_context(iface);
    if (iface_ctx == NULL || !iface_ctx->valid)
    {
        ERROR("%s(): interface context not found", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    rc = eth_feature_ioctl_send(iface, &channels);
    if (rc == 0)
    {
        cfg_oid  *oid_parsed = cfg_convert_oid_str(ta_conf_ctx_oid(ctx));
        size_t    ofst;

        if (oid_parsed == NULL)
        {
            ERROR("%s(): OID parsing failed", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }

        rc = eth_channels_ofst_get(oid_parsed, &ofst);

        cfg_free_oid(oid_parsed);

        if (rc != 0)
        {
            ERROR("%s(): offset search failed: %r", __FUNCTION__, rc);
            return TE_RC(TE_TA_UNIX, rc);
        }

        vp = (uint32_t *)((uint8_t *)&channels + ofst);
    }
    else if (rc != TE_EOPNOTSUPP)
    {
        ERROR("%s(): ioctl failed: %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    *val = (vp != NULL) ? (int32_t)*vp : -1;

    return 0;
}

static te_errno
eth_channels_set(const ta_conf_ctx *ctx, int32_t val)
{
    const char                *iface = ta_conf_ctx_inst(ctx, "interface");
    struct ethtool_channels    channels = { .cmd = ETHTOOL_GCHANNELS };
    cfg_oid                   *oid_parsed;
    struct eth_if_context     *iface_ctx;
    uint32_t                  *vp = NULL;
    size_t                     ofst;
    te_errno                   rc;

    iface_ctx = eth_feature_iface_context(iface);
    if (iface_ctx == NULL || !iface_ctx->valid)
    {
        ERROR("%s(): interface context not found", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (val < 0)
    {
        ERROR("%s(): invalid value '%" PRId32 "'", __FUNCTION__, val);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    if (val > UINT32_MAX)
    {
        ERROR("%s(): too big value '%" PRId32 "'", __FUNCTION__, val);
        return TE_RC(TE_TA_UNIX, TE_ERANGE);
    }

    rc = eth_feature_ioctl_send(iface, &channels);
    if (rc != 0)
    {
        ERROR("%s(): ioctl failed: %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    oid_parsed = cfg_convert_oid_str(ta_conf_ctx_oid(ctx));
    if (oid_parsed == NULL)
    {
        ERROR("%s(): OID parsing failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    rc = eth_channels_ofst_get(oid_parsed, &ofst);

    cfg_free_oid(oid_parsed);

    if (rc != 0)
    {
        ERROR("%s(): offset search failed: %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    vp = (uint32_t *)((uint8_t *)&channels + ofst);

    channels.cmd = ETHTOOL_SCHANNELS;
    *vp = (uint32_t)val;

    rc = eth_feature_ioctl_send(iface, &channels);
    if (rc != 0)
    {
        /*
         * Not all values make sense in set, but dump everything to have
         * it in log.
         */
        ERROR("%s(): ioctl max_rx=%u max_tx=%u max_other=%u max_combined=%u rx_count=%u tx_count=%u other_count=%u combined_count=%u failed: %r",
              __FUNCTION__, channels.max_rx, channels.max_tx,
              channels.max_other, channels.max_combined, channels.rx_count,
              channels.tx_count, channels.other_count,
              channels.combined_count, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

static const ta_conf_node *const node_deviceinfo =
    TA_CONF_NA("deviceinfo",
        TA_CONF_RO_STR("drivername", eth_drivername_get),
        TA_CONF_RO_STR("driverversion", eth_driverversion_get),
        TA_CONF_RO_STR("firmwareversion", eth_firmwareversion_get));

static const ta_conf_node *const node_feature =
    TA_CONF_NODE((.name = "feature", .type = CVT_BOOL,
                  .get = { .as_bool = eth_feature_get },
                  .set = { .as_bool = eth_feature_set },
                  .list = eth_feature_list,
                  .commit = eth_feature_commit),
        TA_CONF_RO_BOOL("readonly", eth_feature_readonly_get));

static const ta_conf_node *const node_private =
    TA_CONF_NA_COMMIT("private", eth_priv_flags_commit,
        TA_CONF_RW_COLL_BOOL("flag", eth_priv_flags_get,
                             eth_priv_flags_set, eth_priv_flags_list));

static const ta_conf_node *const node_ring =
    TA_CONF_NA("ring",
        TA_CONF_NA("rx",
            TA_CONF_RW_INT64("current", eth_ring_rx_current_get,
                           eth_ring_rx_current_set),
            TA_CONF_RO_INT64("max", eth_ring_rx_max_get)),
        TA_CONF_NA("tx",
            TA_CONF_RW_INT64("current", eth_ring_tx_current_get,
                           eth_ring_tx_current_set),
            TA_CONF_RO_INT64("max", eth_ring_tx_max_get)));

static const ta_conf_node *const node_channels =
    TA_CONF_NA("channels",
        TA_CONF_NA("combined",
            TA_CONF_RW_INT32("current", eth_channels_get, eth_channels_set),
            TA_CONF_RO_INT32("maximum", eth_channels_get)),
        TA_CONF_NA("other",
            TA_CONF_RW_INT32("current", eth_channels_get, eth_channels_set),
            TA_CONF_RO_INT32("maximum", eth_channels_get)),
        TA_CONF_NA("rx",
            TA_CONF_RW_INT32("current", eth_channels_get, eth_channels_set),
            TA_CONF_RO_INT32("maximum", eth_channels_get)),
        TA_CONF_NA("tx",
            TA_CONF_RW_INT32("current", eth_channels_get, eth_channels_set),
            TA_CONF_RO_INT32("maximum", eth_channels_get)));

static const ta_conf_node *const node_msglvl =
    TA_CONF_RW_UINT64("msglvl", eth_msglvl_get, eth_msglvl_set);

static const ta_conf_node *const node_reset =
    TA_CONF_RW_STR("reset", eth_reset_get, eth_reset_set);

/**
 * Initialize ethernet interface configuration nodes
 */
te_errno
ta_unix_conf_eth_init(void)
{
    te_errno rc;

    SLIST_INIT(&if_contexts);

    /*
     * Each ta_conf_register() call prepends its node at the head of
     * "/agent/interface"'s children, exactly like the legacy single
     * rcf_pch_add_node() call used to prepend the whole reset->msglvl->
     * channels->ring->private->feature->deviceinfo chain at once.  To
     * reproduce that order, register in reverse: the last call here
     * (reset) ends up first in the resulting sibling order.
     */
    rc = ta_conf_register("/agent/interface", node_deviceinfo);
    if (rc != 0)
        return rc;

    rc = ta_conf_register("/agent/interface", node_feature);
    if (rc != 0)
        return rc;

    rc = ta_conf_register("/agent/interface", node_private);
    if (rc != 0)
        return rc;

    rc = ta_conf_register("/agent/interface", node_ring);
    if (rc != 0)
        return rc;

    rc = ta_conf_register("/agent/interface", node_channels);
    if (rc != 0)
        return rc;

    rc = ta_conf_register("/agent/interface", node_msglvl);
    if (rc != 0)
        return rc;

    return ta_conf_register("/agent/interface", node_reset);
}
#else
te_errno
ta_unix_conf_eth_init(void)
{
    INFO("Extra ethernet interface configurations are not supported");
    return 0;
}
#endif /* !HAVE_LINUX_ETHTOOL_H */
