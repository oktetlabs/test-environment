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

typedef struct ovs_ctx_s {
    te_string         root_path;
    te_string         conf_db_lock_path;
    te_string         conf_db_path;
    te_string         env;

    te_string         dbtool_cmd;
    te_string         dbserver_cmd;
    te_string         vswitchd_cmd;

    pid_t             dbserver_pid;
    pid_t             vswitchd_pid;
} ovs_ctx_t;

static ovs_ctx_t ovs_ctx = {
    .root_path = TE_STRING_INIT,
    .conf_db_lock_path = TE_STRING_INIT,
    .conf_db_path = TE_STRING_INIT,
    .env =TE_STRING_INIT,

    .dbtool_cmd = TE_STRING_INIT,
    .dbserver_cmd = TE_STRING_INIT,
    .vswitchd_cmd = TE_STRING_INIT,

    .dbserver_pid = -1,
    .vswitchd_pid = -1,
};

static ovs_ctx_t *
ovs_ctx_get(const char *ovs)
{
    return (ovs[0] == '\0') ? &ovs_ctx : NULL;
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

    return 0;
}

static te_errno
ovs_stop(ovs_ctx_t *ctx)
{
    INFO("Stopping the facility");

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

RCF_PCH_CFG_NODE_RW(node_ovs_status, "status",
                    NULL, NULL, ovs_status_get, ovs_status_set);

RCF_PCH_CFG_NODE_NA(node_ovs, "ovs", &node_ovs_status, NULL);

static void
ovs_cleanup_static_ctx(void)
{
    INFO("Clearing the facility static context");

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

    if (rc == 0)
        return rcf_pch_add_node("/agent", &node_ovs);

fail:
    ERROR("Failed to initialise the facility");
    ovs_cleanup_static_ctx();

    return TE_RC(TE_TA_UNIX, rc);
}
