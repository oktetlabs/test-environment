/** @file
 * @brief Test API for netperf tool routine
 *
 * Test API to control 'netperf' tool.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#define TE_LGR_USER "TAPI NETPERF"

#include "te_config.h"

#include <sys/socket.h>
#include <math.h>
#include <stddef.h>

#include "tapi_netperf.h"
#include "tapi_mem.h"
#include "te_sockaddr.h"
#include "te_vector.h"
#include "tapi_job_opt.h"
#include "tapi_test.h"

/** Number of tapi_job output channels (one for stdout, one for stderr) */
#define TAPI_NETPERF_CHANNELS_STD_NUM 2
/* Time to wait till data is ready to read from stdout */
#define TAPI_NETPERF_RECEIVE_TIMEOUT_MS 1000
/* The timeout of termination of a job */
#define TAPI_NETPERF_TERM_TIMEOUT_MS 1000

struct tapi_netperf_app_server_t {
    /** TAPI job handle to netserver */
    tapi_job_t *job;
    /** Output channel handles to netserver */
    tapi_job_channel_t *out_chs[TAPI_NETPERF_CHANNELS_STD_NUM];
};

struct tapi_netperf_app_client_t {
    /** TAPI job handle to netperf */
    tapi_job_t *job;
    /** Output channel handles to netperf */
    tapi_job_channel_t *out_chs[TAPI_NETPERF_CHANNELS_STD_NUM];
    /** Test type */
    tapi_netperf_test_name tst_name;
    union {
        struct {
            /** Transactions per second filter */
            tapi_job_channel_t *trps_filter;
        } rr;
        struct {
            /** Megabits per second sending filter */
            tapi_job_channel_t *mbps_send_filter;
            /** Megabits per second receiving filter */
            tapi_job_channel_t *mbps_recv_filter;
        } stream;
    };
};

const tapi_netperf_opt tapi_netperf_default_opt = {
    .test_name = TAPI_NETPERF_TEST_TCP_STREAM,
    .dst_host = NULL,
    .src_host = NULL,
    .port = -1,
    .ipversion = AF_INET,
    .duration = 10,
    .test_opt = {
        .type = TAPI_NETPERF_TYPE_STREAM,
        .stream.buffer_send = -1,
        .stream.buffer_recv = -1,
    },
};

/* Correspondence between TAPI_NETPERF_TEST_*
 * and its string representation.
 */
static const char *tapi_netperf_test_name_maps[] = {
    [TAPI_NETPERF_TEST_TCP_STREAM] = "TCP_STREAM",
    [TAPI_NETPERF_TEST_UDP_STREAM] = "UDP_STREAM",
    [TAPI_NETPERF_TEST_TCP_MAERTS] = "TCP_MAERTS",
    [TAPI_NETPERF_TEST_TCP_RR] = "TCP_RR",
    [TAPI_NETPERF_TEST_UDP_RR] = "UDP_RR"
};

/**
 * Get @c test_type from @c test_name
 *
 * @param[in] name Test name
 *
 * @return Test type
 */
static tapi_netperf_test_type
test_name2test_type(tapi_netperf_test_name name)
{
    switch (name)
    {
        case TAPI_NETPERF_TEST_TCP_MAERTS:
        case TAPI_NETPERF_TEST_TCP_STREAM:
        case TAPI_NETPERF_TEST_UDP_STREAM:
            return TAPI_NETPERF_TYPE_STREAM;

        case TAPI_NETPERF_TEST_TCP_RR:
        case TAPI_NETPERF_TEST_UDP_RR:
            return TAPI_NETPERF_TYPE_RR;

        default:
            return TAPI_NETPERF_TYPE_UNKNOWN;
    }
}

static te_errno
set_test_stream_opt(te_vec *args, int32_t buffer_send,
                    int32_t buffer_recv)
{
    te_errno rc;

    if (buffer_send == -1 && buffer_recv == -1)
        return TE_ENOENT;

    if (buffer_send != -1)
    {
        rc = te_vec_append_str_fmt(args, "-m");
        if (rc != 0)
            return rc;
        rc = te_vec_append_str_fmt(args, "%d", buffer_send);
        if (rc != 0)
            return rc;
    }

    if (buffer_recv != -1)
    {
        rc = te_vec_append_str_fmt(args, "-M");
        if (rc != 0)
            return rc;
        rc = te_vec_append_str_fmt(args, "%d", buffer_recv);
        if (rc != 0)
            return rc;
    }
    return 0;
}

static te_errno
set_test_rr_opt(te_vec *args, int32_t response_size,
                int32_t request_size)
{
    te_errno rc;

    if (response_size == -1 && request_size == -1)
        return TE_ENOENT;

    rc = te_vec_append_str_fmt(args, "-r");
    if (rc != 0)
        return rc;

    /*
     * The RR test type accepts argument "-r" in format:
     * -r a,b; -r a; -r ,b
     */
    if (request_size != -1 && response_size != -1)
    {
        rc = te_vec_append_str_fmt(args, "%d,%d",
                                   request_size, response_size);
        if (rc != 0)
            return rc;
    }
    else if (request_size != -1)
    {
        rc = te_vec_append_str_fmt(args, "%d",
                                   request_size);
        if (rc != 0)
            return rc;
    }
    else if (response_size != -1)
    {
        rc = te_vec_append_str_fmt(args, ",%d",
                                   response_size);
        if (rc != 0)
            return rc;
    }
    return 0;
}
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
create_optional_port(const void *value, te_vec *args)
{
    int num = *(const int *)value;

    /* Set default value: 12865 */
    if (num == -1)
        return TE_ENOENT;

    return te_vec_append_str_fmt(args, "%d", num);
}

/** value type: `netperf_test_spec_cmdline` */
static te_errno
create_optional_test_spec(const void *value, te_vec *args)
{
    const tapi_netperf_test_opt *tst_opt = (const tapi_netperf_test_opt *)value;

    te_errno rc = 0;

    switch (tst_opt->type)
    {
        case TAPI_NETPERF_TYPE_STREAM:
            rc = set_test_stream_opt(args, tst_opt->stream.buffer_send,
                                     tst_opt->stream.buffer_recv);
            break;

        case TAPI_NETPERF_TYPE_RR:
            rc = set_test_rr_opt(args, tst_opt->rr.response_size,
                                 tst_opt->rr.request_size);
            break;

        default:
            ERROR("Unknown test type");
            return TE_RC(TE_TAPI, TE_EINVAL);

    }
    return rc;
}

/** value type: `tapi_netperf_test_name` */
static te_errno
create_optional_test_name(const void *value, te_vec *args)
{
    tapi_netperf_test_name test_name = *(const tapi_netperf_test_name *)value;

    return te_vec_append_str_fmt(args, "%s", tapi_netperf_test_name_maps[test_name]);
}

/** value type: `rpc_socket_proto` */
static te_errno
create_optional_ipversion(const void *value, te_vec *args)
{
    sa_family_t ipversion = *(const sa_family_t *)value;

    switch (ipversion)
    {
        case AF_INET:
             return te_vec_append_str_fmt(args, "-4");

        case AF_INET6:
             return te_vec_append_str_fmt(args, "-6");

        default:
            ERROR("Incorrect IP version");
            return TE_EINVAL;
    }
}
/**@} <!-- END custom tapi_job_opt_formatting --> */

#define CREATE_OPT_PORT(_prefix, _struct, _field) \
    {create_optional_port, _prefix, FALSE, NULL, \
     offsetof(_struct, _field) }

#define CREATE_OPT_TEST_SPEC(_prefix, _struct, _field) \
    {create_optional_test_spec, _prefix, FALSE, NULL, \
     offsetof(_struct, _field) }

#define CREATE_OPT_TEST_NAME(_prefix, _struct, _field) \
    {create_optional_test_name, _prefix, FALSE, NULL, \
     offsetof(_struct, _field) }

#define CREATE_OPT_IPVERSION(_struct, _field) \
    {create_optional_ipversion, NULL, FALSE, NULL, \
     offsetof(_struct, _field) }

/**
 * Сheck that the values in the structure are correct
 *
 * @param[in] opt Command line options.
 *
 * @return Status code.
 */
static te_errno
check_opt(const tapi_netperf_opt *opt)
{
    tapi_netperf_test_type test_type;

    test_type = test_name2test_type(opt->test_name);
    if (test_type == TAPI_NETPERF_TYPE_UNKNOWN)
    {
        ERROR("Unknown test type");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (test_type != opt->test_opt.type)
    {
        ERROR("Test type does not match with test name");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (opt->dst_host == NULL)
    {
        ERROR("Netserver address isn't specified.");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (opt->port < 0 && opt->port != -1)
    {
        ERROR("Port value must be is positive or -1");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    switch (opt->test_opt.type)
    {
        case TAPI_NETPERF_TYPE_RR:
            if (opt->test_opt.rr.response_size < 0 &&
                opt->test_opt.rr.response_size != -1)
            {
                ERROR("Response size value must be is positive or -1");
                return TE_RC(TE_TAPI, TE_EINVAL);
            }

            if (opt->test_opt.rr.request_size < 0 &&
                opt->test_opt.rr.request_size != -1)
            {
                ERROR("Request size value must be is positive or -1");
                return TE_RC(TE_TAPI, TE_EINVAL);
             }
            break;

        case TAPI_NETPERF_TYPE_STREAM:
            if (opt->test_opt.stream.buffer_recv < 0 &&
                opt->test_opt.stream.buffer_recv != -1)
            {
                ERROR("Send buffer value must be is positive or -1");
                return TE_RC(TE_TAPI, TE_EINVAL);
            }

            if (opt->test_opt.stream.buffer_send < 0 &&
                opt->test_opt.stream.buffer_send != -1)
            {
                ERROR("Receive buffer value must be is positive or -1");
                return TE_RC(TE_TAPI, TE_EINVAL);
            }
            break;

        default:
            ERROR("Unknown test type");
            return TE_RC(TE_TAPI, TE_EINVAL);
    }
    return 0;
}

te_errno
tapi_netperf_create_client(tapi_job_factory_t *factory,
                           const tapi_netperf_opt *opt,
                           tapi_netperf_app_client_t **app_client)
{
    tapi_netperf_app_client_t *app;
    te_vec                     args = TE_VEC_INIT(char *);
    te_errno                   rc = 0;
    char                      *path = "netperf";

    app = tapi_calloc(1, sizeof(*app));

    rc = check_opt(opt);
    if (rc != 0)
        goto out;

    const tapi_job_opt_bind netperf_binds[] = TAPI_JOB_OPT_SET(
        CREATE_OPT_TEST_NAME("-t", tapi_netperf_opt, test_name),
        TAPI_JOB_OPT_SOCKADDR_PTR("-H", FALSE, tapi_netperf_opt, dst_host),
        CREATE_OPT_IPVERSION(tapi_netperf_opt, ipversion),
        TAPI_JOB_OPT_SOCKADDR_PTR("-L", FALSE, tapi_netperf_opt, src_host),
        CREATE_OPT_PORT("-p", tapi_netperf_opt, port),
        TAPI_JOB_OPT_UINT("-l", FALSE, NULL, tapi_netperf_opt, duration),
        CREATE_OPT_TEST_SPEC("--", tapi_netperf_opt, test_opt)
    );

    rc = tapi_job_opt_build_args(path, netperf_binds, opt, &args);
    if (rc != 0)
        goto out;

    rc = tapi_job_create(factory, NULL, ((char **)args.data.ptr)[0],
                         (const char **)args.data.ptr, NULL, &app->job);
    if (rc != 0)
        goto out;

    rc = tapi_job_alloc_output_channels(app->job, TE_ARRAY_LEN(app->out_chs),
                                        app->out_chs);
    if (rc != 0)
        goto out;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                "netperf out", FALSE, TE_LL_RING, NULL);
    if (rc != 0)
        goto out;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[1]),
                                "netperf err", FALSE, TE_LL_ERROR, NULL);
    if (rc != 0)
        goto out;

    app->tst_name = opt->test_name;

    switch (opt->test_name)
    {
        case TAPI_NETPERF_TEST_UDP_RR:
        case TAPI_NETPERF_TEST_TCP_RR:
            rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                        "RR filter", TRUE, 0, &app->rr.trps_filter);
            if (rc != 0)
                goto out;
            rc = tapi_job_filter_add_regexp(app->rr.trps_filter,
                                            "per\\s*sec\\s*(?:\\S+\\s*){5}(\\S+)", 1);
            if (rc != 0)
                goto out;
            break;

        case TAPI_NETPERF_TEST_TCP_MAERTS:
        case TAPI_NETPERF_TEST_TCP_STREAM:
            rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                        "TCP STREAM filter", TRUE, 0, &app->stream.mbps_send_filter);
            if (rc != 0)
                goto out;
            rc = tapi_job_filter_add_regexp(app->stream.mbps_send_filter,
                                            "bits\\/sec\\s*(?:\\S+\\s*){4}(\\S+)", 1);
            if (rc != 0)
                 goto out;
            break;

        case TAPI_NETPERF_TEST_UDP_STREAM:
            rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                        "UDP STREAM filter", TRUE, 0, &app->stream.mbps_send_filter);
            if (rc != 0)
                goto out;

            rc = tapi_job_filter_add_regexp(app->stream.mbps_send_filter,
                                            "bits\\/sec\\s*(?:\\S+\\s*){5}(\\S+)", 1);
            if (rc != 0)
                goto out;

            rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                        "UDP STREAM filter", TRUE, 0, &app->stream.mbps_recv_filter);
            if (rc != 0)
                goto out;

            rc = tapi_job_filter_add_regexp(app->stream.mbps_recv_filter,
                                            "bits\\/sec\\s*(?:\\S+\\s*){9}(\\S+)", 1);
            if (rc != 0)
                goto out;
            break;

        default:
            ERROR("Unknown test name");
            return TE_RC(TE_TAPI, TE_EINVAL);

    }
    *app_client = app;
out:
    te_vec_deep_free(&args);
    if (rc != 0)
        free(app);
    return rc;
}

te_errno
tapi_netperf_create_server(tapi_job_factory_t *factory,
                           const tapi_netperf_opt *opt,
                           tapi_netperf_app_server_t **app_server)
{
    tapi_netperf_app_server_t *app;
    te_vec                    args = TE_VEC_INIT(char *);
    te_errno                  rc = 0;
    char                     *path = "netserver";

    app = tapi_calloc(1, sizeof(*app));

    rc = check_opt(opt);
    if (rc != 0)
        goto out;

    const tapi_job_opt_bind netserver_binds[] = TAPI_JOB_OPT_SET(
        TAPI_JOB_OPT_SOCKADDR_PTR("-L", FALSE, tapi_netperf_opt, dst_host),
        CREATE_OPT_PORT("-p", tapi_netperf_opt, port),
        CREATE_OPT_IPVERSION(tapi_netperf_opt, ipversion),
        TAPI_JOB_OPT_DUMMY("-D")
    );

    rc = tapi_job_opt_build_args(path, netserver_binds, opt, &args);
    if (rc != 0)
        goto out;

    rc = tapi_job_create(factory, NULL, path,
                         (const char **)args.data.ptr,
                         NULL, &app->job);
    if (rc != 0)
        goto out;

    rc = tapi_job_alloc_output_channels(app->job, TE_ARRAY_LEN(app->out_chs),
                                        app->out_chs);
    if (rc != 0)
        goto out;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                "netserver out", FALSE, TE_LL_RING, NULL);
    if (rc != 0)
        goto out;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[1]),
                                "netserver err", FALSE, TE_LL_ERROR, NULL);
    if (rc != 0)
        goto out;

    *app_server = app;
out:
    te_vec_deep_free(&args);
    if (rc != 0)
        free(app);
    return rc;
}

te_errno
tapi_netperf_create(tapi_job_factory_t *client_factory,
                    tapi_job_factory_t *server_factory,
                    const tapi_netperf_opt *opt,
                    tapi_netperf_app_client_t **client,
                    tapi_netperf_app_server_t **server)
{
    te_errno rc = 0;

    rc = tapi_netperf_create_server(server_factory, opt, server);
    if (rc != 0)
    {
        ERROR("Failed to create netserver");
        return rc;

    }

    rc = tapi_netperf_create_client(client_factory, opt, client);
    if (rc != 0)
    {
        ERROR("Failed to create netperf");
        return rc;
    }
    return 0;
}

te_errno
tapi_netperf_start_server(tapi_netperf_app_server_t *app)
{
    return tapi_job_start(app->job);
}

te_errno
tapi_netperf_start_client(tapi_netperf_app_client_t *app)
{
    return tapi_job_start(app->job);
}

te_errno
tapi_netperf_start(tapi_netperf_app_client_t *client,
                   tapi_netperf_app_server_t *server)
{
    te_errno rc;

    rc = tapi_netperf_start_server(server);
    if (rc != 0)
    {
        ERROR("Failed to start netserver");
        return rc;
    }

    rc = tapi_netperf_start_client(client);
    if (rc != 0)
    {
        ERROR("Failed to start netperf");
        return rc;
    }
    return 0;
}

te_errno
tapi_netperf_wait_client(tapi_netperf_app_client_t *app, int timeout_ms)
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
 * @param[in]  filter Filter for reading.
 * @param[out] out    String with message from filter.
 *
 * @return Statuc code
 */
static te_errno
read_filter(tapi_job_channel_t *filter, te_string *out)
{
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
    te_errno          rc = 0;

    while (!buf.eos && rc == 0)
    {
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(filter),
                              TAPI_NETPERF_RECEIVE_TIMEOUT_MS, &buf);
        if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT)
        {
            rc = 0;
            break;
        }
    }
    if (rc == 0)
        *out = buf.data;

    return rc;
}

te_errno
tapi_netperf_get_report(tapi_netperf_app_client_t *app,
                        tapi_netperf_report *report)
{
    te_errno  rc = 0;
    te_string buf = TE_STRING_INIT;

    report->tst_type = test_name2test_type(app->tst_name);

    switch (app->tst_name)
    {
        case TAPI_NETPERF_TEST_UDP_RR:
        case TAPI_NETPERF_TEST_TCP_RR:
            read_filter(app->rr.trps_filter, &buf);
            rc = sscanf(buf.ptr, "%lf", &report->rr.trps);
            if (rc <= 0)
            {
                te_string_free(&buf);
                return TE_RC(TE_TAPI, TE_EPROTO);
            }
            break;

        case TAPI_NETPERF_TEST_UDP_STREAM:
            read_filter(app->stream.mbps_send_filter, &buf);
            rc = sscanf(buf.ptr, "%lf", &report->stream.mbps_send);
            if (rc <= 0)
            {
                te_string_free(&buf);
                return TE_RC(TE_TAPI, TE_EPROTO);
            }
            te_string_reset(&buf);
            read_filter(app->stream.mbps_recv_filter, &buf);
            rc = sscanf(buf.ptr, "%lf", &report->stream.mbps_recv);
            if (rc <= 0)
            {
                te_string_free(&buf);
                return TE_RC(TE_TAPI, TE_EPROTO);
            }
            break;

        case TAPI_NETPERF_TEST_TCP_STREAM:
        case TAPI_NETPERF_TEST_TCP_MAERTS:
            read_filter(app->stream.mbps_send_filter, &buf);
            rc = sscanf(buf.ptr, "%lf", &report->stream.mbps_send);
            if (rc <= 0)
            {
                te_string_free(&buf);
                return TE_RC(TE_TAPI, TE_EPROTO);
            }
            te_string_reset(&buf);

            report->stream.mbps_recv = report->stream.mbps_send;
            break;

        default:
            ERROR("Unknown test name");
            return TE_RC(TE_TAPI, TE_EINVAL);
    }
    return 0;
}

te_errno
tapi_netperf_kill_client(tapi_netperf_app_client_t *app, int signo)
{
    return tapi_job_kill(app->job, signo);
}

te_errno
tapi_netperf_kill_server(tapi_netperf_app_server_t *app, int signo)
{
    return tapi_job_kill(app->job, signo);
}

te_errno
tapi_netperf_kill(tapi_netperf_app_client_t *client,
                  tapi_netperf_app_server_t *server,
                  int signo)
{
    te_errno rc;

    rc = tapi_netperf_kill_server(server, signo);
    if (rc != 0)
    {
        ERROR("Failed to kill netserver");
        return rc;
    }

    rc = tapi_netperf_kill_client(client, signo);
    if (rc != 0)
    {
        /*
         * netperf shuts down before calling this function.
         * Therefore the process no longer exists.
         */
        if (TE_RC_GET_ERROR(rc) == TE_ESRCH)
        {
            return 0;
        }
        else
        {
            ERROR("Failed to kill netperf");
            return rc;
        }
    }

    return 0;
}

te_errno
tapi_netperf_destroy_client(tapi_netperf_app_client_t *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_NETPERF_TERM_TIMEOUT_MS);
    if (rc != 0)
        return rc;

    free(app);
    return 0;
}

te_errno
tapi_netperf_destroy_server(tapi_netperf_app_server_t *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_NETPERF_TERM_TIMEOUT_MS);
    if (rc != 0)
        return rc;

    free(app);
    return 0;
}

te_errno
tapi_netperf_destroy(tapi_netperf_app_client_t *client,
                     tapi_netperf_app_server_t *server)
{
    te_errno rc;

    rc = tapi_netperf_destroy_server(server);
    if (rc != 0)
    {
        ERROR("Failed to destroy netserver");
        return rc;
    }

    rc = tapi_netperf_destroy_client(client);
    if (rc != 0)
    {
        ERROR("Failed to destroy netperf");
        return rc;
    }
    return 0;
}
