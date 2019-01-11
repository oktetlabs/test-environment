/** @file
 * @brief Performance Test API to iperf3 tool routines
 *
 * Test API to control 'iperf3' tool.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */
#define TE_LGR_USER     "TAPI iperf3"

#include "te_config.h"
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <jansson.h>
#include "iperf3.h"
#include "te_str.h"
#include "tapi_rpc_misc.h"
#include "tapi_test.h"
#include "performance_internal.h"
#include "tapi_mem.h"

/* The minimal representative duration */
#define IPERF_MIN_REPRESENTATIVE_DURATION   (1.0)
/* Time to wait till data is ready to read from stdout */
#define IPERF3_TIMEOUT_MS                   (500)

/* Prototype of function to set option in iperf3 tool format */
typedef void (*set_opt_t)(te_string *, const tapi_perf_opts *);


/* Map of error messages corresponding to them codes. */
static tapi_perf_error_map errors[] = {
    { TAPI_PERF_ERROR_CONNECT,
        "unable to connect to server: Connection refused" },
    { TAPI_PERF_ERROR_NOROUTE,
        "unable to connect to server: No route to host" },
    { TAPI_PERF_ERROR_BIND,
        "unable to start listener for connections: Address already in use" }
};


/*
 * Set option of IP version in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_ipversion(te_string *cmd, const tapi_perf_opts *options)
{
    static const char *opt[] = {
        [RPC_PROTO_DEF] = "",
        [RPC_IPPROTO_IP] = "-4",
        [RPC_IPPROTO_IPV6] = "-6"
    };
    int idx = options->ipversion;

    if (0 <= idx && (size_t)idx <= TE_ARRAY_LEN(opt))
        CHECK_RC(te_string_append(cmd, " %s", opt[idx]));
    else
        TEST_FAIL("IP version value \"%s\" is not supported",
                  proto_rpc2str(options->ipversion));
}

/*
 * Set option of protocol in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_protocol(te_string *cmd, const tapi_perf_opts *options)
{
    switch (options->protocol)
    {
        case RPC_PROTO_DEF:
        case RPC_IPPROTO_TCP:
            /* Do nothing for default value */
            break;

        case RPC_IPPROTO_UDP:
            CHECK_RC(te_string_append(cmd, " -u"));
            break;

        default:
            TEST_FAIL("Protocol value \"%s\" is not supported",
                      proto_rpc2str(options->protocol));
    }
}

/*
 * Set option of server port to listen on/connect to in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_port(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->port >= 0)
        CHECK_RC(te_string_append(cmd, " -p%u", options->port));
}

/*
 * Set option of target bandwidth in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_bandwidth(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->bandwidth_bits >= 0)
        CHECK_RC(te_string_append(cmd, " -b%"PRId64, options->bandwidth_bits));
}

/*
 * Set option of number of bytes to transmit in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_bytes(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->num_bytes >= 0)
        CHECK_RC(te_string_append(cmd, " -n%"PRId64, options->num_bytes));
}

/*
 * Set option of time in seconds to transmit for in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_time(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->duration_sec >= 0)
        CHECK_RC(te_string_append(cmd, " -t%"PRId32, options->duration_sec));
}

/*
 * Set option of pause in seconds between periodic bandwidth reports in iperf3
 * tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_interval(te_string *cmd, const tapi_perf_opts *options)
{
    int32_t interval = options->interval_sec;

    if (interval == TAPI_PERF_INTERVAL_DISABLED)
        interval = 0;

    if (interval >= 0)
        CHECK_RC(te_string_append(cmd, " -i%"PRId32, interval));
}

/*
 * Set option of length of buffer in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_length(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->length >= 0)
        CHECK_RC(te_string_append(cmd, " -l%"PRId32, options->length));
}

/*
 * Set option of number of parallel client streams in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_streams(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->streams >= 0)
        CHECK_RC(te_string_append(cmd, " -P%"PRId16, options->streams));
}

/*
 * Set option of dual (bidirectional) mode.
 * It is supported since 3.6+ github (not 3.6 release) version.
 * See https://github.com/esnet/iperf/pull/780.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_dual(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->dual)
        CHECK_RC(te_string_append(cmd, " --bidir"));
}

/*
 * Set option of reverse mode.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_reverse(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->reverse)
        CHECK_RC(te_string_append(cmd, " -R"));
}

/*
 * Build command string to run iperf3 server.
 *
 * @param cmd           Buffer to put built command to.
 * @param options       iperf3 tool server options.
 */
static void
build_server_cmd(te_string *cmd, const tapi_perf_opts *options)
{
    set_opt_t set_opt[] = {
        set_opt_port,
        set_opt_interval
    };
    size_t i;

    ENTRY("Build command to run iperf3 server");
    CHECK_RC(te_string_append(cmd, "iperf3 -s -J"));
    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
        set_opt[i](cmd, options);
}

/*
 * Build command string to run iperf3 client.
 *
 * @param cmd           Buffer to put built command to.
 * @param options       iperf3 tool client options.
 */
static void
build_client_cmd(te_string *cmd, const tapi_perf_opts *options)
{
    set_opt_t set_opt[] = {
        set_opt_port,
        set_opt_ipversion,
        set_opt_protocol,
        set_opt_bandwidth,
        set_opt_length,
        set_opt_bytes,
        set_opt_time,
        set_opt_interval,
        set_opt_streams,
        set_opt_reverse,
        set_opt_dual
    };
    size_t i;

    ENTRY("Build command to run iperf3 client");

    if (te_str_is_null_or_empty(options->host))
        TEST_FAIL("Host to connect to is unspecified");

    CHECK_RC(te_string_append(cmd, "iperf3 -c %s -J", options->host));
    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
        set_opt[i](cmd, options);
}

/*
 * Retrieve a double precision number from JSON object
 *
 * @param[in]  jobj   JSON object
 * @param[out] value  Pointer to a variable receiving value retrieved from JSON
 *                    object
 *
 * @return Status code
 */
static te_errno
jsonvalue2double(const json_t *jobj, double *value)
{
    double result;

    if (json_is_integer(jobj))
    {
        result = json_integer_value(jobj);
    }
    else if (json_is_real(jobj))
    {
        result = json_real_value(jobj);
    }
    else
    {
        return TE_EINVAL;
    }

    if (value != NULL)
        *value = result;

    return 0;
}

/*
 * Extract report from JSON object.
 *
 * @param[in]  jrpt         JSON object contains report data.
 * @param[in]  kind         Report kind.
 * @param[out] report       Report.
 *
 * @return Status code.
 */
static te_errno
get_report(const json_t *jrpt, tapi_perf_report_kind kind,
           tapi_perf_report *report)
{
    json_t *jend, *jsum, *jval, *jint;
    tapi_perf_report tmp_report;
    size_t   i;
    double  *seconds;
    double  *bits_per_second;
    double   total_seconds = 0.0;
    uint64_t total_bytes = 0;
    double   total_bits_per_second = 0.0;
    const double eps = 0.00001;
    size_t   total_intervals = 0;

#define GET_REPORT_ERROR(_obj)                                  \
    do {                                                        \
        ERROR("%s: JSON %s is expected", __FUNCTION__, _obj);   \
        return TE_RC(TE_TAPI, TE_EINVAL);                       \
    } while (0)

    if (!json_is_object(jrpt))
        GET_REPORT_ERROR("object");

    jend = json_object_get(jrpt, "intervals");
    if (!json_is_array(jend))
        GET_REPORT_ERROR("array \"intervals\"");

    if (json_array_size(jend) == 0)
        GET_REPORT_ERROR("non-empty array \"intervals\"");

    /*
     * Calculate an average of throughput results weighted by interval
     * durations, skipping completely wrong intervals altogether.
     */
    seconds = tapi_calloc(json_array_size(jend), sizeof(double));
    bits_per_second = tapi_calloc(json_array_size(jend), sizeof(double));
    for (i = 0; i < json_array_size(jend); ++i)
    {
        double  tmp_seconds;
        json_t *jsums;

        jint = json_array_get(jend, i);
        jsums = json_object_get(jint, "sums");
        if (json_is_array(jsums))
        {
            size_t j;

            for (j = 0; j < json_array_size(jsums); ++j)
            {
                jsum = json_array_get(jsums, j);

                if (kind == TAPI_PERF_REPORT_KIND_DEFAULT)
                    break;

                jval = json_object_get(jsum, "sender");
                if (json_is_boolean(jval))
                {
                    te_bool sender;

                    sender = json_boolean_value(jval);
                    if (sender ==
                        (kind == TAPI_PERF_REPORT_KIND_SENDER))
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            jsum = json_object_get(jint, "sum");
        }

        if (!json_is_object(jsum))
        {
            /*
             * This failure isn't fatal - iperf3 report can be
             * incomplete. Skip the entry as there is nothing to retrieve data
             * from
             */
            continue;
        }

        jval = json_object_get(jsum, "seconds");
        if (jsonvalue2double(jval, &tmp_seconds) != 0 ||
            !json_is_integer(json_object_get(jsum, "bytes")) ||
            !json_is_real(json_object_get(jsum, "bits_per_second")))
        {
            /*
             * This failure isn't fatal - some of (or all) specifications are
             * missing or invalid. Skip the entry as we won't be able to
             * retrieve useful data from it
             */
            continue;
        }

        seconds[total_intervals] = tmp_seconds;
        total_seconds += tmp_seconds;

        jval = json_object_get(jsum, "bytes");
        total_bytes += json_integer_value(jval);

        jval = json_object_get(jsum, "bits_per_second");
        bits_per_second[total_intervals] = json_real_value(jval);
        total_intervals++;
    }

    if (total_intervals == 0)
        GET_REPORT_ERROR("array of sane \"interval\" objects");

    if (total_seconds < eps)
        GET_REPORT_ERROR("object \"seconds\"");
    for (i = 0; i < total_intervals; ++i)
    {
        total_bits_per_second += bits_per_second[i] * seconds[i]
            / total_seconds;
    }

    free(bits_per_second);
    free(seconds);

    tmp_report.seconds = total_seconds;
    if (total_seconds < IPERF_MIN_REPRESENTATIVE_DURATION)
    {
        WARN("%s: the retrieved interval of %.1f duration might be "
             "unrepresentative", __FUNCTION__, total_seconds);
    }

    tmp_report.bytes = total_bytes;
    tmp_report.bits_per_second = total_bits_per_second;

    *report = tmp_report;
    return 0;
#undef GET_REPORT_ERROR
}

/*
 * Get error message from the JSON report.
 *
 * @param[in]  jrpt         JSON object contains report data.
 * @param[out] report       Report context.
 *
 * @return Status code.
 */
static te_errno
get_report_error(const json_t *jrpt, tapi_perf_report *report)
{
    const char *str;
    te_errno rc = 0;
    size_t i;

    if (!json_is_object(jrpt))
    {
        ERROR("JSON object is expected");
        report->errors[TAPI_PERF_ERROR_FORMAT]++;
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    str = json_string_value(json_object_get(jrpt, "error"));
    if (te_str_is_null_or_empty(str))
        return 0;

    for (i = 0; i < TE_ARRAY_LEN(errors); i++)
    {
        const char *ptr = strstr(str, errors[i].msg);

        if (ptr != NULL)
        {
            report->errors[errors[i].code]++;
            rc = TE_RC(TE_TAPI, TE_EINVAL);
        }
    }

    return rc;
}

/*
 * Get iperf3 report. The function reads an application output.
 *
 * @param[in]  app          iperf3 tool context.
 * @param[in]  kind         Report kind.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
static te_errno
app_get_report(tapi_perf_app *app, tapi_perf_report_kind kind,
               tapi_perf_report *report)
{
    json_error_t error;
    json_t *jrpt;
    te_errno rc;

    memset(report->errors, 0, sizeof(report->errors));

    if (app->stdout.ptr == NULL || app->stdout.len == 0)
    {
        /* Read tool output */
        rpc_read_fd2te_string(app->rpcs, app->fd_stdout, IPERF3_TIMEOUT_MS, 0,
                              &app->stdout);

        /* Check for available data */
        if (app->stdout.ptr == NULL || app->stdout.len == 0)
        {
            ERROR("There are no data in the output");
            return TE_RC(TE_TAPI, TE_ENODATA);
        }
    }

    INFO("iperf3 stdout:\n%s", app->stdout.ptr);

    /* Parse raw report */
    jrpt = json_loads(app->stdout.ptr, 0, &error);
    if (jrpt == NULL)
    {
        ERROR("json_loads fails with massage: \"%s\", position: %u",
              error.text, error.position);
        report->errors[TAPI_PERF_ERROR_FORMAT]++;
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = get_report_error(jrpt, report);
    if (rc == 0)
    {
        rc = get_report(jrpt, kind, report);
        if (rc != 0)
            report->errors[TAPI_PERF_ERROR_FORMAT]++;
    }

    json_decref(jrpt);
    return rc;
}

/*
 * Start iperf3 server.
 *
 * @param server            Server context.
 * @param rpcs              RPC server handle.
 *
 * @return Status code.
 *
 * @sa server_stop
 */
static te_errno
server_start(tapi_perf_server *server, rcf_rpc_server *rpcs)
{
    te_string cmd = TE_STRING_INIT;

    ENTRY("Start iperf3 server on %s", RPC_NAME(rpcs));

    build_server_cmd(&cmd, &server->app.opts);
    return perf_app_start(rpcs, cmd.ptr, &server->app);
}

/*
 * Start perf3 client.
 *
 * @param client            Client context.
 * @param rpcs              RPC server handle.
 *
 * @return Status code.
 *
 * @sa client_stop
 */
static te_errno
client_start(tapi_perf_client *client, rcf_rpc_server *rpcs)
{
    te_string cmd = TE_STRING_INIT;

    ENTRY("Start iperf3 client on %s", RPC_NAME(rpcs));

    build_client_cmd(&cmd, &client->app.opts);
    return perf_app_start(rpcs, cmd.ptr, &client->app);
}

/*
 * Stop iperf3 server.
 *
 * @param server            Server context.
 *
 * @return Status code.
 *
 * @sa server_start
 */
static te_errno
server_stop(tapi_perf_server *server)
{
    ENTRY("Stop iperf3 server");

    return perf_app_stop(&server->app);
}

/*
 * Stop perf3 client.
 *
 * @param client            Client context.
 *
 * @return Status code.
 *
 * @sa client_start
 */
static te_errno
client_stop(tapi_perf_client *client)
{
    ENTRY("Stop iperf3 client");

    return perf_app_stop(&client->app);
}

/*
 * Wait while client finishes his work.
 *
 * @param client        Client context.
 * @param timeout       Time to wait for client results (seconds). It MUST be
 *                      big enough to finish client normally (it depends
 *                      on client's options), otherwise the function will be
 *                      failed.
 *
 * @return Status code.
 */
static te_errno
client_wait(tapi_perf_client *client, int16_t timeout)
{
    ENTRY("Wait until iperf3 client finishes his work, timeout is %d secs",
          timeout);

    return perf_app_wait(&client->app, timeout);
}

/*
 * Get server report. The function reads server output.
 *
 * @param[in]  server       Server context.
 * @param[in]  kind         Report kind.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
static te_errno
server_get_report(tapi_perf_server *server, tapi_perf_report_kind kind,
                  tapi_perf_report *report)
{
    ENTRY("Get iperf3 server report");

    return app_get_report(&server->app, kind, report);
}

/*
 * Get client report. The function reads client output.
 *
 * @param[in]  client       Client context.
 * @param[in]  kind         Report kind.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
static te_errno
client_get_report(tapi_perf_client *client, tapi_perf_report_kind kind,
                  tapi_perf_report *report)
{
    ENTRY("Get iperf3 client report");

    return app_get_report(&client->app, kind, report);
}


/*
 * iperf3 server specific methods.
 */
static tapi_perf_server_methods server_methods = {
    .start = server_start,
    .stop = server_stop,
    .get_report = server_get_report
};

/*
 * iperf3 client specific methods.
 */
static tapi_perf_client_methods client_methods = {
    .start = client_start,
    .stop = client_stop,
    .wait = client_wait,
    .get_report = client_get_report
};

/* See description in tapi_iperf3.h */
void
iperf3_server_init(tapi_perf_server *server)
{
    server->app.bench = TAPI_PERF_IPERF3;
    server->methods = &server_methods;
}

/* See description in tapi_iperf3.h */
void
iperf3_client_init(tapi_perf_client *client)
{
    client->app.bench = TAPI_PERF_IPERF3;
    client->methods = &client_methods;
}
