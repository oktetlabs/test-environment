/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI network performance test
 *
 * Demonstrate the usage of TAPI perf tool.
 */

#define TE_TEST_NAME "perf_simple"

#include "perf.h"
#include "tapi_performance.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_sockaddr.h"
#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_cfg.h"

#include "te_file.h"
#include "te_units.h"

#define PROTO_TYPE_MAPPING_LIST \
    { "TCP", (int)RPC_IPPROTO_TCP }, \
    { "UDP", (int)RPC_IPPROTO_UDP }

#define TEST_GET_PROTO_TYPE_PARAM(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, PROTO_TYPE_MAPPING_LIST)

#define BANDWIDTH_MAX_MBITS 1000

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut;
    rcf_rpc_server *pco_tst;
    const struct sockaddr *iut_addr;
    const struct sockaddr *tst_addr;

    unsigned int duration_s = 0;
    unsigned int proto;
    unsigned int tool;
    unsigned int stream_n = 0;

    tapi_perf_server *perf_server = NULL;
    tapi_perf_client *perf_client = NULL;
    tapi_perf_opts perf_opts;
    tapi_perf_report perf_server_report;
    tapi_perf_report perf_client_report;
    tapi_job_factory_t *client_factory = NULL;
    tapi_job_factory_t *server_factory = NULL;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_tst);
    TEST_GET_ADDR(pco_iut, iut_addr);
    TEST_GET_ADDR(pco_tst, tst_addr);

    TEST_GET_UINT_PARAM(duration_s);
    TEST_GET_PROTO_TYPE_PARAM(proto);
    TEST_GET_PERF_BENCH(tool);
    TEST_GET_UINT_PARAM(stream_n);

    tapi_perf_opts_init(&perf_opts);

    perf_opts.host = strdup(te_sockaddr_netaddr_to_string(AF_INET,
                                te_sockaddr_get_netaddr(iut_addr)));
    perf_opts.protocol = proto;
    perf_opts.port = te_sockaddr_get_port(iut_addr);
    perf_opts.streams = stream_n;
    perf_opts.bandwidth_bits = TE_UNITS_DEC_M2U(BANDWIDTH_MAX_MBITS) /
                               perf_opts.streams;
    perf_opts.duration_sec = duration_s;
    perf_opts.interval_sec = perf_opts.duration_sec;

    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &server_factory));
    CHECK_RC(tapi_job_factory_rpc_create(pco_tst, &client_factory));

    perf_server = tapi_perf_server_create(tool, &perf_opts, server_factory);
    perf_client = tapi_perf_client_create(tool, &perf_opts, client_factory);

    CHECK_RC(tapi_perf_server_start(perf_server));
    CHECK_RC(tapi_perf_client_start(perf_client));
    CHECK_RC(tapi_perf_client_wait(perf_client, TAPI_PERF_TIMEOUT_DEFAULT));

    VSLEEP(1, "ensure perf server has printed its report");

    CHECK_RC(tapi_perf_client_get_dump_check_report(perf_client, "client",
                                                    &perf_client_report));
    CHECK_RC(tapi_perf_server_get_dump_check_report(perf_server, "server",
                                                    &perf_server_report));

    TEST_SUCCESS;

cleanup:
    tapi_perf_client_destroy(perf_client);
    tapi_perf_server_destroy(perf_server);
    tapi_job_factory_destroy(client_factory);
    tapi_job_factory_destroy(server_factory);

    TEST_END;
}
