/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API for SPDK rpc.py tool routine
 *
 * Test API to control SPDK 'rpc.py' tool.
 *
 * Copyright (C) 2025 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI SPDK RPC"

#include <stddef.h>
#include <signal.h>

#include "tapi_spdk_rpc.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "logger_api.h"

struct tapi_spdk_rpc_app {
    tapi_job_factory_t       *factory;
    char                     *rpc_path;
    tapi_spdk_rpc_server_opt *server_opt;
    tapi_job_channel_t       *out_chs[2];
    tapi_job_channel_t       *error_filter;
};

const tapi_spdk_rpc_server_opt tapi_spdk_rpc_server_default_opt = {
    .server = NULL,
    .timeout = TAPI_JOB_OPT_UINT_UNDEF,
    .port = TAPI_JOB_OPT_UINT_UNDEF,
    .conn_retries = TAPI_JOB_OPT_UINT_UNDEF,
    .verbose = false,
};

/** Server option bindings */
static const tapi_job_opt_bind server_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_STRING("-s", false, tapi_spdk_rpc_server_opt, server),
    TAPI_JOB_OPT_UINT_T("-t", false, NULL, tapi_spdk_rpc_server_opt, timeout),
    TAPI_JOB_OPT_UINT_T("-p", false, NULL, tapi_spdk_rpc_server_opt, port),
    TAPI_JOB_OPT_UINT_T("-r", false, NULL, tapi_spdk_rpc_server_opt,
                        conn_retries),
    TAPI_JOB_OPT_BOOL("-v", tapi_spdk_rpc_server_opt, verbose)
);

/** Bindings for bdev_malloc_create command */
static const tapi_job_opt_bind bdev_malloc_binds[] = TAPI_JOB_OPT_SET(
    /* Positional arguments must come first */
    TAPI_JOB_OPT_UINT(NULL, false, NULL, tapi_spdk_rpc_bdev_malloc_create_opt,
                      size_mb),
    TAPI_JOB_OPT_UINT(NULL, false, NULL, tapi_spdk_rpc_bdev_malloc_create_opt,
                      block_size),
    /* Optional named arguments */
    TAPI_JOB_OPT_STRING("-b", false, tapi_spdk_rpc_bdev_malloc_create_opt,
                        name)
);

/** Bindings for bdev_malloc_delete command */
static const tapi_job_opt_bind bdev_malloc_delete_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_STRING(NULL, false, tapi_spdk_rpc_bdev_malloc_delete_opt,
                        name)
);

static te_errno
create_rpc_job(tapi_spdk_rpc_app *app, const char *method,
               const tapi_job_opt_bind *binds, const void *opt,
               tapi_job_t **job)
{
    te_vec   args = TE_VEC_INIT(char *);
    te_errno rc;

    rc = tapi_job_opt_build_args(app->rpc_path, server_binds, app->server_opt,
                                 &args);
    if (rc != 0)
    {
        ERROR("Failed to build server arguments: %r", rc);
        return rc;
    }

    /* The previous function added 'NULL' to the end of the vector */
    te_vec_remove_index(&args, te_vec_size(&args) - 1);

    rc = te_vec_append_str_fmt(&args, "%s", method);
    if (rc != 0)
    {
        te_vec_deep_free(&args);
    }


    if (binds != NULL && opt != NULL)
    {
        rc = tapi_job_opt_append_args(binds, opt, &args);
        if (rc != 0)
        {
            ERROR("Failed to build method arguments: %r", rc);
            te_vec_deep_free(&args);
            return rc;
        }
    }

    rc = tapi_job_simple_create(
        app->factory,
        &(tapi_job_simple_desc_t){.program = app->rpc_path,
                                  .argv = (const char **)args.data.ptr,
                                  .job_loc = job,
                                  .stdout_loc = &app->out_chs[0],
                                  .stderr_loc = &app->out_chs[1],
                                  .filters = TAPI_JOB_SIMPLE_FILTERS(
                                      {
                                          .use_stderr = true,
                                          .log_level = TE_LL_ERROR,
                                          .readable = true,
                                          .filter_name = "RPC stderr",
                                      },
                                      {
                                          .use_stdout = true,
                                          .log_level = TE_LL_INFO,
                                          .readable = true,
                                          .filter_var = &app->error_filter,
                                          .filter_name = "RPC stdout",
                                      })});
    if (rc != 0)
    {
        ERROR("Failed to create RPC job: %r", rc);
        te_vec_deep_free(&args);
        return rc;
    }

    return 0;
}

te_errno
tapi_spdk_rpc_do_command(tapi_spdk_rpc_app *app, const char *method,
                         const tapi_job_opt_bind *binds, const void *opt)
{
    tapi_job_t       *job = NULL;
    tapi_job_status_t status;
    te_errno          rc;

    rc = create_rpc_job(app, method, binds, opt, &job);
    if (rc != 0)
    {
        goto out;
    }

    rc = tapi_job_start(job);
    if (rc != 0)
    {
        ERROR("Failed to start RPC command: %r", rc);
        goto out;
    }

    rc = tapi_job_wait(job, -1, &status);
    if (rc != 0)
    {
        ERROR("Failed to wait for RPC command completion: %r", rc);
        goto out;
    }

    /* Check exit status */
    if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
    {
        tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;

        /* Try to read error message */
        if (tapi_job_receive(TAPI_JOB_CHANNEL_SET(app->error_filter), 100,
                             &buf) == 0 &&
            buf.data.len > 0)
        {
            ERROR("RPC command '%s' failed:\n %s", method, buf.data.ptr);
        }
        else
        {
            ERROR("RPC command '%s' failed: type=%d, value=%d", method,
                  status.type, status.value);
        }

        te_string_free(&buf.data);
        rc = TE_RC(TE_TAPI, TE_EFAIL);
        goto out;
    }

out:
    if (job != NULL)
    {
        te_errno rc2;

        rc2 = tapi_job_destroy(job, -1);
        if (rc2 != 0)
        {
            ERROR("Failed to destroy RPC job: %r", rc2);
            rc = rc2;
        }
    }

    return rc;
}

te_errno
tapi_spdk_rpc_create(tapi_job_factory_t *factory, const char *rpc_path,
                     const tapi_spdk_rpc_server_opt *opt,
                     tapi_spdk_rpc_app             **app)
{
    tapi_spdk_rpc_app *result;

    if (factory == NULL || rpc_path == NULL || app == NULL || opt == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    result = TE_ALLOC(sizeof(*result));

    result->factory = factory;
    result->rpc_path = TE_STRDUP(rpc_path);
    result->server_opt = TE_ALLOC(sizeof(*opt));
    memcpy(result->server_opt, opt, sizeof(*opt));

    *app = result;
    return 0;
}

void
tapi_spdk_rpc_destroy(tapi_spdk_rpc_app *app)
{
    if (app == NULL)
        return;

    free(app->rpc_path);
    free(app->server_opt);
    free(app);
}

te_errno
tapi_spdk_rpc_bdev_malloc_create(
    tapi_spdk_rpc_app *app, const tapi_spdk_rpc_bdev_malloc_create_opt *opt)
{
    if (app == NULL || opt == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return tapi_spdk_rpc_do_command(app, "bdev_malloc_create",
                                    bdev_malloc_binds, opt);
}
te_errno
tapi_spdk_rpc_bdev_malloc_delete(
    tapi_spdk_rpc_app *app, const tapi_spdk_rpc_bdev_malloc_delete_opt *opt)
{
    if (app == NULL || opt == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return tapi_spdk_rpc_do_command(app, "bdev_malloc_delete",
                                    bdev_malloc_delete_binds, opt);
}

