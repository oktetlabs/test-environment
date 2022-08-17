/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RCF RPC server plugin support
 *
 * Support the interaction RPC server plugin and configuration tree.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include "rcf_pch_internal.h"
#include "rcf_pch.h"

#include "te_errno.h"
#include "te_queue.h"
#include "te_alloc.h"
#include "te_str.h"
#include "rcf_rpc_defs.h"
#include "tarpc.h"

/**
 * Initial size of the buffer for creation of the list of all elements
 */
#define INITIAL_BUFFER_SIZE     1024

/**
 * Multiplier size of the buffer for creation of the list of all elements
 */
#define MULTIPLIER_BUFFER_SIZE  2

/** Size of delimiter (' ') */
#define SIZEOF_DELIMITER        1

/** Data corresponding to one RPC server plugin */
typedef struct rpcserver_plugin {
    LIST_ENTRY(rpcserver_plugin) next;  /**< Next plugin in the list */

    char    name[RCF_MAX_ID];           /**< RPC server plugin name */

    te_bool enable;                     /**< Status of plugin */

    /** Function name for install callback */
    char    install[RCF_MAX_ID];

    /** Function name for action callback */
    char    action[RCF_MAX_ID];

    /** Function name for uninstall callback */
    char    uninstall[RCF_MAX_ID];
} rpcserver_plugin;

/** List of same RPC server plugins */
typedef LIST_HEAD(rpcserver_plugins, rpcserver_plugin) rpcserver_plugins;

/** The list of all RPC server plugins */
static rpcserver_plugins plugins = LIST_HEAD_INITIALIZER(&plugins);

static te_errno rpcserver_plugin_add(
        unsigned int, const char *, const char *, const char *);
static te_errno rpcserver_plugin_del(
        unsigned int, const char *, const char *);
static te_errno rpcserver_plugin_list(
        unsigned int, const char *,
        const char *, char **);
static te_errno rpcserver_plugin_install_get(
        unsigned int, const char *, char *, const char *);
static te_errno rpcserver_plugin_install_set(
        unsigned int, const char *, char *, const char *);
static te_errno rpcserver_plugin_action_get(
        unsigned int, const char *, char *, const char *);
static te_errno rpcserver_plugin_action_set(
        unsigned int, const char *, char *, const char *);
static te_errno rpcserver_plugin_uninstall_get(
        unsigned int, const char *, char *, const char *);
static te_errno rpcserver_plugin_uninstall_set(
        unsigned int, const char *, char *, const char *);
static te_errno rpcserver_plugin_enable_get(
        unsigned int, const char *, char *, const char *);
static te_errno rpcserver_plugin_enable_set(
        unsigned int, const char *, char *, const char *);


RCF_PCH_CFG_NODE_RW(node_rpcserver_plugin_install, "install",
                    NULL, NULL,
                    rpcserver_plugin_install_get,
                    rpcserver_plugin_install_set);
RCF_PCH_CFG_NODE_RW(node_rpcserver_plugin_action, "action",
                    NULL, &node_rpcserver_plugin_install,
                    rpcserver_plugin_action_get,
                    rpcserver_plugin_action_set);
RCF_PCH_CFG_NODE_RW(node_rpcserver_plugin_uninstall, "uninstall",
                    NULL, &node_rpcserver_plugin_action,
                    rpcserver_plugin_uninstall_get,
                    rpcserver_plugin_uninstall_set);
RCF_PCH_CFG_NODE_RW(node_rpcserver_plugin_enable, "enable",
                    NULL, &node_rpcserver_plugin_uninstall,
                    rpcserver_plugin_enable_get,
                    rpcserver_plugin_enable_set);

RCF_PCH_CFG_NODE_COLLECTION(node_rpcserver_plugin, "rpcserver_plugin",
                            &node_rpcserver_plugin_enable, NULL,
                            rpcserver_plugin_add, rpcserver_plugin_del,
                            rpcserver_plugin_list, NULL);

/** Lock for protection of RPC servers list */
static pthread_mutex_t *lock = NULL;

/** Function which helps to call the RPC functions */
static rcf_pch_rpc_call call = 0;

/**
 * Find the RPC server plugin with specified @p name.
 *
 * @param name  The name of RPC server plugin
 *
 * @return  RPC server plugin handle or @c NULL
 */
static rpcserver_plugin *
find_rpcserver_plugin(const char *name)
{
    rpcserver_plugin *plugin;
    LIST_FOREACH(plugin, &plugins, next)
    {
        if (strcmp(plugin->name, name) == 0)
            return plugin;
    }
    return NULL;
}

/**
 * Create RPC server plugin.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instence identifier (unused)
 * @param value     Value of RPC server plugin (empty string, unused)
 * @param name      RPC server plugin name
 *
 * @return Status code
 */
static te_errno
rpcserver_plugin_add(unsigned int gid, const char *oid, const char *value,
                     const char *name)
{
    rpcserver_plugin *plugin;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    pthread_mutex_lock(lock);
    if (find_rpcserver_plugin(name) != NULL)
    {
        pthread_mutex_unlock(lock);
        return TE_RC(TE_RCF_PCH, TE_EEXIST);
    }

    if ((plugin = (rpcserver_plugin *)calloc(1, sizeof(*plugin))) == NULL)
    {
        pthread_mutex_unlock(lock);
        ERROR("%s(): calloc(1, %u) failed", __FUNCTION__,
              (unsigned)sizeof(*plugin));
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }

    strcpy(plugin->name, name);
    plugin->enable = FALSE;
    LIST_INSERT_HEAD(&plugins, plugin, next);

    pthread_mutex_unlock(lock);
    return 0;
}

/**
 * Delete RPC server plugin.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instence identifier (unused)
 * @param name      RPC server plugin name
 *
 * @return Status code
 */
static te_errno
rpcserver_plugin_del(unsigned int gid, const char *oid, const char *name)
{
    rpcserver_plugin *plugin;

    UNUSED(gid);
    UNUSED(oid);

    pthread_mutex_lock(lock);

    plugin = find_rpcserver_plugin(name);
    if (plugin == NULL)
    {
        pthread_mutex_unlock(lock);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    LIST_REMOVE(plugin, next);
    pthread_mutex_unlock(lock);

    free(plugin);
    return 0;
}

/**
 * Collect the list of RPC server plugin names.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full parent object instance identifier (unused)
 * @Param sub_id        ID of the object to be listed (unused)
 * @param list          The list of RPC server plugin names
 *
 * @return Status code
 */
static te_errno
rpcserver_plugin_list(unsigned int gid, const char *oid,
                      const char *sub_id, char **list)
{
    rpcserver_plugin *plugin;

    char       *buf;
    unsigned    buflen = INITIAL_BUFFER_SIZE;
    unsigned    filled_len = 0;
    unsigned    name_len;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    buf = TE_ALLOC(buflen);
    if (buf == NULL)
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);

    *buf = '\0';
    pthread_mutex_lock(lock);
    LIST_FOREACH(plugin, &plugins, next)
    {
        name_len = strlen(plugin->name);
        while (buflen - filled_len <= name_len)
        {
            char *tmp;

            buflen *= MULTIPLIER_BUFFER_SIZE;
            tmp = realloc(buf, buflen);
            if (tmp == NULL)
            {
                pthread_mutex_unlock(lock);
                free(buf);
                ERROR("%s(): realloc failed", __FUNCTION__);
                return TE_RC(TE_RCF_PCH, TE_ENOMEM);
            }
            buf = tmp;
        }
        memcpy(&buf[filled_len], plugin->name, name_len);
        filled_len += name_len;

        buf[filled_len] = ' ';
        filled_len += SIZEOF_DELIMITER;
    }
    pthread_mutex_unlock(lock);
    if (filled_len != 0)
        buf[filled_len - SIZEOF_DELIMITER] = '\0';

    *list = buf;
    return 0;
}

/**
 * Create the getter for specified field of RPC server plugin.
 *
 * @param _field    The field name of RPC server plugin
 */
#define RPCSERVER_PLUGIN_COMMON_GET(_field)             \
static te_errno                                         \
rpcserver_plugin_ ## _field ## _get(                    \
        unsigned int gid, const char *oid, char *value, \
        const char *name)                               \
{                                                       \
    rpcserver_plugin *plugin;                           \
                                                        \
    UNUSED(gid);                                        \
    UNUSED(oid);                                        \
                                                        \
    pthread_mutex_lock(lock);                           \
    plugin = find_rpcserver_plugin(name);               \
    if (plugin == NULL)                                 \
    {                                                   \
        pthread_mutex_unlock(lock);                     \
        return TE_RC(TE_RCF_PCH, TE_ENOENT);            \
    }                                                   \
                                                        \
    sprintf(value, "%s", plugin->_field);               \
    pthread_mutex_unlock(lock);                         \
    return 0;                                           \
}

/**
 * Create the setter for specified field of RPC server plugin.
 *
 * @param _field    The field name of RPC server plugin
 */
#define RPCSERVER_PLUGIN_COMMON_SET(_field)             \
static te_errno                                         \
rpcserver_plugin_ ## _field ## _set(                    \
        unsigned int gid, const char *oid, char *value, \
        const char *name)                               \
{                                                       \
    rpcserver_plugin *plugin;                           \
                                                        \
    UNUSED(gid);                                        \
    UNUSED(oid);                                        \
                                                        \
    pthread_mutex_lock(lock);                           \
    plugin = find_rpcserver_plugin(name);               \
    if (plugin == NULL)                                 \
    {                                                   \
        pthread_mutex_unlock(lock);                     \
        return TE_RC(TE_RCF_PCH, TE_ENOENT);            \
    }                                                   \
                                                        \
    te_strlcpy(plugin->_field, value, RCF_MAX_ID);      \
    pthread_mutex_unlock(lock);                         \
    return 0;                                           \
}

RPCSERVER_PLUGIN_COMMON_GET(install)
RPCSERVER_PLUGIN_COMMON_SET(install)
RPCSERVER_PLUGIN_COMMON_GET(action)
RPCSERVER_PLUGIN_COMMON_SET(action)
RPCSERVER_PLUGIN_COMMON_GET(uninstall)
RPCSERVER_PLUGIN_COMMON_SET(uninstall)

#undef RPCSERVER_PLUGIN_COMMON_GET
#undef RPCSERVER_PLUGIN_COMMON_SET

/**
 * Get the field @b enable of specified RPC server plugin.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instence identifier (unused)
 * @param value     Value of field @b enable in RPC server plugin
 * @param name      RPC server plugin name
 *
 * @return Status code
 */
static te_errno
rpcserver_plugin_enable_get(unsigned int gid, const char *oid, char *value,
                            const char *name)
{
    rpcserver_plugin *plugin;

    UNUSED(gid);
    UNUSED(oid);

    pthread_mutex_lock(lock);
    plugin = find_rpcserver_plugin(name);
    if (plugin == NULL)
    {
        pthread_mutex_unlock(lock);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    sprintf(value, "%i", plugin->enable);

    pthread_mutex_unlock(lock);
    return 0;
}

/**
 * Enable the RPC server plugin inside the RPC server context.
 *
 * @param rpcs      RPC server handle
 * @param plugin    RPC server plugin handle
 *
 * @return Status code
 */
static te_errno
call_rpcserver_plugin_enable(
        struct rpcserver *rpcs, struct rpcserver_plugin *plugin)
{
    te_errno rc;

    tarpc_rpcserver_plugin_enable_in  in;
    tarpc_rpcserver_plugin_enable_out out;

    RING("Enable the plugin '%s' on RPC server '%s'",
         plugin->name, rcf_pch_rpcserver_get_name(rpcs));

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;

    in.install   = plugin->install;
    in.action    = plugin->action;
    in.uninstall = plugin->uninstall;

    rc = call(rpcs, "rpcserver_plugin_enable", &in, &out);
    if (rc != 0)
        return rc;

    if (out.retval != 0)
    {
        ERROR("RPC rpcserver_plugin_enable() failed "
              "on the server '%s' with plugin '%s': %r",
              rcf_pch_rpcserver_get_name(rpcs),
              plugin->name, out.common._errno);
        return (out.common._errno != 0) ?
                   out.common._errno : TE_RC(TE_RCF_PCH, TE_ECORRUPTED);
    }
    return 0;
}

/**
 * Disable the RPC server plugin inside the RPC server context.
 *
 * @param rpcs      RPC server handle
 * @param plugin    RPC server plugin handle
 *
 * @return Status code
 */
static te_errno
call_rpcserver_plugin_disable(
        struct rpcserver *rpcs, struct rpcserver_plugin *plugin)
{
    te_errno rc;

    tarpc_rpcserver_plugin_disable_in  in;
    tarpc_rpcserver_plugin_disable_out out;

    RING("Disable the plugin '%s' on RPC server '%s'",
         plugin->name, rcf_pch_rpcserver_get_name(rpcs));

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.common.op = RCF_RPC_CALL_WAIT;

    rc = call(rpcs, "rpcserver_plugin_disable", &in, &out);
    if (rc != 0)
        return rc;

    if (out.retval != 0)
    {
        ERROR("RPC rpcserver_plugin_disable() failed "
              "on the server '%s' with plugin '%s': %r",
              rcf_pch_rpcserver_get_name(rpcs),
              plugin->name, out.common._errno);
        return (out.common._errno != 0) ?
                   out.common._errno : TE_RC(TE_RCF_PCH, TE_ECORRUPTED);
    }
    return 0;
}

/**
 * Update the RPC server plugin inside RPC servers contexts.
 *
 * @param plugin    RPC server plugin handle
 *
 * @return Status code
 */
static te_errno
rpcserver_plugin_update(struct rpcserver_plugin *plugin)
{
    /* Function definition for enable/disable RPC server plugin */
    typedef te_errno (*call_plugin_update)(
            struct rpcserver *rpcs, struct rpcserver_plugin *plugin);

    const call_plugin_update plugin_update = plugin->enable ?
                call_rpcserver_plugin_enable :
                call_rpcserver_plugin_disable;

    te_errno            rc = 0;
    struct rpcserver   *rpcs;

    if (plugin->name[0] != '\0')
    {
        rpcs = rcf_pch_find_rpcserver(plugin->name);
        if (rpcs != NULL)
            rc = plugin_update(rpcs, plugin);
    }
    else
    {
        rpcs = rcf_pch_rpcserver_first();
        for (; rpcs != NULL; rpcs = rcf_pch_rpcserver_next(rpcs))
        {
            if (find_rpcserver_plugin(
                        rcf_pch_rpcserver_get_name(rpcs)) != NULL)
                continue;

            rc = plugin_update(rpcs, plugin);
            if (rc != 0)
                break;
        }
    }
    if (rpcs != NULL && rc != 0)
    {
        ERROR("Failed to update plugin '%s' on rpcserver '%s': %r",
              plugin->name, rcf_pch_rpcserver_get_name(rpcs), rc);
    }

    return rc;
}

/**
 * Set the field @b enable of specified RPC server plugin.
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instence identifier (unused)
 * @param value     Value of field @b enable in RPC server plugin
 * @param name      RPC server plugin name
 *
 * @return Status code
 */
static te_errno
rpcserver_plugin_enable_set(unsigned int gid, const char *oid, char *value,
                            const char *name)
{
    te_errno    rc = 0;
    te_bool     enable;

    if (strcmp(value, "1") == 0)
        enable = TRUE;
    else if (strcmp(value, "0") == 0)
        enable = FALSE;
    else
        return TE_RC(TE_RCF_PCH, TE_EINVAL);

    rpcserver_plugin *plugin;

    UNUSED(gid);
    UNUSED(oid);

    pthread_mutex_lock(lock);
    plugin = find_rpcserver_plugin(name);
    if (plugin == NULL)
        rc = TE_RC(TE_RCF_PCH, TE_ENOENT);
    else if (plugin->enable != enable)
    {
        plugin->enable = enable;

        rc = rpcserver_plugin_update(plugin);
    }

    pthread_mutex_unlock(lock);
    return rc;
}

/* See description in rcf_pch_internal.h */
void
rcf_pch_rpcserver_plugin_enable(struct rpcserver *rpcs)
{
    rpcserver_plugin *plugin;
    plugin = find_rpcserver_plugin(
                rcf_pch_rpcserver_get_name(rpcs));
    if (plugin == NULL)
        plugin = find_rpcserver_plugin("");

    if (plugin != NULL && plugin->enable)
        call_rpcserver_plugin_enable(rpcs, plugin);
}

/* See description in rcf_pch_internal.h */
void
rcf_pch_rpcserver_plugin_disable(struct rpcserver *rpcs)
{
    rpcserver_plugin *plugin;
    plugin = find_rpcserver_plugin(
                rcf_pch_rpcserver_get_name(rpcs));
    if (plugin == NULL)
        plugin = find_rpcserver_plugin("");

    if (plugin != NULL && plugin->enable)
        call_rpcserver_plugin_disable(rpcs, plugin);
}

/* See description in rcf_pch_internal.h */
void
rcf_pch_rpcserver_plugin_init(
        pthread_mutex_t *rcf_pch_lock, rcf_pch_rpc_call rcf_pch_call)
{
    lock = rcf_pch_lock;
    call = rcf_pch_call;

    rcf_pch_add_node("/agent", &node_rpcserver_plugin);
}
