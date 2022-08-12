/** @file
 * @brief TAPI to manage bttrack torrent tracker
 *
 * TAPI to manage bttrack torrent tracker.
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_LGR_USER "TAPI BTTRACK"

#include <signal.h>

#include "tapi_bttrack.h"
#include "tapi_job_opt.h"
#include "te_alloc.h"

#define TAPI_BTTRACK_DEFAULT_PORT     80
#define TAPI_BTTRACK_TERM_TIMEOUT_MS  1000

static const char *bttrack_binary = "bttrack";

static const tapi_job_opt_bind bttrack_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_STRING("--dfile", FALSE, tapi_bttrack_opt, dfile),
    TAPI_JOB_OPT_UINT_OMITTABLE("--port", FALSE, NULL,
                                            tapi_bttrack_opt, port)
);

const tapi_bttrack_opt tapi_bttrack_default_opt = {
    .dfile = NULL,
    .port  = TAPI_JOB_OPT_OMIT_UINT
};

te_errno
tapi_bttrack_create(tapi_job_factory_t *factory,
                    const char         *ip,
                    tapi_bttrack_opt   *opt,
                    tapi_bttrack_app  **app)
{
    te_errno           rc;
    te_vec             args = TE_VEC_INIT(char *);
    tapi_bttrack_app  *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
    {
        ERROR("Failed to allocate memory for bttrack app");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    if (opt->dfile == NULL)
    {
        ERROR("dfile option must be set!");
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto out;
    }

    if (ip == NULL)
    {
        ERROR("IP address must be specified");
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto out;
    }

    result->ip = ip;
    result->port = (opt->port == TAPI_JOB_OPT_OMIT_UINT) ?
                    TAPI_BTTRACK_DEFAULT_PORT : opt->port;

    rc = tapi_job_opt_build_args(bttrack_binary, bttrack_binds, opt, &args);
    if (rc != 0)
    {
        ERROR("Failed to build job arguments for bttrack");
        goto out;
    }

    rc = tapi_job_simple_create(factory,
                                &(tapi_job_simple_desc_t){
                                    .program    = bttrack_binary,
                                    .argv       = (const char **)args.data.ptr,
                                    .job_loc    = &result->job,
                                    .stdout_loc = &result->out_chs[0],
                                    .stderr_loc = &result->out_chs[1],
                                    .filters    = TAPI_JOB_SIMPLE_FILTERS(
                                        {
                                            .use_stderr  = TRUE,
                                            .readable    = FALSE,
                                            .log_level   = TE_LL_ERROR,
                                            .filter_name = "bttrack's stderr"
                                        }
                                    )
                                });
    if (rc != 0)
    {
        ERROR("Failed to create job for bttrack");
        goto out;
    }

    *app = result;

out:
    te_vec_deep_free(&args);
    if (rc != 0)
        free(result);

    return rc;
}

te_errno
tapi_bttrack_start(tapi_bttrack_app *app)
{
    return tapi_job_start(app->job);
}

te_errno
tapi_bttrack_wait(tapi_bttrack_app *app, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
        return rc;

    TAPI_JOB_CHECK_STATUS(status);

    return 0;
}

te_errno
tapi_bttrack_kill(tapi_bttrack_app *app, int signum)
{
    return tapi_job_kill(app->job, signum);
}

te_errno
tapi_bttrack_stop(tapi_bttrack_app *app)
{
    return tapi_job_stop(app->job, SIGTERM,
                         TAPI_BTTRACK_TERM_TIMEOUT_MS);
}

te_errno
tapi_bttrack_destroy(tapi_bttrack_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_BTTRACK_TERM_TIMEOUT_MS);
    if (rc != 0)
    {
        ERROR("Failed to destroy bttrack app");
        return rc;
    }

    free(app);
    return 0;
}
