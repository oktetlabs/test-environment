/** @file
 * @brief Performance Test API to iperf tool routines
 *
 * Test API to control 'iperf' tool.
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

#include "te_config.h"
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#include "iperf.h"
#include "te_str.h"
#include "tapi_rpc_misc.h"
#include "tapi_test.h"
#include "performance_internal.h"


/* Time to wait till data is ready to read from stdout */
#define IPERF_TIMEOUT_MS (500)

/* Prototype of function to set option in iperf tool format */
typedef void (*set_opt_t)(te_string *, const tapi_perf_opts *);

/* iperf error messages mapping. */
typedef struct error_map {
    tapi_perf_error code;       /* Error code. */
    const char *msg;            /* Error message. */
} error_map;

/* Map of error messages corresponding to them codes. */
static error_map errors[] = {
    { TAPI_PERF_ERROR_READ ,    "read failed: Connection refused" },
    { TAPI_PERF_ERROR_CONNECT , "connect failed: Connection refused" },
    { TAPI_PERF_ERROR_BIND ,    "bind failed: Address already in use" }
};


/*
 * Set option of IP version in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_ipversion(te_string *cmd, const tapi_perf_opts *options)
{
    switch (options->ipversion)
    {
        case RPC_PROTO_DEF:
        case RPC_IPPROTO_IP:
            /* Do nothing for default value */
            break;

        case RPC_IPPROTO_IPV6:
            CHECK_RC(te_string_append(cmd, " -V"));
            break;

        default:
            TEST_FAIL("IP version value \"%s\" is not supported",
                      proto_rpc2str(options->ipversion));
    }
}

/*
 * Set option of protocol in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
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
 * Set option of server port to listen on/connect to in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_port(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->port >= 0)
        CHECK_RC(te_string_append(cmd, " -p%u", options->port));
}

/*
 * Set option of target bandwidth in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_bandwidth(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->bandwidth_bits >= 0)
        CHECK_RC(te_string_append(cmd, " -b%"PRId64, options->bandwidth_bits));
}

/*
 * Set option of number of bytes to transmit in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_bytes(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->num_bytes >= 0)
        CHECK_RC(te_string_append(cmd, " -n%"PRId64, options->num_bytes));
}

/*
 * Set option of time in seconds to transmit for in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_time(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->duration_sec >= 0)
        CHECK_RC(te_string_append(cmd, " -t%"PRId32, options->duration_sec));
}

/*
 * Set option of number of parallel client streams in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_streams(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->streams >= 0)
        CHECK_RC(te_string_append(cmd, " -P%"PRId16, options->streams));
}

/*
 * Build command string to run iperf server.
 *
 * @param cmd           Buffer to put built command to.
 * @param options       iperf tool server options.
 */
static void
build_server_cmd(te_string *cmd, const tapi_perf_opts *options)
{
    set_opt_t set_opt[] = {
        set_opt_port,
        set_opt_ipversion,
        set_opt_protocol
    };
    size_t i;

    ENTRY("Build command to run iperf server");
    CHECK_RC(te_string_append(cmd, "iperf -s"));
    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
        set_opt[i](cmd, options);
}

/*
 * Build command string to run iperf client.
 *
 * @param cmd           Buffer to put built command to.
 * @param options       iperf tool client options.
 */
static void
build_client_cmd(te_string *cmd, const tapi_perf_opts *options)
{
    set_opt_t set_opt[] = {
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

    if (te_str_is_null_or_empty(options->host))
        TEST_FAIL("Host to connect to is unspecified");

    CHECK_RC(te_string_append(cmd, "iperf -c %s", options->host));
    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
        set_opt[i](cmd, options);
}

/*
 * Convert a value according to its unit.
 *
 * @param val           Value to convert.
 * @param unit          Value unit, possible values are in (BbKkMmGg).
 * @param factor        Factor of unit (1000, 1024, and so on).
 *
 * @return Converted unit.
 */
static double
tounit(double val, char unit, int factor)
{
    switch (tolower(unit))
    {
        case 'b':
            return val;

        case 'k':
            return val * factor;

        case 'm':
            return val * factor * factor;

        case 'g':
            return val * factor * factor * factor;

        default:
            return 0;
    }
}

/*
 * Get iperf errors. The function reads an application stderr.
 *
 * @param[in]  app          iperf tool context.
 * @param[out] report       Report context.
 */
static void
app_get_error(tapi_perf_app *app, tapi_perf_report *report)
{
    size_t i;

    /* Read tool output */
    rpc_read_fd2te_string(app->rpcs, app->fd_stderr, IPERF_TIMEOUT_MS, 0,
                          &app->stderr);

    /* Check for available data */
    if (app->stderr.ptr == NULL || app->stderr.len == 0)
    {
        VERB("There is no error message");
        return;
    }
    INFO("iperf stderr:\n%s", app->stderr.ptr);

    for (i = 0; i < TE_ARRAY_LEN(errors); i++)
    {
        const char *ptr = app->stderr.ptr;

        while ((ptr = strstr(ptr, errors[i].msg)) != NULL)
        {
            report->errors[errors[i].code]++;
            ptr += strlen(errors[i].msg);
        }
    }
}

/*
 * Get iperf report. The function reads an application stdout.
 *
 * @param[in]  app          iperf tool context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
static te_errno
app_get_report(tapi_perf_app *app, tapi_perf_report *report)
{
    const char *str;
    double time;
    double bytes;
    double throughput;
    char bytes_unit;
    char throughput_unit;
    int rc;

    /* Get tool errors */
    memset(report->errors, 0, sizeof(report->errors));
    app_get_error(app, report);

    /* Read tool output */
    rpc_read_fd2te_string(app->rpcs, app->fd_stdout, IPERF_TIMEOUT_MS, 0,
                          &app->stdout);

    /* Check for available data */
    if (app->stdout.ptr == NULL || app->stdout.len == 0)
    {
        ERROR("There are no data in the output");
        return TE_RC(TE_TAPI, TE_ENODATA);
    }
    INFO("iperf stdout:\n%s", app->stdout.ptr);

/* Find the first occurrence of the substring _pattern in the string _src */
#define FIND_STRING(_dst, _src, _pattern)                           \
    do {                                                            \
        _dst = strstr(_src, _pattern);                              \
        if (_dst == NULL)                                           \
        {                                                           \
            ERROR("Failed to find the data in the iperf output");   \
            report->errors[TAPI_PERF_ERROR_FORMAT]++;               \
            return TE_RC(TE_TAPI, TE_ENODATA);                      \
        }                                                           \
    } while (0)

    /*
     * The required data is in a line contains [SUM]. If the line is missed,
     * a line with [ID] is followed by the line contains the data.
     * [ ID] Interval       Transfer     Bandwidth
     * [  4]  0.0- 5.1 sec  56.9 MBytes  94.1 Mbits/sec
     * [SUM] 0.0-60.3 sec 544 MBytes 75.6 Mbits/sec
     */
    str = strstr(app->stdout.ptr, "SUM]");
    if (str == NULL)
    {
        FIND_STRING(str, app->stdout.ptr, "ID]");
        FIND_STRING(str, str, "[");  /* Line is below [ID] */
    }
    FIND_STRING(str, str, "]");

#undef FIND_STRING

    /* Extract data */
    rc = sscanf(str, "%*s %*f-%lf %*s %lf %c%*s %lf %c",
                &time, &bytes, &bytes_unit, &throughput, &throughput_unit);
    if (rc == 5)    /* Number of extracted values */
    {
        report->seconds = time;
        report->bytes = tounit(bytes, bytes_unit, 1024);
        report->bits_per_second = tounit(throughput, throughput_unit, 1000);
    }
    else
    {
        ERROR("Failed to extract data of iperf output");
        report->errors[TAPI_PERF_ERROR_FORMAT]++;
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
}


/*
 * Start iperf server.
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

    ENTRY("Start iperf server on %s", RPC_NAME(rpcs));

    build_server_cmd(&cmd, &server->app.opts);
    return perf_app_start(rpcs, cmd.ptr, &server->app);
}

/*
 * Start iperf client.
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

    ENTRY("Start iperf client on %s", RPC_NAME(rpcs));

    build_client_cmd(&cmd, &client->app.opts);
    return perf_app_start(rpcs, cmd.ptr, &client->app);
}

/*
 * Stop iperf server.
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
    ENTRY("Stop iperf server");

    return perf_app_stop(&server->app);
}

/*
 * Stop iperf client.
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
    ENTRY("Stop iperf client");

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
    ENTRY("Wait until iperf client finishes his work, timeout is %u secs",
          timeout);

    return perf_app_wait(&client->app, timeout);
}

/*
 * Get server report. The function reads server stdout.
 *
 * @param[in]  server       Server context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
static te_errno
server_get_report(tapi_perf_server *server, tapi_perf_report *report)
{
    ENTRY("Get iperf server report");

    return app_get_report(&server->app, report);
}

/*
 * Get client report. The function reads client stdout.
 *
 * @param[in]  client       Client context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
static te_errno
client_get_report(tapi_perf_client *client, tapi_perf_report *report)
{
    ENTRY("Get iperf client report");

    return app_get_report(&client->app, report);
}


/*
 * iperf server specific methods.
 */
static tapi_perf_server_methods server_methods = {
    .start = server_start,
    .stop = server_stop,
    .get_report = server_get_report,
};

/*
 * iperf client specific methods.
 */
static tapi_perf_client_methods client_methods = {
    .start = client_start,
    .stop = client_stop,
    .wait = client_wait,
    .get_report = client_get_report,
};

/* See description in tapi_iperf.h */
void
iperf_server_init(tapi_perf_server *server)
{
    server->app.bench = TAPI_PERF_IPERF;
    server->methods = &server_methods;
}

/* See description in tapi_iperf.h */
void
iperf_client_init(tapi_perf_client *client)
{
    client->app.bench = TAPI_PERF_IPERF;
    client->methods = &client_methods;
}
