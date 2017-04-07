/** @file
 * @brief Unix Test Agent
 *
 * Extra ethernet interface configurations
 *
 * Copyright (C) 2004-2013 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
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
        strncpy(features[i].name,
                (char *)(names->data + (i * ETH_GSTRING_LEN)),
                ETH_GSTRING_LEN - 1);
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

    strncpy(if_context->ifname, ifname, IFNAMSIZ - 1);

    rc = eth_feature_alloc_get(if_context);
    if (rc != 0)
    {
       free(if_context);
       return NULL;
    }

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
                 char          **list_out)
{
    te_errno                    rc;
    cfg_oid                    *oid = NULL;
    const char                 *ifname;
    struct eth_if_context     *if_context;
    te_string                   list_container = TE_STRING_INIT;
    unsigned int                i;

    UNUSED(gid);

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

    for (i = 0; i < if_context->nb_features; ++i)
    {
        rc = te_string_append(&list_container, "%s ",
                              if_context->features[i].name);
        if (rc != 0)
            goto fail;
    }

    *list_out = list_container.ptr;

    free(oid);

    return 0;

fail:
    free(oid);
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

/* 'get' method implementation */
static te_errno
eth_feature_get(unsigned int    gid,
                const char     *oid_str,
                char           *value,
                const char     *ifname,
                const char     *feature_name)
{
    te_errno                    rc;
    struct eth_if_context     *if_context;
    unsigned int                feature_index;

    UNUSED(gid);
    UNUSED(oid_str);

    if (feature_name[0] == '\0')
    {
        memset(value, 0, RCF_MAX_VAL);
        return 0;
    }

    if_context = eth_feature_iface_context(ifname);
    if (if_context == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = eth_feature_get_index_by_name(if_context,
                                       feature_name,
                                       &feature_index);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    value[0] = (if_context->features[feature_index].enabled) ? '1' : '0';
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
    struct eth_if_context     *if_context;
    unsigned int                feature_index;
    te_errno                    rc;
    char                       *endp;

    UNUSED(gid);
    UNUSED(oid_str);

    if (feature_name[0] == '\0')
        return 0;

    feature_value = strtol(value, &endp, 10);
    if (*endp != '\0')
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if_context = eth_feature_iface_context(ifname);
    if (if_context == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = eth_feature_get_index_by_name(if_context,
                                       feature_name,
                                       &feature_index);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if_context->features[feature_index].enabled = (feature_value == 1);
    if_context->features[feature_index].need_update = TRUE;


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
    if (if_context == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = eth_feature_set_values(if_context);

    return TE_RC(TE_TA_UNIX, rc);
}

/**
 * Get single ethernet interface configuration value.
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier
 * @param value        Location to save value
 * @param ifname       Interface name
 *
 * @return Status code
 */
static te_errno
eth_cmd_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    struct ifreq            ifr;
    struct ethtool_value    eval;

    UNUSED(gid);

    memset(&ifr, 0, sizeof(ifr));
    memset(&eval, 0, sizeof(eval));

    ifr.ifr_data = (void *)&eval;
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    if (strstr(oid, "/gso:") != NULL)
        eval.cmd = ETHTOOL_GGSO;
#ifdef ETHTOOL_GGRO
    else if (strstr(oid, "/gro:") != NULL)
        eval.cmd = ETHTOOL_GGRO;
#endif
#ifdef ETHTOOL_GTSO
    else if (strstr(oid, "/tso:") != NULL)
        eval.cmd = ETHTOOL_GTSO;
#endif
#ifdef ETHTOOL_GFLAGS
    else if (strstr(oid, "/flags:") != NULL)
        eval.cmd = ETHTOOL_GFLAGS;
#endif
    else
        return TE_EINVAL;

    if (ioctl(cfg_socket, SIOCETHTOOL, &ifr) != 0)
    {
        ERROR("ioctl failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    if (snprintf(value, RCF_MAX_VAL, "%d", eval.data) < 0)
    {
        ERROR("failed to write value to buffer: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/**
 * Set single ethernet interface configuration value.
 *
 * @param gid          Group identifier
 * @param oid          Full object instance identifier
 * @param value        New value
 * @param ifname       Interface name
 *
 * @return Status code
 */
static te_errno
eth_cmd_set(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    struct ifreq            ifr;
    struct ethtool_value    eval;
    char                   *endp;
    int                     rc;
    uint32_t                cmd, data;
    char                    slaves[2][IFNAMSIZ];
    char                    if_par[IF_NAMESIZE];
    int                     slaves_num = 2;
    int                     i;

    UNUSED(gid);
    UNUSED(data);

    memset(&ifr, 0, sizeof(ifr));
    memset(&eval, 0, sizeof(eval));

    ifr.ifr_data = (void *)&eval;
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    eval.data = strtoul(value, &endp, 0);
    if (endp == value || *endp != '\0')
    {
        ERROR("Couldn't convert value '%s' to number, errno %r", value,
              TE_OS_RC(TE_TA_UNIX, errno));
        return TE_EINVAL;
    }

    if (strstr(oid, "/gso:") != NULL)
        cmd = eval.cmd = ETHTOOL_SGSO;
#ifdef ETHTOOL_SGRO
    else if (strstr(oid, "/gro:") != NULL)
        cmd = eval.cmd = ETHTOOL_SGRO;
#endif
#ifdef ETHTOOL_STSO
    else if (strstr(oid, "/tso:") != NULL)
        cmd = eval.cmd = ETHTOOL_STSO;
#endif
#ifdef ETHTOOL_SFLAGS
    else if (strstr(oid, "/flags:") != NULL)
        cmd = eval.cmd = ETHTOOL_SFLAGS;
#endif
#ifdef ETHTOOL_RESET
    else if (strstr(oid, "/reset:") != NULL)
    {
        if (eval.data == 0)
            return 0;
        cmd = eval.cmd = ETHTOOL_RESET;
        data = eval.data = ETH_RESET_ALL;
    }
#endif
    else
        return TE_EINVAL;

    if ((rc = ta_vlan_get_parent(ifname, if_par)) != 0)
        return rc;
    if ((rc = ta_bond_get_slaves(strlen(if_par) == 0 ? ifname : if_par,
                                 slaves, &slaves_num)) != 0)
        return rc;
    if (slaves_num != 0)
    {
        for (i = 0; i < slaves_num; i++)
        {
            eval.cmd = cmd;
#ifdef ETHTOOL_RESET
            if (cmd == ETHTOOL_RESET)
                eval.data = data;
#endif
            strncpy(ifr.ifr_name, slaves[i], sizeof(ifr.ifr_name));

            if (ioctl(cfg_socket, SIOCETHTOOL, &ifr) != 0)
            {
                ERROR("ioctl failed on %s: %s", ifr.ifr_name,
                      strerror(errno));
                return TE_OS_RC(TE_TA_UNIX, errno);
            }
            if (i != slaves_num - 1)
                sleep(10);
        }
        return 0;
    }
    if (strlen(if_par) != 0)
        strncpy(ifr.ifr_name, if_par, sizeof(ifr.ifr_name));

    if (ioctl(cfg_socket, SIOCETHTOOL, &ifr) != 0)
    {
        ERROR("ioctl failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
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

static rcf_pch_cfg_object eth_feature = {
    "feature", 0, NULL, NULL,
    (rcf_ch_cfg_get)eth_feature_get, (rcf_ch_cfg_set)eth_feature_set,
    NULL, NULL, (rcf_ch_cfg_list)eth_feature_list,
    (rcf_ch_cfg_commit)eth_feature_commit, NULL
};



RCF_PCH_CFG_NODE_RW(eth_reset, "reset", NULL, &eth_feature,
                    eth_reset_get, eth_cmd_set);

RCF_PCH_CFG_NODE_RW(eth_gro, "gro", NULL, &eth_reset,
                    eth_cmd_get, eth_cmd_set);

RCF_PCH_CFG_NODE_RW(eth_gso, "gso", NULL, &eth_gro,
                    eth_cmd_get, eth_cmd_set);

RCF_PCH_CFG_NODE_RW(eth_tso, "tso", NULL, &eth_gso,
                    eth_cmd_get, eth_cmd_set);

RCF_PCH_CFG_NODE_RW(eth_flags, "flags", NULL, &eth_tso,
                    eth_cmd_get, eth_cmd_set);

/**
 * Initialize ethernet interface configuration nodes
 */
te_errno
ta_unix_conf_eth_init(void)
{
    SLIST_INIT(&if_contexts);

    return rcf_pch_add_node("/agent/interface", &eth_flags);
}
#else
te_errno
ta_unix_conf_eth_init(void)
{
    INFO("Extra ethernet interface configurations are not supported");
    return 0;
}
#endif /* !HAVE_LINUX_ETHTOOL_H */

