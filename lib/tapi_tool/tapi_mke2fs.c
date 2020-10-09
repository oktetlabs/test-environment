/** @file
 * @brief mke2fs tool TAPI
 *
 * TAPI to handle mke2fs tool.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_LGR_USER "TAPI MKE2FS"

#include <signal.h>

#include "tapi_mke2fs.h"
#include "tapi_job_opt.h"
#include "te_alloc.h"

#define TAPI_MKE2FS_TERM_TIMEOUT_MS     1000
#define TAPI_MKE2FS_RECEIVE_TIMEOUT_MS  1000

struct tapi_mke2fs_app {
    /* TAPI job handle */
    tapi_job_t *job;
    /* Output channels handles */
    tapi_job_channel_t *out_chs[2];
    /* Whether it was requested to use journal in tapi_mke2fs_opt */
    te_bool use_journal;
    /* Filter to check journal creation */
    tapi_job_channel_t *journal_filter;
};

const tapi_mke2fs_opt tapi_mke2fs_default_opt = {
    .block_size   = TAPI_JOB_OPT_OMIT_UINT,
    .use_journal  = FALSE,
    .fs_type      = NULL,
    .device       = NULL,
};

static const tapi_job_opt_bind mke2fs_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_UINT_OMITTABLE("-b", FALSE, NULL, tapi_mke2fs_opt, block_size),
    TAPI_JOB_OPT_BOOL("-j", tapi_mke2fs_opt, use_journal),
    TAPI_JOB_OPT_STRING("-t", FALSE, tapi_mke2fs_opt, fs_type),
    TAPI_JOB_OPT_STRING(NULL, FALSE, tapi_mke2fs_opt, device)
);

te_errno
tapi_mke2fs_create(tapi_job_factory_t *factory, const tapi_mke2fs_opt *opt,
                   tapi_mke2fs_app **app)
{
    te_errno          rc;
    tapi_mke2fs_app  *result;
    const char       *path = "mke2fs";
    te_vec            mke2fs_args = TE_VEC_INIT(char *);

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    result->use_journal = opt->use_journal;

    rc = tapi_job_opt_build_args(path, mke2fs_binds, opt, &mke2fs_args);
    if (rc != 0)
    {
        ERROR("Failed to build mke2fs options");
        goto out;
    }

    rc = tapi_job_simple_create(factory,
                        &(tapi_job_simple_desc_t){
                            .program    = path,
                            .argv       = (const char **)mke2fs_args.data.ptr,
                            .job_loc    = &result->job,
                            .stdout_loc = &result->out_chs[0],
                            .stderr_loc = &result->out_chs[1],
                            .filters    = TAPI_JOB_SIMPLE_FILTERS(
                                {
                                    .use_stdout  = TRUE,
                                    .readable    = TRUE,
                                    .re          = "Creating journal .*: done",
                                    .extract     = 0,
                                    .filter_var  = &result->journal_filter,
                                },
                                {
                                    .use_stdout  = TRUE,
                                    .readable    = FALSE,
                                    .log_level   = TE_LL_RING,
                                    .filter_name = "mke2fs stdout",
                                },
                                {
                                    .use_stderr  = TRUE,
                                    .readable    = FALSE,
                                    .log_level   = TE_LL_ERROR,
                                    .filter_name = "mke2fs stderr",
                                }

                            )
                        });
    if (rc != 0)
    {
        ERROR("Failed to create job instance for mke2fs tool");
        goto out;
    }

    *app = result;

out:
    te_vec_deep_free(&mke2fs_args);
    if (rc != 0)
        free(result);

    return rc;
}

te_errno
tapi_mke2fs_start(tapi_mke2fs_app *app)
{
    return tapi_job_start(app->job);
}

te_errno
tapi_mke2fs_wait(tapi_mke2fs_app *app, int timeout_ms)
{
    te_errno          rc;
    tapi_job_status_t status;

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
        return rc;

    TAPI_JOB_CHECK_STATUS(status);

    return 0;
}

te_errno
tapi_mke2fs_kill(tapi_mke2fs_app *app, int signum)
{
    return tapi_job_kill(app->job, signum);
}

te_errno
tapi_mke2fs_stop(tapi_mke2fs_app *app)
{
    return tapi_job_stop(app->job, SIGTERM, TAPI_MKE2FS_TERM_TIMEOUT_MS);
}

te_errno
tapi_mke2fs_destroy(tapi_mke2fs_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_MKE2FS_TERM_TIMEOUT_MS);
    if (rc != 0)
        ERROR("Failed to destroy mke2fs job");

    free(app);

    return rc;
}

te_errno
tapi_mke2fs_check_journal(tapi_mke2fs_app *app)
{
    /* It was not requested to create FS with journal so we do not care */
    if (!app->use_journal)
        return 0;

    if (!tapi_job_filters_have_data(TAPI_JOB_CHANNEL_SET(app->journal_filter),
                                    TAPI_MKE2FS_RECEIVE_TIMEOUT_MS))
    {
        ERROR("The filesystem was created without journal even though "
              "it was requested");
        return TE_RC(TE_TAPI, TE_EPROTO);
    }

    return 0;
}
