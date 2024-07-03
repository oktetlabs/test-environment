/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief redis-server tool TAPI
 *
 * TAPI to handle redis-server tool.
 *
 * Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
 */

#include "tapi_redis_srv.h"
#include "tapi_file.h"
#include "tapi_rpc_unistd.h"
#include "tapi_job_factory_rpc.h"
#include "te_alloc.h"
#include "te_sockaddr.h"

/**
 * Path to redis-server exec in the case of
 * tapi_redis_srv_opt::exec_path is @c NULL.
 */
static const char *redis_srv_path = "redis-server";

/** Map from tapi_redis_srv_loglevel to string. */
static const te_enum_map tapi_redis_srv_loglevel_map[] = {
    {.name = "debug",   .value = TAPI_REDIS_SRV_LOGLEVEL_DEBUG},
    {.name = "verbose", .value = TAPI_REDIS_SRV_LOGLEVEL_VERBOSE},
    {.name = "notice",  .value = TAPI_REDIS_SRV_LOGLEVEL_NOTICE},
    {.name = "warning", .value = TAPI_REDIS_SRV_LOGLEVEL_WARNING},
    TE_ENUM_MAP_END
};

/** Map from tapi_redis_srv_rdl to string. */
static const te_enum_map tapi_redis_srv_rdl_map[] = {
    {.name = "disabled",    .value = TAPI_REDIS_SRV_RDL_DISABLED},
    {.name = "swapdb",      .value = TAPI_REDIS_SRV_RDL_SWAPDB},
    {.name = "on-empty-db", .value = TAPI_REDIS_SRV_RDL_ON_EMPTY_DB},
    TE_ENUM_MAP_END
};

/** Mapping for yes/no parameters of redis-server. */
static const te_enum_map tapi_redis_srv_yesno_map[] = {
    {.name = "yes", .value = TE_BOOL3_TRUE},
    {.name = "no", .value = TE_BOOL3_FALSE},
    TE_ENUM_MAP_END
};

/** Redis-server command line argument. */
static const tapi_job_opt_bind redis_srv_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_SOCKADDR_PTR("--bind ", true, tapi_redis_srv_opt,
                              server),
    TAPI_JOB_OPT_ENUM_BOOL3("--protected-mode ", true, tapi_redis_srv_opt,
                            protected_mode, tapi_redis_srv_yesno_map),
    TAPI_JOB_OPT_SOCKPORT_PTR("--port ", true, tapi_redis_srv_opt,
                              server),
    TAPI_JOB_OPT_UINT_T("--tcp-backlog ", true, NULL, tapi_redis_srv_opt,
                        tcp_backlog),
    TAPI_JOB_OPT_STRING("--unixsocket ", true, tapi_redis_srv_opt,
                        unixsocket),
    TAPI_JOB_OPT_UINT_T("--timeout ", true, NULL, tapi_redis_srv_opt,
                        timeout),
    TAPI_JOB_OPT_UINT_T("--tcp-keepalive ", true, NULL, tapi_redis_srv_opt,
                        tcp_keepalive),
    TAPI_JOB_OPT_ENUM("--loglevel ", true, tapi_redis_srv_opt, loglevel,
                      tapi_redis_srv_loglevel_map),
    TAPI_JOB_OPT_UINT_T("--databases ", true, NULL, tapi_redis_srv_opt,
                        databases),
    TAPI_JOB_OPT_ENUM_BOOL3("--rdbcompression ", true, tapi_redis_srv_opt,
                            rdbcompression, tapi_redis_srv_yesno_map),
    TAPI_JOB_OPT_ENUM_BOOL3("--rdbchecksum ", true, tapi_redis_srv_opt,
                            rdbchecksum, tapi_redis_srv_yesno_map),
    TAPI_JOB_OPT_ENUM_BOOL3("--repl-diskless-sync ", true, tapi_redis_srv_opt,
                            repl_diskless_sync, tapi_redis_srv_yesno_map),
    TAPI_JOB_OPT_ENUM("--repl-diskless-load ", true, tapi_redis_srv_opt,
                      repl_diskless_load, tapi_redis_srv_rdl_map),
    TAPI_JOB_OPT_ENUM_BOOL3("--appendonly ", true, tapi_redis_srv_opt,
                            appendonly, tapi_redis_srv_yesno_map),
    TAPI_JOB_OPT_ENUM_BOOL3("--activerehashing ", true, tapi_redis_srv_opt,
                            activerehashing, tapi_redis_srv_yesno_map),
    TAPI_JOB_OPT_UINT_T("--io-threads ", true, NULL, tapi_redis_srv_opt,
                        io_threads),
    TAPI_JOB_OPT_ENUM_BOOL3("--io-threads-do-reads ", true, tapi_redis_srv_opt,
                            io_threads_do_reads, tapi_redis_srv_yesno_map)
);

/** Redis-server configuration file default options. */
const tapi_redis_srv_opt tapi_redis_srv_default_opt = {
    .server                             = NULL,
    .protected_mode                     = TE_BOOL3_UNKNOWN,
    .tcp_backlog                        = TAPI_JOB_OPT_UINT_UNDEF,
    .unixsocket                         = NULL,
    .timeout                            = TAPI_JOB_OPT_UINT_UNDEF,
    .tcp_keepalive                      = TAPI_JOB_OPT_UINT_UNDEF,
    .loglevel                           = TAPI_JOB_OPT_ENUM_UNDEF,
    .logfile                            = NULL,
    .databases                          = TAPI_JOB_OPT_UINT_UNDEF,
    .rdbcompression                     = TE_BOOL3_UNKNOWN,
    .rdbchecksum                        = TE_BOOL3_UNKNOWN,
    .repl_diskless_sync                 = TE_BOOL3_UNKNOWN,
    .repl_diskless_load                 = TAPI_JOB_OPT_ENUM_UNDEF,
    .appendonly                         = TE_BOOL3_UNKNOWN,
    .activerehashing                    = TE_BOOL3_UNKNOWN,
    .io_threads                         = TAPI_JOB_OPT_UINT_UNDEF,
    .io_threads_do_reads                = TE_BOOL3_UNKNOWN,
    .exec_path                          = NULL
};

/** See description in tapi_redis_srv.h */
te_errno
tapi_redis_srv_create(tapi_job_factory_t *factory,
                      tapi_redis_srv_opt *opt,
                      tapi_redis_srv_app **app)
{
    te_errno            rc = 0;
    te_vec              args = TE_VEC_INIT(char *);
    tapi_redis_srv_app *new_app = NULL;
    const char         *path = redis_srv_path;

    if (app == NULL)
    {
        rc = TE_EINVAL;
        ERROR("Redis-server app to create job can't be NULL");
        goto cleanup;
    }

    new_app = TE_ALLOC(sizeof(*new_app));

    if (opt->exec_path != NULL)
        path = opt->exec_path;

    rc = tapi_job_opt_build_args(path, redis_srv_binds, opt, &args);
    if (rc != 0)
    {
        ERROR("Failed to build redis-server job arguments: %r", rc);
        goto cleanup;
    }

    rc = tapi_job_simple_create(factory,
                        &(tapi_job_simple_desc_t){
                           .program    = path,
                           .argv       = (const char **)args.data.ptr,
                           .job_loc    = &new_app->job,
                           .stdout_loc = &new_app->out_chs[0],
                           .stderr_loc = &new_app->out_chs[1],
                           .filters    = TAPI_JOB_SIMPLE_FILTERS(
                               {
                                   .use_stdout  = true,
                                   .readable    = false,
                                   .log_level   = TE_LL_RING,
                                   .filter_name = "redis-server stdout"
                               },
                               {
                                   .use_stderr  = true,
                                   .readable    = false,
                                   .log_level   = TE_LL_WARN,
                                   .filter_name = "redis-server stderr"
                               }
                           )
                        });
    if (rc != 0)
    {
        ERROR("Failed to create %s job: %r", path, rc);
        goto cleanup;
    }

    *app = new_app;

cleanup:
    te_vec_deep_free(&args);
    if (rc != 0)
        free(new_app);
    return rc;
}

/** See description in tapi_redis_srv.h */
te_errno
tapi_redis_srv_start(const tapi_redis_srv_app *app)
{
    if (app == NULL)
    {
        ERROR("Redis-server app to start job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_start(app->job);
}

/* See description in tapi_redis_srv.h */
te_errno
tapi_redis_srv_wait(const tapi_redis_srv_app *app, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    if (app == NULL)
    {
        ERROR("Redis-server app to wait job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EINPROGRESS)
            RING("Job was still in process at the end of the wait");

        return rc;
    }

    TAPI_JOB_CHECK_STATUS(status);
    return 0;
}

/** See description in tapi_redis_srv.h */
te_errno
tapi_redis_srv_stop(const tapi_redis_srv_app *app)
{
    if (app == NULL)
    {
        ERROR("Redis-server app to stop job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_stop(app->job, SIGTERM, TAPI_REDIS_SRV_TIMEOUT_MS);
}

/** See description in tapi_redis_srv.h */
te_errno
tapi_redis_srv_kill(const tapi_redis_srv_app *app, int signum)
{
    if (app == NULL)
    {
        ERROR("Redis-server app to kill job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_kill(app->job, signum);
}

/** See description in tapi_redis_srv.h */
te_errno
tapi_redis_srv_destroy(tapi_redis_srv_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_REDIS_SRV_TIMEOUT_MS);
    if (rc != 0)
    {
        ERROR("Failed to destroy redis-server job: %r", rc);
        return rc;
    }

    free(app);
    return 0;
}
