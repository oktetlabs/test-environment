/** @file
 * @brief Unix Test Agent
 *
 * Extra ethernet interface configurations
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey A. Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
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
#include "te_queue.h"

#if HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#include "te_rpc_sys_socket.h"

#include "conf_oid.h"

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
    te_bool     enabled;
    te_bool     readonly;
    te_bool     need_update;
} eth_feature_entry;

typedef struct eth_if_context {
    SLIST_ENTRY(eth_if_context) links;
    char                        ifname[IFNAMSIZ];
    struct eth_feature_entry   *features;
    unsigned int                nb_features;
    te_bool                     valid;
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
    if (names == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    names->cmd = ETHTOOL_GSTRINGS;
    names->string_set = ETH_SS_FEATURES;
    names->len = nb_features;

    rc = eth_feature_ioctl_send(if_context->ifname, names);
    if (rc != 0)
        goto fail;

    features = TE_ALLOC(nb_features * sizeof(*features));
    if (features == NULL)
    {
        rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        goto fail;
    }

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
    if (e_features == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

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
    if (if_context == NULL)
        return NULL;

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
eth_feature_list(unsigned int    gid,
                 const char     *oid_str,
                 const char     *sub_id,
                 char          **list_out)
{
    te_errno                    rc;
    cfg_oid                    *oid = NULL;
    const char                 *ifname;
    struct eth_if_context     *if_context;
    te_string                   list_container = TE_STRING_INIT;
    unsigned int                i;

    UNUSED(gid);
    UNUSED(sub_id);

    oid = cfg_convert_oid_str(oid_str);
    if (oid == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    ifname = CFG_OID_GET_INST_NAME(oid, 2);

    if_context = eth_feature_iface_context(ifname);
    if (if_context == NULL)
    {
        rc = TE_ENOMEM;
        goto fail;
    }

    for (i = 0; if_context->valid && (i < if_context->nb_features); ++i)
    {
        rc = te_string_append(&list_container, "%s ",
                              if_context->features[i].name);
        if (rc != 0)
            goto fail;
    }

    *list_out = list_container.ptr;

    cfg_free_oid(oid);

    return 0;

fail:
    cfg_free_oid(oid);
    te_string_free(&list_container);

    return TE_RC(TE_TA_UNIX, rc);
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
eth_feature_get(unsigned int    gid,
                const char     *oid_str,
                char           *value,
                const char     *ifname,
                const char     *feature_name)
{
    te_errno rc;
    eth_feature_entry *feature;

    UNUSED(gid);
    UNUSED(oid_str);

    if (feature_name[0] == '\0')
    {
        memset(value, 0, RCF_MAX_VAL);
        return 0;
    }

    rc = eth_feature_get_by_name(ifname, feature_name, &feature);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    value[0] = (feature->enabled) ? '1' : '0';
    value[1] = '\0';

    return 0;
}

/* 'set' method implementation */
static te_errno
eth_feature_set(unsigned int    gid,
                const char     *oid_str,
                char           *value,
                const char     *ifname,
                const char     *feature_name)
{
    int                         feature_value;
    te_errno                    rc;
    char                       *endp;

    eth_feature_entry *feature;

    UNUSED(gid);
    UNUSED(oid_str);

    if (feature_name[0] == '\0')
        return 0;

    rc = eth_feature_get_by_name(ifname, feature_name, &feature);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    feature_value = strtol(value, &endp, 10);
    if (*endp != '\0')
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (feature->enabled == (feature_value == 1))
        return 0;

    if (feature->readonly)
    {
        ERROR("Feature '%s' is read-only on interface '%s' and cannot "
              "be changed", feature_name, ifname);
        return TE_RC(TE_TA_UNIX, TE_EACCES);
    }

    feature->enabled = (feature_value == 1);
    feature->need_update = TRUE;

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
    if (e_features == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

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

       if_context->features[i].need_update = FALSE;
    }
    rc = eth_feature_ioctl_send(if_context->ifname, e_features);

    free(e_features);

    return rc;
}

/* 'commit' method implementation */
static te_errno
eth_feature_commit(unsigned int    gid,
                   const cfg_oid  *oid)
{
    const char             *ifname;
    struct eth_if_context *if_context;
    te_errno                rc;

    UNUSED(gid);

    ifname = CFG_OID_GET_INST_NAME(oid, 2);

    if_context = eth_feature_iface_context(ifname);
    if ((if_context == NULL) || !if_context->valid)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = eth_feature_set_values(if_context);

    return TE_RC(TE_TA_UNIX, rc);
}

/** 'get' method implementation for interface/feature/readonly */
static te_errno
eth_feature_readonly_get(unsigned int gid,
                         const char *oid_str,
                         char *value,
                         const char *ifname,
                         const char *feature_name,
                         const char *inst_name)
{
    eth_feature_entry *feature;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid_str);
    UNUSED(inst_name);

    rc = eth_feature_get_by_name(ifname, feature_name, &feature);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    value[0] = (feature->readonly ? '1' : '0');
    value[1] = '\0';

    return 0;
}

/**
 * Reset ethernet interface.
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier
 * @param value        New value
 * @param ifname       Interface name
 *
 * @return Status code
 */
static te_errno
eth_reset_set(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
#ifdef ETHTOOL_RESET
    struct ethtool_value    eval = {.cmd = ETHTOOL_RESET,
                                    .data = ETH_RESET_ALL};
    struct ifreq            ifr = {.ifr_data = (void *)&eval};

    UNUSED(oid);
    UNUSED(gid);

    if (strcmp(value, "0") == 0)
        return 0;

    te_strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    if (ioctl(cfg_socket, SIOCETHTOOL, &ifr) != 0)
    {
        ERROR("ioctl failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
#else
    UNUSED(oid);
    UNUSED(gid);
    UNUSED(value);
    UNUSED(ifname);
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
}

/**
 * Get reset value (dummy)
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier
 * @param value        Location to save value
 * @param ifname       Interface name
 *
 * @return Status code
 */
static te_errno
eth_reset_get(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);

    *value = 0;

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
 * @param val       Buffer to save the parameter value
 * @param len       The buffer length
 *
 * @return Status code
 */
static te_errno
eth_drvinfo_get(const char *ifname, eth_drvinfo parameter, char *val,
                size_t len)
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

#define CHECK_LEN(_str) \
    do {                                                        \
        if (strlen(_str) >= len)                                \
        {                                                       \
            ERROR("%s,%d: returned %s value is too long",       \
                  __FUNCTION__, __LINE__, #_str);               \
            return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);             \
        }                                                       \
    } while (0)

    switch (parameter)
    {
        case ETH_DRVINFO_DRIVER:
            CHECK_LEN(drvinfo.driver);
            strcpy(val, drvinfo.driver);
            break;

        case ETH_DRVINFO_VERSION:
            CHECK_LEN(drvinfo.version);
            strcpy(val, drvinfo.version);
            break;

        case ETH_DRVINFO_FW_VERSION:
            CHECK_LEN(drvinfo.fw_version);
            strcpy(val, drvinfo.fw_version);
            break;

        default:
            ERROR("Unknown parameter value %d", parameter);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#undef CHECK_LEN

    return 0;
}

/**
 * Get firmware version using ioctl(SIOCETHTOOL).
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier
 * @param value        Location to save value
 * @param ifname       Interface name
 * @param deviceinfo   deviceinfo instance name
 *
 * @return Status code
 */
static te_errno
eth_firmwareversion_get(unsigned int gid, const char *oid, char *value,
                        const char *ifname, const char *deviceinfo)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(deviceinfo);

    return eth_drvinfo_get(ifname, ETH_DRVINFO_FW_VERSION, value,
                           RCF_MAX_VAL);
}

/**
 * Get driver version using ioctl(SIOCETHTOOL).
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier
 * @param value        Location to save value
 * @param ifname       Interface name
 * @param deviceinfo   deviceinfo instance name
 *
 * @return Status code
 */
static te_errno
eth_driverversion_get(unsigned int gid, const char *oid, char *value,
                      const char *ifname, const char *deviceinfo)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(deviceinfo);

    return eth_drvinfo_get(ifname, ETH_DRVINFO_VERSION, value, RCF_MAX_VAL);
}

/**
 * Get driver name using ioctl(SIOCETHTOOL).
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier
 * @param value        Location to save value
 * @param ifname       Interface name
 * @param deviceinfo   deviceinfo instance name
 *
 * @return Status code
 */
static te_errno
eth_drivername_get(unsigned int gid, const char *oid, char *value,
                   const char *ifname, const char *deviceinfo)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(deviceinfo);

    return eth_drvinfo_get(ifname, ETH_DRVINFO_DRIVER, value, RCF_MAX_VAL);
}

/* Get driver message level */
static te_errno
eth_msglvl_get(unsigned int gid, const char *oid_str,
               char *value, const char *ifname)
{
    struct ethtool_value eval;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid_str);

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

    rc = te_snprintf(value, RCF_MAX_VAL, "%u", eval.data);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed: %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }
    return 0;
}

static te_errno
eth_ring_size_get(unsigned int  gid,
                  const char   *oid,
                  char         *value,
                  const char   *ifname,
                  bool          is_rx,
                  bool          get_maximum)
{
    struct eth_if_context    *if_context;
    struct ethtool_ringparam  ethtool_ringparam = { .cmd = ETHTOOL_GRINGPARAM };
    te_errno                  rc;

    UNUSED(gid);
    UNUSED(oid);

    if_context = eth_feature_iface_context(ifname);
    if (if_context == NULL || !if_context->valid)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = eth_feature_ioctl_send(ifname, &ethtool_ringparam);
    if (rc == TE_EOPNOTSUPP)
        snprintf(value, RCF_MAX_VAL, "%d", -1);
    else if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);
    else if (get_maximum)
    {
        snprintf(value, RCF_MAX_VAL, "%u", (is_rx ?
                 ethtool_ringparam.rx_max_pending :
                 ethtool_ringparam.tx_max_pending));
    }
    else
    {
        snprintf(value, RCF_MAX_VAL, "%u", (is_rx ?
                 ethtool_ringparam.rx_pending :
                 ethtool_ringparam.tx_pending));
    }

    return 0;
}

static te_errno
eth_ring_tx_max_get(unsigned int  gid,
                    const char   *oid,
                    char         *value,
                    const char   *ifname)
{
    return eth_ring_size_get(gid, oid, value, ifname, false, true);
}

static te_errno
eth_ring_rx_max_get(unsigned int  gid,
                    const char   *oid,
                    char         *value,
                    const char   *ifname)
{
    return eth_ring_size_get(gid, oid, value, ifname, true, true);
}

static te_errno
eth_ring_tx_current_get(unsigned int  gid,
                        const char   *oid,
                        char         *value,
                        const char   *ifname)
{
    return eth_ring_size_get(gid, oid, value, ifname, false, false);
}

static te_errno
eth_ring_rx_current_get(unsigned int  gid,
                        const char   *oid,
                        char         *value,
                        const char   *ifname)
{
    return eth_ring_size_get(gid, oid, value, ifname, true, false);
}

/* Set driver message level */
static te_errno
eth_msglvl_set(unsigned int gid, const char *oid_str,
               char *value, const char *ifname)
{
    unsigned long int parsed_val;
    struct ethtool_value eval;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid_str);

    rc = te_strtoul(value, 0, &parsed_val);
    if (rc != 0)
    {
        ERROR("%s(): invalid value '%s': %r", __FUNCTION__, value, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }
    else if (parsed_val > UINT_MAX)
    {
        ERROR("%s(): too big value '%s'", __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, TE_ERANGE);
    }

    memset(&eval, 0, sizeof(eval));
    eval.cmd = ETHTOOL_SMSGLVL;
    eval.data = parsed_val;

    rc = eth_feature_ioctl_send(ifname, &eval);
    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
eth_ring_size_set(unsigned int gid, const char *oid,
                  char *value, const char *ifname, bool is_rx)
{
    struct eth_if_context    *if_context;
    struct ethtool_ringparam  ethtool_ringparam = { .cmd = ETHTOOL_GRINGPARAM };
    unsigned long int         value_ul;
    te_errno                  rc;

    UNUSED(gid);
    UNUSED(oid);

    if_context = eth_feature_iface_context(ifname);
    if (if_context == NULL || !if_context->valid)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoul(value, 10, &value_ul);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    rc = eth_feature_ioctl_send(ifname, &ethtool_ringparam);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    ethtool_ringparam.cmd = ETHTOOL_SRINGPARAM;

    if (is_rx)
    {
        if (value_ul > ethtool_ringparam.rx_max_pending)
            return TE_RC(TE_TA_UNIX, TE_ERANGE);
        ethtool_ringparam.rx_pending = value_ul;
    }
    else
    {
        if (value_ul > ethtool_ringparam.tx_max_pending)
            return TE_RC(TE_TA_UNIX, TE_ERANGE);
        ethtool_ringparam.tx_pending = value_ul;
    }

    rc = eth_feature_ioctl_send(ifname, &ethtool_ringparam);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return 0;
}

static te_errno
eth_ring_rx_current_set(unsigned int  gid,
                        const char   *oid,
                        char         *value,
                        const char   *ifname)
{
    return eth_ring_size_set(gid, oid, value, ifname, true);
}

static te_errno
eth_ring_tx_current_set(unsigned int  gid,
                        const char   *oid,
                        char         *value,
                        const char   *ifname)
{
    return eth_ring_size_set(gid, oid, value, ifname, false);
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
eth_channels_get(unsigned int  *gid,
                 const char    *oid,
                 char          *value,
                 const char    *iface)
{
    struct ethtool_channels   channels = { .cmd = ETHTOOL_GCHANNELS };
    struct eth_if_context    *iface_ctx;
    uint32_t                 *vp = NULL;
    te_errno                  rc;

    UNUSED(gid);

    iface_ctx = eth_feature_iface_context(iface);
    if (iface_ctx == NULL || !iface_ctx->valid)
    {
        ERROR("%s(): interface context not found", __FUNCTION__);
        rc = TE_ENOENT;
        goto exit;
    }

    rc = eth_feature_ioctl_send(iface, &channels);
    if (rc == 0)
    {
        cfg_oid  *oid_parsed = NULL;
        size_t    ofst;

        oid_parsed = cfg_convert_oid_str(oid);
        if (oid_parsed == NULL)
        {
            ERROR("%s(): OID parsing failed", __FUNCTION__);
            rc = TE_EFAULT;
            goto exit;
        }

        rc = eth_channels_ofst_get(oid_parsed, &ofst);

        cfg_free_oid(oid_parsed);

        if (rc != 0)
        {
            ERROR("%s(): offset search failed: %r", __FUNCTION__, rc);
            goto exit;
        }

        vp = (uint32_t *)((uint8_t *)&channels + ofst);
    }
    else if (rc != TE_EOPNOTSUPP)
    {
        ERROR("%s(): ioctl failed: %r", __FUNCTION__, rc);
        goto exit;
    }

    rc = (vp != NULL) ?
         te_snprintf(value, RCF_MAX_VAL, "%u", *vp) :
         te_snprintf(value, RCF_MAX_VAL, "%d", -1);

    if (rc != 0)
        ERROR("%s(): te_snprintf() failed: %r", __FUNCTION__, rc);

exit:
    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
eth_channels_set(unsigned int  *gid,
                 const char    *oid,
                 const char    *value,
                 const char    *iface)
{
    struct ethtool_channels   channels = { .cmd = ETHTOOL_GCHANNELS };
    unsigned long             value_parsed;
    cfg_oid                  *oid_parsed;
    struct eth_if_context    *iface_ctx;
    uint32_t                 *vp = NULL;
    size_t                    ofst;
    te_errno                  rc;

    UNUSED(gid);

    iface_ctx = eth_feature_iface_context(iface);
    if (iface_ctx == NULL || !iface_ctx->valid)
    {
        ERROR("%s(): interface context not found", __FUNCTION__);
        rc = TE_ENOENT;
        goto exit;
    }

    rc = eth_feature_ioctl_send(iface, &channels);
    if (rc != 0)
    {
        ERROR("%s(): ioctl failed: %r", __FUNCTION__, rc);
        goto exit;
    }

    oid_parsed = cfg_convert_oid_str(oid);
    if (oid_parsed == NULL)
    {
        ERROR("%s(): OID parsing failed", __FUNCTION__);
        rc = TE_EFAULT;
        goto exit;
    }

    rc = eth_channels_ofst_get(oid_parsed, &ofst);

    cfg_free_oid(oid_parsed);

    if (rc != 0)
    {
        ERROR("%s(): offset search failed: %r", __FUNCTION__, rc);
        goto exit;
    }

    vp = (uint32_t *)((uint8_t *)&channels + ofst);

    rc = te_strtoul(value, 0, &value_parsed);
    if (rc != 0)
    {
        ERROR("%s(): invalid value '%s': %r", __FUNCTION__, value, rc);
        goto exit;
    }
    else if (value_parsed > UINT32_MAX)
    {
        ERROR("%s(): too big value '%s'", __FUNCTION__, value);
        rc = TE_ERANGE;
        goto exit;
    }

    channels.cmd = ETHTOOL_SCHANNELS;
    *vp = value_parsed;

    rc = eth_feature_ioctl_send(iface, &channels);
    if (rc != 0)
    {
        ERROR("%s(): ioctl failed: %r", __FUNCTION__, rc);
        goto exit;
    }

exit:
    return TE_RC(TE_TA_UNIX, rc);
}

RCF_PCH_CFG_NODE_RO(firmwareversion, "firmwareversion", NULL, NULL,
                    eth_firmwareversion_get);

RCF_PCH_CFG_NODE_RO(driverversion, "driverversion", NULL, &firmwareversion,
                    eth_driverversion_get);

RCF_PCH_CFG_NODE_RO(drivername, "drivername", NULL, &driverversion,
                    eth_drivername_get);

RCF_PCH_CFG_NODE_NA(deviceinfo, "deviceinfo", &drivername, NULL);

RCF_PCH_CFG_NODE_RO(eth_feature_readonly, "readonly", NULL, NULL,
                    eth_feature_readonly_get);

static rcf_pch_cfg_object eth_feature = {
    "feature", 0, &eth_feature_readonly, &deviceinfo,
    (rcf_ch_cfg_get)eth_feature_get, (rcf_ch_cfg_set)eth_feature_set,
    NULL, NULL, (rcf_ch_cfg_list)eth_feature_list,
    (rcf_ch_cfg_commit)eth_feature_commit, NULL, NULL
};

RCF_PCH_CFG_NODE_RO(eth_ring_tx_max, "max", NULL, NULL,
                    eth_ring_tx_max_get);

RCF_PCH_CFG_NODE_RW(eth_ring_tx_current, "current", NULL, &eth_ring_tx_max,
                    eth_ring_tx_current_get, eth_ring_tx_current_set);

RCF_PCH_CFG_NODE_NA(eth_ring_tx, "tx", &eth_ring_tx_current, NULL);

RCF_PCH_CFG_NODE_RO(eth_ring_rx_max, "max", NULL, NULL,
                    eth_ring_rx_max_get);

RCF_PCH_CFG_NODE_RW(eth_ring_rx_current, "current", NULL, &eth_ring_rx_max,
                    eth_ring_rx_current_get, eth_ring_rx_current_set);

RCF_PCH_CFG_NODE_NA(eth_ring_rx, "rx", &eth_ring_rx_current, &eth_ring_tx);

RCF_PCH_CFG_NODE_NA(eth_ring, "ring", &eth_ring_rx, &eth_feature);

RCF_PCH_CFG_NODE_RO(eth_channels_tx_maximum, "maximum",
                    NULL, NULL, eth_channels_get);

RCF_PCH_CFG_NODE_RW(eth_channels_tx_current, "current",
                    NULL, &eth_channels_tx_maximum,
                    eth_channels_get,
                    eth_channels_set);

RCF_PCH_CFG_NODE_NA(eth_channels_tx, "tx", &eth_channels_tx_current, NULL);

RCF_PCH_CFG_NODE_RO(eth_channels_rx_maximum, "maximum",
                    NULL, NULL, eth_channels_get);

RCF_PCH_CFG_NODE_RW(eth_channels_rx_current, "current",
                    NULL, &eth_channels_rx_maximum,
                    eth_channels_get,
                    eth_channels_set);

RCF_PCH_CFG_NODE_NA(eth_channels_rx, "rx", &eth_channels_rx_current,
                    &eth_channels_tx);

RCF_PCH_CFG_NODE_RO(eth_channels_other_maximum, "maximum",
                    NULL, NULL, eth_channels_get);

RCF_PCH_CFG_NODE_RW(eth_channels_other_current, "current",
                    NULL, &eth_channels_other_maximum,
                    eth_channels_get, eth_channels_set);

RCF_PCH_CFG_NODE_NA(eth_channels_other, "other",
                    &eth_channels_other_current,
                    &eth_channels_rx);

RCF_PCH_CFG_NODE_RO(eth_channels_combined_maximum, "maximum",
                    NULL, NULL, eth_channels_get);

RCF_PCH_CFG_NODE_RW(eth_channels_combined_current, "current",
                    NULL, &eth_channels_combined_maximum,
                    eth_channels_get, eth_channels_set);

RCF_PCH_CFG_NODE_NA(eth_channels_combined, "combined",
                    &eth_channels_combined_current,
                    &eth_channels_other);

RCF_PCH_CFG_NODE_NA(eth_channels, "channels",
                    &eth_channels_combined,
                    &eth_ring);

RCF_PCH_CFG_NODE_RW(eth_msglvl, "msglvl", NULL, &eth_channels,
                    eth_msglvl_get, eth_msglvl_set);

RCF_PCH_CFG_NODE_RW(eth_reset, "reset", NULL, &eth_msglvl,
                    eth_reset_get, eth_reset_set);

/**
 * Initialize ethernet interface configuration nodes
 */
te_errno
ta_unix_conf_eth_init(void)
{
    SLIST_INIT(&if_contexts);

    return rcf_pch_add_node("/agent/interface", &eth_reset);
}
#else
te_errno
ta_unix_conf_eth_init(void)
{
    INFO("Extra ethernet interface configurations are not supported");
    return 0;
}
#endif /* !HAVE_LINUX_ETHTOOL_H */

