/** @file
 * @brief Generic Test API to network throughput test tools
 *
 * Generic high level test API to control a network throughput test tool.
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
#define TE_LGR_USER     "TAPI performance"

#include "tapi_test_log.h"
#include "tapi_mem.h"
#include "performance_internal.h"
#include "iperf.h"
#include "iperf3.h"


/*
 * Initialize perf application context.
 *
 * @param app           Perf tool context.
 */
static void
app_init(tapi_perf_app *app, const tapi_perf_opts *options)
{
    app->rpcs = NULL;
    app->pid = -1;
    app->fd_stdout = -1;
    app->fd_stderr = -1;
    app->cmd = NULL;
    app->stdout = (te_string)TE_STRING_INIT;
    app->stderr = (te_string)TE_STRING_INIT;

    if (options == NULL)
        tapi_perf_opts_init(&app->opts);
    else
    {
        app->opts = *options;
        if (options->host != NULL)
            app->opts.host = tapi_strdup(options->host);
    }
}

/*
 * Uninitialize perf application context.
 *
 * @param app           Perf tool context.
 */
static void
app_fini(tapi_perf_app *app)
{
    perf_app_close_descriptors(app);
    free(app->cmd);
    te_string_free(&app->stdout);
    te_string_free(&app->stderr);
    free(app->opts.host);
}


/* See description in tapi_performance.h */
void
tapi_perf_opts_init(tapi_perf_opts *opts)
{
    opts->host = NULL;
    opts->port = -1;
    opts->ipversion = RPC_IPPROTO_IP;
    opts->protocol = RPC_IPPROTO_UDP;
    opts->bandwidth_bits = -1;
    opts->num_bytes = -1;
    opts->duration_sec = 30;
    opts->length = 1470;
    opts->streams = 1;
    opts->reverse = FALSE;
}

/* See description in tapi_performance.h */
tapi_perf_server *
tapi_perf_server_create(tapi_perf_bench bench, const tapi_perf_opts *options)
{
    tapi_perf_server *server = NULL;

    ENTRY("Create perf server");

    if (bench != TAPI_PERF_IPERF && bench != TAPI_PERF_IPERF3)
        TEST_FAIL("Unsupported perf tool");

    server = tapi_calloc(1, sizeof(*server));
    app_init(&server->app, options);
    server->methods = NULL;

    switch (bench)
    {
        case TAPI_PERF_IPERF:
            iperf_server_init(server);
            break;

        case TAPI_PERF_IPERF3:
            iperf3_server_init(server);
            break;
    }

    return server;
}

/* See description in tapi_performance.h */
void
tapi_perf_server_destroy(tapi_perf_server *server)
{
    ENTRY("Destroy perf server");

    if (server == NULL)
        return;

    tapi_perf_server_stop(server);
    app_fini(&server->app);
    free(server);
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_server_start(tapi_perf_server *server, rcf_rpc_server *rpcs)
{
    ENTRY("Start perf server");

    if (server == NULL ||
        server->methods == NULL ||
        server->methods->start == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return server->methods->start(server, rpcs);
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_server_stop(tapi_perf_server *server)
{
    ENTRY("Stop perf server");

    if (server == NULL ||
        server->methods == NULL ||
        server->methods->stop == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return server->methods->stop(server);
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_server_get_report(tapi_perf_server *server, tapi_perf_report *report)
{
    ENTRY("Get perf server report");

    if (server == NULL ||
        server->methods == NULL ||
        server->methods->get_report == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return server->methods->get_report(server, report);
}


/* See description in tapi_performance.h */
tapi_perf_client *
tapi_perf_client_create(tapi_perf_bench bench, const tapi_perf_opts *options)
{
    tapi_perf_client *client = NULL;

    ENTRY("Create perf client");

    if (bench != TAPI_PERF_IPERF && bench != TAPI_PERF_IPERF3)
        TEST_FAIL("Unsupported perf tool");

    client = tapi_calloc(1, sizeof(*client));
    app_init(&client->app, options);
    client->methods = NULL;

    switch (bench)
    {
        case TAPI_PERF_IPERF:
            iperf_client_init(client);
            break;

        case TAPI_PERF_IPERF3:
            iperf3_client_init(client);
            break;
    }

    return client;
}

/* See description in tapi_performance.h */
void
tapi_perf_client_destroy(tapi_perf_client *client)
{
    ENTRY("Destroy perf client");

    if (client == NULL)
        return;

    tapi_perf_client_stop(client);
    app_fini(&client->app);
    free(client);
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_client_start(tapi_perf_client *client, rcf_rpc_server *rpcs)
{
    ENTRY("Start perf client");

    if (client == NULL ||
        client->methods == NULL ||
        client->methods->start == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return client->methods->start(client, rpcs);
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_client_stop(tapi_perf_client *client)
{
    ENTRY("Stop perf client");

    if (client == NULL ||
        client->methods == NULL ||
        client->methods->stop == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return client->methods->stop(client);
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_client_wait(tapi_perf_client *client, uint16_t timeout)
{
    ENTRY("Wait for perf client");

    if (client == NULL ||
        client->methods == NULL ||
        client->methods->wait == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return client->methods->wait(client, timeout);
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_client_get_report(tapi_perf_client *client, tapi_perf_report *report)
{
    ENTRY("Get perf client report");

    if (client == NULL ||
        client->methods == NULL ||
        client->methods->get_report == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return client->methods->get_report(client, report);
}

/* See description in tapi_performance.h */
const char *
tapi_perf_error2str(tapi_perf_error error)
{
    static const char *errors[] = {
        [TAPI_PERF_ERROR_FORMAT] = "wrong report format",
        [TAPI_PERF_ERROR_READ] = "read failed",
        [TAPI_PERF_ERROR_CONNECT] = "connect failed",
        [TAPI_PERF_ERROR_BIND] = "bind failed",
    };

    if (error >= TAPI_PERF_ERROR_MAX || errors[error] == NULL)
        TEST_FAIL("Unknown error code");

    return errors[error];
}

/* See description in tapi_performance.h */
const char *
tapi_perf_bench2str(tapi_perf_bench bench)
{
    static const char *names[] = {
        [TAPI_PERF_IPERF] = "iperf",
        [TAPI_PERF_IPERF3] = "iperf3"
    };

    if (bench >= TE_ARRAY_LEN(names))
        TEST_FAIL("Unknown tool");

    return names[bench];
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_server_check_report(tapi_perf_server *server,
                              tapi_perf_report *report,
                              const char *tag)
{
    return perf_app_check_report(&server->app, report, tag);
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_client_check_report(tapi_perf_client *client,
                              tapi_perf_report *report,
                              const char *tag)
{
    return perf_app_check_report(&client->app, report, tag);
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_server_get_check_report(tapi_perf_server *server,
                                  const char *tag,
                                  te_bool dump_enabled,
                                  tapi_perf_report *report)
{
    tapi_perf_report dummy_report;
    tapi_perf_report *work_report;
    te_errno rc_get;
    te_errno rc_check;

    work_report = (report != NULL ? report : &dummy_report);
    rc_get = tapi_perf_server_get_report(server, work_report);

    if (dump_enabled)
        perf_app_dump_output(&server->app, tag);

    rc_check = tapi_perf_server_check_report(server, work_report, tag);
    return (rc_get != 0 ? rc_get : rc_check);
}

/* See description in tapi_performance.h */
te_errno
tapi_perf_client_get_check_report(tapi_perf_client *client,
                                  const char *tag,
                                  te_bool dump_enabled,
                                  tapi_perf_report *report)
{
    tapi_perf_report dummy_report;
    tapi_perf_report *work_report;
    te_errno rc_get;
    te_errno rc_check;

    work_report = (report != NULL ? report : &dummy_report);
    rc_get = tapi_perf_client_get_report(client, work_report);

    if (dump_enabled)
        perf_app_dump_output(&client->app, tag);

    rc_check = tapi_perf_client_check_report(client, work_report, tag);
    return (rc_get != 0 ? rc_get : rc_check);
}
