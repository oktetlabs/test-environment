/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI to manage ctorrent
 *
 * TAPI to manage ctorrent – a BitTorrent client.
 *
 * Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI CTORRENT"

#include <signal.h>

#include "tapi_ctorrent.h"
#include "tapi_job_opt.h"
#include "te_alloc.h"
#include "te_sleep.h"

#define TAPI_CTORRENT_CREATE_TERM_TIMEOUT_MS            1000
#define TAPI_CTORRENT_CREATE_CHECK_EXISTS_TIMEOUT_MS    1000
#define TAPI_CTORRENT_MAX_REPETITION                    50
#define TAPI_CTORRENT_WAIT_COMPLETION_TIMEOUT_S         1

static const char *ctorrent_binary = "ctorrent";

struct tapi_ctorrent_app {
    /* TAPI job handle */
    tapi_job_t *job;
    /* Output channel handles */
    tapi_job_channel_t *out_chs[2];
    /* Number of completed pieces filter */
    tapi_job_channel_t *completion_filter;
};

/* Options only for metainfo files creation */
typedef struct tapi_ctorrent_create_opt {
    /* URL of a torrent tracker */
    char *tracker_url;
    /* Metainfo file that will be created */
    const char *metainfo_file;
    /* File or directory to be shared */
    const char *target;
} tapi_ctorrent_create_opt;

static const tapi_job_opt_bind ctorrent_create_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_STRING("-tu", FALSE, tapi_ctorrent_create_opt, tracker_url),
    TAPI_JOB_OPT_STRING("-s",  FALSE, tapi_ctorrent_create_opt, metainfo_file),
    TAPI_JOB_OPT_STRING(NULL,  FALSE, tapi_ctorrent_create_opt, target)
);

static const tapi_job_opt_bind ctorrent_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_UINT_OMITTABLE("-e", FALSE, NULL,
                                tapi_ctorrent_opt, hours_to_seed),
    TAPI_JOB_OPT_STRING("-i", FALSE, tapi_ctorrent_opt, ip),
    TAPI_JOB_OPT_UINT_OMITTABLE("-p", FALSE, NULL, tapi_ctorrent_opt, port),
    TAPI_JOB_OPT_STRING("-s", FALSE, tapi_ctorrent_opt, save_to_file),
    TAPI_JOB_OPT_UINT_OMITTABLE("-M", FALSE, NULL,
                                tapi_ctorrent_opt, max_peers),
    TAPI_JOB_OPT_UINT_OMITTABLE("-m", FALSE, NULL,
                                tapi_ctorrent_opt, min_peers),
    TAPI_JOB_OPT_UINT_OMITTABLE("-D", FALSE, NULL,
                                tapi_ctorrent_opt, download_rate),
    TAPI_JOB_OPT_UINT_OMITTABLE("-U", FALSE, NULL,
                                tapi_ctorrent_opt, upload_rate),
    TAPI_JOB_OPT_STRING(NULL, FALSE, tapi_ctorrent_opt, metainfo_file)
);

const tapi_ctorrent_opt tapi_ctorrent_default_opt = {
    .hours_to_seed = TAPI_JOB_OPT_OMIT_UINT,
    .ip            = NULL,
    .port          = TAPI_JOB_OPT_OMIT_UINT,
    .save_to_file  = NULL,
    .max_peers     = TAPI_JOB_OPT_OMIT_UINT,
    .min_peers     = TAPI_JOB_OPT_OMIT_UINT,
    .download_rate = TAPI_JOB_OPT_OMIT_UINT,
    .upload_rate   = TAPI_JOB_OPT_OMIT_UINT,
    .metainfo_file = NULL,
};

static te_errno
fill_ctorrent_create_opt(tapi_ctorrent_create_opt  *opt,
                         tapi_bttrack_app          *tracker,
                         const char                *metainfo_file,
                         const char                *target)
{
    opt->metainfo_file = metainfo_file;
    opt->target        = target;
    opt->tracker_url   = te_string_fmt("http://%s:%u/announce",
                                       tracker->ip, tracker->port);
    if (opt->tracker_url == NULL)
    {
        ERROR("Failed to process tracker URL");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    return 0;
}

static te_errno
create_ctorrent_create_job(tapi_job_factory_t        *factory,
                           tapi_job_t               **job,
                           tapi_job_channel_t        *stderr,
                           tapi_job_channel_t       **metainfo_exists,
                           tapi_bttrack_app          *tracker,
                           const char                *metainfo_file,
                           const char                *target)
{
    te_errno                  rc;
    tapi_ctorrent_create_opt  opt;
    te_vec                    args = TE_VEC_INIT(char *);

    rc = fill_ctorrent_create_opt(&opt, tracker, metainfo_file, target);
    if (rc != 0)
        return rc;

    rc = tapi_job_opt_build_args(ctorrent_binary, ctorrent_create_binds,
                                 &opt, &args);
    if (rc == 0)
    {
        rc = tapi_job_simple_create(factory,
                            &(tapi_job_simple_desc_t){
                                .program    = ctorrent_binary,
                                .argv       = (const char **)args.data.ptr,
                                .job_loc    = job,
                                .stderr_loc = &stderr,
                                .filters    = TAPI_JOB_SIMPLE_FILTERS(
                                    {
                                        .use_stderr = TRUE,
                                        .readable   = TRUE,
                                        .re         = "file .* already exists",
                                        .extract    = 0,
                                        .filter_var = metainfo_exists
                                    },
                                    {
                                        .use_stderr  = TRUE,
                                        .readable    = FALSE,
                                        .log_level   = TE_LL_ERROR,
                                        .filter_name = "ctorrent create stderr"
                                    }
                                )
                            });
    }

    free(opt.tracker_url);
    te_vec_deep_free(&args);

    return rc;
}

te_errno
tapi_ctorrent_create_metainfo_file(tapi_job_factory_t        *factory,
                                   tapi_bttrack_app          *tracker,
                                   const char                *metainfo_file,
                                   const char                *target,
                                   int                        timeout_ms)
{
    te_errno            rc;
    te_errno            rc_destroy;
    tapi_job_t         *job    = NULL;
    tapi_job_channel_t *stderr = NULL;
    tapi_job_channel_t *metainfo_exists;
    tapi_job_status_t   status;

    rc = create_ctorrent_create_job(factory, &job, stderr, &metainfo_exists,
                                    tracker, metainfo_file, target);
    if (rc != 0)
    {
        ERROR("Failed to create job");
        return rc;
    }

    rc = tapi_job_start(job);
    if (rc != 0)
    {
        ERROR("Failed to start job");
        goto out;
    }

    rc = tapi_job_wait(job, timeout_ms, &status);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EINPROGRESS)
            ERROR("Creation was still in process at the moment of termination");
    }
    else if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
    {
        ERROR("Failed to create metainfo file");

        /* Check whether metainfo file already exists */
        if (tapi_job_filters_have_data(TAPI_JOB_CHANNEL_SET(metainfo_exists),
                               TAPI_CTORRENT_CREATE_CHECK_EXISTS_TIMEOUT_MS))
            rc = TE_RC(TE_TAPI, TE_EEXIST);
        else
            rc = TE_RC(TE_TAPI, TE_ESHCMD);
    }

out:
    rc_destroy = tapi_job_destroy(job, TAPI_CTORRENT_CREATE_TERM_TIMEOUT_MS);
    if (rc != 0)
        return rc;

    if (rc_destroy != 0)
    {
        ERROR("Failed to destroy job");
        return rc_destroy;
    }

    return 0;
}

te_errno
tapi_ctorrent_create_app(tapi_job_factory_t *factory, tapi_ctorrent_opt *opt,
                         tapi_ctorrent_app  **app)
{
    te_errno            rc;
    te_vec              args = TE_VEC_INIT(char *);
    tapi_ctorrent_app  *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
    {
        ERROR("Failed to allocate memory for ctorrent app");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    rc = tapi_job_opt_build_args(ctorrent_binary, ctorrent_binds, opt, &args);
    if (rc != 0)
    {
        ERROR("Failed to build job arguments");
        goto out;
    }

    rc = tapi_job_simple_create(factory,
                        &(tapi_job_simple_desc_t){
                           .program    = ctorrent_binary,
                           .argv       = (const char **)args.data.ptr,
                           .job_loc    = &result->job,
                           .stdout_loc = &result->out_chs[0],
                           .stderr_loc = &result->out_chs[1],
                           .filters    = TAPI_JOB_SIMPLE_FILTERS(
                               {
                                   .use_stdout  = TRUE,
                                   .readable    = TRUE,
                                   .re          = "\\[[0-9]+/[0-9]+/[0-9]+\\]",
                                   .extract     = 0,
                                   .filter_var  = &result->completion_filter
                               },
                               {
                                   .use_stderr  = TRUE,
                                   .readable    = FALSE,
                                   .log_level   = TE_LL_ERROR,
                                   .filter_name = "ctorrent stderr"
                               }
                           )
                        });
    if (rc != 0)
    {
        ERROR("Failed to create job");
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
tapi_ctorrent_start(tapi_ctorrent_app *app)
{
    return tapi_job_start(app->job);
}

te_errno
tapi_ctorrent_kill(tapi_ctorrent_app *app, int signum)
{
    return tapi_job_kill(app->job, signum);
}

te_errno
tapi_ctorrent_stop(tapi_ctorrent_app *app, int timeout_ms)
{
    return tapi_job_stop(app->job, SIGTERM, timeout_ms);
}

te_errno
tapi_ctorrent_destroy(tapi_ctorrent_app *app, int timeout_ms)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, timeout_ms);
    if (rc != 0)
    {
        ERROR("Failed to destroy ctorrent job");
        return rc;
    }

    free(app);

    return 0;
}

static te_errno
parse_completion_status(const char *str, unsigned int *completed_pieces,
                        unsigned int *total_pieces)
{
    int rc;

    rc = sscanf(str, "[%u/%u/%*u]", completed_pieces, total_pieces);
    if (rc != 2)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return 0;
}

static te_errno
read_completion_status(tapi_ctorrent_app *app, int receive_timeout_ms,
                       unsigned int *completed_pieces,
                       unsigned int *total_pieces)
{
    te_errno           rc;
    tapi_job_buffer_t  buf = TAPI_JOB_BUFFER_INIT;

    rc = tapi_job_receive_last(TAPI_JOB_CHANNEL_SET(app->completion_filter),
                               receive_timeout_ms, &buf);
    if (rc != 0 || buf.eos)
    {
        ERROR("Failed to get ctorrent completion status");
        if (rc == 0)
            rc = TE_RC(TE_TAPI, TE_EFAIL);
    }

    if (rc == 0)
    {
        rc = parse_completion_status(buf.data.ptr, completed_pieces,
                                     total_pieces);
        if (rc != 0)
            ERROR("Failed to parse completion status");
    }

    te_string_free(&buf.data);

    return rc;
}

te_errno
tapi_ctorrent_check_completion(tapi_ctorrent_app *app, int receive_timeout_ms,
                               te_bool *completed)
{
    te_errno      rc;
    unsigned int  completed_pieces;
    unsigned int  total_pieces;

    *completed = FALSE;
    rc = read_completion_status(app, receive_timeout_ms, &completed_pieces,
                                &total_pieces);
    if (rc != 0)
        return rc;

    if (completed_pieces > total_pieces)
    {
        ERROR("The number of completed pieces cannot exceed "
              "the total number of pieces");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    else if (completed_pieces == total_pieces)
    {
        *completed = TRUE;
    }

    return 0;
}

te_errno
tapi_ctorrent_wait_completion(tapi_ctorrent_app *app, int receive_timeout_ms)
{
    te_errno      rc;
    unsigned int  completed;
    unsigned int  total;
    unsigned int  completed_cur  = 0;
    unsigned int  repetition_cnt = 0;

    /*
     * Return TE_ETIMEDOUT if the number of completed pieces
     * was not increased for too long
     */
    while (repetition_cnt < TAPI_CTORRENT_MAX_REPETITION)
    {
        rc = read_completion_status(app, receive_timeout_ms,
                                    &completed, &total);
        if (rc != 0)
            return rc;

        if (completed == total)
            return 0;

        if (completed == completed_cur)
        {
            repetition_cnt++;
        }
        else
        {
            completed_cur = completed;
            repetition_cnt = 0;
        }

        te_sleep(TAPI_CTORRENT_WAIT_COMPLETION_TIMEOUT_S);
    }

    return TE_RC(TE_TAPI, TE_ETIMEDOUT);
}
