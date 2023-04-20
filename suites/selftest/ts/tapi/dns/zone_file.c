/** @file
 * @brief TAPI Job test suite check zone file generation.
 *
 * TAPI Job test suite check zone file generation.
 *
 * Copyright (C) 2023 OKTET Labs Ltd., St.-Petersburg, Russia
 */

/** @page dns-dns_zone_file TAPI Job test suite check zone file generation.
 *
 * @objective TAPI Job test suite check zone file generation.
 *
 * @par Test sequence:
 *
 * @author Ivan Khodyrev <Ivan.Khodyrev@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME    "dns_zone_file"

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
#include "tapi_dns_zone_file.h"

#include "tapi_file.h"
#include "tapi_sockaddr.h"

#include <arpa/inet.h>

#define OPT_CLASS TAPI_DNS_ZONE_FILE_RRC_IN

#define OPT_SOA_SERIAL 1234
#define OPT_SOA_REFRESH 5
#define OPT_SOA_RETRY 6
#define OPT_SOA_EXPIRE 7
#define OPT_SOA_MINIMUM 8

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    tapi_job_factory_t *factory = NULL;

    const char *addr;
    const char *domain = NULL;
    const char *subdomain = NULL;
    const char *primary = NULL;
    const char *hostmaster = NULL;
    unsigned int ttl = 0;

    char *zone_file = NULL;
    tapi_dns_zone_file_rr resource[3];
    const char *expected_zone_file_data = NULL;
    char *recieved_zone_file_data = NULL;
    te_string primary_name_server = TE_STRING_INIT;
    te_string hostmaster_email = TE_STRING_INIT;
    struct sockaddr_in addr_sin;

    TEST_START;
    TEST_GET_PCO(pco_iut);
    TEST_GET_STRING_PARAM(domain);
    TEST_GET_STRING_PARAM(subdomain);
    TEST_GET_STRING_PARAM(primary);
    TEST_GET_STRING_PARAM(hostmaster);
    TEST_GET_UINT_PARAM(ttl);
    TEST_GET_STRING_PARAM(addr);
    TEST_GET_STRING_PARAM(expected_zone_file_data);

    TEST_STEP("Initialize factory");
    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory));

    TEST_STEP("Create zone file");
    addr_sin.sin_family = AF_INET;
    inet_aton(addr, &addr_sin.sin_addr);

    te_string_append(&primary_name_server, "%s.%s", primary, domain);
    te_string_append(&hostmaster_email, "%s.%s", hostmaster, domain);

    resource[0].owner = domain;
    resource[0].ttl = TAPI_JOB_OPT_UINT_VAL(ttl);
    resource[0].class = OPT_CLASS;
    resource[0].rdata.type = TAPI_DNS_ZONE_FILE_RRT_SOA;
    resource[0].rdata.u.soa.primary_name_server = primary_name_server.ptr;
    resource[0].rdata.u.soa.hostmaster_email = hostmaster_email.ptr;
    resource[0].rdata.u.soa.serial = OPT_SOA_SERIAL;
    resource[0].rdata.u.soa.refresh = OPT_SOA_REFRESH;
    resource[0].rdata.u.soa.retry = OPT_SOA_RETRY;
    resource[0].rdata.u.soa.expire = OPT_SOA_EXPIRE;
    resource[0].rdata.u.soa.minimum = OPT_SOA_MINIMUM;

    resource[1].owner = domain;
    resource[1].ttl = TAPI_JOB_OPT_UINT_VAL(ttl);
    resource[1].class = OPT_CLASS;
    resource[1].rdata.type = TAPI_DNS_ZONE_FILE_RRT_NS;
    resource[1].rdata.u.ns.nsdname = primary;

    resource[2].owner = subdomain;
    resource[2].ttl = TAPI_JOB_OPT_UINT_VAL(ttl);
    resource[2].class = OPT_CLASS;
    resource[2].rdata.type = TAPI_DNS_ZONE_FILE_RRT_A;
    resource[2].rdata.u.a = (tapi_dns_zone_file_rr_a){
                                .addr = (struct sockaddr *)&addr_sin
                            };

    tapi_dns_zone_file_create(pco_iut->ta, resource, 3, NULL, NULL, &zone_file);
    rc = tapi_file_read_ta(pco_iut->ta, zone_file, &recieved_zone_file_data);
    if (rc != 0)
        TEST_VERDICT("Failed to read zone file");

    TEST_STEP("Check zone file");
    if (!te_str_is_equal_nospace(recieved_zone_file_data,
                                 expected_zone_file_data))
    {
        TEST_VERDICT("Generaged zone file doesn't match, recieved: \n\"%s\"",
                     recieved_zone_file_data);
    }

    TEST_SUCCESS;

cleanup:
    tapi_job_factory_destroy(factory);
    te_string_free(&primary_name_server);
    te_string_free(&hostmaster_email);
    free(recieved_zone_file_data);
    CLEANUP_CHECK_RC(tapi_dns_zone_file_destroy(pco_iut->ta, zone_file));
    free(zone_file);

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
