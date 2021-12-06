/** @file
 * @brief Test API for sfnt-pingpong tool routine
 *
 * Test API to control 'sfnt-pingpong' tool.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */
#define TE_LGR_USER "TAPI SFNT-PINGPONG"

#include "te_config.h"

#include <netinet/in.h>

#include "tapi_sfnt_pingpong.h"
#include "tapi_job_opt.h"
#include "te_vector.h"
#include "tapi_test.h"
#include "tapi_mem.h"
#include "te_mi_log.h"

/** Number of tapi_job output channels (one for stdout, one for stderr) */
#define TAPI_SFNT_PP_CHANNELS_STD_NUM 2
/** Time to wait till data is ready to read from stdout */
#define TAPI_SFNT_PP_RECEIVE_TIMEOUT_MS 1000
/** The timeout of termination of a job */
#define TAPI_SFNT_PP_TERM_TIMEOUT_MS 1000
/** Number of measurements in the output table */
#define TAPI_SFNT_PP_NUM_MEAS 7
/** Path to sfnt-pingpong */
#define TAPI_SFNT_PATH_SFNT_PINGPONG "sfnt-pingpong"

struct tapi_sfnt_pp_app_client_t {
    /** TAPI job handle */
    tapi_job_t *job;
    /** Output channel handles */
    tapi_job_channel_t *out_chs[TAPI_SFNT_PP_CHANNELS_STD_NUM];
    /** Output filter */
    tapi_job_channel_t *filter;
};

struct tapi_sfnt_pp_app_server_t {
    /** TAPI job handle */
    tapi_job_t *job;
    /** Output channel handles */
    tapi_job_channel_t *out_chs[TAPI_SFNT_PP_CHANNELS_STD_NUM];
};

/**
 * @defgroup tapi_job_opt_formatting custom functions for argument formatting
 * @{
 *
 * @param[in]     value     Pointer to an argument.
 * @param[inout]  args      Argument vector to which formatted argument
 *                          is appended.
 */

/** value type: `int` */
static te_errno
create_optional_int(const void *value, te_vec *args)
{
    const int num = *(const int *)value;

    if (num == -1)
        return TE_ENOENT;

    return te_vec_append_str_fmt(args, "%d", num);
}

/** value type: `uint8_t` */
static te_errno
create_optional_proto(const void *value, te_vec *args)
{
    uint8_t proto = *(const uint8_t *)value;

    switch (proto)
    {
        case IPPROTO_TCP:
            return te_vec_append_str_fmt(args, "tcp");

        case IPPROTO_UDP:
            return te_vec_append_str_fmt(args, "udp");

        default:
            ERROR("Unknown transport protocol");
            return TE_EINVAL;
    }
}

/** value type `tapi_sfnt_pp_muxer' */
static te_errno
create_optional_muxer(const void *value, te_vec *args)
{
    tapi_sfnt_pp_muxer muxer = *(const tapi_sfnt_pp_muxer *)value;

    switch (muxer)
    {
        case TAPI_SFNT_PP_MUXER_NONE:
            return te_vec_append_str_fmt(args, "none");

        case TAPI_SFNT_PP_MUXER_EPOLL:
            return te_vec_append_str_fmt(args, "epoll");

        case TAPI_SFNT_PP_MUXER_POLL:
            return te_vec_append_str_fmt(args, "poll");

        case TAPI_SFNT_PP_MUXER_SELECT:
            return te_vec_append_str_fmt(args, "select");

        default:
            ERROR("Unknown muxer type. select, poll, epoll or none only");
            return TE_EINVAL;
    }
}

/** value type `te_vec *` */
static te_errno
create_optional_sizes(const void *value, te_vec *args)
{
    const te_vec  **sizes = (const te_vec **)value;
    int           *elem;
    te_errno       rc;
    te_string      tmp = TE_STRING_INIT;

    if (*sizes == NULL)
        return TE_ENOENT;

    TE_VEC_FOREACH((te_vec *)*sizes, elem)
    {
        rc = te_string_append(&tmp, "%d,", *elem);
        if (rc != 0)
        {
            te_string_free(&tmp);
            return rc;
        }
    }
    rc = te_vec_append_str_fmt(args, "%s", tmp.ptr);
    if (rc != 0)
    {
        te_string_free(&tmp);
        return rc;
    }

    te_string_free(&tmp);
    return 0;
}

/** value type: `sa_family_t` */
static te_errno
create_optional_ipversion(const void *value, te_vec *args)
{
    sa_family_t ipversion = *(const sa_family_t *)value;

    switch (ipversion)
    {
        case AF_INET:
             return te_vec_append_str_fmt(args, "--ipv4");

        case AF_INET6:
             return te_vec_append_str_fmt(args, "--ipv6");

        default:
            ERROR("Incorrect IP version");
            return TE_EINVAL;
    }
}

/**@} <!-- END custom tapi_job_opt_formatting --> */

#define CREATE_OPT_INT(_prefix, _concat_prefix, _struct, _filed) \
    { create_optional_int, _prefix, _concat_prefix, NULL, \
      offsetof(_struct, _filed) }

#define CREATE_OPT_PROTO(_struct, _filed) \
    { create_optional_proto, NULL, FALSE, NULL, \
      offsetof(_struct, _filed) }

#define CREATE_OPT_MUXER(_prefix, _struct, _filed) \
    { create_optional_muxer, _prefix, TRUE, NULL, \
      offsetof(_struct, _filed) }

#define CREATE_OPT_SIZES(_prefix, _struct, _filed) \
    { create_optional_sizes, _prefix, TRUE, NULL, \
      offsetof(_struct, _filed) }

#define CREATE_OPT_IPVERSION(_struct, _field) \
    {create_optional_ipversion, NULL, FALSE, NULL, \
     offsetof(_struct, _field) }

const tapi_sfnt_pp_opt tapi_sfnt_pp_opt_default_opt = {
    .server = NULL,
    .proto = IPPROTO_UDP,
    .ipversion = AF_INET,
    .min_msg = -1,
    .max_msg = -1,
    .min_ms = -1,
    .max_ms = -1,
    .min_iter = -1,
    .max_iter = -1,
    .spin = FALSE,
    .muxer = TAPI_SFNT_PP_MUXER_NONE,
    .timeout_ms = -1,
    .sizes = NULL,
};


te_errno
tapi_sfnt_pp_create_client(tapi_job_factory_t *factory,
                           const tapi_sfnt_pp_opt *opt,
                           tapi_sfnt_pp_app_client_t **app)
{
    tapi_sfnt_pp_app_client_t *client;
    te_vec                     args = TE_VEC_INIT(char *);
    te_errno                   rc = 0;
    const char                *path = TAPI_SFNT_PATH_SFNT_PINGPONG;

    const tapi_job_opt_bind client_binds[] = TAPI_JOB_OPT_SET(
        CREATE_OPT_SIZES("--sizes=", tapi_sfnt_pp_opt, sizes),
        CREATE_OPT_INT("--minmsg=", TRUE, tapi_sfnt_pp_opt, min_msg),
        CREATE_OPT_INT("--maxmsg=", TRUE, tapi_sfnt_pp_opt, max_msg),
        CREATE_OPT_INT("--minms=", TRUE, tapi_sfnt_pp_opt, min_ms),
        CREATE_OPT_INT("--maxms=", TRUE, tapi_sfnt_pp_opt, max_ms),
        CREATE_OPT_INT("--miniter=", TRUE, tapi_sfnt_pp_opt, min_iter),
        CREATE_OPT_INT("--maxiter=", TRUE, tapi_sfnt_pp_opt, max_iter),
        TAPI_JOB_OPT_BOOL("--spin", tapi_sfnt_pp_opt, spin),
        CREATE_OPT_MUXER("--muxer=", tapi_sfnt_pp_opt, muxer),
        CREATE_OPT_INT("--timeout=", TRUE, tapi_sfnt_pp_opt, timeout_ms),
        CREATE_OPT_IPVERSION(tapi_sfnt_pp_opt, ipversion),
        CREATE_OPT_PROTO(tapi_sfnt_pp_opt, proto),
        TAPI_JOB_OPT_SOCKADDR_PTR(NULL, FALSE, tapi_sfnt_pp_opt, server)
    );

    if (opt->server == NULL)
    {
        ERROR("Server address must be set");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    client = tapi_calloc(1, sizeof(*client));

    rc = tapi_job_opt_build_args(path, client_binds, opt, &args);
    if (rc != 0)
        goto out;

    rc = tapi_job_simple_create(factory,
                                &(tapi_job_simple_desc_t){
                                    .program = path,
                                    .argv = (const char **)args.data.ptr,
                                    .job_loc = &client->job,
                                    .stdout_loc = &client->out_chs[0],
                                    .stderr_loc = &client->out_chs[1],
                                    .filters = TAPI_JOB_SIMPLE_FILTERS(
                                        {.use_stdout = TRUE,
                                         .readable = TRUE,
                                         .re = "\\s*(\\d+[\\t ]+\\d+[\\t ]+"
                                               "\\d+[\\t ]+\\d+[\\t ]+\\d+[\\t ]+"
                                               "\\d+[\\t ]+\\d+[\\t ]+\\d+)\\s*",
                                         .extract = 0,
                                         .filter_var = &client->filter,
                                        },
                                        {.use_stdout = TRUE,
                                         .log_level = TE_LL_RING,
                                         .readable = FALSE,
                                         .filter_name = "out",
                                        },
                                        {.use_stderr = TRUE,
                                         .log_level = TE_LL_ERROR,
                                         .readable = FALSE,
                                         .filter_name = "err",
                                        }
                                    )
                                });

    if (rc != 0)
        goto out;

    *app = client;

out:
    if (rc != 0)
    {
        te_vec_deep_free(&args);
        free(client);
    }

    return rc;
}

te_errno
tapi_sfnt_pp_create_server(tapi_job_factory_t *factory,
                           const tapi_sfnt_pp_opt *opt,
                           tapi_sfnt_pp_app_server_t **app)
{
    tapi_sfnt_pp_app_server_t *server;
    te_vec                     args = TE_VEC_INIT(char *);
    te_errno                   rc = 0;
    const char                *path = TAPI_SFNT_PATH_SFNT_PINGPONG;

    server = tapi_calloc(1, sizeof(*server));

    rc = tapi_job_opt_build_args(path, NULL, NULL, &args);
    if (rc != 0)
        goto out;

    rc = tapi_job_simple_create(factory,
                                &(tapi_job_simple_desc_t){
                                    .program = path,
                                    .argv = (const char **)args.data.ptr,
                                    .job_loc = &server->job,
                                    .stdout_loc = &server->out_chs[0],
                                    .stderr_loc = &server->out_chs[1],
                                    .filters = TAPI_JOB_SIMPLE_FILTERS(
                                        {.use_stdout = TRUE,
                                         .log_level = TE_LL_RING,
                                         .readable = FALSE,
                                         .filter_name = "out",
                                        },
                                        {.use_stderr = TRUE,
                                         .log_level = TE_LL_ERROR,
                                         .readable = FALSE,
                                         .filter_name = "err",
                                        }
                                    )
                                });

    if (rc != 0)
        goto out;

    *app = server;

out:
    if (rc != 0)
    {
        te_vec_deep_free(&args);
        free(server);
    }

    return rc;
}

te_errno
tapi_sfnt_pp_create(tapi_job_factory_t *client_factory,
                    tapi_job_factory_t *server_factory,
                    const tapi_sfnt_pp_opt *opt,
                    tapi_sfnt_pp_app_client_t **client,
                    tapi_sfnt_pp_app_server_t **server)
{
    te_errno rc;

    rc = tapi_sfnt_pp_create_server(server_factory, opt, server);
    if (rc != 0)
    {
        ERROR("Failed to create server");
        return rc;
    }

    rc = tapi_sfnt_pp_create_client(client_factory, opt, client);
    if (rc != 0)
    {
        ERROR("Failed to create client");
        return rc;
    }

    return 0;
}

te_errno
tapi_sfnt_pp_start_client(tapi_sfnt_pp_app_client_t *app)
{
    return tapi_job_start(app->job);
}

te_errno
tapi_sfnt_pp_start_server(tapi_sfnt_pp_app_server_t *app)
{
    return tapi_job_start(app->job);
}

te_errno
tapi_sfnt_pp_start(tapi_sfnt_pp_app_client_t *client,
                   tapi_sfnt_pp_app_server_t *server)
{
    te_errno rc;

    rc = tapi_sfnt_pp_start_server(server);
    if (rc != 0)
    {
        ERROR("Failed to start server");
        return rc;
    }

    rc = tapi_sfnt_pp_start_client(client);
    if (rc != 0)
    {
        ERROR("Failed to start client");
        return rc;
    }

    return 0;
}

te_errno
tapi_sfnt_pp_wait_client(tapi_sfnt_pp_app_client_t *app,
                         int timeout_ms)
{
    tapi_job_status_t status;
    te_errno          rc;

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
        return rc;

    if (status.type == TAPI_JOB_STATUS_UNKNOWN ||
        (status.type == TAPI_JOB_STATUS_EXITED && status.value != 0))
    {
        return TE_RC(TE_TAPI, TE_EFAIL);
    }
    return 0;
}

te_errno
tapi_sfnt_pp_wait_server(tapi_sfnt_pp_app_server_t *app,
                         int timeout_ms)
{
    tapi_job_status_t status;
    te_errno          rc;

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
        return rc;

    if (status.type == TAPI_JOB_STATUS_UNKNOWN ||
        (status.type == TAPI_JOB_STATUS_EXITED && status.value != 0))
    {
        return TE_RC(TE_TAPI, TE_EFAIL);
    }
    return 0;
}

/**
 * Read data from filter.
 *
 * @param[in]  filter     Filter for reading.
 * @param[out] out        String with message from filter.
 * @param[out] count_line Number of rows in the output table.
 *
 * @return Statuc code
 */
static te_errno
read_filter(tapi_job_channel_t *filter, te_string *out, int *count_line)
{
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
    te_errno          rc = 0;

    *count_line = 0;

    while (!buf.eos && rc == 0)
    {
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(filter),
                              TAPI_SFNT_PP_RECEIVE_TIMEOUT_MS, &buf);
        if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT)
        {
            rc = 0;
            break;
        }
        ++*count_line;
    }
    if (rc == 0)
    {
        *out = buf.data;
        --*count_line;
    }
    return rc;
}

te_errno
tapi_sfnt_pp_get_report(tapi_sfnt_pp_app_client_t *app,
                        tapi_sfnt_pp_report **report)
{
    te_errno             rc = 0;
    te_string            buf = TE_STRING_INIT;
    int                  count_line = 0;
    tapi_sfnt_pp_report *new_report;
    size_t               offset = 0;
    int                  i;

    rc = read_filter(app->filter, &buf, &count_line);
    if (rc != 0 || count_line == 0)
    {
        ERROR("Failed to read data from filter");
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    new_report = tapi_calloc(count_line, sizeof(*new_report));

    /* output may or may not contain the character '\n' */
    if (buf.ptr[offset] == '\n')
        offset++;

    for (i = 0; i < count_line ; i++)
    {
        rc = sscanf(buf.ptr + offset, "%d %d %d %d %d %d %d %*d",
                    &new_report[i].size, &new_report[i].mean, &new_report[i].min,
                    &new_report[i].median, &new_report[i].max, &new_report[i].percentile,
                    &new_report[i].stddev);

        if (rc != TAPI_SFNT_PP_NUM_MEAS)
            return TE_RC(TE_TAPI, TE_EPROTO);

        while(buf.ptr[offset] != '\n')
            ++offset;
        ++offset;
    }

    *report = new_report;
    return 0;
}

static te_errno
tapi_sfnt_pp_kill(tapi_job_t *job, int signo)
{
    te_errno rc;

    rc = tapi_job_kill(job, signo);

    if (rc != 0)
    {
        /*
         * sfnt-pingpong client/server process ends on it's own.
         * Therefore the process no longer exists.
         */
        if (TE_RC_GET_ERROR(rc) == TE_ESRCH)
            return 0;
        return rc;
    }
    return 0;

}

te_errno
tapi_sfnt_pp_kill_client(tapi_sfnt_pp_app_client_t *app, int signo)
{
    te_errno rc;

    rc = tapi_sfnt_pp_kill(app->job, signo);
    if (rc != 0)
        ERROR("Failed to kill client");

    return rc;
}

te_errno
tapi_sfnt_pp_kill_server(tapi_sfnt_pp_app_server_t *app, int signo)
{
    te_errno rc;

    rc = tapi_sfnt_pp_kill(app->job, signo);
    if (rc != 0)
        ERROR("Failed to kill server");

    return rc;
}

te_errno
tapi_sfnt_pp_destroy_client(tapi_sfnt_pp_app_client_t *app)
{
    te_errno rc = 0;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_SFNT_PP_TERM_TIMEOUT_MS);
    free(app);

    return rc;
}

te_errno
tapi_sfnt_pp_destroy_server(tapi_sfnt_pp_app_server_t *app)
{
    te_errno rc = 0;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_SFNT_PP_TERM_TIMEOUT_MS);
    free(app);

    return rc;
}

te_errno
tapi_sfnt_pp_mi_report(const tapi_sfnt_pp_report *report)
{
    te_mi_logger *logger;
    te_errno      rc;

    rc = te_mi_logger_meas_create("sfnt-pingpong", &logger);
    if (rc != 0)
        return rc;

    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_LATENCY, "1/2 RTT latency",
                          TE_MI_MEAS_AGGR_MEAN, report->mean,
                          TE_MI_MEAS_MULTIPLIER_NANO);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_LATENCY, "1/2 RTT latency",
                          TE_MI_MEAS_AGGR_MIN, report->min,
                          TE_MI_MEAS_MULTIPLIER_NANO);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_LATENCY, "1/2 RTT latency",
                          TE_MI_MEAS_AGGR_MEDIAN, report->median,
                          TE_MI_MEAS_MULTIPLIER_NANO);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_LATENCY, "1/2 RTT latency",
                          TE_MI_MEAS_AGGR_MAX, report->max,
                          TE_MI_MEAS_MULTIPLIER_NANO);
     te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_LATENCY, "1/2 RTT latency (99)",
                          TE_MI_MEAS_AGGR_PERCENTILE, report->percentile,
                          TE_MI_MEAS_MULTIPLIER_NANO);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_LATENCY, "1/2 RTT latency",
                          TE_MI_MEAS_AGGR_STDEV, report->stddev,
                          TE_MI_MEAS_MULTIPLIER_NANO);
    te_mi_logger_add_meas_key(logger, NULL, "Size", "%d", report->size);
    te_mi_logger_destroy(logger);
    return 0;
}

te_errno
tapi_sfnt_pp_client_wrapper_add(tapi_sfnt_pp_app_client_t *app,
                                const char *tool, const char **argv,
                                tapi_job_wrapper_priority_t priority,
                                tapi_job_wrapper_t **wrap)
{
    return tapi_job_wrapper_add(app->job, tool, argv, priority, wrap);
}

te_errno
tapi_sfnt_pp_client_add_sched_param(tapi_sfnt_pp_app_client_t *app,
                                    tapi_job_sched_param *sched_param)
{
    return tapi_job_add_sched_param(app->job, sched_param);
}
