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
#define TE_LGR_USER     "TAPI iperf"
#define TE_LOG_LEVEL    0x003f  /* FIXME remove after debugging */

#include "te_config.h"
#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <jansson.h>
#include "tapi_iperf.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_test.h"


/* Error message about wrong JSON format. */
#define ERROR_INVALID_JSON_FORMAT   "invalid json format"
/* Name of RPC Server for iperf server process. */
#define PCO_IPERF_SERVER_NAME "pco_iperf_server"
/* Name of RPC Server for iperf client process. */
#define PCO_IPERF_CLIENT_NAME "pco_iperf_client"

/** iperf tool work mode */
typedef enum {
    IPERF_SERVER,   /**< Server mode */
    IPERF_CLIENT    /**< Client mode */
} iperf_mode;

/* Prototype of function to set option in iperf tool format */
typedef void (*set_opt_t)(te_string *, const tapi_iperf_options *);


/**
 * Set option of format to report in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_format(te_string *cmd, const tapi_iperf_options *options)
{
    static const char *opt[] = {
        [TAPI_IPERF_FORMAT_DEFAULT] = "",
        [TAPI_IPERF_FORMAT_KBITS] = "-f k",
        [TAPI_IPERF_FORMAT_MBITS] = "-f m",
        [TAPI_IPERF_FORMAT_KBYTES] = "-f K",
        [TAPI_IPERF_FORMAT_MBYTES] = "-f M"
    };
    int idx = options->format;

    if (0 <= idx && (size_t)idx <= TE_ARRAY_LEN(opt))
        CHECK_RC(te_string_append(cmd, " %s", opt[idx]));
    else
        TEST_FAIL("Format value is unknown");
}

/**
 * Set option of IP version in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_ipversion(te_string *cmd, const tapi_iperf_options *options)
{
    static const char *opt[] = {
        [TAPI_IPERF_IPVDEFAULT] = "",
        [TAPI_IPERF_IPV4] = "-4",
        [TAPI_IPERF_IPV6] = "-6"
    };
    int idx = options->client.ipversion;

    if (0 <= idx && (size_t)idx <= TE_ARRAY_LEN(opt))
        CHECK_RC(te_string_append(cmd, " %s", opt[idx]));
    else
        TEST_FAIL("IP version value is unknown");
}

/**
 * Set option of protocol in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_protocol(te_string *cmd, const tapi_iperf_options *options)
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
 * Set option of server port to listen on/connect to in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_port(te_string *cmd, const tapi_iperf_options *options)
{
    if (options->port == TAPI_IPERF_PORT_DEFAULT)
        return;
    if (options->port >= 0)
        CHECK_RC(te_string_append(cmd, " -p %u", options->port));
    else
        TEST_FAIL("Wrong value of port number");
}

/**
 * Set option of target bandwidth in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_bandwidth(te_string *cmd, const tapi_iperf_options *options)
{
    if (options->client.bandwidth != TAPI_IPERF_OPT_BANDWIDTH_DEFAULT)
        CHECK_RC(te_string_append(cmd, " -b %"PRIu64, options->client.bandwidth));
}

/**
 * Set option of number of bytes to transmit in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_bytes(te_string *cmd, const tapi_iperf_options *options)
{
    if (options->client.bytes != TAPI_IPERF_OPT_BYTES_DEFAULT)
        CHECK_RC(te_string_append(cmd, " -n %"PRIu64, options->client.bytes));
}

/**
 * Set option of time in seconds to transmit for in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_time(te_string *cmd, const tapi_iperf_options *options)
{
    if (options->client.time != TAPI_IPERF_OPT_TIME_DEFAULT)
        CHECK_RC(te_string_append(cmd, " -t %"PRIu32, options->client.time));
}

/**
 * Set option of number of parallel client streams in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_streams(te_string *cmd, const tapi_iperf_options *options)
{
    if (options->client.streams != TAPI_IPERF_OPT_STREAMS_DEFAULT)
        CHECK_RC(te_string_append(cmd, " -P %"PRIu16, options->client.streams));
}

/**
 * Build command string to run iperf server.
 *
 * @param cmd           Buffer to put built command to.
 * @param options       iperf tool server options.
 */
static void
build_iperf_server_cmd(te_string *cmd, const tapi_iperf_options *options)
{
    set_opt_t set_opt[] = {
        set_opt_format,
        set_opt_port
    };
    size_t i;

    ENTRY("Build command to run iperf server");
    CHECK_RC(te_string_append(cmd, "iperf3 -s -i0"));
    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
        set_opt[i](cmd, options);
}

/**
 * Build command string to run iperf client.
 *
 * @param cmd           Buffer to put built command to.
 * @param options       iperf tool client options.
 */
static void
build_iperf_client_cmd(te_string *cmd, const tapi_iperf_options *options)
{
    set_opt_t set_opt[] = {
        set_opt_format,
        set_opt_port,
        set_opt_ipversion,
        set_opt_protocol,
        set_opt_bandwidth,
        set_opt_bytes,
        set_opt_time,
        set_opt_streams
    };
    size_t i;

    ENTRY("Build command to run iperf client");
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
get_report(const json_t *jrpt, tapi_iperf_report *report)
{
    json_t *jend, *jsum, *jval;

    if (!json_is_object(jrpt))
    {
        ERROR("Invalid input data: JSON object is expected");
        return TE_EINVAL;
    }
    jend = json_object_get(jrpt, "end");
    if (jend == NULL || !json_is_object(jend))
    {
        ERROR("JSON object \"end\" is expected");
        return TE_EINVAL;
    }
    jsum = json_object_get(jend, "sum_sent");   /* TCP test */
    if (jsum == NULL)
        jsum = json_object_get(jend, "sum");    /* UDP test */
    if (jsum == NULL || !json_is_object(jsum))
    {
        ERROR("JSON object \"sum\"/\"sum_sent\" is expected");
        return TE_EINVAL;
    }
    report->bytes = json_integer_value(json_object_get(jsum, "bytes"));
    jval = json_object_get(jsum, "seconds");
    report->seconds = (json_is_integer(jval) ? json_integer_value(jval)
                                             : json_real_value(jval));
    report->bits_per_second =
        json_real_value(json_object_get(jsum, "bits_per_second"));

    return 0;
}

/**
 * Get error message from the client report.
 *
 * @param client        Client context.
 */
static void
get_client_error(tapi_iperf_client *client)
{
    const char *str = ERROR_INVALID_JSON_FORMAT;
    json_t *jrpt = NULL;
    json_error_t error;

    /* Parse raw report */
    jrpt = json_loads(client->report.ptr, 0, &error);
    if (jrpt == NULL)
    {
        ERROR("json_loads fails with massage: \"%s\", position: %u",
              error.text, error.position);
    }
    else if (json_is_object(jrpt))
    {
        str = json_string_value(json_object_get(jrpt, "error"));
        if (str == NULL)
            str = "report does not contain any errors";
    }

    te_string_append(&client->err, str);
    /* Note, json_decref makes obtained json string invalid, so it must be
     * called after string building. */
    json_decref(jrpt);
}

/**
 * Start iperf application. Start auxiliary RPC server and initialize application
 * context. It is possible to start up to 65536 applications at the same time
 * on the same agent of the same mode. Be careful since function doesn't check
 * how many applications do you start.
 * Note, @b tapi_iperf_app_stop should be called to stop the application and
 * to release it's resources.
 *
 * @param[in]    mode       iperf tool mode.
 * @param[in]    rpcs       RPC server handle.
 * @param[in]    options    iperf tool options.
 * @param[inout] app        iperf tool context.
 *
 * @return Status code.
 *
 * @sa tapi_iperf_app_stop
 */
static te_errno
tapi_iperf_app_start(iperf_mode mode,
                     rcf_rpc_server *rpcs,
                     const tapi_iperf_options *options,
                     tapi_iperf_app *app)
{
    te_string cmd = TE_STRING_INIT;
    tarpc_pid_t pid = TAPI_IPERF_PID_INVALID;
    int stdout = -1;
    te_errno rc = 0;

    switch (mode)
    {
        case IPERF_SERVER:
            build_iperf_server_cmd(&cmd, options);
            RING("Run \"%s\"", cmd.ptr);
            pid = rpc_te_shell_cmd(rpcs, cmd.ptr, -1, NULL, NULL, NULL);
            break;

        case IPERF_CLIENT:
            build_iperf_client_cmd(&cmd, options);
            RING("Run \"%s\"", cmd.ptr);
            pid = rpc_te_shell_cmd(rpcs, cmd.ptr, -1, NULL, &stdout, NULL);
            break;
    }

    if (pid >= 0)
    {
        app->rpcs = rpcs;
        app->pid = pid;
        app->stdout = stdout;
        app->cmd = cmd.ptr;
    }
    else
    {
        ERROR("%s:%d: Failed to run \"%s\"", __FUNCTION__, __LINE__, cmd.ptr);
        te_string_free(&cmd);
        rc = TE_EFAIL;
    }

    return TE_RC(TE_TAPI, rc);
}

/**
 * Stop iperf application, and release its context.
 *
 * @param[inout] app        iperf tool context.
 *
 * @return Status code.
 *
 * @sa tapi_iperf_app_start
 */
static te_errno
tapi_iperf_app_stop(tapi_iperf_app *app)
{
    rpc_ta_kill_death(app->rpcs, app->pid);
    app->pid = TAPI_IPERF_PID_INVALID;
    if (app->stdout >= 0)
        RPC_CLOSE(app->rpcs, app->stdout);
    app->rpcs = NULL;

    return 0;   /* Just to use it similarly to app_start function */
}


/* See description in tapi_iperf.h. */
te_errno
tapi_iperf_server_start(rcf_rpc_server *rpcs,
                        const tapi_iperf_options *options,
                        tapi_iperf_server *server)
{
    ENTRY("Start iperf server on %s", rpcs->ta);
    assert(server != NULL);

    return tapi_iperf_app_start(IPERF_SERVER, rpcs, options, &server->app);
}

/* See description in tapi_iperf.h. */
te_errno
tapi_iperf_server_stop(tapi_iperf_server *server)
{
    ENTRY("Stop iperf server");

    if (server == NULL || server->app.pid < 0)
        return 0;

    return tapi_iperf_app_stop(&server->app);
}

/* See description in tapi_iperf.h. */
te_errno
tapi_iperf_server_release(tapi_iperf_server *server)
{
    ENTRY("Release iperf server");

    free(server->app.cmd);
    return tapi_iperf_server_stop(server);
}

/* See description in tapi_iperf.h. */
te_errno
tapi_iperf_client_start(rcf_rpc_server *rpcs,
                        const tapi_iperf_options *options,
                        tapi_iperf_client *client)
{
    ENTRY("Start iperf client on %s", rpcs->ta);
    assert(client != NULL);

    te_string_reset(&client->report);
    te_string_reset(&client->err);
    return tapi_iperf_app_start(IPERF_CLIENT, rpcs, options, &client->app);
}

/* See description in tapi_iperf.h. */
te_errno
tapi_iperf_client_stop(tapi_iperf_client *client)
{
    ENTRY("Stop iperf client");

    if (client == NULL || client->app.pid < 0)
        return 0;

    return tapi_iperf_app_stop(&client->app);
}

/* See description in tapi_iperf.h. */
te_errno
tapi_iperf_client_release(tapi_iperf_client *client)
{
    ENTRY("Release iperf client");

    free(client->app.cmd);
    te_string_free(&client->report);
    te_string_free(&client->err);
    return tapi_iperf_client_stop(client);
}

/* See description in tapi_iperf.h. */
te_errno
tapi_iperf_client_wait(tapi_iperf_client *client, uint16_t timeout)
{
    rpc_wait_status stat;
    rcf_rpc_server *rpcs;
    int rc;

    ENTRY("Wait until iperf client finishes the work, timeout is %u secs",
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
    client->app.pid = TAPI_IPERF_PID_INVALID;

    /* Read tool output */
    CHECK_RC(tapi_rpc_append_fd_to_te_string(rpcs, client->app.stdout,
                                             &client->report));
    INFO("iperf stdout:\n%s", client->report.ptr);

    /* Check for errors */
    if (stat.value != 0 || stat.flag != RPC_WAIT_STATUS_EXITED)
    {
        get_client_error(client);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    return 0;
}

/* See description in tapi_iperf.h. */
te_errno
tapi_iperf_client_get_report(tapi_iperf_client *client,
                             tapi_iperf_report *report)
{
    json_error_t error;
    json_t *jrpt;
    te_errno rc;

    ENTRY("Get iperf report");

    if (client->report.ptr == NULL || client->report.len == 0)
    {
        ERROR("%s:%d: There are no data in the report", __FUNCTION__, __LINE__);
        return TE_RC(TE_TAPI, TE_ENODATA);
    }

    /* Parse raw report */
    jrpt = json_loads(client->report.ptr, 0, &error);
    if (jrpt == NULL)
    {
        ERROR("json_loads fails with massage: \"%s\", position: %u",
              error.text, error.position);
        te_string_append(&client->err, ERROR_INVALID_JSON_FORMAT);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    rc = get_report(jrpt, report);
    if (rc != 0)
        te_string_append(&client->err, ERROR_INVALID_JSON_FORMAT);

    json_decref(jrpt);
    return rc;
}

/* See description in tapi_iperf.h. */
void
tapi_iperf_client_print_report(const tapi_iperf_report *report)
{
    RING("IPERF3_BITSSEC_REPORT: %"PRIu64" bytes, %.1f secs, %.1f bits/sec",
         report->bytes, report->seconds, report->bits_per_second);
}
