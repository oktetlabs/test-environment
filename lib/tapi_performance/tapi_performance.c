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
#define TE_LOG_LEVEL    0x003f  /* FIXME remove after debugging */

#include "tapi_performance.h"
#include "tapi_mem.h"
#include "tapi_iperf3.h"
#include "tapi_rpcsock_macros.h"


/**
 * Reset perf application context.
 *
 * @param app           Perf tool context.
 */
static void
app_reset(tapi_perf_app *app)
{
    app->opts = NULL;
    app->rpcs = NULL;
    app->pid = -1;
    app->stdout = -1;
    app->stderr = -1;
    app->cmd = NULL;
    app->report = (te_string)TE_STRING_INIT;
    app->err = (te_string)TE_STRING_INIT;
}


/* See description in tapi_performance.h */
tapi_perf_server *
tapi_perf_server_create(tapi_perf_type type, const void *options)
{
    tapi_perf_server *server = NULL;
    te_errno rc;

    ENTRY("Create perf server");

    if (type != TAPI_PERF_IPERF3)
    {
        ERROR("Unsupported perf tool type");
        return NULL;
    }
    if (type != ((tapi_perf_opts *)options)->type)
    {
        ERROR("Incompatible options type");
        return NULL;
    }

    server = tapi_calloc(1, sizeof(*server));
    app_reset(&server->app);
    server->methods = NULL;
    tapi_iperf3_server_init(server, (const tapi_iperf3_options *)options);

    return server;
}

/* See description in tapi_performance.h */
void
tapi_perf_server_destroy(tapi_perf_server *server)
{
    ENTRY("Destroy perf server");

    if (server == NULL)
        return;

    tapi_iperf3_server_fini(server);
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
tapi_perf_client_create(tapi_perf_type type, const void *options)
{
    tapi_perf_client *client = NULL;
    te_errno rc;

    ENTRY("Create perf client");

    if (type != TAPI_PERF_IPERF3)
    {
        ERROR("Unsupported perf tool type");
        return NULL;
    }
    if (type != ((tapi_perf_opts *)options)->type)
    {
        ERROR("Incompatible options type");
        return NULL;
    }

    client = tapi_calloc(1, sizeof(*client));
    app_reset(&client->app);
    client->methods = NULL;
    tapi_iperf3_client_init(client, (const tapi_iperf3_options *)options);

    return client;
}

/* See description in tapi_performance.h */
void
tapi_perf_client_destroy(tapi_perf_client *client)
{
    ENTRY("Destroy perf client");

    if (client == NULL)
        return;

    tapi_iperf3_client_fini(client);
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
