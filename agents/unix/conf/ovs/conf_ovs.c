/** @file
 * @brief Unix Test Agent
 *
 * Unix TA Open vSwitch deployment.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER "TA Unix OVS"

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "te_alloc.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_shell_cmd.h"
#include "te_sleep.h"
#include "te_string.h"

#include "agentlib.h"
#include "logger_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "unix_internal.h"

#include "conf_ovs.h"

#define OVS_SLEEP_MS_MAX 256

typedef struct log_module_s {
    char *name;
    char *level;
} log_module_t;

typedef struct interface_entry {
    SLIST_ENTRY(interface_entry)  links;
    char                         *name;
    char                         *type;
    te_bool                       temporary;
    te_bool                       active;
} interface_entry;

typedef SLIST_HEAD(interface_list_t, interface_entry) interface_list_t;

typedef struct ovs_ctx_s {
    te_string         root_path;
    te_string         conf_db_lock_path;
    te_string         conf_db_path;
    te_string         env;

    te_string         dbtool_cmd;
    te_string         dbserver_cmd;
    te_string         vswitchd_cmd;
    te_string         vlog_list_cmd;

    pid_t             dbserver_pid;
    pid_t             vswitchd_pid;

    unsigned int      nb_log_modules;
    log_module_t     *log_modules;

    interface_list_t  interfaces;
} ovs_ctx_t;

static ovs_ctx_t ovs_ctx = {
    .root_path = TE_STRING_INIT,
    .conf_db_lock_path = TE_STRING_INIT,
    .conf_db_path = TE_STRING_INIT,
    .env =TE_STRING_INIT,

    .dbtool_cmd = TE_STRING_INIT,
    .dbserver_cmd = TE_STRING_INIT,
    .vswitchd_cmd = TE_STRING_INIT,
    .vlog_list_cmd = TE_STRING_INIT,

    .dbserver_pid = -1,
    .vswitchd_pid = -1,

    .nb_log_modules = 0,
    .log_modules = NULL,

    .interfaces = SLIST_HEAD_INITIALIZER(interfaces),
};

static const char *log_levels[] = { "EMER", "ERR", "WARN",
                                    "INFO", "DBG", NULL };

static const char *interface_types[] = { "system", "internal", NULL };

static ovs_ctx_t *
ovs_ctx_get(const char *ovs)
{
    return (ovs[0] == '\0') ? &ovs_ctx : NULL;
}

static te_bool
ovs_value_is_valid(const char **supported_values,
                   const char  *test_value)
{
    char **valuep = (char **)supported_values;

    for (; *valuep != NULL; ++valuep)
    {
        if (strcmp(test_value, *valuep) == 0)
            return TRUE;
    }

    return FALSE;
}

static te_errno
ovs_log_get_nb_modules(ovs_ctx_t    *ctx,
                       unsigned int *nb_modules_out)
{
    unsigned int  nb_modules = 0;
    int           out_fd;
    te_errno      rc = 0;
    int           ret;
    FILE         *f;

    INFO("Querying the number of log modules");

    ret = te_shell_cmd(ctx->vlog_list_cmd.ptr, -1, NULL, &out_fd, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-appctl");
        return TE_ECHILD;
    }

    f = fdopen(out_fd, "r");
    if (f == NULL)
    {
        ERROR("Failed to access ovs-appctl output");
        rc = TE_EBADF;
        goto out;
    }

    while (fscanf(f, "%*[^\n]\n") != EOF)
        ++nb_modules;

    *nb_modules_out = nb_modules;

out:
    if (f != NULL)
        fclose(f);
    else
        close(out_fd);

    ta_waitpid(ret, NULL, 0);

    return rc;
}

static te_errno
ovs_log_init_modules(ovs_ctx_t *ctx)
{
    unsigned int  nb_modules = 0;
    log_module_t *modules = NULL;
    unsigned int  entry_idx = 0;
    int           out_fd = -1;
    FILE         *f = NULL;
    int           ret;
    te_errno      rc;

    INFO("Constructing log module context");

    assert(ctx->nb_log_modules == 0);
    assert(ctx->log_modules == NULL);

    rc = ovs_log_get_nb_modules(ctx, &nb_modules);
    if (rc != 0)
    {
        ERROR("Failed to query the number of log modules");
        return rc;
    }

    modules = TE_ALLOC(nb_modules * sizeof(*modules));
    if (modules == NULL)
    {
        ERROR("Failed to allocate log module context storage");
        return TE_ENOMEM;
    }

    ret = te_shell_cmd(ctx->vlog_list_cmd.ptr, -1, NULL, &out_fd, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-appctl");
        rc = TE_ECHILD;
        goto out;
    }

    f = fdopen(out_fd, "r");
    if (f == NULL)
    {
        ERROR("Failed to access ovs-appctl output");
        rc = TE_EBADF;
        goto out;
    }

    while (entry_idx < nb_modules)
    {
        if (fscanf(f, "%ms %ms %*[^\n]\n", &modules[entry_idx].name,
                   &modules[entry_idx].level) == EOF)
        {
            int error_code_set = ferror(f);

            ERROR("Failed to read entry no. %u from ovs-appctl output",
                  entry_idx);
            rc = (error_code_set != 0) ? te_rc_os2te(errno) : TE_ENODATA;
            goto out;
        }

        ++entry_idx;
    }

    ctx->nb_log_modules = nb_modules;
    ctx->log_modules = modules;

out:
    if (rc != 0)
    {
        unsigned int i;

        for (i = 0; i < entry_idx; ++i)
        {
            free(modules[i].level);
            free(modules[i].name);
        }
    }

    if (f != NULL)
        fclose(f);
    else if (out_fd != -1)
        close(out_fd);

    if (ret != -1)
        ta_waitpid(ret, NULL, 0);

    if (rc != 0)
        free(modules);

    return rc;
}

static void
ovs_log_fini_modules(ovs_ctx_t *ctx)
{
    unsigned int i;

    INFO("Dismantling log module context");

    for (i = 0; i < ctx->nb_log_modules; ++i)
    {
        free(ctx->log_modules[i].level);
        free(ctx->log_modules[i].name);
    }

    free(ctx->log_modules);
    ctx->log_modules = NULL;
    ctx->nb_log_modules = 0;
}

static interface_entry *
ovs_interface_alloc(const char *name,
                    const char *type,
                    te_bool     temporary)
{
    interface_entry *interface;

    INFO("Allocating the interface list entry for '%s'", name);

    interface = TE_ALLOC(sizeof(*interface));
    if (interface == NULL)
    {
        ERROR("Failed to allocate memory for the interface context");
        return NULL;
    }

    interface->name = strdup(name);
    if (interface->name == NULL)
    {
        ERROR("Failed to allocate memory for the interface name");
        goto fail;
    }

    interface->type = (type != NULL && type[0] != '\0') ? strdup(type) :
                                                          strdup("system");
    if (interface->type == NULL)
    {
        ERROR("Failed to allocate memory for the interface type");
        goto fail;
    }

    interface->temporary = temporary;
    interface->active = FALSE;

    return interface;

fail:
    free(interface->type);
    free(interface->name);
    free(interface);

    return NULL;
}

static void
ovs_interface_free(interface_entry *interface)
{
    INFO("Freeing the interface list entry for '%s'", interface->name);

    free(interface->type);
    free(interface->name);
    free(interface);
}

static interface_entry *
ovs_interface_find(ovs_ctx_t  *ctx,
                   const char *name)
{
    interface_entry *interface;

    SLIST_FOREACH(interface, &ctx->interfaces, links)
    {
        if (strcmp(name, interface->name) == 0)
            return interface;
    }

    return NULL;
}

static te_errno
ovs_interface_init(ovs_ctx_t        *ctx,
                   const char       *name,
                   const char       *type,
                   te_bool           activate,
                   interface_entry **interface_out)
{
    interface_entry *interface = NULL;

    INFO("Initialising the interface list entry for '%s'", name);

    if (activate)
        interface = ovs_interface_find(ctx, name);

    if (!activate || interface == NULL)
    {
        interface = ovs_interface_alloc(name, type, activate);
        if (interface == NULL)
        {
            ERROR("Failed to allocate the interface list entry");
            return TE_ENOMEM;
        }

        SLIST_INSERT_HEAD(&ctx->interfaces, interface, links);
    }

    if (interface->active)
    {
        ERROR("The interface is already in use");
        return TE_EBUSY;
    }

    interface->active = activate;

    if (interface_out != NULL)
        *interface_out = interface;

    return 0;
}

static void
ovs_interface_fini(ovs_ctx_t       *ctx,
                   interface_entry *interface)
{
    INFO("Finalising the interface list entry for '%s'", interface->name);

    if (interface->temporary == interface->active)
    {
        SLIST_REMOVE(&ctx->interfaces, interface, interface_entry, links);
        ovs_interface_free(interface);
    }
    else if (interface->active)
    {
        interface->active = FALSE;
    }
    else
    {
        assert(FALSE);
    }
}

static void
ovs_interface_fini_all(ovs_ctx_t *ctx)
{
    interface_entry *interface_tmp;
    interface_entry *interface;

    INFO("Finalising the interface list entries");

    SLIST_FOREACH_SAFE(interface, &ctx->interfaces, links, interface_tmp)
    {
        assert(!interface->active);
        ovs_interface_fini(ctx, interface);
    }
}

static void
ovs_daemon_stop(ovs_ctx_t  *ctx,
                const char *name)
{
    te_string cmd = TE_STRING_INIT;
    int       ret;

    INFO("Trying to stop the daemon '%s'", name);

    if (te_string_append(&cmd, "%s ovs-appctl -t %s exit",
                         ctx->env.ptr, name) != 0)
    {
        ERROR("Failed to construct ovs-appctl invocation command line");
        goto out;
    }

    ret = te_shell_cmd(cmd.ptr, -1, NULL, NULL, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-appctl");
        goto out;
    }

    ta_waitpid(ret, NULL, 0);

out:
    te_string_free(&cmd);
}

static te_bool
ovs_dbserver_is_running(ovs_ctx_t *ctx)
{
    return (ctx->dbserver_pid == -1) ?
           FALSE : (ta_waitpid(ctx->dbserver_pid, NULL, WNOHANG) == 0);
}

static te_bool
ovs_vswitchd_is_running(ovs_ctx_t *ctx)
{
    return (ctx->vswitchd_pid == -1) ?
           FALSE : (ta_waitpid(ctx->vswitchd_pid, NULL, WNOHANG) == 0);
}

static te_errno
ovs_dbserver_start(ovs_ctx_t *ctx)
{
    int ret;

    INFO("Starting the database server");

    ret = te_shell_cmd(ctx->dbtool_cmd.ptr, -1, NULL, NULL, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovsdb-tool");
        return TE_ECHILD;
    }

    ta_waitpid(ret, NULL, 0);

    ctx->dbserver_pid = te_shell_cmd(ctx->dbserver_cmd.ptr,
                                     -1, NULL, NULL, NULL);
    if (ctx->dbserver_pid == -1)
    {
        ERROR("Failed to invoke ovsdb-server");
        return TE_ECHILD;
    }

    return 0;
}

static void
ovs_dbserver_stop(ovs_ctx_t *ctx)
{
    unsigned int total_sleep_ms = 0;
    unsigned int sleep_ms = 1;

    INFO("Stopping the database server");

    ovs_daemon_stop(ctx, "ovsdb-server");

    do {
        te_msleep(sleep_ms);

        total_sleep_ms += sleep_ms;
        sleep_ms <<= 1;

        if (!ovs_dbserver_is_running(ctx))
            goto done;
    } while (total_sleep_ms < OVS_SLEEP_MS_MAX);

    ERROR("Failed to perform stop gracefully");
    WARN("Killing the parent process");
    ta_kill_death(ctx->dbserver_pid);

done:
    if (unlink(ctx->conf_db_lock_path.ptr) == -1)
        ERROR("Failed to unlink the database lock file (%r)",
              te_rc_os2te(errno));

    if (unlink(ctx->conf_db_path.ptr) == -1)
        ERROR("Failed to unlink the database file (%r)", te_rc_os2te(errno));

    ctx->dbserver_pid = -1;
}

static te_errno
ovs_vswitchd_start(ovs_ctx_t *ctx)
{
    INFO("Starting the switch daemon");

    ctx->vswitchd_pid = te_shell_cmd(ctx->vswitchd_cmd.ptr,
                                     -1, NULL, NULL, NULL);
    if (ctx->vswitchd_pid == -1)
    {
        ERROR("Failed to invoke ovs-vswitchd");
        return TE_ECHILD;
    }

    return 0;
}

static void
ovs_vswitchd_stop(ovs_ctx_t *ctx)
{
    unsigned int total_sleep_ms = 0;
    unsigned int sleep_ms = 1;

    INFO("Stopping the switch daemon");

    ovs_daemon_stop(ctx, "ovs-vswitchd");

    do {
        te_msleep(sleep_ms);

        total_sleep_ms += sleep_ms;
        sleep_ms <<= 1;

        if (!ovs_vswitchd_is_running(ctx))
            goto done;
    } while (total_sleep_ms < OVS_SLEEP_MS_MAX);

    ERROR("Failed to perform stop gracefully");
    WARN("Killing the parent process");
    ta_kill_death(ctx->vswitchd_pid);

done:
    ctx->vswitchd_pid = -1;
}

static te_errno
ovs_await_resource(ovs_ctx_t  *ctx,
                   const char *resource_name)
{
    te_string    resource_path = TE_STRING_INIT;
    unsigned int total_sleep_ms = 0;
    unsigned int sleep_ms = 1;
    te_errno     rc;

    INFO("Waiting for '%s' to get ready", resource_name);

    rc = te_string_append(&resource_path, "%s/%s",
                          ctx->root_path.ptr, resource_name);
    if (rc != 0)
    {
        ERROR("Failed to construct the resource path");
        return rc;
    }

    do {
        te_msleep(sleep_ms);

        total_sleep_ms += sleep_ms;
        sleep_ms <<= 1;

        if (access(resource_path.ptr, F_OK) == 0)
        {
            te_string_free(&resource_path);
            return 0;
        }
    } while (total_sleep_ms < OVS_SLEEP_MS_MAX);

    ERROR("Failed to wait for the resource to get ready");

    te_string_free(&resource_path);

    return TE_EIO;
}

static te_errno
ovs_start(ovs_ctx_t *ctx)
{
    te_errno rc;

    INFO("Starting the facility");

    rc = ovs_dbserver_start(ctx);
    if (rc != 0)
    {
        ERROR("Failed to start the database server");
        return rc;
    }

    rc = ovs_await_resource(ctx, "ovsdb-server.pid");
    if (rc != 0)
    {
        ERROR("Failed to check the database server responsiveness");
        ovs_dbserver_stop(ctx);
        return rc;
    }

    rc = ovs_vswitchd_start(ctx);
    if (rc != 0)
    {
        ERROR("Failed to start the switch daemon");
        ovs_dbserver_stop(ctx);
        return rc;
    }

    rc = ovs_await_resource(ctx, "ovs-vswitchd.pid");
    if (rc != 0)
    {
        ERROR("Failed to check the switch daemon responsiveness");
        ovs_vswitchd_stop(ctx);
        ovs_dbserver_stop(ctx);
        return rc;
    }

    rc = ovs_log_init_modules(ctx);
    if (rc != 0)
    {
        ERROR("Failed to construct log module context");
        ovs_vswitchd_stop(ctx);
        ovs_dbserver_stop(ctx);
        return rc;
    }

    return 0;
}

static te_errno
ovs_stop(ovs_ctx_t *ctx)
{
    INFO("Stopping the facility");

    ovs_interface_fini_all(ctx);
    ovs_log_fini_modules(ctx);
    ovs_vswitchd_stop(ctx);
    ovs_dbserver_stop(ctx);

    return 0;
}

static int
ovs_is_running(ovs_ctx_t *ctx)
{
    te_bool dbserver_is_running = ovs_dbserver_is_running(ctx);
    te_bool vswitchd_is_running = ovs_vswitchd_is_running(ctx);

    if (dbserver_is_running != vswitchd_is_running)
    {
        WARN("One of the compulsory services was not found running. Stopping.");
        ovs_stop(ctx);
        return 0;
    }
    else
    {
        return (vswitchd_is_running) ? 1 : 0;
    }
}

static te_errno
ovs_status_get(unsigned int  gid,
               const char   *oid,
               char         *value,
               const char   *ovs)
{
    te_errno   rc = 0;
    ovs_ctx_t *ctx;
    int        ret;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Querying the facility status");

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        rc = TE_ENOENT;
        goto out;
    }

    ret = snprintf(value, RCF_MAX_VAL, "%d", ovs_is_running(ctx));
    if (ret < 0)
    {
        ERROR("Failed to indicate status");
        rc = TE_EFAULT;
    }

out:
    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_status_set(unsigned int  gid,
               const char   *oid,
               const char   *value,
               const char   *ovs)
{
    te_bool    enable = !!atoi(value);
    ovs_ctx_t *ctx;
    te_errno   rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Toggling the facility status");

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        rc = TE_ENOENT;
        goto out;
    }

    if (enable == ovs_is_running(ctx))
    {
        INFO("The facility status does not need to be updated");
        return 0;
    }

    rc = (enable) ? ovs_start(ctx) : ovs_stop(ctx);
    if (rc != 0)
        ERROR("Failed to change status");

out:
    return TE_RC(TE_TA_UNIX, rc);
}

static log_module_t *
ovs_log_module_find(ovs_ctx_t  *ctx,
                    const char *name)
{
    unsigned int i;

    for (i = 0; i < ctx->nb_log_modules; ++i)
    {
        if (strcmp(name, ctx->log_modules[i].name) == 0)
            return &ctx->log_modules[i];
    }

    return NULL;
}

static te_errno
ovs_log_get(unsigned int  gid,
            const char   *oid,
            char         *value,
            const char   *ovs,
            const char   *name)
{
    log_module_t *module;
    ovs_ctx_t    *ctx;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Querying log level word for the module '%s'", name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ovs_is_running(ctx) == 0)
    {
        ERROR("The facility is not running");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    module = ovs_log_module_find(ctx, name);
    if (module == NULL)
    {
        ERROR("The log module does not exist");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    strncpy(value, module->level, RCF_MAX_VAL);
    value[RCF_MAX_VAL - 1] = '\0';

    return 0;
}

static te_errno
ovs_log_set(unsigned int  gid,
            const char   *oid,
            const char   *value,
            const char   *ovs,
            const char   *name)
{
    te_string     cmd = TE_STRING_INIT;
    log_module_t *module;
    char         *level;
    ovs_ctx_t    *ctx;
    int           ret;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(oid);

    INFO("Setting log level word '%s' for the module '%s'", value, name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ovs_is_running(ctx) == 0)
    {
        ERROR("The facility is not running");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    module = ovs_log_module_find(ctx, name);
    if (module == NULL)
    {
        ERROR("The log module does not exist");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (!ovs_value_is_valid(log_levels, value))
    {
        ERROR("The log level word is illicit");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    level = strdup(value);
    if (level == NULL)
    {
        ERROR("Failed to copy the log level word");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    rc = te_string_append(&cmd, "%s ovs-appctl -t ovs-vswitchd vlog/set %s:%s",
                          ctx->env.ptr, module->name, level);
    if (rc != 0)
    {
        ERROR("Failed to construct ovs-appctl invocation command line");
        goto out;
    }

    ret = te_shell_cmd(cmd.ptr, -1, NULL, NULL, NULL);
    if (ret == -1)
    {
        ERROR("Failed to invoke ovs-appctl");
        rc = TE_ECHILD;
        goto out;
    }

    ta_waitpid(ret, NULL, 0);

    free(module->level);
    module->level = level;

out:
    te_string_free(&cmd);

    if (rc != 0)
        free(level);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_log_list(unsigned int   gid,
             const char    *oid_str,
             const char    *sub_id,
             char         **list)
{
    te_string    list_container = TE_STRING_INIT;
    te_errno     rc = 0;
    cfg_oid     *oid;
    const char  *ovs;
    ovs_ctx_t   *ctx;
    unsigned int i;

    UNUSED(gid);
    UNUSED(sub_id);

    INFO("Constructing the list of log modules");

    oid = cfg_convert_oid_str(oid_str);
    if (oid == NULL)
    {
        ERROR("Failed to convert the OID string to native OID handle");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    ovs = CFG_OID_GET_INST_NAME(oid, 2);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        rc = TE_ENOENT;
        goto out;
    }

    for (i = 0; i < ctx->nb_log_modules; ++i)
    {
        rc = te_string_append(&list_container, "%s ", ctx->log_modules[i].name);
        if (rc != 0)
        {
            ERROR("Failed to construct the list");
            te_string_free(&list_container);
            goto out;
        }
    }

    *list = list_container.ptr;

out:
    free(oid);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_value_replace(char       **dst,
                  const char  *src)
{
    char *value = strdup(src);

    if (value == NULL)
    {
        ERROR("Failed to copy the value '%s'", src);
        return TE_ENOMEM;
    }

    free(*dst);
    *dst = value;

    return 0;
}

static te_errno
ovs_interface_pick(const char       *ovs,
                   const char       *interface_name,
                   te_bool           writable,
                   ovs_ctx_t       **ctx_out,
                   interface_entry **interface_out)
{
    ovs_ctx_t       *ctx;
    interface_entry *interface;

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ovs_is_running(ctx) == 0)
    {
        ERROR("The facility is not running");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    interface = ovs_interface_find(ctx, interface_name);
    if (interface == NULL)
    {
        ERROR("The interface does not exist");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (writable && interface->active)
    {
        ERROR("The interface is in use");
        return TE_RC(TE_TA_UNIX, TE_EBUSY);
    }

    if (ctx_out != NULL)
        *ctx_out = ctx;

    if (interface_out != NULL)
        *interface_out = interface;

    return 0;
}

static te_errno
ovs_interface_add(unsigned int  gid,
                  const char   *oid,
                  const char   *value,
                  const char   *ovs,
                  const char   *name)
{
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Adding the interface '%s'", name);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (ovs_is_running(ctx) == 0)
    {
        ERROR("The facility is not running");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    if (name[0] == '\0')
    {
        ERROR("The interface name is empty");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    interface = ovs_interface_find(ctx, name);
    if (interface != NULL)
    {
        ERROR("The interface already exists");
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    if (value[0] != '\0')
    {
        if (!ovs_value_is_valid(interface_types, value))
        {
            ERROR("The interface type is unsupported");
            return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
        }
    }

    rc = ovs_interface_init(ctx, name, value, FALSE, NULL);
    if (rc != 0)
        ERROR("Failed to initialise the interface list entry");

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_interface_del(unsigned int  gid,
                  const char   *oid,
                  const char   *ovs,
                  const char   *name)
{
    interface_entry *interface;
    ovs_ctx_t       *ctx;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Removing the interface '%s'", name);

    rc = ovs_interface_pick(ovs, name, TRUE, &ctx, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    ovs_interface_fini(ctx, interface);

    return 0;
}

static te_errno
ovs_interface_get(unsigned int  gid,
                  const char   *oid,
                  char         *value,
                  const char   *ovs,
                  const char   *name)
{
    interface_entry *interface;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Querying the type of the interface '%s'", name);

    rc = ovs_interface_pick(ovs, name, FALSE, NULL, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    strncpy(value, interface->type, RCF_MAX_VAL);
    value[RCF_MAX_VAL - 1] = '\0';

    return 0;
}

static te_errno
ovs_interface_set(unsigned int  gid,
                  const char   *oid,
                  const char   *value,
                  const char   *ovs,
                  const char   *name)
{
    interface_entry *interface;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ovs);

    INFO("Setting the type '%s' for the interface '%s'", value, name);

    rc = ovs_interface_pick(ovs, name, TRUE, NULL, &interface);
    if (rc != 0)
    {
        ERROR("Failed to pick the interface entry");
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (!ovs_value_is_valid(interface_types, value))
    {
        ERROR("The interface type is unsupported");
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    rc = ovs_value_replace(&interface->type, value);
    if (rc != 0)
        ERROR("Failed to store the new interface type value");

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
ovs_interface_list(unsigned int   gid,
                   const char    *oid_str,
                   const char    *sub_id,
                   char         **list)
{
    te_string        list_container = TE_STRING_INIT;
    interface_entry *interface;
    te_errno         rc = 0;
    cfg_oid         *oid;
    const char      *ovs;
    ovs_ctx_t       *ctx;

    UNUSED(gid);
    UNUSED(sub_id);

    INFO("Constructing the list of interfaces");

    oid = cfg_convert_oid_str(oid_str);
    if (oid == NULL)
    {
        ERROR("Failed to convert the OID string to native OID handle");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    ovs = CFG_OID_GET_INST_NAME(oid, 2);

    ctx = ovs_ctx_get(ovs);
    if (ctx == NULL)
    {
        ERROR("Failed to find the facility context");
        rc = TE_ENOENT;
        goto out;
    }

    SLIST_FOREACH(interface, &ctx->interfaces, links)
    {
        rc = te_string_append(&list_container, "%s ", interface->name);
        if (rc != 0)
        {
            ERROR("Failed to construct the list");
            te_string_free(&list_container);
            goto out;
        }
    }

    *list = list_container.ptr;

out:
    free(oid);

    return TE_RC(TE_TA_UNIX, rc);
}

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ovs_interface, "interface",
                               NULL, NULL,
                               ovs_interface_get, ovs_interface_set,
                               ovs_interface_add, ovs_interface_del,
                               ovs_interface_list, NULL);

static rcf_pch_cfg_object node_ovs_log = {
    "log", 0, NULL, &node_ovs_interface,
    (rcf_ch_cfg_get)ovs_log_get, (rcf_ch_cfg_set)ovs_log_set,
    NULL, NULL, (rcf_ch_cfg_list)ovs_log_list, NULL, NULL
};

RCF_PCH_CFG_NODE_RW(node_ovs_status, "status",
                    NULL, &node_ovs_log, ovs_status_get, ovs_status_set);

RCF_PCH_CFG_NODE_NA(node_ovs, "ovs", &node_ovs_status, NULL);

static void
ovs_cleanup_static_ctx(void)
{
    INFO("Clearing the facility static context");

    te_string_free(&ovs_ctx.vlog_list_cmd);
    te_string_free(&ovs_ctx.vswitchd_cmd);
    te_string_free(&ovs_ctx.dbserver_cmd);
    te_string_free(&ovs_ctx.dbtool_cmd);
    te_string_free(&ovs_ctx.env);
    te_string_free(&ovs_ctx.conf_db_path);
    te_string_free(&ovs_ctx.conf_db_lock_path);
    te_string_free(&ovs_ctx.root_path);
}

te_errno
ta_unix_conf_ovs_init(void)
{
    te_errno rc;

    INFO("Initialising the facility static context");

    rc = te_string_append(&ovs_ctx.root_path, "%s", ta_dir);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.conf_db_lock_path,
                          "%s/.conf.db.~lock~", ta_dir);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.conf_db_path, "%s/conf.db", ta_dir);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.env,
                          "OVS_RUNDIR=%s OVS_DBDIR=%s OVS_PKGDATADIR=%s",
                          ta_dir, ta_dir, ta_dir);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.dbtool_cmd,
                          "%s ovsdb-tool create", ovs_ctx.env.ptr);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.dbserver_cmd, "%s ovsdb-server "
                          "--remote=punix:db.sock --pidfile", ovs_ctx.env.ptr);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.vswitchd_cmd,
                          "%s ovs-vswitchd --pidfile", ovs_ctx.env.ptr);
    if (rc != 0)
        goto fail;

    rc = te_string_append(&ovs_ctx.vlog_list_cmd,
                          "%s ovs-appctl -t ovs-vswitchd "
                          "vlog/list | tail -n +3", ovs_ctx.env.ptr);

    if (rc == 0)
        return rcf_pch_add_node("/agent", &node_ovs);

fail:
    ERROR("Failed to initialise the facility");
    ovs_cleanup_static_ctx();

    return TE_RC(TE_TA_UNIX, rc);
}
