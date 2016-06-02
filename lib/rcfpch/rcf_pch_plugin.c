/** @file
 * @brief RCF RPC server plugin support
 *
 * Support the interaction RPC server plugin and configuration tree.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include "rcf_pch_internal.h"
#include "rcf_pch.h"

#include "te_errno.h"
#include "te_queue.h"
#include "te_alloc.h"
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
        unsigned int, const char *, char **);
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
 * @param oid           Full object instence identifier (unused)
 * @param list          The list of RPC server plugin names
 *
 * @return Status code
 */
static te_errno
rpcserver_plugin_list(unsigned int gid, const char *oid, char **list)
{
    rpcserver_plugin *plugin;

    char       *buf;
    unsigned    buflen = INITIAL_BUFFER_SIZE;
    unsigned    filled_len = 0;
    unsigned    name_len;

    UNUSED(gid);
    UNUSED(oid);

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
    strncpy(plugin->_field, value, RCF_MAX_ID);         \
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
    }

    pthread_mutex_unlock(lock);
    return rc;
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
