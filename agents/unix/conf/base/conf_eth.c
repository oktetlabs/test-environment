/** @file
 * @brief Unix Test Agent
 *
 * Extra ethernet interface configurations
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey A. Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
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

#ifdef HAVE_STROPTS_H
#include <stropts.h>
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

    if (feature->readonly)
    {
        ERROR("Feature '%s' is read-only on interface '%s' and cannot "
              "be changed", feature_name, ifname);
        return TE_RC(TE_TA_UNIX, TE_EACCES);
    }

    feature_value = strtol(value, &endp, 10);
    if (*endp != '\0')
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

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

static te_errno
eth_ring_rx_max_get(unsigned int  gid,
                    const char   *oid,
                    char         *value,
                    const char   *ifname)
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
    else
        snprintf(value, RCF_MAX_VAL, "%u", ethtool_ringparam.rx_max_pending);

    return 0;
}

static te_errno
eth_ring_rx_current_get(unsigned int  gid,
                        const char   *oid,
                        char         *value,
                        const char   *ifname)
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
    else
        snprintf(value, RCF_MAX_VAL, "%u", ethtool_ringparam.rx_pending);

    return 0;
}

static te_errno
eth_ring_rx_current_set(unsigned int  gid,
                        const char   *oid,
                        char         *value,
                        const char   *ifname)
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

    if (value_ul > ULONG_MAX)
        return TE_RC(TE_TA_UNIX, TE_EOVERFLOW);

    rc = eth_feature_ioctl_send(ifname, &ethtool_ringparam);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (value_ul > ethtool_ringparam.rx_max_pending)
        return TE_RC(TE_TA_UNIX, TE_ERANGE);

    ethtool_ringparam.cmd = ETHTOOL_SRINGPARAM;
    ethtool_ringparam.rx_pending = value_ul;

    rc = eth_feature_ioctl_send(ifname, &ethtool_ringparam);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return 0;
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
    (rcf_ch_cfg_commit)eth_feature_commit, NULL
};

RCF_PCH_CFG_NODE_RO(eth_ring_rx_max, "max", NULL, NULL,
                    eth_ring_rx_max_get);

RCF_PCH_CFG_NODE_RW(eth_ring_rx_current, "current", NULL, &eth_ring_rx_max,
                    eth_ring_rx_current_get, eth_ring_rx_current_set);

RCF_PCH_CFG_NODE_NA(eth_ring_rx, "rx", &eth_ring_rx_current, NULL);

RCF_PCH_CFG_NODE_NA(eth_ring, "ring", &eth_ring_rx, &eth_feature);

RCF_PCH_CFG_NODE_RW(eth_reset, "reset", NULL, &eth_ring,
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

