/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI to manage dnsperf
 *
 * TAPI to manage dnsperf.
 */

#define TE_LGR_USER "TAPI DNSPERF"

#include <assert.h>
#include <signal.h>

#include "te_alloc.h"
#include "te_defs.h"
#include "te_enum.h"
#include "te_mi_log.h"
#include "te_str.h"
#include "tapi_dnsperf.h"
#include "tapi_file.h"

#define TAPI_DNSPERF_TIMEOUT_MS 10000

/**
 * Path to dnsperf exec in the case of tapi_dnsperf_opt.dnsperf_path
 * is @c NULL.
 */
static const char *dnsperf_path = "dnsperf";

/** Mapping of possible values for dnsperf family option. */
static const te_enum_map tapi_dnsperf_addr_family_mapping[] = {
    {.name = "inet", .value = TAPI_DNSPERF_ADDR_FAMILY_INET},
    {.name = "inet6", .value = TAPI_DNSPERF_ADDR_FAMILY_INET6},
    {.name = "any", .value = TAPI_DNSPERF_ADDR_FAMILY_ANY},
    TE_ENUM_MAP_END
};

/** Mapping of possible values for dnsperf mode option. */
static const te_enum_map tapi_dnsperf_transport_mode_mapping[] = {
    {.name = "udp", .value = TAPI_DNSPERF_TRANSPORT_MODE_UDP},
    {.name = "tcp", .value = TAPI_DNSPERF_TRANSPORT_MODE_TCP},
    {.name = "dot", .value = TAPI_DNSPERF_TRANSPORT_MODE_DOT},
    {.name = "doh", .value = TAPI_DNSPERF_TRANSPORT_MODE_DOH},
    TE_ENUM_MAP_END
};

/* Possible dnsperf command line arguments */
static const tapi_job_opt_bind dnsperf_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_STRING("-a", FALSE, tapi_dnsperf_opt, local_addr),
    TAPI_JOB_OPT_UINT_T("-b", FALSE, NULL, tapi_dnsperf_opt, bufsize),
    TAPI_JOB_OPT_UINT_T("-c", FALSE, NULL, tapi_dnsperf_opt, clients),
    TAPI_JOB_OPT_STRING("-d", FALSE, tapi_dnsperf_opt, datafile),
    TAPI_JOB_OPT_BOOL("-D", tapi_dnsperf_opt, enable_dnssec_ok),
    TAPI_JOB_OPT_BOOL("-e", tapi_dnsperf_opt, enable_edns0),
    TAPI_JOB_OPT_STRING("-E", FALSE, tapi_dnsperf_opt, edns_opt),
    TAPI_JOB_OPT_ENUM("-f", FALSE, tapi_dnsperf_opt, addr_family,
                      tapi_dnsperf_addr_family_mapping),
    TAPI_JOB_OPT_UINT_T("-l", FALSE, NULL, tapi_dnsperf_opt, limit),
    TAPI_JOB_OPT_UINT_T("-n", FALSE, NULL, tapi_dnsperf_opt, runs_through_file),
    TAPI_JOB_OPT_UINT_T("-p", FALSE, NULL, tapi_dnsperf_opt, port),
    TAPI_JOB_OPT_UINT_T("-q", FALSE, NULL, tapi_dnsperf_opt, num_queries),
    TAPI_JOB_OPT_UINT_T("-Q", FALSE, NULL, tapi_dnsperf_opt, max_qps),
    TAPI_JOB_OPT_ENUM("-m", FALSE, tapi_dnsperf_opt, transport_mode,
                      tapi_dnsperf_transport_mode_mapping),
    TAPI_JOB_OPT_STRING("-O", FALSE, tapi_dnsperf_opt, ext_opt),
    TAPI_JOB_OPT_STRING("-s", FALSE, tapi_dnsperf_opt, server),
    TAPI_JOB_OPT_UINT_T("-S", FALSE, NULL, tapi_dnsperf_opt, stats_interval),
    TAPI_JOB_OPT_UINT_T("-t", FALSE, NULL, tapi_dnsperf_opt, timeout),
    TAPI_JOB_OPT_UINT_T("-T", FALSE, NULL, tapi_dnsperf_opt, threads),
    TAPI_JOB_OPT_BOOL("-v", tapi_dnsperf_opt, verbose),
    TAPI_JOB_OPT_BOOL("-W", tapi_dnsperf_opt, stdout_only),
    TAPI_JOB_OPT_UINT_T("-x", FALSE, NULL, tapi_dnsperf_opt, local_port)
);

/** Type of DNS resource records. */
typedef enum tapi_dnsperf_rr_type {
    TAPI_DNSPERF_RR_TYPE_A,
    TAPI_DNSPERF_RR_TYPE_AAAA,
} tapi_dnsperf_rr_type;

/** One DNS query item. */
typedef struct tapi_dnsperf_query {
    char *host;
    tapi_dnsperf_rr_type rr_type;
} tapi_dnsperf_query;

/* Default values of dnsperf command line arguments */
const tapi_dnsperf_opt tapi_dnsperf_default_opt = {
    .local_addr         = NULL,
    .bufsize            = TAPI_JOB_OPT_UINT_UNDEF,
    .clients            = TAPI_JOB_OPT_UINT_UNDEF,
    .datafile           = NULL,
    .enable_dnssec_ok   = FALSE,
    .enable_edns0       = FALSE,
    .edns_opt           = NULL,
    .addr_family        = TAPI_DNSPERF_ADDR_FAMILY_UNDEF,
    .limit              = TAPI_JOB_OPT_UINT_UNDEF,
    .runs_through_file  = TAPI_JOB_OPT_UINT_UNDEF,
    .port               = TAPI_JOB_OPT_UINT_UNDEF,
    .num_queries        = TAPI_JOB_OPT_UINT_UNDEF,
    .ext_opt            = NULL,
    .max_qps            = TAPI_JOB_OPT_UINT_UNDEF,
    .transport_mode     = TAPI_DNSPERF_TRANSPORT_MODE_UNDEF,
    .server             = NULL,
    .stats_interval     = TAPI_JOB_OPT_UINT_UNDEF,
    .timeout            = TAPI_JOB_OPT_UINT_UNDEF,
    .threads            = TAPI_JOB_OPT_UINT_UNDEF,
    .verbose            = FALSE,
    .stdout_only        = FALSE,
    .local_port         = TAPI_JOB_OPT_UINT_UNDEF,
    .queries            = TE_VEC_INIT(tapi_dnsperf_query),
    .dnsperf_path       = NULL,
};

/**
 * Add new DNS query.
 *
 * @param opt      dnsperf options
 * @param host     host name
 * @param rr_type  type of resource record
 */
static void
tapi_dnsperf_opt_query_add(tapi_dnsperf_opt *opts, const char *host,
                           tapi_dnsperf_rr_type rr_type)
{
    tapi_dnsperf_query item;

    assert(opts != NULL);
    assert(host != NULL);

    item.host = strdup(host);
    item.rr_type = rr_type;

    TE_VEC_APPEND(&opts->queries, item);
}

/* See description in tapi_dnsperf.h */
void
tapi_dnsperf_opt_query_add_a(tapi_dnsperf_opt *opts, const char *host)
{
    tapi_dnsperf_opt_query_add(opts, host, TAPI_DNSPERF_RR_TYPE_A);
}

/* See description in tapi_dnsperf.h */
void
tapi_dnsperf_opt_query_add_aaaa(tapi_dnsperf_opt *opts, const char *host)
{
    tapi_dnsperf_opt_query_add(opts, host, TAPI_DNSPERF_RR_TYPE_AAAA);
}

/* See description in tapi_dnsperf.h */
void
tapi_dnsperf_opt_queries_free(tapi_dnsperf_opt *opts)
{
    tapi_dnsperf_query *item;

    TE_VEC_FOREACH(&opts->queries, item)
        free(item->host);

    te_vec_free(&opts->queries);
}

/**
 * Create a DNS queries input file
 *
 * @param ta       test agent name
 * @param queries  list of queries
 *
 * @return name of the created file or @c NULL in case of error
 */
char *
tapi_dnsperf_cfg_create(const char *ta, te_vec *queries)
{
    te_errno rc;
    char *fname = NULL;
    tapi_dnsperf_query *item = NULL;
    te_string str = TE_STRING_INIT;

    TE_VEC_FOREACH(queries, item)
    {
        te_string_append(&str, "%s %s\n", item->host,
                    item->rr_type == TAPI_DNSPERF_RR_TYPE_AAAA ? "AAAA" : "A");
    }

    fname = tapi_file_make_name(NULL);

    rc = tapi_file_create_ta(ta, fname, "%s", str.ptr);
    if (rc != 0)
    {
        ERROR("%s(): failed to create config file", __func__);
        free(fname);
        fname = NULL;
    }

    te_string_free(&str);

    return fname;
}

/* See description in tapi_dnsperf.h */
te_errno
tapi_dnsperf_create(tapi_job_factory_t *factory,
                    tapi_dnsperf_opt *opts,
                    tapi_dnsperf_app **app)
{
    te_errno            rc;
    tapi_dnsperf_app   *new_app;
    const char         *exec_path = dnsperf_path;

    const char *ta = NULL;
    char *tmp_fname = NULL;

    assert(factory != NULL);
    assert(opts != NULL);
    assert(app != NULL);

    ta = tapi_job_factory_ta(factory);
    if (ta == NULL)
        return TE_RC(TE_TAPI, TE_ENOENT);

    if ((opts->datafile == NULL && te_vec_size(&opts->queries) == 0) ||
        (opts->datafile != NULL && te_vec_size(&opts->queries) != 0))
    {
        ERROR("One of the parameters must be set: datafile or queries");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (te_vec_size(&opts->queries) != 0)
    {
        char *fname;

        fname = tapi_dnsperf_cfg_create(ta, &opts->queries);
        if (fname == NULL)
            return TE_RC(TE_TAPI, TE_EFAIL);

        opts->datafile = fname;
        tmp_fname = fname;
    }

    new_app = TE_ALLOC(sizeof(tapi_dnsperf_app));
    new_app->ta = ta;
    new_app->tmp_fname = tmp_fname;

    if (opts->dnsperf_path != NULL)
        exec_path = opts->dnsperf_path;

    rc = tapi_job_opt_build_args(exec_path, dnsperf_binds,
                                 opts, &new_app->cmd);
    if (rc != 0)
    {
        ERROR("Failed to build dnsperf job command line arguments: %r", rc);
        te_vec_deep_free(&new_app->cmd);
        free(tmp_fname);
        free(new_app);
        return rc;
    }

    rc = tapi_job_simple_create(factory,
                    &(tapi_job_simple_desc_t){
                        .program    = exec_path,
                        .argv       = (const char **)new_app->cmd.data.ptr,
                        .job_loc    = &new_app->job,
                        .stdout_loc = &new_app->out_chs[0],
                        .stderr_loc = &new_app->out_chs[1],
                        .filters    = TAPI_JOB_SIMPLE_FILTERS(
                            {
                                .use_stdout = TRUE,
                                .readable = TRUE,
                                .re = "Queries sent:\\s*([0-9.]+).*",
                                .extract = 1,
                                .filter_var = &new_app->flt_queries_sent,
                            },
                            {
                                .use_stdout = TRUE,
                                .readable = TRUE,
                                .re = "Queries completed:\\s*([0-9.]+).*",
                                .extract = 1,
                                .filter_var = &new_app->flt_queries_completed,
                            },
                            {
                                .use_stdout = TRUE,
                                .readable = TRUE,
                                .re = "Queries lost:\\s*([0-9.]+).*",
                                .extract = 1,
                                .filter_var = &new_app->flt_queries_lost,
                            },
                            {
                                .use_stdout = TRUE,
                                .readable = TRUE,
                                .re = "Average packet size:\\s*request\\s*([0-9.]+),\\s*response\\s*([0-9.]+)",
                                .extract = 1,
                                .filter_var = &new_app->flt_avg_request_size,
                            },
                            {
                                .use_stdout = TRUE,
                                .readable = TRUE,
                                .re = "Average packet size:\\s*request\\s*([0-9.]+),\\s*response\\s*([0-9.]+)",
                                .extract = 2,
                                .filter_var = &new_app->flt_avg_response_size,
                            },
                            {
                                .use_stdout = TRUE,
                                .readable = TRUE,
                                .re = "Run time \\(s\\):\\s*([0-9.]+)",
                                .extract = 1,
                                .filter_var = &new_app->flt_run_time,
                            },
                            {
                                .use_stdout = TRUE,
                                .readable = TRUE,
                                .re = "Queries per second:\\s*([0-9.]+)",
                                .extract = 1,
                                .filter_var = &new_app->flt_rps,
                            },
                            {
                                .use_stdout  = TRUE,
                                .readable    = TRUE,
                                .log_level   = TE_LL_RING,
                                .filter_name = "dnsperf stdout"
                            },
                            {
                                .use_stderr  = TRUE,
                                .readable    = FALSE,
                                .log_level   = TE_LL_WARN,
                                .filter_name = "dnsperf stderr"
                            }
                        )
                    });
    if (rc != 0)
    {
        ERROR("Failed to create %s job: %r", exec_path, rc);
        te_vec_deep_free(&new_app->cmd);
        free(tmp_fname);
        free(new_app);
        return rc;
    }

    *app = new_app;
    return 0;
}

/* See description in tapi_dnsperf.h */
te_errno
tapi_dnsperf_start(const tapi_dnsperf_app *app)
{
    assert(app != NULL);

    return tapi_job_start(app->job);
}

/* See description in tapi_dnsperf.h */
te_errno
tapi_dnsperf_wait(const tapi_dnsperf_app *app, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    assert(app != NULL);

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

/* See description in tapi_dnsperf.h */
te_errno
tapi_dnsperf_stop(const tapi_dnsperf_app *app)
{
    assert(app != NULL);

    return tapi_job_stop(app->job, SIGTERM, TAPI_DNSPERF_TIMEOUT_MS);
}

/* See description in tapi_dnsperf.h */
te_errno
tapi_dnsperf_kill(const tapi_dnsperf_app *app, int signum)
{
    assert(app != NULL);
    return tapi_job_kill(app->job, signum);
}

/* See description in tapi_dnsperf.h */
te_errno
tapi_dnsperf_destroy(tapi_dnsperf_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_DNSPERF_TIMEOUT_MS);
    if (rc != 0)
    {
        ERROR("Failed to destroy dnsperf job: %r", rc);
        return rc;
    }

    te_vec_deep_free(&app->cmd);

    if (app->tmp_fname != NULL)
    {
        rc = tapi_file_ta_unlink_fmt(app->ta, "%s", app->tmp_fname);
        free(app->tmp_fname);
        if (rc != 0)
        {
            ERROR("Failed to remove dnsperf configuration file %s "
                  "on TA %s: %r", app->tmp_fname, app->ta, rc);
            return TE_RC(TE_TAPI, rc);
        }

    }

    free(app);
    return 0;
}

/**
 * Converts dnsperf arguments into string for mi_logger.
 *
 * @param[in]  vec  vector of dnsperf argumets
 * @param[out] str  string for output converted arguments
 */
static void
tapi_dnsperf_args2str(te_vec *vec, char **str)
{
    te_string arguments = TE_STRING_INIT;
    const char *separator = " ";
    void **arg;

    assert(str != NULL);

    TE_VEC_FOREACH(vec, arg)
    {
        if (*arg != NULL)
            te_string_append(&arguments, "%s%s", *arg, separator);
    }

    te_string_cut(&arguments, strlen(separator));
    *str = arguments.ptr;
}

/* See description in tapi_dnsperf.h */
te_errno
tapi_dnsperf_get_report(tapi_dnsperf_app *app, tapi_dnsperf_report *report)
{
    te_errno rc = 0;

    assert(app != NULL);
    assert(report != NULL);

#define TAPI_DNS_PERF_READ_ONE_FILTER(val_, val_double_) \
    do {                                                                    \
        tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;                       \
        void *value = &report->val_;                                        \
                                                                            \
        rc = tapi_job_receive(                                              \
                TAPI_JOB_CHANNEL_SET(app->flt_##val_), -1, &buf);           \
        if (rc != 0)                                                        \
        {                                                                   \
            ERROR("Failed to read data from filter '%s' (%r)", #val_, rc);  \
            return rc;                                                      \
        }                                                                   \
        if (val_double_)                                                    \
            rc = te_strtod(buf.data.ptr, value);                            \
        else                                                                \
            rc = te_strtoui(buf.data.ptr, 10, value);                       \
        if (rc != 0)                                                        \
        {                                                                   \
            ERROR("Failed to parse '%s' value '%s' (%r)",                   \
                  #val_, buf.data.ptr, rc);                                 \
        }                                                                   \
        te_string_free(&buf.data);                                          \
        if (rc != 0)                                                        \
            return rc;                                                      \
    } while(0)

    TAPI_DNS_PERF_READ_ONE_FILTER(queries_sent, FALSE);
    TAPI_DNS_PERF_READ_ONE_FILTER(queries_completed, FALSE);
    TAPI_DNS_PERF_READ_ONE_FILTER(queries_lost, FALSE);
    TAPI_DNS_PERF_READ_ONE_FILTER(avg_request_size, TRUE);
    TAPI_DNS_PERF_READ_ONE_FILTER(avg_response_size, TRUE);
    TAPI_DNS_PERF_READ_ONE_FILTER(run_time, TRUE);
    TAPI_DNS_PERF_READ_ONE_FILTER(rps, TRUE);

    tapi_dnsperf_args2str(&app->cmd, &report->cmd);

    /* Calculate the rest values */
    report->queries_lost_percent = 100.0 * report->queries_lost /
                                   report->queries_sent;

    report->net_rate = (report->queries_sent * report->avg_request_size +
                        report->queries_completed * report->avg_response_size) *
                        8 /* bits */ / ( report->run_time * 1024 * 1024);

#undef TAPI_DNS_PERF_READ_ONE_FILTER
    return rc;
}

/* See description in tapi_dnsperf.h */
te_errno
tapi_dnsperf_report_mi_log(const tapi_dnsperf_report *report)
{
    te_errno      rc;
    te_mi_logger *logger;

    assert(report != NULL);

    rc = te_mi_logger_meas_create("dnsperf", &logger);
    if (rc != 0)
    {
        ERROR("Failed to create MI logger, error: %r", rc);
        return rc;
    }

    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_RPS, "RPS",
                          TE_MI_MEAS_AGGR_SINGLE, report->rps,
                          TE_MI_MEAS_MULTIPLIER_PLAIN);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_PERCENTAGE, "Queries lost",
                          TE_MI_MEAS_AGGR_SINGLE, report->queries_lost_percent,
                          TE_MI_MEAS_MULTIPLIER_PLAIN);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_THROUGHPUT, "Net_rate",
                          TE_MI_MEAS_AGGR_SINGLE, report->net_rate,
                          TE_MI_MEAS_MULTIPLIER_MEBI);
    te_mi_logger_add_comment(logger, NULL, "command", "%s", report->cmd);

    te_mi_logger_destroy(logger);
    return 0;
}

/* See description in tapi_dnsperf.h */
void
tapi_dnsperf_destroy_report(tapi_dnsperf_report *report)
{
    assert(report != NULL);

    free(report->cmd);
}
