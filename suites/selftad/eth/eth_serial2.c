/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Test Package: POE Switch Management Interface
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page serial test
 *
 * @objective This test verifies for creation ethernet frame flow
 *              using 'length' payload specificator.
 *
 */

#define TE_LOG_LEVEL 3
#include "te_config.h"

#include "logger_api.h"
#include "conf_api.h"
#include "rcf_api.h"
#include "ndn_eth.h"
#include "tapi_eth.h"
#include "tapi_tad.h"
#include "tapi_test.h"


/* The number of packets to be processed */
#define PKTS_TO_PROCESS                10

/*NOTE: internal buffer length allocated 20000 bytes */
#define PAYLOAD_LENGTH                 1400


/* Source MAC addresses are used in tests */
#define SRC_MAC  "21:07:21:06:24:30"

/* Destination MAC addresses are used in tests */
#define DST_MAC  "21:07:21:06:24:41"

#define TEST_CANCELATION \
    do {                                              \
        rcf_ta_csap_destroy(agent_a, sid_a, tx_csap); \
        rcf_ta_csap_destroy(agent_b, sid_b, rx_csap); \
    } while (0)

#define TEST_TERMINATION(_x...) \
    do {                        \
        VERB(_x);  \
        TEST_CANCELATION;       \
        return (1);             \
    } while (0)


int main()
{
    int              rc, syms = 4;

    int      recv_pkts;      /* the number of waned/received packets */

    int              num_pkts = PKTS_TO_PROCESS; /* number of packets
                                                       to be generated */

    char            *agent_a;       /* first linux agent name */
    char            *agent_a_if;    /* first agent interface name */
    int              sid_a;         /* session id for first agent */
    char            *agent_b;       /* second linux agent name */
    char            *agent_b_if;    /* second agent interface name */
    int              sid_b;         /* session id for second session */
    csap_handle_t    tx_csap;  /* the CSAP handle for frame transmission */
    csap_handle_t    rx_csap;  /* the CSAP handle for frame reception */
    unsigned long    tx_counter; /* returned from CSAP total byte counter */
    char             tx_counter_txt[20];/* buffer for tx_counter */
    unsigned long    rx_counter; /* returned from CSAP total byte counter */
    char             rx_counter_txt[20];/* buffer for rx_counter */

    char       *src_mac = SRC_MAC;
    char       *dst_mac = DST_MAC;

    struct timeval duration;

    uint8_t    src_bin_mac[ETHER_ADDR_LEN];
    uint8_t    dst_bin_mac[ETHER_ADDR_LEN];

    uint16_t   eth_type = ETH_P_IP;

    uint32_t    eth_payload_length = PAYLOAD_LENGTH;

    asn_value *csap_spec = NULL;
    asn_value *pattern;
        /* ether frame pattern used for recv csap filtering */
    asn_value *template;
        /* ether frame template used for traffic generation */

    /* Test configuration preambule */

    {
        size_t len = 100, agt_a_len;
        agent_a = malloc (100);
        if (agent_a == NULL)
        {
            fprintf(stderr, "malloc failed \n");
            return 1;
        }
        if ((rc = rcf_get_ta_list(agent_a, &len)) != 0)
        {
            fprintf(stderr, "rcf_get_ta_list failed %x\n", rc);
            return 1;
        }
        VERB("Using agent: %s, len: %d\n", agent_a, len);

        agt_a_len = strlen(agent_a);

        if (agt_a_len + 1 >= len)
        {
            VERB(" using only one agent, agt_a_len %d, len %d\n",
                    agt_a_len, len);
            agent_b = agent_a;
        }
        else
        {
            agent_b = agent_a + agt_a_len + 1;
            VERB("number of agent more then one, agent_b: %s\n",
                 agent_b);
        }

        agent_b_if = agent_a_if = strdup("eth0");
    }

    if (rcf_ta_create_session(agent_a, &sid_a))
    {
        TEST_TERMINATION(" first session creation error");
    }

    if (rcf_ta_create_session(agent_b, &sid_b))
    {
        TEST_TERMINATION(" second session creation error");
    }

    memcpy (dst_bin_mac, ether_aton(dst_mac), ETHER_ADDR_LEN);
    memcpy (src_bin_mac, ether_aton(src_mac), ETHER_ADDR_LEN);

    if (tapi_eth_csap_create(agent_a, sid_a, agent_a_if,
                             TAD_ETH_RECV_DEF & ~TAD_ETH_RECV_OTHER,
                             dst_bin_mac, src_bin_mac,
                             &eth_type, &tx_csap))
    {
        TEST_TERMINATION(" TX CSAP creation failure");
    }

    if (agent_a == agent_b)
    {
        CHECK_RC(tapi_eth_add_csap_layer(&csap_spec, agent_b_if,
                                         TAD_ETH_RECV_ALL, NULL,
                                         dst_bin_mac, NULL,
                                         TE_BOOL3_ANY /* tagged/untagged */,
                                         TE_BOOL3_ANY /* Ethernet2/LLC */));
        if ((rc = tapi_tad_csap_create(agent_b, sid_b, "eth", csap_spec,
                                       &rx_csap)) != 0)
        {
            TEST_FAIL(" RX CSAP creation failure");
        }
        asn_free_value(csap_spec); csap_spec = NULL;
    }
    else
    {
        if (tapi_eth_csap_create(agent_b, sid_b, agent_b_if,
                                 TAD_ETH_RECV_DEF,
                                 NULL, NULL,
                                 NULL, &rx_csap))
        {
            TEST_TERMINATION(" RX CSAP creation failure");
        }
    }


    /* Fill in value for iteration */
    rc = asn_parse_value_text("{ arg-sets { simple-for:{begin 1} }, "
                              "  delays plain:10, "
                              "  pdus     { eth:{} } }",
                              ndn_traffic_template,
                              &template, &syms);
    VERB("tempalte parse rc %x, syms %d", rc, syms);
    if (rc == 0)
        rc = asn_write_value_field(template, &num_pkts, sizeof(num_pkts),
                                   "arg-sets.0.#simple-for.end");

    VERB("tempalte write simple-for rc %x", rc);


#if 1
    if (rc == 0)
    {
        char eth_dst_script[] = "expr:(0x010203040500 + $0)";

        asn_free_subvalue(template, "pdus.0.#eth.src-addr");
        rc = asn_write_value_field(template, eth_dst_script,
                sizeof(eth_dst_script), "pdus.0.#eth.src-addr.#script");
        VERB("tempalte write expr %x", rc);
    }
#else
    rc = asn_write_value_field(template, dst_bin_mac,
                sizeof(dst_bin_mac), "pdus.0.#eth.dst-addr.#plain");
    rc = asn_write_value_field(template, src_bin_mac,
                sizeof(dst_bin_mac), "pdus.0.#eth.src-addr.#plain");
#endif

    /* Fill in method creating ethernet frame with UDP payload */
    if (rc == 0)
        rc = asn_write_value_field (template, &eth_payload_length,
                                    sizeof(eth_payload_length),
                                    "payload.#length");
    VERB("tempalte write payload %x", rc);

    if (rc)
    {
        TEST_TERMINATION(" traffic template creation error %x, sym %d",
                rc, syms);
    }

    /* Create pattern for recv csap filtering */
    rc = asn_parse_value_text("{{ action echo:NULL, pdus { eth:{ }} }}",
                              ndn_traffic_pattern, &pattern, &syms);

    VERB("parse pattern rc %x, syms %d", rc, syms);
#if 1
    if (rc == 0)
        rc = asn_write_value_field(pattern, dst_bin_mac,
                                   ETHER_ADDR_LEN,
                                   "0.pdus.0.#eth.dst-addr.#plain");
    if (rc)
    {
        TEST_TERMINATION(" pattern creation error %x", rc);
    }
#endif


    /* Start recieving process */
    recv_pkts = PKTS_TO_PROCESS;
    rc = tapi_tad_trrecv_start(agent_b, sid_b, rx_csap, pattern,
                               TAD_TIMEOUT_INF, recv_pkts,
                               RCF_TRRECV_COUNT);
    if (rc)
    {
        TEST_TERMINATION(" recieving process error %x", rc);
    }


    /* Start sending process */
    rc = tapi_tad_trsend_start(agent_a, sid_a, tx_csap, template,
                               RCF_MODE_NONBLOCKING);
    if (rc)
    {
        TEST_TERMINATION(" transmitting process error %x", rc);
    }

    {
        int prev, num = 0;
        tad_csap_status_t status;         /* Status of TX CSAP */

    do {
        prev = num;
        sleep(1);
        printf ("before get status\n");
        fflush (stdout);
        rc = tapi_csap_get_status(agent_a, sid_a, tx_csap, &status);
        if (rc)
        {
            TEST_TERMINATION("v1: port A. TX CSAP get status error %x",
                             rc);
        }
        printf ("TX status: %d\n", status);
        fflush (stdout);

        rc = rcf_ta_trrecv_get(agent_b, sid_b, rx_csap, NULL, NULL, &num);
        if (rc)
        {
            TEST_TERMINATION("v1: port A. RX CSAP get traffic error %x",
                             rc);
        }
        printf ("prev num: %d, RX num: %d\n", prev, num);
        fflush (stdout);
    } while(status == CSAP_BUSY);
    /*  && prev < num */
    }
    sleep (3);

    /* Stop recieving process */
    rc = rcf_ta_trrecv_stop(agent_b, sid_b, rx_csap, NULL, NULL,
                            &recv_pkts);
    if (rc)
    {
        TEST_TERMINATION(" receiving process shutdown error %x", rc);
    }

    /* Retrieve total TX bytes sent */
    rc = rcf_ta_csap_param(agent_a, sid_a, tx_csap, "total_bytes",
                           sizeof(tx_counter_txt), tx_counter_txt);
    if (rc)
    {
        TEST_TERMINATION(" total TX counter retrieving error %x", rc);
    }
    tx_counter = atoi(tx_counter_txt);

    /* Retrieve total RX bytes received */
    rc = rcf_ta_csap_param(agent_b, sid_b, rx_csap, "total_bytes",
                           sizeof(rx_counter_txt), rx_counter_txt);
    if (rc)
    {
        TEST_TERMINATION(" total RX counter retrieving error %x", rc);
    }
    rx_counter = atoi(rx_counter_txt);


    rc = tapi_csap_get_duration(agent_b, sid_b, rx_csap, &duration);
    VERB("rx_duration: rc %x sec %d, usec %d\n",
            rc, (int)duration.tv_sec, (int)duration.tv_usec);

    rc = tapi_csap_get_duration(agent_a, sid_a, tx_csap, &duration);
    VERB("tx_duration: rc %x sec %d, usec %d\n",
            rc, (int)duration.tv_sec, (int)duration.tv_usec);

    VERB("recv_pkts: %d, rx_counter: %d, tx_counter: %d\n",
            recv_pkts, (int)rx_counter, (int)tx_counter);

    fflush(stdout);
    if (recv_pkts != PKTS_TO_PROCESS)
    {
        TEST_TERMINATION(" some frames from flow are lost; got %d, "
                         "should %d",
                recv_pkts, PKTS_TO_PROCESS);
    }

    /* Check port counters for both ingress and egress ports */
    if (tx_counter != rx_counter)
    {
        TEST_TERMINATION(" TX/RX process has traffic inconsistence");
    }

    VERB("TEST PASS: recv_pkts:%d, rx_counter: %d, "
              "tx_counter: %d\n", recv_pkts, rx_counter, tx_counter);

    return 0;
}
