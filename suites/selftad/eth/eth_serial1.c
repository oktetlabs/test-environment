/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Test Package: POE Switch Management Interface
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page serial test
 *
 * @objective This test verifies for creation ethernet frame flow
 *            incapsulating UDP payload. The payload length can be managed
 *            from test.
 *
 */

#define TE_TEST_NAME    "eth/serial_udp_pld"

#define TE_LOG_LEVEL 255

#include "te_config.h"
#include <linux/if_ether.h>

#include "tapi_test.h"
#include "logger_api.h"
#include "conf_api.h"
#include "rcf_api.h"
#include "ndn_eth.h"
#include "tapi_eth.h"
#include "tapi_tad.h"


#define PAYLOAD_CREATION_METHOD        "eth_udp_payload"


/* The number of packets to be processed */
#define PKTS_TO_PROCESS                2000

/*NOTE: internal buffer length allocated 20000 bytes */
#define PAYLOAD_LENGTH                 1460

/*IP addresses for packet creation */
#define SRC_IP     "192.168.200.10"
#define DST_IP     "192.168.220.10"

/* UDP ports for packet creation */
#define SRC_PORT   9000
#define DST_PORT   9001



/* Source MAC addresses are used in tests */
#define SRC1_MAC  "20:03:20:04:14:30"

/* Destination MAC addresses are used in tests */
#define DST1_MAC  "20:03:20:06:24:41"



static int
mi_set_agent_params(char *agent, int sid, int payload_length,
                    const char *src_addr, const char *dst_addr,
                    unsigned short src_port, unsigned short dst_port)
{
    int rc;
    rc = rcf_ta_set_var(agent, sid, "mi_payload_length", RCF_INT32,
                        (const char *)&payload_length);
    if (rc == 0)
        rc = rcf_ta_set_var(agent, sid, "mi_src_addr_human",
                            RCF_STRING, src_addr);
    if (rc == 0)
        rc = rcf_ta_set_var(agent, sid, "mi_dst_addr_human",
                            RCF_STRING, dst_addr);
    if (rc == 0)
        rc = rcf_ta_set_var(agent, sid, "mi_src_port",
                            RCF_UINT16, (const char *)&src_port);
    if (rc == 0)
        rc = rcf_ta_set_var(agent, sid, "mi_dst_port",
                            RCF_UINT16, (const char *)&dst_port);
   return rc;
}

int
main(int argc, char *argv[])
{
    tad_csap_status_t status;         /* Status of TX CSAP */

    int   syms = 4;
    int   recv_pkts;  /* the number of waned/received packets */
    int   num_pkts = PKTS_TO_PROCESS; /* number of packets
                                         to be generated */
    char  agent_a[100];  /* first linux agent name */
    char *agent_b;       /* second linux agent name */

    char *agent_a_if;    /* first agent interface name */
    int   sid_a;         /* session id for first agent */
    char *agent_b_if;    /* second agent interface name */
    int   sid_b;         /* session id for second session */

    csap_handle_t    tx_csap = CSAP_INVALID_HANDLE;
    csap_handle_t    rx_csap = CSAP_INVALID_HANDLE;

    char       *src_mac = SRC1_MAC;
    char       *dst_mac = DST1_MAC;

    struct timeval duration;

    uint8_t    src_bin_mac[ETHER_ADDR_LEN];
    uint8_t    dst_bin_mac[ETHER_ADDR_LEN];

    uint16_t   eth_type = ETH_P_IP;
    size_t     pld_len = PAYLOAD_LENGTH;

    asn_value *csap_spec = NULL;
    asn_value *pattern; /* eth frame pattern  for filtering */
    asn_value *template;/* eth frame template for traffic generation */

    size_t len = 100;

    int prev = 0, num = 0;
    int i;

    TEST_START;

    if ((rc = rcf_get_ta_list(agent_a, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed %x", rc);
    VERB(" Using agent: %s\n", agent_a);

    agent_b = agent_a + strlen(agent_a) + 1;

    if (agent_b - agent_a > (int)len)
        TEST_FAIL("Second TA not found, at least two agents required");

    agent_b_if = agent_a_if = strdup("eth0");

    if (rcf_ta_create_session(agent_a, &sid_a))
        TEST_FAIL("first session creation error");

    if (rcf_ta_create_session(agent_b, &sid_b))
        TEST_FAIL(" second session creation error");

    memcpy (dst_bin_mac, ether_aton(dst_mac), ETHER_ADDR_LEN);
    memcpy (src_bin_mac, ether_aton(src_mac), ETHER_ADDR_LEN);

    if ((rc = tapi_eth_csap_create(agent_a, sid_a, agent_a_if,
                                   TAD_ETH_RECV_DEF & ~TAD_ETH_RECV_OTHER,
                                   dst_bin_mac, src_bin_mac,
                                   &eth_type, &tx_csap)) != 0)
    {
        TEST_FAIL("TX CSAP creation failure: %r", rc);
    }

    CHECK_RC(tapi_eth_add_csap_layer(&csap_spec, agent_b_if,
                                     TAD_ETH_RECV_ALL,
                                     src_bin_mac, dst_bin_mac,
                                     &eth_type,
                                     TE_BOOL3_ANY /* tagged/untagged */,
                                     TE_BOOL3_ANY /* Ethernet2/LLC */));
    if ((rc = tapi_tad_csap_create(agent_b, sid_b, "eth", csap_spec,
                                   &rx_csap)) != 0)
    {
        TEST_FAIL(" RX CSAP creation failure");
    }
    asn_free_value(csap_spec); csap_spec = NULL;

    /* Set AGENT side function parameters  */
    if ((rc = mi_set_agent_params(agent_a, sid_a, PAYLOAD_LENGTH,
                                  SRC_IP, DST_IP,
                                  SRC_PORT, DST_PORT)) != 0)
    {
        TEST_FAIL(" AGENT side parameters setting up failure");
    }


    /* Fill in value for iteration */
    rc = asn_parse_value_text("{ arg-sets { simple-for:{begin 1} }, "
                              "  pdus     {} }",
                              ndn_traffic_template,
                              &template, &syms);
    if (rc == 0)
        rc = asn_write_value_field(template, &num_pkts, sizeof(num_pkts),
                                   "arg-sets.0.#simple-for.end");

    /* Fill in method creating ethernet frame with UDP payload */
    if (rc == 0)
#if 0
        rc = asn_write_value_field (template, PAYLOAD_CREATION_METHOD,
                                    strlen(PAYLOAD_CREATION_METHOD),
                                    "payload.#function");
#else
        rc = asn_write_value_field (template, &pld_len, sizeof(pld_len),
                                    "payload.#length");
#endif
    if (rc)
    {
        TEST_FAIL(" traffic template creation error %x, sym %d",
                rc, syms);
    }

    /* Create pattern for recv csap filtering */
    rc = asn_parse_value_text("{{ pdus { eth:{ }}}}",
                              ndn_traffic_pattern, &pattern, &syms);
    if (rc == 0)
        rc = asn_write_value_field(pattern,
                                   (unsigned char *)ether_aton(dst_mac),
                                   ETHER_ADDR_LEN,
                                   "0.pdus.0.#eth.dst-addr.#plain");
    if (rc)
    {
        TEST_FAIL(" pattern creation error %x", rc);
    }


#if 0
    /* Start recieving process */
    recv_pkts = PKTS_TO_PROCESS;
#else
    recv_pkts = 0;
#endif
    rc = tapi_tad_trrecv_start(agent_b, sid_b, rx_csap, pattern,
                               TAD_TIMEOUT_INF, recv_pkts,
                               RCF_TRRECV_COUNT);
    if (rc)
    {
        TEST_FAIL(" recieving process error %x", rc);
    }


    /* Start sending process */
    rc = tapi_tad_trsend_start(agent_a, sid_a, tx_csap, template,
                               RCF_MODE_NONBLOCKING);
    if (rc)
    {
        TEST_FAIL(" transmitting process error %x", rc);
    }

    do {
        sleep(1);
        VERB("before get status\n");
        rc = tapi_csap_get_status(agent_a, sid_a, tx_csap, &status);
        if (rc)
        {
            TEST_FAIL("v1: port A. TX CSAP get status error %x",
                             rc);
        }
        VERB("get status: %d\n", status);
    } while(status == CSAP_BUSY);

    if (status == CSAP_ERROR)
    {
        rc = rcf_ta_trsend_stop(agent_a, sid_a, tx_csap, &num);
        if (rc)
            TEST_FAIL("v1: send stop return error %x", rc);
    }

    rc = tapi_csap_get_status(agent_b, sid_b, rx_csap, &status);
    i = 0;
    while (i < 3 && status == CSAP_BUSY)
    {
        prev = num;
        if (rc != 0)
        {
            TEST_FAIL("v1: port A. RX CSAP get status error %x",
                             rc);
        }
        rc = rcf_ta_trrecv_get(agent_b, sid_b, rx_csap, NULL, NULL, &num);
        sleep(1);
        if (rc != 0)
        {
            TEST_FAIL("v1: port A. RX CSAP get traffic error %x",
                             rc);
        }

        if (prev == num || status != CSAP_BUSY)
            break;

        sleep(1);
        i++;
        rc = tapi_csap_get_status(agent_b, sid_b, rx_csap, &status);
        if (rc != 0)
            TEST_FAIL("get status of CSAP %s:%d fails %r",
                      agent_b, rx_csap, rc);
    }

    /* Stop recieving process */
    rc = rcf_ta_trrecv_stop(agent_b, sid_b, rx_csap, NULL, NULL,
                            &recv_pkts);
    if (rc)
    {
        TEST_FAIL(" receiving process shutdown error %x", rc);
    }


    rc = tapi_csap_get_duration(agent_b, sid_b, rx_csap, &duration);
    VERB("rx_duration: rc %x sec %d, usec %d\n",
         rc, duration.tv_sec, duration.tv_usec);

    rc = tapi_csap_get_duration(agent_a, sid_a, tx_csap, &duration);
    VERB("tx_duration: rc %x sec %d, usec %d\n",
         rc, duration.tv_sec, duration.tv_usec);


    if (recv_pkts != PKTS_TO_PROCESS)
        TEST_FAIL("some frames from flow are lost; got %d, should %d",
                  recv_pkts, PKTS_TO_PROCESS);


    RING("TEST PASS: recv_pkts:%d", recv_pkts);

    TEST_SUCCESS;

cleanup:

    if (tx_csap != CSAP_INVALID_HANDLE &&
        (rc = rcf_ta_csap_destroy(agent_a, sid_a, tx_csap) != 0) )
        ERROR("ETH listen csap destroy fails, rc %X", rc);

    if (rx_csap != CSAP_INVALID_HANDLE &&
        (rc = rcf_ta_csap_destroy(agent_b, sid_b, rx_csap) != 0) )
        ERROR("ETH listen csap destroy fails, rc %X", rc);

    asn_free_value(csap_spec); csap_spec = NULL;

    TEST_END;

}
