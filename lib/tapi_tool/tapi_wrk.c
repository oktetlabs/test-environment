/** @file
 * @brief Test API for wrk tool routine
 *
 * Test API to control 'wrk' tool.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#define TE_LGR_USER "TAPI WRK"

#include <stddef.h>
#include <math.h>
#include <ctype.h>

#include "te_defs.h"
#include "te_vector.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "tapi_file.h"
#include "te_units.h"
#include "tapi_wrk.h"
#include "conf_api.h"

#define TAPI_WRK_UNITS_MAX 16
#define TAPI_WRK_RECEIVE_TIMEOUT_MS 1000
#define TAPI_WRK_TERM_TIMEOUT_MS 1000
#define TAPI_WRK_PARSE_BUF_SIZE 128
#define TAPI_WRK_SCRIPT_FILE_NAME_SUFFIX "wrk_script.lua"

const tapi_wrk_opt tapi_wrk_default_opt = {
    .connections = 1,
    .n_threads = 1,
    .duration_s = 1,
    .timeout_ms = 2000,
    .script_path = NULL,
    .script_content = NULL,
    .latency = FALSE,
    .host = NULL,
    .rate = {.value = 1000, .defined = FALSE},
    .affinity = NULL,
};

static const tapi_job_opt_bind wrk_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_UINT("--connections", FALSE, NULL, tapi_wrk_opt, connections),
    TAPI_JOB_OPT_UINT("--threads", FALSE, NULL, tapi_wrk_opt, n_threads),
    TAPI_JOB_OPT_UINT("--duration", FALSE, "s", tapi_wrk_opt, duration_s),
    TAPI_JOB_OPT_BOOL("--latency", tapi_wrk_opt, latency),
    TAPI_JOB_OPT_STRING(NULL, FALSE, tapi_wrk_opt, host),
    TAPI_JOB_OPT_ARRAY(tapi_wrk_opt, n_headers, headers,
                       TAPI_JOB_OPT_STRING("--header", FALSE,
                                           tapi_wrk_opt, headers[0])),
    TAPI_JOB_OPT_STRING("--script", FALSE, tapi_wrk_opt, script_path),
    TAPI_JOB_OPT_UINT_T("--rate", FALSE, NULL, tapi_wrk_opt, rate),
    TAPI_JOB_OPT_STRING("--affinity", FALSE, tapi_wrk_opt, affinity)
);

static char *
tapi_wrk_args2str(te_vec *vec)
{
    void **arg;
    te_string str = TE_STRING_INIT;
    te_errno rc = 0;
    const char *separator = " ";

    if (te_vec_size(vec) == 0)
        return strdup("");

    TE_VEC_FOREACH(vec, arg)
    {
        if (rc == 0 && *arg != NULL)
            rc = te_string_append(&str, "%s%s", *arg, separator);
    }

    if (rc != 0)
    {
        te_string_free(&str);
        return NULL;
    }

    te_string_cut(&str, strlen(separator));

    return str.ptr;
}

/** Time units starting from microseconds */
static const te_unit_list time_units_us = {
    .scale = 1,
    .start_pow = 0,
    .non_uniform_scale = (double[]){ 1e-9, 1e-3, 1.0, 60.0, 3600.0 },
    .units = (const char * const[]){ "us", "ms", "s", "m", "h", NULL },
};

/** Time units starting from microseconds */
static const te_unit_list plain_units = {
    .scale = 1,
    .start_pow = 0,
    .units = (const char * const[]){ "", NULL },
};

static const te_unit_list percent_units = {
    .scale = 100,
    .start_pow = -1,
    .units = (const char * const[]){ "%", NULL },
};

static const te_unit_list binary_units = {
    .scale = 1024,
    .start_pow = 0,
    .units = (const char * const[]){ "", "K", "M", "G", "T", "P", NULL },
};

static const te_unit_list metric_units = {
    .scale = 1000,
    .start_pow = 0,
    .units = (const char * const[]){ "", "k", "M", "G", "T", "P", NULL },
};

static te_errno
generate_script_filename(const char *ta, char **file_name)
{
    char *working_dir = NULL;
    char *result;
    te_errno rc;

    rc = cfg_get_instance_fmt(NULL, &working_dir, "/agent:%s/dir:", ta);
    if (rc != 0)
    {
        ERROR("Failed to get working directory");
        return rc;
    }

    rc = te_asprintf(&result, "%s/%s_%s", working_dir,
                     tapi_file_generate_name(),
                     TAPI_WRK_SCRIPT_FILE_NAME_SUFFIX);
    free(working_dir);
    if (rc < 0)
    {
        ERROR("Failed to create wrk script file name");
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    *file_name = result;

    return 0;
}

te_errno
tapi_wrk_create(tapi_job_factory_t *factory, const tapi_wrk_opt *opt,
                tapi_wrk_app **app)
{
    tapi_wrk_app *result;
    te_errno rc;
    const char *path = "wrk";
    tapi_wrk_opt opt_effective;
    char *script_filename = NULL;

    opt_effective = *opt;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto out;
    }

    if (opt_effective.script_content != NULL)
    {
        const char *ta;

        ta = tapi_job_factory_ta(factory);
        if (ta == NULL)
        {
            ERROR("Failed to get test agent from job factory");
            rc = TE_RC(TE_TAPI, TE_EFAIL);
            goto out;
        }

        if (opt_effective.script_path == NULL)
        {
            rc = generate_script_filename(ta, &script_filename);
            if (rc != 0)
                goto out;

            opt_effective.script_path = script_filename;
        }

        if (tapi_file_create_ta(ta, opt_effective.script_path,
                                "%s", opt_effective.script_content) != 0)
        {
            ERROR("Failed to create script file on TA for wrk");
            rc = TE_RC(TE_TAPI, TE_EFAIL);
            goto out;
        }
    }

    rc = tapi_job_opt_build_args(path, wrk_binds, &opt_effective,
                                 &result->wrk_args);
    if (rc != 0)
        goto out;


    rc = tapi_job_simple_create(factory,
                          &(tapi_job_simple_desc_t){
                                .program = path,
                                .argv = (const char **)result->wrk_args.data.ptr,
                                .job_loc = &result->job,
                                .stdout_loc = &result->out_chs[0],
                                .stderr_loc = &result->out_chs[1],
                                .filters = TAPI_JOB_SIMPLE_FILTERS(
                                    {.use_stdout = TRUE,
                                     .readable = TRUE,
                                     .re = "Transfer/sec:\\s*([^\\s]+)B",
                                     .extract = 1,
                                     .filter_var = &result->bps_filter,
                                    },
                                    {.use_stdout = TRUE,
                                     .readable = TRUE,
                                     .re = "Requests/sec:\\s*([^\\s]+)",
                                     .extract = 1,
                                     .filter_var = &result->req_total_filter,
                                    },
                                    {.use_stdout = TRUE,
                                     .readable = TRUE,
                                     .re = "Latency\\s*(.*%)",
                                     .extract = 1,
                                     .filter_var = &result->lat_filter,
                                    },
                                    {.use_stdout = TRUE,
                                     .readable = TRUE,
                                     .re = "Req/Sec\\s*(.*%)",
                                     .extract = 1,
                                     .filter_var = &result->req_filter,
                                    },
                                    {.use_stdout = TRUE,
                                     .readable = TRUE,
                                     .re = "(?m)Latency Distribution\n((\\s+[0-9]+%.*)+)",
                                     .extract = 1,
                                     .filter_var = &result->lat_distr_filter,
                                    },
                                    {.use_stderr = TRUE,
                                     .log_level = TE_LL_ERROR,
                                     .readable = FALSE,
                                     .filter_name = "err",
                                    },
                                    {.use_stdout = TRUE,
                                     .log_level = TE_LL_RING,
                                     .readable = FALSE,
                                     .filter_name = "out",
                                    }
                                 )
                          });

    if (rc != 0)
        goto out;

    *app = result;

out:
    if (rc != 0)
    {
        te_vec_deep_free(&result->wrk_args);
        free(result);
    }

    free(script_filename);

    return rc;
}

te_errno
tapi_wrk_start(tapi_wrk_app *app)
{
    return tapi_job_start(app->job);
}

te_errno
tapi_wrk_wait(tapi_wrk_app *app, int timeout_ms)
{
    tapi_job_status_t status;
    te_errno rc;

    if ((rc = tapi_job_wait(app->job, timeout_ms, &status)) != 0)
        return rc;

    if (status.type == TAPI_JOB_STATUS_UNKNOWN ||
        (status.type == TAPI_JOB_STATUS_EXITED && status.value != 0))
    {
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    return 0;
}

te_errno
tapi_wrk_kill(tapi_wrk_app *app, int signo)
{
    return tapi_job_kill(app->job, signo);
}

te_errno
tapi_wrk_destroy(tapi_wrk_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_WRK_TERM_TIMEOUT_MS);
    if (rc != 0)
        return rc;

    te_vec_deep_free(&app->wrk_args);
    free(app);

    return 0;
}

static te_errno
parse_unit(const char *str, const te_unit_list *unit_list, double *result)
{
    return te_unit_list_value_from_string(str, unit_list, result);
}

static te_errno
parse_latency_distr(const char *str, size_t size,
                    tapi_wrk_latency_percentile *percentiles)
{
    char buf[TAPI_WRK_PARSE_BUF_SIZE] = {0};
    tapi_wrk_latency_percentile *result;
    size_t offset;
    te_errno rc;
    size_t i;

    if (strlen(str) + 1 > sizeof(buf))
        return TE_RC(TE_TAPI, TE_ENOBUFS);

    result = TE_ALLOC(size * sizeof(*result));
    if (result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    for (i = 0, offset = 0; i < size * 2; i++, offset += strlen(buf))
    {
        while (isspace(str[offset]))
            offset++;

        rc = sscanf(str + offset, "%s", buf);
        if (rc != 1)
        {
            free(result);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        if (i % 2 == 0)
            rc = parse_unit(buf, &percent_units, &result[i / 2].percentile);
        else
            rc = parse_unit(buf, &time_units_us, &result[i / 2].latency);

        if (rc != 0)
        {
            free(result);
            return rc;
        }
    }

    for (i = 0; i < size; i++)
        percentiles[i] = result[i];

    free(result);

    return 0;
}

static te_errno
parse_thread_stats(const char *str, tapi_wrk_thread_stats *stats,
                   const te_unit_list *value_units)
{
    char buf[4][TAPI_WRK_PARSE_BUF_SIZE] = {{0}};
    tapi_wrk_thread_stats result;
    te_errno rc;

    memset(&result, 0, sizeof(result));

    if (strlen(str) + 1 > sizeof(buf[0]))
        return TE_RC(TE_TAPI, TE_ENOBUFS);

    rc = sscanf(str, "%s %s %s %s", buf[0], buf[1], buf[2], buf[3]);
    if (rc <= 0)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = parse_unit(buf[0], value_units, &result.mean);
    if (rc == 0)
        rc = parse_unit(buf[1], value_units, &result.stdev);
    if (rc == 0)
        rc = parse_unit(buf[2], value_units, &result.max);
    if (rc == 0)
        rc = parse_unit(buf[3], &percent_units, &result.within_stdev);

    if (rc != 0)
        return rc;

    *stats = result;

    return 0;
}

te_errno
tapi_wrk_get_report(tapi_wrk_app *app, tapi_wrk_report *report)
{
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
    const unsigned int wrk_reports_min = 4;
    const unsigned int wrk_reports_max = 10;
    tapi_wrk_report result;
    te_errno rc;
    unsigned reports_received;

    memset(&result, 0, sizeof(result));

    for (reports_received = 0; reports_received < wrk_reports_max;)
    {
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(app->bps_filter,
                                                   app->req_total_filter,
                                                   app->lat_filter,
                                                   app->req_filter,
                                                   app->lat_distr_filter),
                              TAPI_WRK_RECEIVE_TIMEOUT_MS, &buf);


        if (rc != 0)
        {
            if (rc == TE_ETIMEDOUT && reports_received >= wrk_reports_min)
                break;

            ERROR("Failed to get essential report from wrk");
            te_string_free(&buf.data);
            return rc;
        }

        if (buf.eos)
            continue;
        else
            reports_received++;

        if (buf.filter == app->bps_filter)
        {
            rc = parse_unit(buf.data.ptr, &binary_units, &result.bps);
        }
        else if (buf.filter == app->req_total_filter)
        {
            rc = parse_unit(buf.data.ptr, &plain_units, &result.req_per_sec);
        }
        else if (buf.filter == app->lat_filter)
        {
            rc = parse_thread_stats(buf.data.ptr, &result.thread_latency,
                                    &time_units_us);
        }
        else if (buf.filter == app->req_filter)
        {
            rc = parse_thread_stats(buf.data.ptr, &result.thread_req_per_sec,
                                    &metric_units);
        }
        else if (buf.filter == app->lat_distr_filter)
        {
            rc = parse_latency_distr(buf.data.ptr,
                                     TE_ARRAY_LEN(result.lat_distr),
                                     result.lat_distr);
        }
        else
        {
            ERROR("Message is received from an unknown filter");
            rc = TE_RC(TE_TAPI, TE_EINVAL);
        }

        if (rc != 0)
        {
            ERROR("Failed to parse report from wrk");
            te_string_free(&buf.data);
            return rc;
        }

        te_string_reset(&buf.data);
    }

    result.arguments = tapi_wrk_args2str(&app->wrk_args);
    if (result.arguments == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    *report = result;

    te_string_free(&buf.data);

    return 0;
}

void
tapi_wrk_report_mi_log(te_mi_logger *logger, const tapi_wrk_report *report)
{
    const char *pt = "per-thread";

    te_mi_logger_add_meas_vec(logger, NULL, TE_MI_MEAS_V(
            TE_MI_MEAS(THROUGHPUT, NULL, MEAN,
                       TE_UNITS_DEC_U2M(report->bps * 8.0), MEGA),
            TE_MI_MEAS(RPS, NULL, MEAN, report->req_per_sec, PLAIN),
            TE_MI_MEAS(LATENCY, pt, MEAN, report->thread_latency.mean / 1000.,
                       MILLI),
            TE_MI_MEAS(LATENCY, pt, MAX, report->thread_latency.max / 1000.,
                       MILLI),
            TE_MI_MEAS(LATENCY, pt, STDEV, report->thread_latency.stdev / 1000.,
                       MILLI),
            TE_MI_MEAS(RPS, pt, MEAN, report->thread_req_per_sec.mean, PLAIN),
            TE_MI_MEAS(RPS, pt, MAX, report->thread_req_per_sec.max, PLAIN),
            TE_MI_MEAS(RPS, pt, STDEV, report->thread_req_per_sec.stdev, PLAIN)));

    /*
     * Since the first argument contains the executable file name, string with
     * all arguments will represent the command that was used to run the tool.
     */
    te_mi_logger_add_comment(logger, NULL, "command", "%s",
                             report->arguments);
}
