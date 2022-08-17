/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Simple RAW Ethernet test: receive first broadcast ethernet frame
 * from first ('eth0') network card.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME    "eth/caught_any"

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/ether.h>
#include <unistd.h>
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"

#include "tapi_test.h"
#include "ndn_eth.h"
#include "tapi_eth.h"

#undef TE_LOG_LEVEL
#define TE_LOG_LEVEL 255

#include "logger_api.h"

#define TE_LOG_LEVEL 255


static int cb_called = 0;

static void
local_eth_frame_handler(const asn_value *packet, int layer,
                        const ndn_eth_header_plain *header,
                        const uint8_t *payload, uint16_t plen,
                        void *userdata)
{
    UNUSED(packet);
    UNUSED(layer);
    UNUSED(payload);
    UNUSED(userdata);

    INFO("Ethernet frame received\n");
    INFO("dst: %Tm", header->dst_addr, ETHER_ADDR_LEN);
    INFO("src: %Tm", header->src_addr, ETHER_ADDR_LEN);
    INFO("payload len: %d", plen);

    cb_called++;
}


int
main(int argc, char *argv[])
{
    char   ta[32];
    size_t len = sizeof(ta);
    int  sid;
    int  num_pkts;
    int  timeout;

    te_bool pass_results;
    te_bool dump_packets;
    te_bool blocked_mode;

    int      caught_num = 0;
    uint16_t eth_type = ETH_P_IP;

    csap_handle_t              eth_listen_csap = CSAP_INVALID_HANDLE;

    asn_value *pattern = NULL;
    char       eth_device[] = "eth0";


    TEST_START;
    TEST_GET_INT_PARAM(num_pkts);
    TEST_GET_INT_PARAM(timeout);
    TEST_GET_BOOL_PARAM(pass_results);
    TEST_GET_BOOL_PARAM(dump_packets);
    TEST_GET_BOOL_PARAM(blocked_mode);

    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed %r", rc);

    VERB(" Using agent: %s\n", ta);


    if (rcf_ta_create_session(ta, &sid) != 0)
    {
        fprintf(stderr, "rcf_ta_create_session failed\n");
        VERB("rcf_ta_create_session failed\n");
        return 1;
    }
    VERB("Test: Created session: %d\n", sid);


    rc = tapi_eth_csap_create(ta, sid, eth_device,
                              TAD_ETH_RECV_DEF,
                              NULL, NULL,
                              NULL, &eth_listen_csap);
    if (rc)
        TEST_FAIL("csap for listen create error: %x\n", rc);
    else
        VERB("csap for listen created, id: %d\n", (int)eth_listen_csap);

    rc = tapi_eth_add_pdu(&pattern, NULL, TRUE, NULL, NULL, &eth_type,
                          TE_BOOL3_ANY /* tagged/untagged */,
                          TE_BOOL3_ANY /* Ethernet2/LLC */);
    if (rc != 0)
        TEST_FAIL("prepare eth pattern unit fails %X", rc);

    if (dump_packets)
    {
        char dump_fname[] = "tad_dump_hex";

        rc = asn_write_value_field(pattern,
                                   dump_fname, sizeof(dump_fname),
                                   "0.actions.0.#function");
        if (rc != 0)
            TEST_FAIL("set action 'function' for pattern unit fails %r",
                      rc);
    }

    rc = tapi_tad_trrecv_start(ta, sid, eth_listen_csap, pattern,
                               timeout, num_pkts,
                               pass_results ? RCF_TRRECV_PACKETS :
                                              RCF_TRRECV_COUNT);

    if (rc != 0)
        TEST_FAIL("tapi_tad_trrecv_start failed: %r", rc);
    VERB("eth recv start num: %d", num_pkts);

    caught_num = 0;
#if 1
    if (blocked_mode)
        rc = tapi_tad_trrecv_wait(ta, sid, eth_listen_csap,
                                  pass_results ?
                                      tapi_eth_trrecv_cb_data(
                                          local_eth_frame_handler, NULL) :
                                      NULL,
                                  &caught_num);
    else
    {
        sleep(timeout / 1000 + 1);
        rc = tapi_tad_trrecv_stop(ta, sid, eth_listen_csap,
                                  pass_results ?
                                      tapi_eth_trrecv_cb_data(
                                          local_eth_frame_handler, NULL) :
                                      NULL,
                                  &caught_num);
    }
#else
        rc = tapi_tad_trrecv_get(ta, sid, eth_listen_csap,
                                  pass_results ?
                                      tapi_eth_trrecv_cb_data(
                                          local_eth_frame_handler, NULL) :
                                      NULL,
                                  &caught_num);
    RING("Caught %d", caught_num);
    caught_num = num_pkts;
#endif


    if (rc == TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT))
    {
        RING("Wait for eth frames timedout");
        if (caught_num >= num_pkts || caught_num < 0)
            TEST_FAIL("Wrong number of caught pkts in timeout: %d",
                      caught_num);
    }
    else if (rc != 0)
        TEST_FAIL("trrecv wait on ETH CSAP fails: %r", rc);
    else if (caught_num != num_pkts)
        TEST_FAIL("Wrong number of caught pkts: %d, wanted %d",
                  caught_num, num_pkts);

    if (pass_results && (cb_called != caught_num))
        TEST_FAIL("user callback called %d != caught pkts %d",
                  cb_called, caught_num);

    RING("caught packts %d, wait/stop rc %r", caught_num, rc);

    asn_free_value(pattern);

    TEST_SUCCESS;

cleanup:

    if (eth_listen_csap != CSAP_INVALID_HANDLE &&
        (rc = rcf_ta_csap_destroy(ta, sid, eth_listen_csap) != 0) )
        ERROR("ETH listen csap destroy fails, rc %X", rc);

    TEST_END;
}
