/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief redis-benchmark tool TAPI
 *
 * TAPI to handle redis-benchmark tool.
 *
 * Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
 */

#include "tapi_redis_benchmark.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "te_str.h"
#include "logger_api.h"

/**
 * Path to redis-benchmark exec in the case of
 * tapi_redis_benchmark_opt::exec_path is @c NULL.
 */
static const char *redis_benchmark_path = "redis-benchmark";

/* Supported redis-benchmark command line arguments. */
static const tapi_job_opt_bind redis_benchmark_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_SOCKADDR_PTR("-h", FALSE, tapi_redis_benchmark_opt, server),
    TAPI_JOB_OPT_SOCKPORT_PTR("-p", FALSE, tapi_redis_benchmark_opt, server),
    TAPI_JOB_OPT_STRING("-s", FALSE, tapi_redis_benchmark_opt, socket),
    TAPI_JOB_OPT_UINT_T("-c", FALSE, NULL, tapi_redis_benchmark_opt, clients),
    TAPI_JOB_OPT_UINT_T("-n", FALSE, NULL, tapi_redis_benchmark_opt, requests),
    TAPI_JOB_OPT_UINT_T("-d", FALSE, NULL, tapi_redis_benchmark_opt, size),
    TAPI_JOB_OPT_UINT_T("--dbnum", FALSE, NULL, tapi_redis_benchmark_opt,
                        dbnum),
    TAPI_JOB_OPT_UINT_T("-k", FALSE, NULL, tapi_redis_benchmark_opt,
                        keep_alive),
    TAPI_JOB_OPT_UINT_T("-r", FALSE, NULL, tapi_redis_benchmark_opt,
                        keyspacelen),
    TAPI_JOB_OPT_UINT_T("-P", FALSE, NULL, tapi_redis_benchmark_opt, pipelines),
    TAPI_JOB_OPT_BOOL("-e", tapi_redis_benchmark_opt, show_srv_errors),
    TAPI_JOB_OPT_UINT_T("--threads", FALSE, NULL, tapi_redis_benchmark_opt,
                        threads),
    TAPI_JOB_OPT_STRING("-t", FALSE, tapi_redis_benchmark_opt, tests),
    TAPI_JOB_OPT_BOOL("-I", tapi_redis_benchmark_opt, idle)
);

/* Default values of redis-benchmark command line arguments. */
const tapi_redis_benchmark_opt tapi_redis_benchmark_default_opt = {
    .server             = NULL,
    .socket             = NULL,
    .clients            = TAPI_JOB_OPT_UINT_UNDEF,
    .requests           = TAPI_JOB_OPT_UINT_UNDEF,
    .size               = TAPI_JOB_OPT_UINT_UNDEF,
    .dbnum              = TAPI_JOB_OPT_UINT_UNDEF,
    .keep_alive         = TAPI_JOB_OPT_UINT_UNDEF,
    .keyspacelen        = TAPI_JOB_OPT_UINT_UNDEF,
    .pipelines          = TAPI_JOB_OPT_UINT_UNDEF,
    .show_srv_errors    = TRUE,
    .threads            = TAPI_JOB_OPT_UINT_UNDEF,
    .tests              = NULL,
    .idle               = FALSE,
    .exec_path          = NULL
};

/* See description in tapi_redis_benchmark.h */
te_errno
tapi_redis_benchmark_create(tapi_job_factory_t *factory,
                            const tapi_redis_benchmark_opt *opt,
                            tapi_redis_benchmark_app **app)
{
    te_errno rc;
    te_vec args = TE_VEC_INIT(char *);
    tapi_redis_benchmark_app *new_app;
    const char *path = redis_benchmark_path;

    if (factory == NULL || opt == NULL || app == NULL)
    {
        ERROR("%s() arguments cannot be NULL", __func__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    new_app = TE_ALLOC(sizeof(tapi_redis_benchmark_app));

    if (opt->exec_path != NULL)
        path = opt->exec_path;

    rc = tapi_job_opt_build_args(path, redis_benchmark_binds, opt, &args);
    if (rc != 0)
    {
        ERROR("Failed to build redis-benchmark command line arguments: %r", rc);
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
                                    .use_stdout = TRUE,
                                    .readable = TRUE,
                                    .re = "======\\s*([^=]*)\\s======",
                                    .extract = 1,
                                    .filter_var = &new_app->filter_test_name,
                                },
                                {
                                    .use_stdout = TRUE,
                                    .readable = TRUE,
                                    .re = "completed in ([0-9.]*) seconds",
                                    .extract = 1,
                                    .filter_var = &new_app->filter_time,
                                },
                                {
                                    .use_stdout = TRUE,
                                    .readable = TRUE,
                                    .re = "([0-9.]*) requests per second",
                                    .extract = 1,
                                    .filter_var = &new_app->filter_rps,
                                },
                                {
                                    .use_stdout  = TRUE,
                                    .readable    = TRUE,
                                    .log_level   = TE_LL_RING,
                                    .filter_name = "redis-benchmark stdout"
                                },
                                {
                                    .use_stderr  = TRUE,
                                    .readable    = FALSE,
                                    .log_level   = TE_LL_WARN,
                                    .filter_name = "redis-benchmark stderr"
                                }
                            )
                        });
    if (rc != 0)
        ERROR("Failed to create %s job: %r", path, rc);

cleanup:
    te_vec_deep_free(&args);
    if (rc == 0)
        *app = new_app;
    else
        free(new_app);

    return rc;
}

/* See description in tapi_redis_benchmark.h */
te_errno
tapi_redis_benchmark_start(const tapi_redis_benchmark_app *app)
{
    if (app == NULL)
    {
        ERROR("Redis-benchmark app to start job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_start(app->job);
}

/* See description in tapi_redis_benchmark.h */
te_errno
tapi_redis_benchmark_wait(const tapi_redis_benchmark_app *app, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    if (app == NULL)
    {
        ERROR("Redis-benchmark app to wait job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EINPROGRESS)
            RING("Job was still in process at the end of the wait");

        return TE_RC(TE_TAPI, rc);
    }

    TAPI_JOB_CHECK_STATUS(status);
    return 0;
}

/* See description in tapi_redis_benchmark.h */
te_errno
tapi_redis_benchmark_stop(const tapi_redis_benchmark_app *app)
{
    if (app == NULL)
    {
        ERROR("Redis-benchmark app to stop job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_stop(app->job, SIGTERM, TAPI_REDIS_BENCHMARK_TIMEOUT_MS);
}

/* See description in tapi_redis_benchmark.h */
te_errno
tapi_redis_benchmark_kill(const tapi_redis_benchmark_app *app, int signum)
{
    if (app == NULL)
    {
        ERROR("Redis-benchmark app to kill job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_kill(app->job, signum);
}

/* See description in tapi_redis_benchmark.h */
te_errno
tapi_redis_benchmark_destroy(tapi_redis_benchmark_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_REDIS_BENCHMARK_TIMEOUT_MS);
    if (rc != 0)
    {
        ERROR("Failed to destroy redis-benchmark job: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    free(app);
    return 0;
}

/* See description in tapi_redis_benchmark.h */
void
tapi_redis_benchmark_destroy_report(tapi_redis_benchmark_report *entry)
{
    tapi_redis_benchmark_stat *node;

    while (!SLIST_EMPTY(entry))
    {
        node = SLIST_FIRST(entry);
        SLIST_REMOVE_HEAD(entry, next);
        free(node->test_name);
        free(node);
    }
}

/* See description in tapi_redis_benchmark.h */
te_errno
tapi_redis_benchmark_get_report(tapi_redis_benchmark_app *app,
                                tapi_redis_benchmark_report *report)
{
    te_errno rc;
    unsigned int i;
    tapi_redis_benchmark_stat *stat;
    tapi_redis_benchmark_stat *prev = NULL;
    tapi_redis_benchmark_report result = SLIST_HEAD_INITIALIZER(result);

    tapi_job_buffer_t *buf_names = NULL;
    tapi_job_buffer_t *buf_times = NULL;
    tapi_job_buffer_t *buf_rps = NULL;

    unsigned int count_names = 0;
    unsigned int count_times = 0;
    unsigned int count_rps = 0;

    if (app == NULL || report == NULL)
    {
        ERROR("%s() arguments cannot be NULL", __func__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_job_receive_many(TAPI_JOB_CHANNEL_SET(app->filter_test_name),
                               -1, &buf_names, &count_names);
    if (rc == 0)
        rc = tapi_job_receive_many(TAPI_JOB_CHANNEL_SET(app->filter_time),
                                   -1, &buf_times, &count_times);
    if (rc == 0)
        rc = tapi_job_receive_many(TAPI_JOB_CHANNEL_SET(app->filter_rps),
                                   -1, &buf_rps, &count_rps);
    if (rc != 0)
    {
        ERROR("tapi_job_receive_many() returned unexpected result: %r", rc);
        goto cleanup;
    }

    if (count_names != count_times || count_names != count_rps)
    {
        ERROR("%s() the number of filtered items must match", __FUNCTION__);
        rc = TE_RC(TE_TAPI, TE_ERANGE);
        goto cleanup;
    }

    for (i = 0; i < count_names; i++)
    {
        if (buf_names[i].eos)
            break;

        stat = TE_ALLOC(sizeof(tapi_redis_benchmark_stat));

        stat->test_name = strdup(buf_names[i].data.ptr);

        rc = te_strtod(buf_times[i].data.ptr, &stat->time);
        if (rc == 0)
            rc = te_strtod(buf_rps[i].data.ptr, &stat->rps);

        if (rc != 0)
        {
            ERROR("%s() conversion failed with error: %r", __FUNCTION__, rc);
            break;
        }

        if (prev == NULL)
            SLIST_INSERT_HEAD(&result, stat, next);
        else
            SLIST_INSERT_AFTER(prev, stat, next);

        prev = stat;
    }

cleanup:
    tapi_job_buffers_free(buf_names, count_names);
    tapi_job_buffers_free(buf_times, count_times);
    tapi_job_buffers_free(buf_rps, count_rps);

    if (rc == 0)
        *report = result;
    else
        tapi_redis_benchmark_destroy_report(&result);

    return rc;
}

/* See description in tapi_redis_benchmark.h */
tapi_redis_benchmark_stat *
tapi_redis_benchmark_report_get_stat(tapi_redis_benchmark_report *report,
                                     const char *test_name)
{
    tapi_redis_benchmark_stat *iter;
    tapi_redis_benchmark_stat *stat;

    if (report == NULL || test_name == NULL)
    {
        ERROR("%s() arguments connot be NULL", __func__);
        return NULL;
    }

    SLIST_FOREACH(iter, report, next)
    {
        if (strcmp(iter->test_name, test_name) == 0)
        {
            stat = iter;
            break;
        }
    }

    return stat;
}

/* See description in tapi_redis_benchmark.h */
te_errno
tapi_redis_benchmark_report_mi_log(te_mi_logger *logger,
                                   tapi_redis_benchmark_report *report)
{
    tapi_redis_benchmark_stat *iter;
    te_string str = TE_STRING_INIT;

    if (logger == NULL || report == NULL)
    {
        ERROR("%s() arguments connot be NULL", __func__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    SLIST_FOREACH(iter, report, next)
    {
        te_string_reset(&str);
        te_string_append(&str, "Execution time for test %s", iter->test_name);
        te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_LATENCY, str.ptr,
                              TE_MI_MEAS_AGGR_SINGLE, iter->time,
                              TE_MI_MEAS_MULTIPLIER_PLAIN);

        te_string_reset(&str);
        te_string_append(&str, "Requests per second in test %s", iter->test_name);
        te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_RPS, str.ptr,
                              TE_MI_MEAS_AGGR_SINGLE, iter->rps,
                              TE_MI_MEAS_MULTIPLIER_PLAIN);
    }
    te_string_free(&str);

    return 0;
}
