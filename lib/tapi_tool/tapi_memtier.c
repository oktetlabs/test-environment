/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI to manage memtier_benchmark
 *
 * TAPI to manage memtier_benchmark.
 */

#define TE_LGR_USER "TAPI MEMTIER"

#include <signal.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "te_defs.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_sockaddr.h"
#include "tapi_job_opt.h"
#include "te_mi_log.h"
#include "te_enum.h"
#include "tapi_memtier.h"

/* How long to wait for application termination */
#define MEMTIER_TERM_TIMEOUT_MS 10000

/* Default path to memtier_benchmark */
static const char *memtier_path = "memtier_benchmark";

/** Mapping of possible values for --protocol option */
static const te_enum_map memtier_proto_mapping[] = {
    { .name = "redis", .value = TAPI_MEMTIER_PROTO_REDIS },
    { .name = "resp2", .value = TAPI_MEMTIER_PROTO_RESP2 },
    { .name = "resp3", .value = TAPI_MEMTIER_PROTO_RESP3 },
    { .name = "memcache_text", .value = TAPI_MEMTIER_PROTO_MEMCACHE_TEXT },
    { .name = "memcache_binary",
      .value = TAPI_MEMTIER_PROTO_MEMCACHE_BINARY },
    TE_ENUM_MAP_END
};

/* Command line options bindings */
static const tapi_job_opt_bind memtier_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_SOCKADDR_PTR("--server=", true, tapi_memtier_opt, server),
    TAPI_JOB_OPT_SOCKPORT_PTR("--port=", true, tapi_memtier_opt, server),
    TAPI_JOB_OPT_ENUM("--protocol=", true, tapi_memtier_opt, protocol,
                      memtier_proto_mapping),
    TAPI_JOB_OPT_UINT_T("--run-count=", true, NULL, tapi_memtier_opt,
                        run_count),
    TAPI_JOB_OPT_UINT_T("--requests=", true, NULL, tapi_memtier_opt,
                        requests),
    TAPI_JOB_OPT_UINT_T("--clients=", true, NULL, tapi_memtier_opt,
                        clients),
    TAPI_JOB_OPT_UINT_T("--threads=", true, NULL, tapi_memtier_opt,
                        threads),
    TAPI_JOB_OPT_UINT_T("--pipeline=", true, NULL, tapi_memtier_opt,
                        pipeline),
    TAPI_JOB_OPT_UINT_T("--test-time=", true, NULL, tapi_memtier_opt,
                        test_time),
    TAPI_JOB_OPT_UINT_T("--data-size=", true, NULL, tapi_memtier_opt,
                        data_size),
    TAPI_JOB_OPT_BOOL("--random-data", tapi_memtier_opt, random_data),
    TAPI_JOB_OPT_STRING("--ratio=", true, tapi_memtier_opt, ratio),
    TAPI_JOB_OPT_STRING("--key-prefix=", true, tapi_memtier_opt,
                        key_prefix),
    TAPI_JOB_OPT_STRING("--key-pattern=", true, tapi_memtier_opt,
                        key_pattern),
    TAPI_JOB_OPT_UINT_T("--key-minimum=", true, NULL, tapi_memtier_opt,
                        key_minimum),
    TAPI_JOB_OPT_UINT_T("--key-maximum=", true, NULL, tapi_memtier_opt,
                        key_maximum),
    TAPI_JOB_OPT_BOOL("--hide-histogram", tapi_memtier_opt, hide_histogram),
    TAPI_JOB_OPT_BOOL("--debug", tapi_memtier_opt, debug)
);

/* Default values of memtier command line arguments */
const tapi_memtier_opt tapi_memtier_default_opt = {
    .server = NULL,
    .protocol = TAPI_MEMTIER_PROTO_NONE,
    .run_count = TAPI_JOB_OPT_UINT_UNDEF,
    .requests = TAPI_JOB_OPT_UINT_UNDEF,
    .clients = TAPI_JOB_OPT_UINT_UNDEF,
    .threads = TAPI_JOB_OPT_UINT_UNDEF,
    .pipeline = TAPI_JOB_OPT_UINT_UNDEF,
    .test_time = TAPI_JOB_OPT_UINT_UNDEF,
    .data_size = TAPI_JOB_OPT_UINT_UNDEF,
    .random_data = false,
    .ratio = NULL,
    .key_prefix = NULL,
    .key_pattern = NULL,
    .key_minimum = TAPI_JOB_OPT_UINT_UNDEF,
    .key_maximum = TAPI_JOB_OPT_UINT_UNDEF,
    .hide_histogram = false,
    .memtier_path = NULL
};

/* Initializer for tapi_memtier_op_stats */
#define MEMTIER_STATS_INIT { .tps = 0.0, .net_rate = 0.0, .parsed = false }

/* Initializer for report structure */
const tapi_memtier_report tapi_memtier_default_report = {
    .gets = MEMTIER_STATS_INIT,
    .sets = MEMTIER_STATS_INIT,
    .totals = MEMTIER_STATS_INIT,
    .cmd = NULL,
};

/* See description in tapi_memtier.h */
te_errno
tapi_memtier_create(tapi_job_factory_t *factory,
                    const tapi_memtier_opt *opt,
                    tapi_memtier_app **app)
{
    te_errno rc;
    tapi_memtier_app *new_app;
    const char *exec_path = memtier_path;

    if (factory == NULL)
    {
        ERROR("%s(): job factory is NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (opt == NULL)
    {
        ERROR("%s(): job options are NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (app == NULL)
    {
        ERROR("%s(): app is NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    new_app = TE_ALLOC(sizeof(tapi_memtier_app));

    if (opt->memtier_path != NULL)
        exec_path = opt->memtier_path;

    rc = tapi_job_opt_build_args(exec_path, memtier_binds,
                                 opt, &new_app->cmd);
    if (rc != 0)
    {
        ERROR("%s(): failed to build command line arguments: %r",
              __FUNCTION__, rc);
        te_vec_deep_free(&new_app->cmd);
        free(new_app);
        return rc;
    }

    rc = tapi_job_simple_create(
            factory,
            &(tapi_job_simple_desc_t){
                .program    = exec_path,
                .argv       = (const char **)new_app->cmd.data.ptr,
                .job_loc    = &new_app->job,
                .stdout_loc = &new_app->out_chs[0],
                .stderr_loc = &new_app->out_chs[1],
                .filters    = TAPI_JOB_SIMPLE_FILTERS(
                    {
                        .use_stdout = true,
                        .readable = true,
                        .re = "^[a-zA-Z]+\\s+([0-9.-]+\\s+){2,}[0-9.-]+\\s*$",
                        .filter_var = &new_app->stats_filter,
                    },
                    {
                        .use_stdout  = true,
                        .readable    = true,
                        .log_level   = TE_LL_RING,
                        .filter_name = "memtier_benchmark stdout"
                    },
                    {
                        .use_stderr  = true,
                        .readable    = true,
                        .log_level   = TE_LL_WARN,
                        .filter_name = "memtier_benchmark stderr"
                    }
                )
            });

    if (rc != 0)
    {
        ERROR("%s(): failed to create a job: %r", __FUNCTION__, rc);
        te_vec_deep_free(&new_app->cmd);
        free(new_app);
        return rc;
    }

    *app = new_app;
    return 0;
}

/* See description in tapi_memtier.h */
te_errno
tapi_memtier_start(const tapi_memtier_app *app)
{
    if (app == NULL)
    {
        ERROR("%s(): app cannot be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_start(app->job);
}

/* See description in tapi_memtier.h */
te_errno
tapi_memtier_wait(const tapi_memtier_app *app, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    if (app == NULL)
    {
        ERROR("%s(): app cannot be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EINPROGRESS)
        {
            RING("%s(): job was still in process at the end of the wait",
                 __FUNCTION__);
        }

        return rc;
    }

    TAPI_JOB_CHECK_STATUS(status);
    return 0;
}

/* See description in tapi_memtier.h */
te_errno
tapi_memtier_stop(const tapi_memtier_app *app)
{
    if (app == NULL)
    {
        ERROR("%s(): app cannot be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_stop(app->job, SIGTERM, MEMTIER_TERM_TIMEOUT_MS);
}

/* See description in tapi_memtier.h */
te_errno
tapi_memtier_kill(const tapi_memtier_app *app, int signum)
{
    if (app == NULL)
    {
        ERROR("%s(): app cannot be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_kill(app->job, signum);
}

/* See description in tapi_memtier.h */
te_errno
tapi_memtier_destroy(tapi_memtier_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, MEMTIER_TERM_TIMEOUT_MS);
    if (rc != 0)
    {
        ERROR("%s(): failed to destroy the job: %r", __FUNCTION__, rc);
        return rc;
    }

    te_vec_deep_free(&app->cmd);
    free(app);
    return 0;
}

/* Parse a row of statistics table */
static te_errno
parse_stats(char *start, char *end, tapi_memtier_op_stats *stats)
{
    te_errno rc = 0;
    char *s;

    if (start == NULL || end == NULL || stats == NULL)
    {
        ERROR("%s(): arguments cannot be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    for ( ; *start != '\0' && isspace(*start); start++);

    for (s = start; *s != '\0' && !isspace(*s); s++);
    *s = '\0';

    rc = te_strtod(start, &stats->tps);
    if (rc != 0)
        return rc;

    for ( ; end != start && isspace(*end); end--)
        *end = '\0';

    for (s = end; s != start && *s != '\0' && !isspace(*s); s--);
    s++;

    rc = te_strtod(s, &stats->net_rate);
    if (rc != 0)
        return rc;

    /*
     * Let's retrieve Mbit/sec to match TE_MI_MEAS_THROUGHPUT units
     * and memaslap.
     */
    stats->net_rate = stats->net_rate / 1024 * 8;

    stats->parsed = true;

    return 0;
}

/* See description in tapi_memtier.h */
te_errno
tapi_memtier_get_report(tapi_memtier_app *app, tapi_memtier_report *report)
{
    te_errno rc = 0;
    te_string cmd_buf = TE_STRING_INIT;
    tapi_job_buffer_t *bufs = NULL;
    tapi_job_buffer_t *buf = NULL;
    unsigned int count = 0;
    unsigned int i;
    char *s;

    char *start = NULL;
    char *end = NULL;
    bool parsed = false;

    if (app == NULL || report == NULL)
    {
        ERROR("%s(): arguments cannot be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    *report = tapi_memtier_default_report;

    te_string_join_vec(&cmd_buf, &app->cmd, " ");
    report->cmd = cmd_buf.ptr;

    /*
     * There may be more than one statistics table if --run-count was
     * set to more than 1. The last table is average for all runs in that
     * case, we always get information from it here.
     */
    rc = tapi_job_receive_many(TAPI_JOB_CHANNEL_SET(app->stats_filter),
                               0, &bufs, &count);
    if (rc != 0)
        goto finish;

    for (i = 0; i < count; i++)
    {
        if (bufs[i].eos)
            break;

        buf = &bufs[i];

        if (buf == NULL || buf->data.len == 0)
            continue;

        s = start = buf->data.ptr;
        end = s + buf->data.len - 1;
        for ( ; !isspace(*start) && *start != '\0'; start++);

        if (strcmp_start("Sets", s) == 0)
            rc = parse_stats(start, end, &report->sets);
        else if (strcmp_start("Gets", s) == 0)
            rc = parse_stats(start, end, &report->gets);
        else if (strcmp_start("Totals", s) == 0)
            rc = parse_stats(start, end, &report->totals);

        if (rc != 0)
            goto finish;

        parsed = true;
    }

    if (!parsed)
    {
        ERROR("%s(): failed to find statistics in memtier_benchmark "
              "output", __FUNCTION__);
        rc = TE_RC(TE_TAPI, TE_ENOENT);
    }

finish:

    tapi_job_buffers_free(bufs, count);

    if (rc != 0)
        tapi_memtier_destroy_report(report);

    return rc;
}

/* Print statistics for a specific operation in MI log */
static void
op_stats_mi_log(te_mi_logger *logger,
                const tapi_memtier_op_stats *stats,
                const char *op_name)
{
    te_string param_name = TE_STRING_INIT_STATIC(256);

    te_string_append(&param_name, "%s.TPS", op_name);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_RPS,
                          te_string_value(&param_name),
                          TE_MI_MEAS_AGGR_SINGLE, stats->tps,
                          TE_MI_MEAS_MULTIPLIER_PLAIN);

    te_string_reset(&param_name);
    te_string_append(&param_name, "%s.Net_rate", op_name);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_THROUGHPUT,
                          te_string_value(&param_name),
                          TE_MI_MEAS_AGGR_SINGLE, stats->net_rate,
                          TE_MI_MEAS_MULTIPLIER_MEBI);
}

/* See description in tapi_memtier.h */
te_errno
tapi_memtier_report_mi_log(const tapi_memtier_report *report)
{
    te_errno rc;
    te_mi_logger *logger;

    if (report == NULL)
    {
        ERROR("%s(): report is NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = te_mi_logger_meas_create("memtier_benchmark", &logger);
    if (rc != 0)
    {
        ERROR("%s(): failed to create MI logger, error: %r",
              __FUNCTION__, rc);
        return rc;
    }

    if (report->sets.parsed)
        op_stats_mi_log(logger, &report->sets, "Sets");
    if (report->gets.parsed)
        op_stats_mi_log(logger, &report->gets, "Gets");
    if (report->totals.parsed)
        op_stats_mi_log(logger, &report->totals, "Totals");

    te_mi_logger_add_comment(logger, NULL, "command", "%s",
                             report->cmd);

    te_mi_logger_destroy(logger);
    return 0;
}

/* See description in tapi_memtier.h */
void
tapi_memtier_destroy_report(tapi_memtier_report *report)
{
    if (report == NULL)
        return;

    free(report->cmd);

    *report = (tapi_memtier_report)tapi_memtier_default_report;
}
