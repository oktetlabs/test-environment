/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Simple RCF test
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME    "cli/shell"

#if 0
#define TE_LOG_LEVEL 0xff
#endif

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <pcap.h>
#include <pcap-bpf.h>

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "tapi_test.h"
#include "tapi_pcap.h"
#include "rcf_api.h"

#include "logger_api.h"

#if 0
#define PCAP_DEBUG(args...) \
    do {                                        \
        fprintf(stdout, "\nTEST PCAP " args);    \
        INFO("TEST PCAP " args);                 \
    } while (0)

#undef ERROR
#define ERROR(args...) PCAP_DEBUG("ERROR: " args)

#undef RING
#define RING(args...) PCAP_DEBUG("RING: " args)

#undef WARN
#define WARN(args...) PCAP_DEBUG("WARN: " args)

#undef VERB
#define VERB(args...) PCAP_DEBUG("VERB: " args)
#endif


static int pkt_num = 0;

void
pcap_recv_cb(const int filter_id, const uint8_t *pkt_data,
             const uint16_t pkt_len, void *userdata)
{
    VERB("Packet: FID %d, pkt_num %d at 0x%x of %d bytes, userdata at 0x%x",
         filter_id, pkt_num, pkt_data, pkt_len, userdata);
    pkt_num++;
}


int
main(int argc, char *argv[])
{
    char           *ta;
    char           *pcap_filter = "port 22";
    char           *pcap_filter2 = "port nfs";
    char           *pcap_ifname = "eth0";
    int             pcap_iftype = DLT_EN10MB;
    int             pcap_filter_id = 1;
    int             pcap_filter2_id = 2;
    int             pcap_recv_mode = PCAP_RECV_MODE_DEF;

    int             sid;
    csap_handle_t   pcap_csap;

    asn_value      *pcap_pattern = NULL;

    int             pcap_num = 0;

    int   i;
    int   try_count = 3;

    TEST_START;
    TEST_GET_STRING_PARAM(ta);

    CHECK_RC(rcf_ta_create_session(ta, &sid));

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    VERB("Try to create PCAP CSAP, ifname=%s, iftype=%d, recv_mode=%x",
         pcap_ifname, pcap_iftype, pcap_recv_mode);

    CHECK_RC(tapi_pcap_csap_create(ta, sid, pcap_ifname, pcap_iftype,
                                   pcap_recv_mode, &pcap_csap));


    VERB("Create recv pattern for filter \"%s\"", pcap_filter);

    CHECK_RC(tapi_pcap_pattern_add(pcap_filter, pcap_filter_id, &pcap_pattern));


    VERB("Add to recv pattern to filter \"%s\"", pcap_filter2);

    CHECK_RC(tapi_pcap_pattern_add(pcap_filter2, pcap_filter2_id, &pcap_pattern));


    VERB("Try to recv_start()");

    CHECK_RC(tapi_tad_trrecv_start(ta, sid, pcap_csap, pcap_pattern,
                                   10000000, 100, RCF_TRRECV_PACKETS));

    VERB("recv_start() finished");


    sleep(10);

    VERB("Try to recv_stop()");

    CHECK_RC(tapi_tad_trrecv_stop(ta, sid, pcap_csap,
                                  tapi_pcap_trrecv_cb_data(pcap_recv_cb,
                                                           NULL),
                                  &pcap_num));

    VERB("recv_stop() finished, %d packets received", pcap_num);


    VERB("Try to destroy PCAP CSAP");

    CHECK_RC(rcf_ta_csap_destroy(ta, sid, pcap_csap));

    VERB("PCAP CSAP destroyed");

    TEST_SUCCESS;

cleanup:

    TEST_END;

}
