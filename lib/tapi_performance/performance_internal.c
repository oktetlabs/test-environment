/** @file
 * @brief Auxiliary functions to performance TAPI
 *
 * Auxiliary functions for internal use in performance TAPI
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */
#define TE_LGR_USER     "TAPI performance"

#include "te_config.h"
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include "performance_internal.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_test_log.h"
#include "tapi_test.h"
#include "te_units.h"

/* Timeout to wait for process to stop */
#define TAPI_PERF_STOP_TIMEOUT_MS (10000)
/* Time to wait till data is ready to read from filter */
#define TAPI_PERF_READ_TIMEOUT_MS (500)

/*
 * Get default timeout according to application options.
 *
 * @param opts          Application options.
 *
 * @return Timeout (seconds).
 */
static int16_t
get_default_timeout(const tapi_perf_opts *opts)
{
    int64_t timeout_sec;

    if (opts->num_bytes > 0)
        timeout_sec = opts->num_bytes * 8 / 10000000;   /* Suppose 10Mbit/s */
    else
        timeout_sec = opts->duration_sec;

    timeout_sec += 10;  /* Just in case of some delay */

    if (timeout_sec > SHRT_MAX)
        timeout_sec = SHRT_MAX;

    return timeout_sec;
}


/* See description in performance_internal.h */
te_errno
perf_app_read_output(tapi_job_channel_t *filter, te_string *str)
{
    tapi_job_buffer_t   buf = TAPI_JOB_BUFFER_INIT;
    te_errno            rc = 0;

    while (!buf.eos && rc == 0)
    {
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(filter),
                              TAPI_PERF_READ_TIMEOUT_MS, &buf);
        if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT)
        {
            /*
             * In this case, server keep alive at a time when the client
             * ended work and we are getting correct data. Timeout let
             * to detect that data was read because we can't get eos while
             * the job isn't stopped.
             */
            rc = 0;
            break;
        }
    }
    if (rc == 0)
        *str = buf.data;

    return rc;
}

/*
 * Prepare a job for running perf tool
 *
 * @param[in]  factory      Job factory
 * @param[in]  args         List with command and its arguments to execute
 *                          (start) application
 * @param[out] job          Job handle
 * @param[out] out_filter   Filter channel of tool stdout
 * @param[out] err_filter   Filter channel of tool stderr
 *
 * @return Status code
 */
static te_errno
perf_app_create_job(tapi_job_factory_t *factory, te_vec *args,
                    tapi_job_t **job,
                    tapi_job_channel_t **out_filter,
                    tapi_job_channel_t **err_filter)
{
    tapi_job_channel_t *out_chs[2];
    char **cmd_args = (char **)args->data.ptr;
    te_errno rc;

    rc = tapi_job_create(factory, NULL, cmd_args[0], (const char **)cmd_args,
                         NULL, job);
    if (rc != 0)
        return rc;

    rc = tapi_job_alloc_output_channels(*job, TE_ARRAY_LEN(out_chs), out_chs);
    if (rc == 0)
    {
        rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_chs[0]),
                                    "Perf_output_filter", TRUE, 0, out_filter);
    }
    if (rc == 0)
    {
        rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(out_chs[1]),
                                    "Perf_error_filter", TRUE, 0, err_filter);
    }

    if (rc != 0)
        tapi_job_destroy(*job, -1);

    return rc;
}

/* See description in performance_internal.h */
te_errno
perf_app_create_job_from_args(tapi_job_factory_t *factory, te_vec *args,
                              tapi_perf_app *app)
{
    tapi_job_t *job;
    tapi_job_channel_t *out_filter;
    tapi_job_channel_t *err_filter;
    te_string cmd = TE_STRING_INIT;
    te_errno rc;
    char **arg;

    TE_VEC_FOREACH(args, arg)
    {
        if (*arg == NULL)
            break;  /* the last item is not an argument but terminator */

        rc = te_string_append(&cmd, "%s ", *arg);
        if (rc != 0)
        {
            te_string_free(&cmd);
            return rc;
        }
    }

    rc = perf_app_create_job(factory, args, &job, &out_filter, &err_filter);
    if (rc != 0)
    {
        te_string_free(&cmd);
        return rc;
    }

    app->job  = job;
    app->cmd  = cmd.ptr;
    app->out_filter = out_filter;
    app->err_filter = err_filter;

    return rc;
}

/* See description in performance_internal.h */
te_errno
perf_client_create(tapi_perf_client *client, tapi_job_factory_t *factory)
{
    te_vec   args = TE_VEC_INIT(char *);
    te_errno rc;

    client->methods->build_args(&args, &client->app.opts);
    rc = perf_app_create_job_from_args(factory, &args, &client->app);
    if (rc == 0)
        client->app.factory = factory;

    te_vec_deep_free(&args);

    return rc;
}

/* See description in performance_internal.h */
te_errno
perf_server_create(tapi_perf_server *server, tapi_job_factory_t *factory)
{
    te_vec   args = TE_VEC_INIT(char *);
    te_errno rc;

    server->methods->build_args(&args, &server->app.opts);
    rc = perf_app_create_job_from_args(factory, &args, &server->app);
    if (rc == 0)
        server->app.factory = factory;

    te_vec_deep_free(&args);

    return rc;
}

/* See description in performance_internal.h */
te_errno
perf_app_start(tapi_perf_app *app)
{

    if (app->job == NULL)
    {
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    else if (tapi_job_is_running(app->job))
    {
        return TE_RC(TE_TAPI, TE_EINPROGRESS);
    }

    RING("Run \"%s\" on %s", app->cmd, tapi_job_factory_ta(app->factory));

    return tapi_job_start(app->job);
}

/* See description in performance_internal.h */
te_errno
perf_app_stop(tapi_perf_app *app)
{
    tapi_job_t *job = app->job;
    tapi_job_status_t status;
    te_errno rc;

    rc = tapi_job_kill(job, SIGTERM);
    if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ESRCH)
        return rc;

    rc = tapi_job_wait(job, TAPI_PERF_STOP_TIMEOUT_MS, &status);
    if (rc == TE_EINPROGRESS)
    {
        rc = tapi_job_kill(job, SIGKILL);
        if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ESRCH)
            return rc;
        rc = tapi_job_wait(job, 0, &status);
    }
    return rc;
}

/* See description in performance_internal.h */
te_errno
perf_app_wait(tapi_perf_app *app, int16_t timeout)
{
    tapi_job_status_t status;
    te_errno rc;

    if (timeout == TAPI_PERF_TIMEOUT_DEFAULT)
        timeout = get_default_timeout(&app->opts);

    rc = tapi_job_wait(app->job, TE_SEC2MS(timeout), &status);
    if (rc == 0 && status.type == TAPI_JOB_STATUS_UNKNOWN)
        rc = TE_EFAIL;

    return rc;
}

/* See description in performance_internal.h */
te_errno
perf_app_check_report(tapi_perf_app *app, tapi_perf_report *report,
                      const char *tag)
{
    size_t i;
    te_errno rc = 0;

    for (i = 0; i < TE_ARRAY_LEN(report->errors); i++)
    {
        if (report->errors[i] > 0)
        {
            rc = TE_RC(TE_TAPI, TE_EFAIL);
            ERROR_VERDICT("%s %s error: %s", tapi_perf_bench2str(app->bench),
                          tag, tapi_perf_error2str(i));
        }
    }
    return rc;
}

/* See description in performance_internal.h */
void
perf_app_dump_output(tapi_perf_app *app, const char *tag)
{
    RING("%s %s stdout:\n%s", tapi_perf_bench2str(app->bench),
         tag, app->stdout.ptr);
    RING("%s %s stderr:\n%s", tapi_perf_bench2str(app->bench),
         tag, app->stderr.ptr);
}

/* See description in performance_internal.h */
void
perf_get_tool_input_tuple(const tapi_perf_server *server,
                          const tapi_perf_client *client,
                          te_string *output_str)
{
    te_unit bandwidth;
    const tapi_perf_opts *opts = &client->app.opts;

    bandwidth = te_unit_pack(opts->bandwidth_bits);

    CHECK_RC(te_string_append(output_str, "ip=%s, ", proto_rpc2str(opts->ipversion)));
    CHECK_RC(te_string_append(output_str, "protocol=%s, ",
                              proto_rpc2str(opts->protocol)));
    CHECK_RC(te_string_append(output_str, "bandwidth=%.1f%sbits/sec, ",
                              bandwidth.value, te_unit_prefix2str(bandwidth.unit)));
    CHECK_RC(te_string_append(output_str, "num_bytes=%"PRId64", ", opts->num_bytes));
    CHECK_RC(te_string_append(output_str, "duration=%"PRId32"sec, ", opts->duration_sec));
    CHECK_RC(te_string_append(output_str, "length=%"PRId32"bytes, ", opts->length));
    CHECK_RC(te_string_append(output_str, "num_streams=%"PRId16", ", opts->streams));
    CHECK_RC(te_string_append(output_str, "server_cmd=\"%s\", ", server->app.cmd));
    CHECK_RC(te_string_append(output_str, "client_cmd=\"%s\", ", client->app.cmd));
}

void
perf_get_tool_result_tuple(const tapi_perf_report *report,
                           te_string *output_str)
{
    te_unit throughput;

    throughput = te_unit_pack(report->bits_per_second);

    CHECK_RC(te_string_append(output_str, "res_num_bytes=%"PRIu64", ", report->bytes));
    CHECK_RC(te_string_append(output_str, "res_time=%.1fsec, ", report->seconds));
    CHECK_RC(te_string_append(output_str, "res_throughput=%.1f%sbits/sec",
                              throughput.value, te_unit_prefix2str(throughput.unit)));
}
