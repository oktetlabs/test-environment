/** @file
 * @brief NPtcp tool TAPI
 *
 * TAPI to handle NPtcp tool
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_LGR_USER "TAPI NPTCP"

#include "tapi_nptcp.h"
#include "te_alloc.h"
#include "te_str.h"

#define TAPI_NPTCP_TERM_TIMEOUT_MS 1000
#define TAPI_NPTCP_RECEIVE_TIMEOUT_MS 1000
#define TAPI_NPTCP_WAIT_RECEIVER_TIMEOUT_MS 1000

static const char *path_to_nptcp_binary = "NPtcp";

/* Indices of tapi_nptcp_report_entry fields to be used in array of vectors */
enum entry_fields {
    ENTRY_NUMBER,
    ENTRY_BYTES,
    ENTRY_TIMES,
    ENTRY_THROUGHPUT,
    ENTRY_RTT,
    /* Used for convenient iteration */
    ENTRY_MAX
};

const tapi_nptcp_opt tapi_nptcp_default_opt = {
    .tcp_buffer_size = TAPI_JOB_OPT_OMIT_UINT,
    .host = NULL,
    .invalidate_cache = FALSE,
    .starting_msg_size = TAPI_JOB_OPT_OMIT_UINT,
    .nrepeats = TAPI_JOB_OPT_OMIT_UINT,
    .offsets = NULL,
    .output_filename = NULL,
    .perturbation_size = TAPI_JOB_OPT_OMIT_UINT,
    .reset_sockets = FALSE,
    .streaming_mode = FALSE,
    .upper_bound = TAPI_JOB_OPT_OMIT_UINT,
    .bi_directional_mode = FALSE,
};

static const tapi_job_opt_bind nptcp_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_UINT_OMITTABLE("-b", FALSE, NULL,
                                tapi_nptcp_opt, tcp_buffer_size),

    TAPI_JOB_OPT_STRING("-h", FALSE, tapi_nptcp_opt, host),
    TAPI_JOB_OPT_BOOL("-I", tapi_nptcp_opt, invalidate_cache),
    TAPI_JOB_OPT_UINT_OMITTABLE("-l", FALSE, NULL,
                                tapi_nptcp_opt, starting_msg_size),

    TAPI_JOB_OPT_UINT_OMITTABLE("-n", FALSE, NULL,
                                tapi_nptcp_opt, nrepeats),

    TAPI_JOB_OPT_STRING("-O", FALSE, tapi_nptcp_opt, offsets),
    TAPI_JOB_OPT_STRING("-o", FALSE, tapi_nptcp_opt, output_filename),
    TAPI_JOB_OPT_UINT_OMITTABLE("-p", FALSE, NULL,
                                tapi_nptcp_opt, perturbation_size),

    TAPI_JOB_OPT_BOOL("-r", tapi_nptcp_opt, reset_sockets),
    TAPI_JOB_OPT_BOOL("-s", tapi_nptcp_opt, streaming_mode),
    TAPI_JOB_OPT_UINT_OMITTABLE("-u", FALSE, NULL,
                                tapi_nptcp_opt, upper_bound),

    TAPI_JOB_OPT_BOOL("-2", tapi_nptcp_opt, bi_directional_mode)
);

static te_errno
build_args(const tapi_nptcp_opt *opt_receiver,
           const tapi_nptcp_opt *opt_transmitter,
           te_vec *nptcp_args_receiver, te_vec *nptcp_args_transmitter)
{
    te_errno rc;

    rc = tapi_job_opt_build_args(path_to_nptcp_binary, nptcp_binds,
                                 opt_receiver, nptcp_args_receiver);
    if (rc != 0)
        return rc;

    rc = tapi_job_opt_build_args(path_to_nptcp_binary, nptcp_binds,
                                 opt_transmitter, nptcp_args_transmitter);
    return rc;
}

static te_errno
create_jobs(tapi_job_factory_t *factory_receiver,
            tapi_job_factory_t *factory_transmitter,
            tapi_nptcp_app *result,
            te_vec *nptcp_args_receiver, te_vec *nptcp_args_transmitter)
{
    te_errno rc;

    tapi_job_simple_desc_t job_desc_receiver = {
        .program = path_to_nptcp_binary,
        .argv = (const char **)nptcp_args_receiver->data.ptr,
        .job_loc = &result->job_receiver,
        .stderr_loc = &result->out_chs[0],
        .filters = TAPI_JOB_SIMPLE_FILTERS(
            {
                .use_stderr = TRUE,
                .readable = TRUE,
                .re = "Send and receive buffers are",
                .extract = 0,
                .filter_var = &result->receiver_listens_filter,
            }
        )
    };

    tapi_job_simple_desc_t job_desc_transmitter = {
        .program = path_to_nptcp_binary,
        .argv = (const char **)nptcp_args_transmitter->data.ptr,
        .job_loc = &result->job_transmitter,
        .stdout_loc = &result->out_chs[1],
        .stderr_loc = &result->out_chs[2],
        .filters = TAPI_JOB_SIMPLE_FILTERS(
            {
                .use_stderr = TRUE,
                .readable = TRUE,
                .re = "[0-9]+(?=:)",
                .extract = 0,
                .filter_var = &result->num_filter,
            },
            {
                .use_stderr = TRUE,
                .readable = TRUE,
                .re = ":\\s*([0-9]+)(?= bytes)",
                .extract = 1,
                .filter_var = &result->bytes_filter,
            },
            {
                .use_stderr = TRUE,
                .readable = TRUE,
                .re = "[0-9]+(?= times)",
                .extract = 0,
                .filter_var = &result->times_filter,
            },
            {
                .use_stderr = TRUE,
                .readable = TRUE,
                .re = "[0-9]+\\.[0-9]+(?= Mbps)",
                .extract = 0,
                .filter_var = &result->throughput_filter,
            },
            {
                .use_stderr = TRUE,
                .readable = TRUE,
                .re = "[0-9]+\\.[0-9]+(?= usec)",
                .extract = 0,
                .filter_var = &result->rtt_filter,
            },
            {
                .use_stdout = TRUE,
                .log_level = TE_LL_RING,
                .readable = FALSE,
                .filter_name = "transmitter's stdout",
            }
        )
    };

    rc = tapi_job_simple_create(factory_receiver, &job_desc_receiver);
    if (rc != 0)
        return rc;

    rc = tapi_job_simple_create(factory_transmitter, &job_desc_transmitter);
    if (rc != 0)
    {
        if (tapi_job_destroy(result->job_receiver,
                             TAPI_NPTCP_TERM_TIMEOUT_MS) == 0)
            result->job_receiver = NULL;
    }

    return rc;
}

te_errno
tapi_nptcp_create(tapi_job_factory_t *factory_receiver,
                  tapi_job_factory_t *factory_transmitter,
                  const tapi_nptcp_opt *opt_receiver,
                  const tapi_nptcp_opt *opt_transmitter,
                  tapi_nptcp_app **app)
{
    te_errno rc;
    te_vec nptcp_args_receiver = TE_VEC_INIT(char *);
    te_vec nptcp_args_transmitter = TE_VEC_INIT(char *);
    tapi_nptcp_app *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    rc = build_args(opt_receiver, opt_transmitter,
                    &nptcp_args_receiver, &nptcp_args_transmitter);
    if (rc != 0)
        goto out;

    rc = create_jobs(factory_receiver, factory_transmitter, result,
                     &nptcp_args_receiver, &nptcp_args_transmitter);
    if (rc != 0)
        goto out;

    *app = result;

out:
    te_vec_deep_free(&nptcp_args_receiver);
    te_vec_deep_free(&nptcp_args_transmitter);

    if (rc != 0)
        free(result);

    return rc;
}

te_errno
tapi_nptcp_start(tapi_nptcp_app *app)
{
    te_errno rc;

    rc = tapi_job_clear(TAPI_JOB_CHANNEL_SET(app->receiver_listens_filter));
    if (rc == 0)
        rc = tapi_job_clear(TAPI_JOB_CHANNEL_SET(app->num_filter,
                                                 app->bytes_filter,
                                                 app->times_filter,
                                                 app->throughput_filter,
                                                 app->rtt_filter));
    if (rc != 0)
        return rc;

    rc = tapi_job_start(app->job_receiver);
    if (rc != 0)
        return rc;

    /* Wait for receiver to start listening */
    rc = tapi_job_poll(TAPI_JOB_CHANNEL_SET(app->receiver_listens_filter),
                       TAPI_NPTCP_WAIT_RECEIVER_TIMEOUT_MS);
    if (rc != 0)
    {
        ERROR("Failed to wait for NPtcp on receiver's side to start listening");
        return rc;
    }

    return tapi_job_start(app->job_transmitter);
}

static te_errno
wait_job(tapi_job_t *job, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    rc = tapi_job_wait(job, timeout_ms, &status);
    if (rc != 0)
        return rc;

    TAPI_JOB_CHECK_STATUS(status);

    return 0;
}

te_errno
tapi_nptcp_wait_receiver(tapi_nptcp_app *app, int timeout_ms)
{
    return wait_job(app->job_receiver, timeout_ms);
}

te_errno
tapi_nptcp_wait_transmitter(tapi_nptcp_app *app, int timeout_ms)
{
    return wait_job(app->job_transmitter, timeout_ms);
}

te_errno
tapi_nptcp_wait(tapi_nptcp_app *app, int timeout_ms)
{
    te_errno rc;

    rc = tapi_nptcp_wait_receiver(app, timeout_ms);
    if (rc != 0)
        return rc;

    return tapi_nptcp_wait_transmitter(app, 0);
}

te_errno
tapi_nptcp_kill_receiver(tapi_nptcp_app *app, int signum)
{
    return tapi_job_kill(app->job_receiver, signum);
}

te_errno
tapi_nptcp_kill_transmitter(tapi_nptcp_app *app, int signum)
{
    return tapi_job_kill(app->job_transmitter, signum);
}

te_errno
tapi_nptcp_stop(tapi_nptcp_app *app)
{
    te_errno rc;

    rc = tapi_job_stop(app->job_receiver, SIGTERM, TAPI_NPTCP_TERM_TIMEOUT_MS);
    if (rc != 0)
        return rc;

    return tapi_job_stop(app->job_transmitter, SIGTERM,
                         TAPI_NPTCP_TERM_TIMEOUT_MS);
}

te_errno
tapi_nptcp_destroy(tapi_nptcp_app *app)
{
    te_errno rc_receiver, rc_transmitter;

    if (app == NULL)
        return 0;

    rc_receiver    = tapi_job_destroy(app->job_receiver,
                                      TAPI_NPTCP_TERM_TIMEOUT_MS);
    rc_transmitter = tapi_job_destroy(app->job_transmitter,
                                      TAPI_NPTCP_TERM_TIMEOUT_MS);
    if (rc_receiver != 0)
        return rc_receiver;
    if (rc_transmitter != 0)
        return rc_transmitter;

    free(app);

    return 0;
}

static te_errno
put_uint_from_string_to_vec(char *str, te_vec *out)
{
    te_errno rc;
    unsigned int val;

    rc = te_strtoui(str, 10, &val);
    if (rc != 0)
        return rc;

    return TE_VEC_APPEND(out, val);
}

static te_errno
put_double_from_string_to_vec(char *str, te_vec *out)
{
    te_errno rc;
    double val;

    rc = te_strtod(str, &val);
    if (rc != 0)
        return rc;

    return TE_VEC_APPEND(out, val);
}

/**
 * Read data from a filter and put it in a vector.
 *
 * @param[in]  filter                         filter to read from.
 * @param[in]  put_value_from_string_to_vec   function that takes a value
 *                                            from a string and puts it to
 *                                            a vector.
 * @param[out] out                            vector to store the received data.
 *
 * @return Status code.
 */
static te_errno
read_filter(tapi_job_channel_t *filter,
            te_errno (*put_value_from_string_to_vec)(char *str, te_vec *out),
            te_vec *out)
{
    te_errno rc;
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;

    while (1)
    {
        te_string_reset(&buf.data);
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(filter),
                              TAPI_NPTCP_RECEIVE_TIMEOUT_MS, &buf);
        if (rc != 0 || buf.eos)
            break;

        rc = put_value_from_string_to_vec(buf.data.ptr, out);
        if (rc != 0)
            break;
    }

    te_string_free(&buf.data);

    if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT && te_vec_size(out) > 0)
        rc = 0;
    else if (rc == 0 && te_vec_size(out) == 0)
        rc = TE_RC(TE_TAPI, TE_EFAIL);

    return rc;
}

static te_errno
read_all_filters(tapi_nptcp_app *app, te_vec *stats)
{
    te_errno rc;

    rc = read_filter(app->num_filter, put_uint_from_string_to_vec,
                     &stats[ENTRY_NUMBER]);
    if (rc == 0)
        rc = read_filter(app->bytes_filter, put_uint_from_string_to_vec,
                         &stats[ENTRY_BYTES]);
    if (rc == 0)
        rc = read_filter(app->times_filter, put_uint_from_string_to_vec,
                         &stats[ENTRY_TIMES]);
    if (rc == 0)
        rc = read_filter(app->throughput_filter, put_double_from_string_to_vec,
                         &stats[ENTRY_THROUGHPUT]);
    if (rc == 0)
        rc = read_filter(app->rtt_filter, put_double_from_string_to_vec,
                         &stats[ENTRY_RTT]);

    return rc;
}

/**
 * Get number of complete report entries. It is a minimum of vector sizes
 * in 'stats' array.
 */
static int
get_num_of_complete_entries(te_vec *stats, te_bool *incomplete)
{
    size_t min_size;
    size_t cur_size;
    size_t i;

    *incomplete = FALSE;
    min_size = te_vec_size(&stats[0]);
    for (i = 1; i < ENTRY_MAX; i++)
    {
        cur_size = te_vec_size(&stats[i]);
        if (cur_size != min_size && !(*incomplete))
            *incomplete = TRUE;

        min_size = MIN(min_size, cur_size);
    }

    return min_size;
}

static te_errno
fill_report_with_entries(te_vec *report, te_vec *stats)
{
    te_errno rc;
    tapi_nptcp_report_entry entry;
    size_t i;
    size_t entries_cnt;
    te_bool incomplete;

    entries_cnt = get_num_of_complete_entries(stats, &incomplete);
    if (incomplete)
        WARN("The report might be incomplete");

    for (i = 0; i < entries_cnt; i++)
    {
        memset(&entry, 0, sizeof(entry));

        entry.number = TE_VEC_GET(unsigned int, &stats[ENTRY_NUMBER], i);
        entry.bytes = TE_VEC_GET(unsigned int, &stats[ENTRY_BYTES], i);
        entry.times = TE_VEC_GET(unsigned int, &stats[ENTRY_TIMES], i);
        entry.throughput = TE_VEC_GET(double, &stats[ENTRY_THROUGHPUT], i);
        entry.rtt = TE_VEC_GET(double, &stats[ENTRY_RTT], i);

        rc = TE_VEC_APPEND(report, entry);
        if (rc != 0)
            return rc;
    }

    return 0;
}

te_errno
tapi_nptcp_get_report(tapi_nptcp_app *app, te_vec *report)
{
    te_errno rc;
    /* Array of vectors for data from num_filter, bytes_filter... */
    te_vec stats[ENTRY_MAX];
    size_t i;

    *report = TE_VEC_INIT(tapi_nptcp_report_entry);
    stats[ENTRY_NUMBER] = stats[ENTRY_BYTES] = stats[ENTRY_TIMES] =
                                                    TE_VEC_INIT(unsigned int);
    stats[ENTRY_THROUGHPUT] = stats[ENTRY_RTT] = TE_VEC_INIT(double);

    rc = read_all_filters(app, stats);
    if (rc == 0)
        rc = fill_report_with_entries(report, stats);

    for (i = 0; i < ENTRY_MAX; i++)
        te_vec_free(&stats[i]);

    return rc;
}

void
tapi_nptcp_report_mi_log(te_mi_logger *logger, te_vec *report)
{
    tapi_nptcp_report_entry *entry;

    TE_VEC_FOREACH(report, entry)
    {
        te_mi_logger_add_meas_vec(logger, NULL, TE_MI_MEAS_V(
                TE_MI_MEAS(THROUGHPUT, NULL, SINGLE, entry->throughput, MEBI),
                TE_MI_MEAS(LATENCY, NULL, SINGLE, entry->rtt, MICRO)));
    }
}
