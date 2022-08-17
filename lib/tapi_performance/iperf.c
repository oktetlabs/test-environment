/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Performance Test API to iperf tool routines
 *
 * Test API to control 'iperf' tool.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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


/* Map of error messages corresponding to them codes. */
static tapi_perf_error_map errors[] = {
    { TAPI_PERF_ERROR_READ,     "read failed: Connection refused" },
    { TAPI_PERF_ERROR_WRITE_CONN_RESET,
                                "write failed: Connection reset by peer" },
    { TAPI_PERF_ERROR_CONNECT,  "connect failed: Connection refused" },
    { TAPI_PERF_ERROR_NOROUTE,  "connect failed: No route to host" },
    { TAPI_PERF_ERROR_BIND,     "bind failed: Address already in use" }
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
            CHECK_RC(te_string_append(cmd, "-V"));
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
            CHECK_RC(te_string_append(cmd, "-u"));
            break;

        default:
            TEST_FAIL("Protocol value \"%s\" is not supported",
                      proto_rpc2str(options->protocol));
    }
}

/*
 * Set option of source host to originate traffic from for iperf tool.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_src_host(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->src_host != NULL && options->src_host[0] != '\0')
        CHECK_RC(te_string_append(cmd, "-B%s", options->src_host));
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
        CHECK_RC(te_string_append(cmd, "-p%u", options->port));
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
        CHECK_RC(te_string_append(cmd, "-b%"PRId64, options->bandwidth_bits));
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
        CHECK_RC(te_string_append(cmd, "-n%"PRId64, options->num_bytes));
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
        CHECK_RC(te_string_append(cmd, "-t%"PRId32, options->duration_sec));
}

/*
 * Set option of pause in seconds between periodic bandwidth reports in iperf
 * tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_interval(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->interval_sec >= 0)
        CHECK_RC(te_string_append(cmd, "-i%"PRId32, options->interval_sec));
}

/*
 * Set option of length of buffer in iperf tool format.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_length(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->length >= 0)
        CHECK_RC(te_string_append(cmd, "-l%"PRId32, options->length));
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
        CHECK_RC(te_string_append(cmd, "-P%"PRId16, options->streams));
}

/*
 * Set option of dual (bidirectional) mode.
 *
 * @param cmd           Buffer contains a command to add option to.
 * @param options       iperf tool options.
 */
static void
set_opt_dual(te_string *cmd, const tapi_perf_opts *options)
{
    if (options->dual)
        CHECK_RC(te_string_append(cmd, "-d"));
}

/* Get option by index */
static char *
get_option(set_opt_t *set_opt, const size_t index,
           const tapi_perf_opts *options)
{
    te_string opt = TE_STRING_INIT;

    set_opt[index](&opt, options);
    return opt.ptr;
}

/*
 * Build command string to run iperf server.
 *
 * @param args          List of built commands line arguments.
 * @param options       iperf tool server options.
 */
static void
build_server_args(te_vec *args, const tapi_perf_opts *options)
{
    set_opt_t set_opt[] = {
        set_opt_port,
        set_opt_ipversion,
        set_opt_protocol,
        set_opt_length,
        set_opt_interval
    };
    size_t i;

    ENTRY("Build command to run iperf server");
    CHECK_RC(te_vec_append_strarray(args,
                                    (const char *[]){"iperf", "-s", NULL}));

    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
    {
        char *opt = get_option(set_opt, i, options);

        if (opt != NULL)
            CHECK_RC(TE_VEC_APPEND(args, opt));
    }
    CHECK_RC(TE_VEC_APPEND_RVALUE(args, char *, NULL));
}

/*
 * Build command string to run iperf client.
 *
 * @param args          List of built commands line arguments.
 * @param options       iperf tool client options.
 */
static void
build_client_args(te_vec *args, const tapi_perf_opts *options)
{
    set_opt_t set_opt[] = {
        set_opt_src_host,
        set_opt_port,
        set_opt_ipversion,
        set_opt_protocol,
        set_opt_bandwidth,
        set_opt_length,
        set_opt_bytes,
        set_opt_time,
        set_opt_interval,
        set_opt_streams,
        set_opt_dual
    };
    size_t i;

    ENTRY("Build command to run iperf client");

    if (te_str_is_null_or_empty(options->host))
        TEST_FAIL("Host to connect to is unspecified");

    CHECK_RC(te_vec_append_strarray(args,
                                    (const char *[]){"iperf", "-c",
                                                     options->host, NULL}));
    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
    {
        char *opt = get_option(set_opt, i, options);

        if (opt != NULL)
            CHECK_RC(TE_VEC_APPEND(args, opt));
    }
    CHECK_RC(TE_VEC_APPEND_RVALUE(args, char *, NULL));
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
 *
 * @return Status code.
 *
 */
static te_errno
app_get_error(tapi_perf_app *app, tapi_perf_report *report,
              te_bool ignore_connect_write_errors)
{
    size_t i;
    te_errno rc;

    if (app->stderr.ptr == NULL || app->stderr.len == 0)
    {
        /* Read tool output */
        if ((rc = perf_app_read_output(app->err_filter, &app->stderr)) != 0)
            return rc;

        /* Check for available data */
        if (app->stderr.ptr == NULL || app->stderr.len == 0)
        {
            VERB("There is no error message");
            return 0;
        }
    }

    INFO("iperf stderr:\n%s", app->stderr.ptr);

    for (i = 0; i < TE_ARRAY_LEN(errors); i++)
    {
        const char *ptr = app->stderr.ptr;

        /* TAPI_PERF_ERROR_READ is mostly just a warning, not an error */
        if (errors[i].code == TAPI_PERF_ERROR_READ)
            continue;

        if (ignore_connect_write_errors &&
            (errors[i].code == TAPI_PERF_ERROR_CONNECT ||
             errors[i].code == TAPI_PERF_ERROR_WRITE_CONN_RESET))
        {
            continue;
        }

        while ((ptr = strstr(ptr, errors[i].msg)) != NULL)
        {
            report->errors[errors[i].code]++;
            ptr += strlen(errors[i].msg);
        }
    }
    return 0;
}

/*
 * Get iperf report. The function reads an application stdout.
 *
 * @param[in]  app          iperf tool context.
 * @param[in]  kind         Report kind.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
static te_errno
app_get_report(tapi_perf_app *app, tapi_perf_report_kind kind,
               tapi_perf_report *report, te_bool ignore_connect_write_errors)
{
    const char *str;
    double time;
    double bytes;
    double throughput;
    char bytes_unit;
    char throughput_unit;
    int rc;
    te_errno err;

    /* Get tool errors */
    memset(report->errors, 0, sizeof(report->errors));

    err = app_get_error(app, report, ignore_connect_write_errors);
    if (err != 0)
        return err;

    if (kind != TAPI_PERF_REPORT_KIND_DEFAULT)
    {
        ERROR("iperf TAPI doesn't support non-default report kind");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (app->stdout.ptr == NULL || app->stdout.len == 0)
    {
        /* Read tool output */
        err = perf_app_read_output(app->out_filter, &app->stdout);
        if (err != 0)
            return err;

        /* Check for available data */
        if (app->stdout.ptr == NULL || app->stdout.len == 0)
        {
            ERROR("There are no data in the output");
            return TE_RC(TE_TAPI, TE_ENODATA);
        }
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
     * The required data is in a line contains [SUM].
     * If the line is missed (in case of single stream), a line with [ID] is
     * followed by the line contains the data.
     * [ ID] Interval       Transfer     Bandwidth
     * [  4]  0.0- 5.1 sec  56.9 MBytes  94.1 Mbits/sec
     * [SUM] 0.0-60.3 sec 544 MBytes 75.6 Mbits/sec
     */
    if (app->opts.streams > 1)
        FIND_STRING(str, app->stdout.ptr, "SUM]");
    else
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
        /* Not supported */
        report->zero_intervals = 0;
        report->min_bps_per_stream = 0;
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
    ENTRY("Wait until iperf client finishes his work, timeout is %d secs",
          timeout);

    return perf_app_wait(&client->app, timeout);
}

/*
 * Get server report. The function reads server stdout.
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
    ENTRY("Get iperf server report");

    /*
     * There is an issue with iperf client's dual mode option. When the
     * option is enabled, iperf server produces "Connection refused" and
     * "Connection reset by peer" errors that seem to be not critical.
     */

    return app_get_report(&server->app, kind, report, server->app.opts.dual);
}

/*
 * Get client report. The function reads client stdout.
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
    ENTRY("Get iperf client report");

    return app_get_report(&client->app, kind, report, FALSE);
}


/*
 * iperf server specific methods.
 */
static tapi_perf_server_methods server_methods = {
    .build_args = build_server_args,
    .get_report = server_get_report,
};

/*
 * iperf client specific methods.
 */
static tapi_perf_client_methods client_methods = {
    .build_args = build_client_args,
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
