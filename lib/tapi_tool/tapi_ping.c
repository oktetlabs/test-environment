/** @file
 * @brief ping tool TAPI
 *
 * TAPI to handle ping tool.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_LGR_USER "TAPI PING"

#include <signal.h>
#include <sys/socket.h>

#include "tapi_ping.h"
#include "tapi_job_opt.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_sockaddr.h"


#define TAPI_PING_TERM_TIMEOUT_MS     1000
#define TAPI_PING_RECEIVE_TIMEOUT_MS  1000
/*
 * Minimum value for packet_size ping option with which
 * rtt statistics will be produced
 */
#define TAPI_PING_MIN_PACKET_SIZE_FOR_RTT_STATS  16

struct tapi_ping_app {
    /* TAPI job handle */
    tapi_job_t *job;
    /* Output channel handles */
    tapi_job_channel_t *out_chs[2];
    /* Number of packets transmitted filter */
    tapi_job_channel_t *trans_filter;
    /* Number of packets received filter */
    tapi_job_channel_t *recv_filter;
    /* Percentage of packets lost filter */
    tapi_job_channel_t *lost_filter;
    /* RTT filter */
    tapi_job_channel_t *rtt_filter;
    /*
     * Number of data to send
     * (required here to check if rtt stats will be produced)
     */
    unsigned int packet_size;
};

const tapi_ping_opt tapi_ping_default_opt = {
    .packet_count     = TAPI_JOB_OPT_OMIT_UINT,
    .packet_size      = TAPI_JOB_OPT_OMIT_UINT,
    .interface        = NULL,
    .destination      = NULL,
};

static const tapi_job_opt_bind ping_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_UINT_OMITTABLE("-c", FALSE, NULL, tapi_ping_opt, packet_count),
    TAPI_JOB_OPT_UINT_OMITTABLE("-s", FALSE, NULL, tapi_ping_opt, packet_size),
    TAPI_JOB_OPT_STRING("-I", FALSE, tapi_ping_opt, interface),
    TAPI_JOB_OPT_STRING(NULL, FALSE, tapi_ping_opt, destination)
);

static te_errno
tapi_ping_create_with_v(tapi_job_factory_t *factory, const tapi_ping_opt *opt,
                 tapi_ping_app **app, te_bool use_ping6)
{
    te_errno        rc;
    tapi_ping_app  *result;
    const char     *path;
    te_vec          ping_args = TE_VEC_INIT(char *);

    path = use_ping6 ? "ping6" : "ping";

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
    {
        ERROR("Failed to allocate memory for ping app");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    rc = tapi_job_opt_build_args(path, ping_binds, opt, &ping_args);
    if (rc != 0)
    {
        ERROR("Failed to build ping options");
        goto out;
    }

    rc = tapi_job_simple_create(factory,
                            &(tapi_job_simple_desc_t){
                                .program    = path,
                                .argv       = (const char **)ping_args.data.ptr,
                                .job_loc    = &result->job,
                                .stdout_loc = &result->out_chs[0],
                                .stderr_loc = &result->out_chs[1],
                                .filters    = TAPI_JOB_SIMPLE_FILTERS(
                                    {
                                        .use_stdout  = TRUE,
                                        .readable    = TRUE,
                                        .re = "([0-9]+) packets transmitted",
                                        .extract     = 1,
                                        .filter_var  = &result->trans_filter,
                                    },
                                    {
                                        .use_stdout  = TRUE,
                                        .readable    = TRUE,
                                        .re = "([0-9]+) received",
                                        .extract     = 1,
                                        .filter_var  = &result->recv_filter,
                                    },
                                    {
                                        .use_stdout  = TRUE,
                                        .readable    = TRUE,
                                        .re = "([0-9]+)% packet loss",
                                        .extract     = 1,
                                        .filter_var  = &result->lost_filter,
                                    },
                                    {
                                        .use_stdout  = TRUE,
                                        .readable    = TRUE,
                                        .re = "rtt.*= (.*) ms",
                                        .extract     = 1,
                                        .filter_var  = &result->rtt_filter,
                                    },
                                    {
                                        .use_stderr  = TRUE,
                                        .readable    = FALSE,
                                        .log_level   = TE_LL_ERROR,
                                        .filter_name = "err",
                                    }
                                )
                            });
    if (rc != 0)
    {
        ERROR("Failed to create job instance for ping tool");
        goto out;
    }

    result->packet_size = opt->packet_size;

    *app = result;

out:
    te_vec_deep_free(&ping_args);
    if (rc != 0)
        free(result);

    return rc;
}

te_errno
tapi_ping_create(tapi_job_factory_t *factory, const tapi_ping_opt *opt,
                 tapi_ping_app **app)
{
    tapi_ping_opt                           ping_opts_v = tapi_ping_default_opt;
    tapi_ping_app                          *ping_app_v = NULL;
    struct sockaddr                         dest_addr;
    te_bool                                 use_ping6 = FALSE;
    te_errno                                rc;

    rc = te_sockaddr_netaddr_from_string(opt->destination, &dest_addr);
    if (rc != 0)
        goto out;

    /* Check if ping supports IPv6, switch to ping6 if it does not. */
    if (dest_addr.sa_family == AF_INET6)
    {
        ping_opts_v.destination = "::1";
        ping_opts_v.packet_count = 1;

        rc = tapi_ping_create_with_v(factory, &ping_opts_v, &ping_app_v, FALSE);
        if (rc != 0)
            goto out;
        rc = tapi_ping_start(ping_app_v);
        if (rc != 0)
            goto out;
        rc = tapi_ping_wait(ping_app_v, 1000);
        if (rc == TE_RC(TE_TAPI, TE_ESHCMD))
            use_ping6 = TRUE;
        else if (rc != 0)
            goto out;
    }

    rc = tapi_ping_create_with_v(factory, opt, app, use_ping6);

out:
    tapi_ping_destroy(ping_app_v);
    return rc;
}

te_errno
tapi_ping_start(tapi_ping_app *app)
{
    te_errno rc;

    /* We do this to be independent from all previous runs of the ping tool */
    rc = tapi_job_clear(TAPI_JOB_CHANNEL_SET(app->trans_filter,
                                             app->recv_filter,
                                             app->lost_filter,
                                             app->rtt_filter));
    if (rc != 0)
    {
        ERROR("Failed to clear filters");
        return rc;
    }

    return tapi_job_start(app->job);
}

te_errno
tapi_ping_wait(tapi_ping_app *app, int timeout_ms)
{
    te_errno           rc;
    tapi_job_status_t  status;

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
        return rc;

    TAPI_JOB_CHECK_STATUS(status);

    return 0;
}

te_errno
tapi_ping_kill(tapi_ping_app *app, int signum)
{
    return tapi_job_kill(app->job, signum);
}

te_errno
tapi_ping_stop(tapi_ping_app *app)
{
    return tapi_job_stop(app->job, SIGINT, TAPI_PING_TERM_TIMEOUT_MS);
}

te_errno
tapi_ping_destroy(tapi_ping_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_PING_TERM_TIMEOUT_MS);
    if (rc != 0)
        ERROR("Failed to destroy ping job");

    free(app);

    return rc;
}

static te_errno
parse_rtt_stats(const char *str, tapi_ping_rtt_stats *rtt)
{
    int ret;

    ret = sscanf(str, "%lf/%lf/%lf/%lf",
                &rtt->min, &rtt->avg, &rtt->max, &rtt->mdev);
    if (ret != 4)
    {
        ERROR("Failed to parse RTT statistics report");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
}

/* Read data from filter and assure that there is only one match */
static te_errno
read_filter(tapi_job_channel_t *filter, te_string *val)
{
    te_errno           rc;
    tapi_job_buffer_t  buf = TAPI_JOB_BUFFER_INIT;
    te_bool            matched = FALSE;

    while (1)
    {
        /**
         * There is no te_string_reset since we fail anyway if there are
         * at least two successfull receives.
         */
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(filter),
                              TAPI_PING_RECEIVE_TIMEOUT_MS, &buf);
        if (rc != 0)
        {
            if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT)
                break;

            ERROR("Failed to receive report from ping tool");
            te_string_free(&buf.data);

            return rc;
        }

        if (buf.eos)
            break;

        if (matched)
        {
            ERROR("Failed to receive report: multiple matches");
            te_string_free(&buf.data);

            return TE_EPROTO;
        }

        matched = TRUE;
        *val = buf.data;
    }

    if (!matched)
    {
        ERROR("Failed to receive report: no data");
        te_string_free(&buf.data);

        return TE_EPROTO;
    }

    return 0;
}

static te_errno
process_filter_uint_data(tapi_job_channel_t *filter, unsigned int *field)
{
    te_errno   rc;
    te_string  val = TE_STRING_INIT;

    rc = read_filter(filter, &val);
    if (rc == 0)
        rc = te_strtoui(val.ptr, 10, field);

    te_string_free(&val);

    return rc;
}

static te_errno
process_filter_rtt_data(tapi_job_channel_t *filter, tapi_ping_rtt_stats *field)
{
    te_errno   rc;
    te_string  val = TE_STRING_INIT;

    rc = read_filter(filter, &val);
    if (rc == 0)
        rc = parse_rtt_stats(val.ptr, field);

    te_string_free(&val);

    return rc;
}

te_errno
tapi_ping_get_report(tapi_ping_app *app, tapi_ping_report *report)
{
    te_errno rc;

    memset(report, 0, sizeof(*report));

    rc = process_filter_uint_data(app->trans_filter, &report->transmitted);
    if (rc != 0)
        return rc;

    rc = process_filter_uint_data(app->recv_filter, &report->received);
    if (rc != 0)
        return rc;

    rc = process_filter_uint_data(app->lost_filter, &report->lost_percentage);
    if (rc != 0)
        return rc;

    if (app->packet_size >= TAPI_PING_MIN_PACKET_SIZE_FOR_RTT_STATS)
    {
        report->with_rtt = TRUE;

        return process_filter_rtt_data(app->rtt_filter, &report->rtt);
    }
    else
    {
        WARN("Ping did not produce RTT statistics since payload size "
             "(packet_size option) is too small");
        report->with_rtt = FALSE;

        return 0;
    }
}

void
tapi_ping_report_mi_log(te_mi_logger *logger, tapi_ping_report *report)
{
    tapi_ping_rtt_stats *rtt = &report->rtt;

    if (!report->with_rtt)
        return;

    te_mi_logger_add_meas_vec(logger, NULL, TE_MI_MEAS_V(
            TE_MI_MEAS(RTT, NULL, MIN, rtt->min, MILLI),
            TE_MI_MEAS(RTT, NULL, MEAN, rtt->avg, MILLI),
            TE_MI_MEAS(RTT, NULL, MAX, rtt->max, MILLI),
            TE_MI_MEAS(RTT, NULL, STDEV, rtt->mdev, MILLI)));
}
