/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Simple IPv4 CSAP test
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME    "ipstack/ip4_recv_send"

#define TE_LOG_LEVEL 0xff

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

#include "conf_api.h"
#include "tapi_tad.h"
#include "tapi_eth.h"

#include "tapi_test.h"
#include "tapi_ip4.h"
#include "tapi_udp.h"
#include "tapi_tcp.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"


int
main(int argc, char *argv[])
{
    int  syms;
    int  sid_a;
    int  sid_b;
    char ta[32];
    char *agt_a = ta;
    char *agt_b;
    size_t  len = sizeof(ta);

    te_bool enum_iterator;

    csap_handle_t ip4_send_csap = CSAP_INVALID_HANDLE;
    csap_handle_t ip4_listen_csap = CSAP_INVALID_HANDLE;
    csap_handle_t eth_listen_csap_1 = CSAP_INVALID_HANDLE;
    csap_handle_t eth_listen_csap_2 = CSAP_INVALID_HANDLE;

    asn_value *template;/*  iteration template for traffic generation */
    asn_value *eth_pattern;

    int num_pkts;
    int pld_len;
    int delay;

    uint8_t tst_proto = 0;


    TEST_START;

    TEST_GET_INT_PARAM(num_pkts);
    TEST_GET_INT_PARAM(pld_len);
    TEST_GET_BOOL_PARAM(enum_iterator);

    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed: %r", rc);

    INFO("Found first TA: %s; len %d", ta, len);

    agt_a = ta;
    if (strlen(ta) + 1 >= len)
        TEST_FAIL("There is no second Test Agent");

    agt_b = ta + strlen(ta) + 1;

    INFO("Found second TA: %s", agt_b, len);

    if (rcf_ta_create_session(agt_a, &sid_a) != 0)
    {
        TEST_FAIL("rcf_ta_create_session failed");
    }
    INFO("Test: Created session for A agt: %d", sid_a);

    if (rcf_ta_create_session(agt_b, &sid_b) != 0)
    {
        TEST_FAIL("rcf_ta_create_session failed");
    }
    INFO("Test: Created session for B: %d", sid_b);

    rc = asn_parse_value_text("{{ pdus {  eth:{}} }}",
                              ndn_traffic_pattern,
                              &eth_pattern, &syms);
    if (rc != 0)
        TEST_FAIL("parse of pattern failed %X, syms %d", rc, syms);

    /* Fill in value for iteration */
    if (enum_iterator)
    {
        rc = asn_parse_value_text("{ arg-sets { ints:{}, ints-assoc:{} }, "
                                  "  pdus     { ip4:{}, eth:{}} }",
                                  ndn_traffic_template,
                                  &template, &syms);
    }
    else
    {
        rc = asn_parse_value_text("{ arg-sets { simple-for:{begin 1} }, "
                                  "  pdus     { ip4:{}, eth:{}} }",
                                  ndn_traffic_template,
                                  &template, &syms);
    }
    if (rc != 0)
        TEST_FAIL("parse of template failed %X, syms %d", rc, syms);

    if (enum_iterator)
    {
        int j;
        for (j = 0; j < num_pkts && rc == 0; j++)
        {
            asn_value *int_val = asn_init_value(asn_base_integer);

            asn_write_int32(int_val, j * 2 + 10, "");
            rc = asn_insert_indexed(template,
                                    asn_copy_value(int_val), -1,
                                    "arg-sets.0.#ints");

            asn_write_int32(int_val, j * 2 + 41, "");
            rc = asn_insert_indexed(template,
                                    asn_copy_value(int_val), -1,
                                    "arg-sets.1.#ints-assoc");
        }
        if (rc != 0)
            TEST_FAIL("write enum failed %X", rc);
    }
    else
    {
        rc = asn_write_value_field(template, &num_pkts, sizeof(num_pkts),
                                   "arg-sets.0.#simple-for.end");
        if (rc != 0)
            TEST_FAIL("write num_pkts failed %X", rc);
    }
    asn_save_to_file(template, "/tmp/traffic_template.asn");

    rc = asn_write_value_field(template, &pld_len, sizeof(pld_len),
                               "payload.#length");
    if (rc != 0)
        TEST_FAIL("write payload len failed %X", rc);

    delay = 100;
    rc = asn_write_value_field(template, &delay, sizeof(delay),
                               "delays.#plain");
    if (rc != 0)
        TEST_FAIL("write delay failed %X", rc);

    do {
        uint8_t mac_a[6] = {
            0x01,0x02,0x03,0x04,0x05,0x06};
        uint8_t mac_b[6] = {
            0x16,0x15,0x14,0x13,0x12,0x11};
        in_addr_t ip_a = inet_addr("192.168.123.231");
        in_addr_t ip_b = inet_addr("192.168.123.232");

        int num;

        rc = tapi_ip4_eth_csap_create(agt_a, sid_a, "eth0",
                                      TAD_ETH_RECV_DEF &
                                      ~TAD_ETH_RECV_OTHER,
                                      mac_a, mac_b,
                                      ip_a, ip_b, tst_proto,
                                      &ip4_send_csap);
        if (rc != 0)
            TEST_FAIL("CSAP create failed, rc from module %d is %r\n",
                        TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc));

        rc = tapi_ip4_eth_csap_create(agt_b, sid_b, "eth0",
                                      TAD_ETH_RECV_DEF,
                                      mac_b, mac_a,
                                      ip_b, ip_a, tst_proto,
                                      &ip4_listen_csap);
        if (rc != 0)
            TEST_FAIL("CSAP create failed, rc from mod %d is %r\n",
                        TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc));

        rc = tapi_eth_csap_create(agt_b, sid_b, "eth0",
                                  TAD_ETH_RECV_DEF,
                                  mac_a, mac_b,
                                  NULL, &eth_listen_csap_1);
        if (rc != 0)
            TEST_FAIL("ETH CSAP create failed, rc from mod %d is %r\n",
                        TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc));

        rc = tapi_eth_csap_create(agt_b, sid_b, "eth0",
                                  TAD_ETH_RECV_DEF,
                                  mac_a, mac_b,
                                  NULL, &eth_listen_csap_2);
        if (rc != 0)
            TEST_FAIL("ETH CSAP create failed, rc from mod %d is %r\n",
                        TE_RC_GET_MODULE(rc), TE_RC_GET_ERROR(rc));

        rc = tapi_tad_trrecv_start(agt_b, sid_b, ip4_listen_csap, NULL,
                                   5000, num_pkts, RCF_TRRECV_COUNT);
        if (rc != 0)
            TEST_FAIL("recv start failed %X", rc);

        rc = tapi_tad_trrecv_start(agt_b, sid_b, eth_listen_csap_2,
                                   eth_pattern, 5000, num_pkts,
                                   RCF_TRRECV_COUNT);

        rc = tapi_tad_trrecv_start(agt_b, sid_b, eth_listen_csap_1,
                                   eth_pattern, 5000, num_pkts,
                                   RCF_TRRECV_COUNT);
        if (rc != 0)
            TEST_FAIL("Eth recv start failed %X", rc);

        rc = tapi_tad_trsend_start(agt_a, sid_a, ip4_send_csap,
                                   template, RCF_MODE_NONBLOCKING);
        if (rc != 0)
            TEST_FAIL("send start failed %X", rc);

        INFO ("try to wait\n");
        rc = rcf_ta_trrecv_wait(agt_b, sid_b, ip4_listen_csap,
                                NULL, NULL, &num);
        RING("trrecv_wait: %r num: %d\n", rc, num);

        rc = rcf_ta_trrecv_stop(agt_b, sid_b, eth_listen_csap_2,
                                NULL, NULL, &num);
        RING("Eth trrecv_stop: %r num: %d\n", rc, num);

        rc = rcf_ta_trrecv_stop(agt_b, sid_b, eth_listen_csap_1,
                                NULL, NULL, &num);
        RING("Eth trrecv_stop: %r num: %d\n", rc, num);

    } while(0);

    if (rc)
        TEST_FAIL("Failed, rc %X", rc);

    TEST_SUCCESS;

cleanup:
#define CLEANUP_CSAP(ta_, sid_, csap_) \
    do {                                                                \
        csap_handle_t csap_tmp = csap_;                                 \
        csap_ = CSAP_INVALID_HANDLE;                                    \
                                                                        \
        if (csap_tmp != CSAP_INVALID_HANDLE &&                          \
            (rc = rcf_ta_csap_destroy(ta_, sid_, csap_tmp)) != 0)       \
            TEST_FAIL("CSAP destroy %d on agt %s failure %X",           \
                      csap_tmp, ta_, rc);                               \
    } while (0)                                                         \

    if (ip4_send_csap != CSAP_INVALID_HANDLE)
        rcf_ta_trsend_stop(agt_a, sid_a, ip4_send_csap, NULL);

    CLEANUP_CSAP(agt_a, sid_a, ip4_send_csap);
    CLEANUP_CSAP(agt_b, sid_b, ip4_listen_csap);
    CLEANUP_CSAP(agt_b, sid_b, eth_listen_csap_1);
    CLEANUP_CSAP(agt_b, sid_b, eth_listen_csap_2);

    TEST_END;
}
