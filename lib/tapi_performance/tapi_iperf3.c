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
# include <inttypes.h>
#endif
#include <jansson.h>
#include "tapi_iperf3.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_misc.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_test.h"
#include "tapi_mem.h"


/* Error message about wrong JSON format. */
#define ERROR_INVALID_JSON_FORMAT   "invalid json format"
/* Time to wait till data is ready to read from stdout */
#define IPERF3_TIMEOUT_MS           (500)

/* Prototype of function to set option in iperf3 tool format */
typedef void (*set_opt_t)(te_string *, const tapi_iperf3_options *);


/**
 * Set option of IP version in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_ipversion(te_string *cmd, const tapi_iperf3_options *options)
{
    static const char *opt[] = {
        [TAPI_IPERF3_IPVDEFAULT] = "",
        [TAPI_IPERF3_IPV4] = "-4",
        [TAPI_IPERF3_IPV6] = "-6"
    };
    int idx = options->client.ipversion;

    if (0 <= idx && (size_t)idx <= TE_ARRAY_LEN(opt))
        CHECK_RC(te_string_append(cmd, " %s", opt[idx]));
    else
        TEST_FAIL("IP version value is unknown");
}

/**
 * Set option of protocol in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_protocol(te_string *cmd, const tapi_iperf3_options *options)
{
    switch (options->client.protocol)
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
                      proto_rpc2str(options->client.protocol));
    }
}

/**
 * Set option of server port to listen on/connect to in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_port(te_string *cmd, const tapi_iperf3_options *options)
{
    if (options->port == TAPI_IPERF3_PORT_DEFAULT)
        return;
    if (options->port >= 0)
        CHECK_RC(te_string_append(cmd, " -p%u", options->port));
    else
        TEST_FAIL("Wrong value of port number");
}

/**
 * Set option of target bandwidth in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_bandwidth(te_string *cmd, const tapi_iperf3_options *options)
{
    if (options->client.bandwidth != TAPI_IPERF3_OPT_BANDWIDTH_DEFAULT)
        CHECK_RC(te_string_append(cmd, " -b%"PRIu64, options->client.bandwidth));
}

/**
 * Set option of number of bytes to transmit in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_bytes(te_string *cmd, const tapi_iperf3_options *options)
{
    if (options->client.bytes != TAPI_IPERF3_OPT_BYTES_DEFAULT)
        CHECK_RC(te_string_append(cmd, " -n%"PRIu64, options->client.bytes));
}

/**
 * Set option of time in seconds to transmit for in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_time(te_string *cmd, const tapi_iperf3_options *options)
{
    if (options->client.time != TAPI_IPERF3_OPT_TIME_DEFAULT)
        CHECK_RC(te_string_append(cmd, " -t%"PRIu32, options->client.time));
}

/**
 * Set option of length of buffer in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_length(te_string *cmd, const tapi_iperf3_options *options)
{
    if (options->client.length != TAPI_IPERF3_OPT_LENGTH_DEFAULT)
        CHECK_RC(te_string_append(cmd, " -l%"PRIu32, options->client.length));
}

/**
 * Set option of number of parallel client streams in iperf3 tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_streams(te_string *cmd, const tapi_iperf3_options *options)
{
    if (options->client.streams != TAPI_IPERF3_OPT_STREAMS_DEFAULT)
        CHECK_RC(te_string_append(cmd, " -P%"PRIu16, options->client.streams));
}

/**
 * Set option of reverse mode.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf3 tool options.
 */
static void
set_opt_reverse(te_string *cmd, const tapi_iperf3_options *options)
{
    if (options->client.reverse)
        CHECK_RC(te_string_append(cmd, " -R"));
}

/**
 * Build command string to run iperf3 server.
 *
 * @param cmd           Buffer to put built command to.
 * @param options       iperf3 tool server options.
 */
static void
build_server_cmd(te_string *cmd, const tapi_iperf3_options *options)
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

/**
 * Build command string to run iperf3 client.
 *
 * @param cmd           Buffer to put built command to.
 * @param options       iperf3 tool client options.
 */
static void
build_client_cmd(te_string *cmd, const tapi_iperf3_options *options)
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
    CHECK_RC(te_string_append(cmd, "iperf3 -c %s -J -i0", options->client.host));
    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
        set_opt[i](cmd, options);
}

/**
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

/**
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

/**
 * Allocate memory and copy options. Note, the function jumps to cleanup in case
 * of failure of memory allocation.
 *
 * @param options       Options to copy from.
 *
 * @return Allocated options.
 *
 * @sa server_options_free
 */
static tapi_iperf3_options *
server_options_copy(const tapi_iperf3_options *options)
{
    tapi_iperf3_options *opts;

    opts = tapi_calloc(1, sizeof(*opts));
    *opts = *options;
    return opts;
}

/**
 * Free options allocated by @ref server_options_copy.
 *
 * @param options       Options to free.
 *
 * @sa server_options_copy
 */
static void
server_options_free(tapi_perf_opts *options)
{
    free(options);
}

/**
 * Allocate memory and copy options. Note, the function jumps to cleanup in case
 * of failure of memory allocation.
 *
 * @param options       Options to copy from.
 *
 * @return Allocated options.
 *
 * @sa client_options_free
 */
static tapi_iperf3_options *
client_options_copy(const tapi_iperf3_options *options)
{
    tapi_iperf3_options *opts;

    opts = tapi_calloc(1, sizeof(*opts));
    *opts = *options;
    opts->client.host = tapi_strdup(options->client.host);
    return opts;
}

/**
 * Free options allocated by @ref client_options_copy.
 *
 * @param options       Options to free.
 *
 * @sa client_options_copy
 */
static void
client_options_free(tapi_perf_opts *options)
{
    tapi_iperf3_options *opts = (tapi_iperf3_options *)options;

    free(opts->client.host);
    free(opts);
}

/**
 * Start iperf3 application. Note, @b app_stop should be called to stop the
 * application.
 *
 * @param[in]    rpcs       RPC server handle.
 * @param[in]    cmd        Command to execute (start) application. Note, in
 *                          case of failure @p cmd will be free, just to make
 *                          @ref client_start and @ref server_start simple.
 * @param[inout] app        iperf3 tool context.
 *
 * @return Status code.
 *
 * @sa app_stop
 */
static te_errno
app_start(rcf_rpc_server *rpcs, char *cmd, tapi_perf_app *app)
{
    tarpc_pid_t pid;
    int stdout = -1;

    RING("Run \"%s\"", cmd);
    pid = rpc_te_shell_cmd(rpcs, cmd, -1, NULL, &stdout, NULL);
    if (pid < 0)
    {
        ERROR("Failed to start iperf3");
        free(cmd);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    if (app->rpcs != NULL)
    {
        if (app>stdout >= 0)
            RPC_CLOSE(app->rpcs, app->stdout);
        free(app->cmd);
    }
    app->rpcs = rpcs;
    app->pid = pid;
    app->stdout = stdout;
    app->cmd = cmd;

    return 0;
}

/**
 * Stop iperf3 application.
 *
 * @param[inout] app        iperf3 tool context.
 *
 * @return Status code.
 *
 * @sa app_start
 */
static te_errno
app_stop(tapi_perf_app *app)
{
    rpc_ta_kill_death(app->rpcs, app->pid);
    app->pid = -1;

    return 0;   /* Just to use it similarly to app_start function */
}

/**
 * Release iperf3 application context.
 *
 * @param app               iperf3 tool context.
 */
static void
app_fini(tapi_perf_app *app)
{
    if (app->stdout >= 0)
        RPC_CLOSE(app->rpcs, app->stdout);
    free(app->cmd);
    te_string_free(&app->report);
    te_string_free(&app->err);
    app->rpcs = NULL;
    app->opts = NULL;
}

/**
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

    te_string_reset(&app->err);
    /* Read tool output */
    rpc_read_fd2te_string(app->rpcs, app->stdout, IPERF3_TIMEOUT_MS, 0,
                          &app->report);
    INFO("iperf3 stdout:\n%s", app->report.ptr);

    /* Check for available data */
    if (app->report.ptr == NULL || app->report.len == 0)
    {
        ERROR("There are no data in the report");
        return TE_RC(TE_TAPI, TE_ENODATA);
    }

    /* Parse raw report */
    jrpt = json_loads(app->report.ptr, 0, &error);
    if (jrpt == NULL)
    {
        ERROR("json_loads fails with massage: \"%s\", position: %u",
              error.text, error.position);
        te_string_append(&app->err, ERROR_INVALID_JSON_FORMAT);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    rc = get_report(jrpt, report);
    if (rc != 0)
    {
        if (get_report_error(jrpt, &app->err) != 0)
            te_string_append(&app->err, ERROR_INVALID_JSON_FORMAT);
    }

    json_decref(jrpt);
    return rc;
}


/**
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

    build_server_cmd(&cmd, (tapi_iperf3_options *)server->app.opts);
    return app_start(rpcs, cmd.ptr, &server->app);
}

/**
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

    if (server->app.pid < 0)
        return 0;

    return app_stop(&server->app);
}

/**
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


/**
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

    build_client_cmd(&cmd, (tapi_iperf3_options *)client->app.opts);
    return app_start(rpcs, cmd.ptr, &client->app);
}

/**
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

    if (client->app.pid < 0)
        return 0;

    return app_stop(&client->app);
}

/**
 * Wait while client finishes his work. Note, function jumps to cleanup if
 * timeout is expired.
 *
 * @param client        Client context.
 * @param timeout       Time to wait for client results (seconds). It MUST be
 *                      big enough to finish client normally (it depends
 *                      on client's options), otherwise the function will be
 *                      failed.
 *
 * @return Status code.
 * @retval 0            No errors.
 * @retval TE_ESHCMD    You can call @ref client_get_report to get an error
 *                      message.
 * @retval TE_EFAIL     Critical error, client should be stopped.
 */
static te_errno
client_wait(tapi_perf_client *client, uint16_t timeout)
{
    rpc_wait_status stat;
    rcf_rpc_server *rpcs;
    int rc;

    ENTRY("Wait until iperf3 client finishes his work, timeout is %u secs",
          timeout);

    rpcs = client->app.rpcs;
    rpcs->timeout = TE_SEC2MS(timeout);
    RPC_AWAIT_IUT_ERROR(rpcs);
    rc = rpc_waitpid(rpcs, client->app.pid, &stat, 0);
    if (rc != client->app.pid)
    {
        ERROR("waitpid() failed with errno %r", RPC_ERRNO(rpcs));
        return TE_RC(TE_TAPI, TE_EFAIL);
    }
    client->app.pid = -1;

    /* Check for errors */
    if (stat.value != 0 || stat.flag != RPC_WAIT_STATUS_EXITED)
        return TE_RC(TE_TAPI, TE_ESHCMD);

    return 0;
}

/**
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

/**
 * iperf3 server specific methods.
 */
static tapi_perf_server_methods server_methods = {
    .start = server_start,
    .stop = server_stop,
    .get_report = server_get_report
};

/**
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
tapi_iperf3_server_init(tapi_perf_server *server,
                        const tapi_iperf3_options *options)
{
    server->app.opts = (tapi_perf_opts *)server_options_copy(options);
    server->methods = &server_methods;
}

/* See description in tapi_iperf3.h */
void
tapi_iperf3_server_fini(tapi_perf_server *server)
{
    server->methods->stop(server);
    server_options_free(server->app.opts);
    app_fini(&server->app);
    server->methods = NULL;
}

/* See description in tapi_iperf3.h */
void
tapi_iperf3_client_init(tapi_perf_client *client,
                        const tapi_iperf3_options *options)
{
    client->app.opts = (tapi_perf_opts *)client_options_copy(options);
    client->methods = &client_methods;
}

/* See description in tapi_iperf3.h */
void
tapi_iperf3_client_fini(tapi_perf_client *client)
{
    client->methods->stop(client);
    client_options_free(client->app.opts);
    app_fini(&client->app);
    client->methods = NULL;
}
