/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI Job test suite starts Unbound DNS server by RPC.
 *
 * TAPI Job test suite starts Unbound DNS server by RPC.
 */

/** @page dns-dns_unbound TAPI Job test suite starts Unbound DNS server by RPC.
 *
 * @objective TAPI Job test suite starts Unbound DNS server by RPC.
 *
 * @par Test sequence:
 *
 * @author Ivan Khodyrev <Ivan.Khodyrev@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME    "dns_unbound"

#ifndef TEST_START_VARS
/**
 * Test suite specific variables of the test @b main() function.
 */
#define TEST_START_VARS TEST_START_ENV_VARS
#endif

#ifndef TEST_START_SPECIFIC
/**
 * Test suite specific the first actions of the test.
 */
#define TEST_START_SPECIFIC TEST_START_ENV
#endif

#ifndef TEST_END_SPECIFIC
/**
 * Test suite specific part of the last action of the test @b main() function.
 */
#define TEST_END_SPECIFIC TEST_END_ENV
#endif

#include "te_config.h"
#include "tapi_test.h"
#include "tapi_job.h"
#include "tapi_job_factory_rpc.h"
#include "rcf_rpc.h"
#include "tapi_rpc_unistd.h"
#include "tapi_env.h"
#include "tapi_dns_unbound.h"
#include "tapi_cfg_base.h"
#include "tapi_dns_zone_file.h"

#include "tapi_file.h"
#include "tapi_sockaddr.h"

#include <arpa/inet.h>

#define VERBOSE_LEVEL_MAPPING_LIST \
    { "NOT_VERBOSE", (int)TAPI_DNS_UNBOUND_NOT_VERBOSE },  \
    { "VERBOSE", (int)TAPI_DNS_UNBOUND_VERBOSE },          \
    { "MORE_VERBOSE", (int)TAPI_DNS_UNBOUND_MORE_VERBOSE }

#define TEST_GET_VERBOSE_LEVEL_PARAM(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, VERBOSE_LEVEL_MAPPING_LIST)

#define OPT_CHROOT ""
#define OPT_WORKDIR ""
#define OPT_USERNAME ""
#define OPT_ADDR "127.0.0.1"
#define OPT_OUTSIDE_ADDR "10.0.0.1"
#define OPT_PRIVATE_DOMAIN "private_domain."
#define OPT_SUBNET_PREFIX_LEN 8
#define OPT_NUM_THREADS 4
#define OPT_NUM_QUERIES_PER_THREAD 10
#define OPT_JOSTLE_TIMEOUT 10
#define OPT_INCOMING_NUM_TCP 4
#define OPT_OUTGOING_NUM_TCP 4
#define OPT_CACHE_MAX_TTL 2
#define OPT_CACHE_MIN_TTL 1
#define OPT_SO_RCVBUF 1024
#define OPT_SO_SNDBUF 1024

#define OPT_AUTH_ZONE_TTL 40

#define START_DNS_UNBOUND_TIMEOUT TE_SEC2MS(1)

static void
log_file(const char *ta, const char *filename)
{
    char *buf = NULL;
    te_errno rc;

    rc = tapi_file_read_ta(ta, filename, &buf);
    if (rc != 0)
        return;

    RING("%s", buf);
    free(buf);
}

static char *
create_zone_file_example(const char *ta)
{
    tapi_dns_zone_file_rr resource[4];
    struct sockaddr_in addr;
    char *res_path;

    addr.sin_family = AF_INET;
    inet_aton(OPT_ADDR, &addr.sin_addr);

    resource[0].owner = "example.";
    resource[0].ttl = TAPI_JOB_OPT_UINT_VAL(OPT_AUTH_ZONE_TTL);
    resource[0].class = TAPI_DNS_ZONE_FILE_RRC_IN;
    resource[0].rdata.type = TAPI_DNS_ZONE_FILE_RRT_SOA;
    resource[0].rdata.u.soa.primary_name_server = "ns.example.";
    resource[0].rdata.u.soa.hostmaster_email = "hostmaster.example.";
    resource[0].rdata.u.soa.serial = 20230530;
    resource[0].rdata.u.soa.refresh = 5;
    resource[0].rdata.u.soa.retry = 6;
    resource[0].rdata.u.soa.expire = 7;
    resource[0].rdata.u.soa.minimum = 8;

    resource[1].owner = "example.";
    resource[1].ttl = TAPI_JOB_OPT_UINT_VAL(OPT_AUTH_ZONE_TTL);
    resource[1].class = TAPI_DNS_ZONE_FILE_RRC_IN;
    resource[1].rdata.type = TAPI_DNS_ZONE_FILE_RRT_NS;
    resource[1].rdata.u.ns.nsdname = "ns";

    resource[2].owner = "ns";
    resource[2].ttl = TAPI_JOB_OPT_UINT_VAL(OPT_AUTH_ZONE_TTL);
    resource[2].class = TAPI_DNS_ZONE_FILE_RRC_IN;
    resource[2].rdata.type = TAPI_DNS_ZONE_FILE_RRT_A;
    resource[2].rdata.u.a = (tapi_dns_zone_file_rr_a){
                                .addr = (struct sockaddr *)&addr
                            };

    resource[3].owner = "www";
    resource[3].ttl = TAPI_JOB_OPT_UINT_VAL(OPT_AUTH_ZONE_TTL);
    resource[3].class = TAPI_DNS_ZONE_FILE_RRC_IN;
    resource[3].rdata.type = TAPI_DNS_ZONE_FILE_RRT_A;
    resource[3].rdata.u.a = (tapi_dns_zone_file_rr_a){
                                .addr = (struct sockaddr *)&addr
                            };

    tapi_dns_zone_file_create(ta, resource, 4, NULL, NULL, &res_path);

    return res_path;
}

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    const struct if_nameindex  *if_a = NULL;
    tapi_job_factory_t *factory = NULL;
    char *ta_dir = NULL;

    tapi_dns_unbound_app *app = NULL;
    tapi_dns_unbound_opt opts = tapi_dns_unbound_default_opt;

    tapi_dns_unbound_cfg_opt cfg_opts = tapi_dns_unbound_cfg_default_opt;
    const struct sockaddr *iut_addr;
    struct sockaddr_in outside_addr;
    unsigned int port;
    tapi_dns_unbound_verbose verbosity;
    const struct sockaddr *addresses[1];
    tapi_dns_unbound_cfg_address address[2];
    tapi_dns_unbound_cfg_auth_zone auth_zone[2];
    tapi_dns_unbound_cfg_ac access_control;
    char *includes[2];
    te_string include_path = TE_STRING_INIT;
    size_t i = 0;

    char *zone_file = NULL;
    const char *auth_zone_name;
    const char *auth_zone_url;

    TEST_START;
    TEST_GET_PCO(pco_iut);
    TEST_GET_IF(if_a);
    TEST_GET_VERBOSE_LEVEL_PARAM(verbosity);
    TEST_GET_ADDR(pco_iut, iut_addr);
    TEST_GET_STRING_PARAM(auth_zone_name);
    TEST_GET_STRING_PARAM(auth_zone_url);

    port = SIN(iut_addr)->sin_port;

    TEST_STEP("Initialize factory");
    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory));
    ta_dir = tapi_cfg_base_get_ta_dir(pco_iut->ta, TAPI_CFG_BASE_TA_DIR_TMP);

    outside_addr.sin_family = AF_INET;
    inet_aton(OPT_OUTSIDE_ADDR, &outside_addr.sin_addr);

    TEST_STEP("Create zone file");
    zone_file = create_zone_file_example(pco_iut->ta);
    log_file(pco_iut->ta, zone_file);

    TEST_STEP("Create unbound app");
    auth_zone[0] = (tapi_dns_unbound_cfg_auth_zone){
                       .name = auth_zone_name,
                       .primaries.n = 0,
                       .primary_urls.n = 0,
                       .zonefile = zone_file,
                   };
    auth_zone[1] = (tapi_dns_unbound_cfg_auth_zone){
                       .name = auth_zone_name,
                       .primaries.n = 0,
                       .primary_urls.url = &auth_zone_url,
                       .primary_urls.n = 1,
                       .zonefile = NULL,
                   };
    cfg_opts.auth_zones.zone = auth_zone;
    cfg_opts.auth_zones.n = 2;

    /* Set interfaces for example. */
    address[0] = (tapi_dns_unbound_cfg_address){ .addr = if_a->if_name };
    address[1] = (tapi_dns_unbound_cfg_address){
                     .addr = OPT_ADDR,
                     .port = TAPI_JOB_OPT_UINT_VAL(port),
                 };
    cfg_opts.interfaces.addr = address;
    cfg_opts.interfaces.n = 2;

    /* Set access control rules for example. */
    access_control = (tapi_dns_unbound_cfg_ac){
                         .action = TAPI_DNS_UNBOUND_CFG_AC_ALLOW,
                         .subnet.addr = (struct sockaddr *)&outside_addr,
                         .subnet.prefix_len = OPT_SUBNET_PREFIX_LEN,
                     };
    cfg_opts.access_controls.rule = &access_control;
    cfg_opts.access_controls.n = 1;

    /* Set empty includes for example. */
    te_string_append(&include_path, "%s/include.XXXXXX", ta_dir);
    rpc_mkstemp(pco_iut, include_path.ptr, &includes[0]);
    rpc_mkstemp(pco_iut, include_path.ptr, &includes[1]);
    te_string_free(&include_path);
    cfg_opts.includes.filename = (const char **)includes;
    cfg_opts.includes.n = TE_ARRAY_LEN(includes);

    /* Set outgoing interface and private address for example. */
    addresses[0] = (struct sockaddr *)&outside_addr;
    cfg_opts.outgoing_interfaces.addr = addresses;
    cfg_opts.outgoing_interfaces.n = 1;
    cfg_opts.private_addresses.addr = &(te_sockaddr_subnet){
        .addr = (struct sockaddr *)&outside_addr,
        .prefix_len = OPT_SUBNET_PREFIX_LEN,
    };
    cfg_opts.private_addresses.n = 1;

    /* Set the remaining options for example. */
    cfg_opts.chroot = OPT_CHROOT;
    cfg_opts.directory = OPT_WORKDIR;
    cfg_opts.username = OPT_USERNAME;
    cfg_opts.private_domain = OPT_PRIVATE_DOMAIN;
    cfg_opts.port = TAPI_JOB_OPT_UINT_VAL(port);
    cfg_opts.num_threads = TAPI_JOB_OPT_UINT_VAL(OPT_NUM_THREADS);
    cfg_opts.num_queries_per_thread =
        TAPI_JOB_OPT_UINT_VAL(OPT_NUM_QUERIES_PER_THREAD);
    cfg_opts.jostle_timeout = TAPI_JOB_OPT_UINT_VAL(OPT_JOSTLE_TIMEOUT);
    cfg_opts.incoming_num_tcp = TAPI_JOB_OPT_UINT_VAL(OPT_INCOMING_NUM_TCP);
    cfg_opts.outgoing_num_tcp = TAPI_JOB_OPT_UINT_VAL(OPT_OUTGOING_NUM_TCP);
    cfg_opts.cache_max_ttl = TAPI_JOB_OPT_UINT_VAL(OPT_CACHE_MAX_TTL);
    cfg_opts.cache_min_ttl = TAPI_JOB_OPT_UINT_VAL(OPT_CACHE_MIN_TTL);
    cfg_opts.so_rcvbuf = TAPI_JOB_OPT_UINT_VAL(OPT_SO_RCVBUF);
    cfg_opts.so_sndbuf = TAPI_JOB_OPT_UINT_VAL(OPT_SO_SNDBUF);
    cfg_opts.verbosity = verbosity;

    /* Set program arguments. */
    opts.verbose = TAPI_DNS_UNBOUND_NOT_VERBOSE;
    opts.cfg_file = NULL;
    opts.cfg_opt = &cfg_opts;
    CHECK_RC(tapi_dns_unbound_create(factory, &opts, &app));
    log_file(pco_iut->ta, app->generated_cfg_file);

    TEST_STEP("Start unbound app");
    rc = tapi_dns_unbound_start(app);
    if (rc != 0)
        TEST_VERDICT("Failed to start unbound server");

    TEST_STEP("Wait for the process to run");
    rc = tapi_dns_unbound_wait(app, START_DNS_UNBOUND_TIMEOUT);
    if (rc != TE_EINPROGRESS)
        TEST_VERDICT("Unbound DNS server crushed");

    rc = tapi_dns_unbound_kill(app, SIGKILL);
    if (rc != 0)
        TEST_VERDICT("Failed to stop unbound server");

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_dns_unbound_destroy(app));
    CLEANUP_CHECK_RC(tapi_dns_zone_file_destroy(pco_iut->ta, zone_file));
    tapi_job_factory_destroy(factory);
    free(ta_dir);
    for (i = 0; i  < TE_ARRAY_LEN(includes); i++)
        free(includes[i]);
    free(zone_file);

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
