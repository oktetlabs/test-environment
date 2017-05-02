/** @file
 * @brief Performance Test API to iperf3 tool routines
 *
 * Test API to control 'iperf3' tool.
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */
#define TE_LGR_USER     "TAPI iperf3"
#define TE_LOG_LEVEL    0x003f  /* FIXME remove after debugging */

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


/* Error message about wrong JSON format. */
#define ERROR_INVALID_JSON_FORMAT   "invalid json format"
/* Time to wait till data is ready to read from stdout */
#define IPERF3_TIMEOUT_MS           (500)

/* Prototype of function to set option in iperf3 tool format */
typedef void (*set_opt_t)(te_string *, const tapi_perf_opts *);


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
        set_opt_port
    };
    size_t i;

    ENTRY("Build command to run iperf3 server");
    CHECK_RC(te_string_append(cmd, "iperf3 -s -J -i0"));
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
        set_opt_streams,
        set_opt_reverse
    };
    size_t i;

    ENTRY("Build command to run iperf3 client");

    if (te_str_is_null_or_empty(options->host))
        TEST_FAIL("Host to connect to is unspecified");

    CHECK_RC(te_string_append(cmd, "iperf3 -c %s -J -i0", options->host));
    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
        set_opt[i](cmd, options);
}

/*
 * Extract report from JSON object.
 *
 * @param[in]  jrpt         JSON object contains report data.
 * @param[out] report       Report.
 *
 * @return Status code.
 */
static te_errno
get_report(const json_t *jrpt, tapi_perf_report *report)
{
    json_t *jend, *jsum, *jval, *jint;
    tapi_perf_report tmp_report;

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

    jint = json_array_get(jend, json_array_size(jend) - 1);
    jsum = json_object_get(jint, "sum");
    if (!json_is_object(jsum))
        GET_REPORT_ERROR("object \"sum\"");

    jval = json_object_get(jsum, "bytes");
    if (json_is_integer(jval))
        tmp_report.bytes = json_integer_value(jval);
    else
        GET_REPORT_ERROR("value \"bytes\"");

    jval = json_object_get(jsum, "seconds");
    if (json_is_integer(jval))
        tmp_report.seconds = json_integer_value(jval);
    else if (json_is_real(jval))
        tmp_report.seconds = json_real_value(jval);
    else
        GET_REPORT_ERROR("value \"seconds\"");

    jval = json_object_get(jsum, "bits_per_second");
    if (json_is_real(jval))
        tmp_report.bits_per_second = json_real_value(jval);
    else
        GET_REPORT_ERROR("value \"bits_per_second\"");

    *report = tmp_report;
    return 0;
#undef GET_REPORT_ERROR
}

/*
 * Get error message from the JSON report.
 *
 * @param[in]  jrpt         JSON object contains report data.
 * @param[out] err          Buffer to allocate error massage.
 *
 * @return Status code.
 */
static te_errno
get_report_error(const json_t *jrpt, te_string *err)
{
    const char *str;

    if (!json_is_object(jrpt))
    {
        ERROR("JSON object is expected");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    str = json_string_value(json_object_get(jrpt, "error"));
    if (str != NULL)
        te_string_append(err, str);

    return 0;
}

/*
 * Get iperf3 report. The function reads an application output.
 *
 * @param[in]  app          iperf3 tool context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
static te_errno
app_get_report(tapi_perf_app *app, tapi_perf_report *report)
{
    json_error_t error;
    json_t *jrpt;
    te_errno rc;

    te_string_reset(&app->stderr);
    /* Read tool output */
    rpc_read_fd2te_string(app->rpcs, app->fd_stdout, IPERF3_TIMEOUT_MS, 0,
                          &app->stdout);
    INFO("iperf3 stdout:\n%s", app->stdout.ptr);

    /* Check for available data */
    if (app->stdout.ptr == NULL || app->stdout.len == 0)
    {
        ERROR("There are no data in the output");
        return TE_RC(TE_TAPI, TE_ENODATA);
    }

    /* Parse raw report */
    jrpt = json_loads(app->stdout.ptr, 0, &error);
    if (jrpt == NULL)
    {
        ERROR("json_loads fails with massage: \"%s\", position: %u",
              error.text, error.position);
        te_string_append(&app->stderr, ERROR_INVALID_JSON_FORMAT);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    rc = get_report(jrpt, report);
    if (rc != 0)
    {
        if (get_report_error(jrpt, &app->stderr) != 0)
            te_string_append(&app->stderr, ERROR_INVALID_JSON_FORMAT);
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
client_wait(tapi_perf_client *client, uint16_t timeout)
{
    ENTRY("Wait until iperf3 client finishes his work, timeout is %u secs",
          timeout);

    return perf_app_wait(&client->app, timeout);
}

/*
 * Get server report. The function reads server output.
 *
 * @param[in]  server       Server context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
static te_errno
server_get_report(tapi_perf_server *server, tapi_perf_report *report)
{
    ENTRY("Get iperf3 server report");

    return app_get_report(&server->app, report);
}

/*
 * Get client report. The function reads client output.
 *
 * @param[in]  client       Client context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
static te_errno
client_get_report(tapi_perf_client *client, tapi_perf_report *report)
{
    ENTRY("Get iperf3 client report");

    return app_get_report(&client->app, report);
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
