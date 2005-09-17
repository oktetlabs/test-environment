/** @file
 * @brief Test Environment
 *
 * Simple RAW Ethernet test: receive first broadcast ethernet frame 
 * from first ('eth0') network card.
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * 
 * $Id$
 */

#define TE_TEST_NAME    "eth/caught_any"

#include "config.h"

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

void
local_eth_frame_handler(const ndn_eth_header_plain *header, 
                        const uint8_t *payload, uint16_t plen, 
                        void *userdata)
{
    UNUSED(payload);
    UNUSED(userdata);

    INFO("Ethernet frame received\n");
    INFO("dst: %Tm", header->dst_addr, ETH_ALEN);
    INFO("src: %Tm", header->src_addr, ETH_ALEN); 
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

    csap_handle_t eth_listen_csap = CSAP_INVALID_HANDLE;

    asn_value *pattern_unit;
    asn_value *pattern;
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

                    
    rc = tapi_eth_csap_create(ta, sid, eth_device, NULL, NULL, 
                          NULL, &eth_listen_csap); 
    if (rc)
        TEST_FAIL("csap for listen create error: %x\n", rc);
    else 
        VERB("csap for listen created, id: %d\n", (int)eth_listen_csap);

    rc = tapi_eth_prepare_pattern_unit(NULL, NULL, eth_type,
                                       &pattern_unit);
    if (rc != 0)
        TEST_FAIL("prepare eth pattern unit fails %X", rc);



    if (dump_packets)
    {
        char dump_fname[] = "tad_dump_hex";
        rc = asn_write_value_field(pattern_unit,
                                   dump_fname, sizeof(dump_fname), 
                                   "action.#function");
        if (rc != 0)
            TEST_FAIL("set action 'function' for pattern unit fails %X",
                      rc);
    } 

    pattern = asn_init_value(ndn_traffic_pattern);

    rc = asn_insert_indexed(pattern, pattern_unit, -1, ""); 
    if (rc != 0)
        TEST_FAIL("Add pattern unit fails %X", rc);

    rc = tapi_eth_recv_start(ta, sid, eth_listen_csap, pattern, 
                             pass_results ? local_eth_frame_handler : NULL,
                             NULL, timeout, num_pkts);

    if (rc != 0)
        TEST_FAIL("tapi_eth_recv_start failed: %r", rc);
    VERB("eth recv start num: %d", num_pkts);

    if (blocked_mode)
        rc = rcf_ta_trrecv_wait(ta, sid, eth_listen_csap, &caught_num);
    else
    {
        sleep(timeout / 1000 + 1);
        rc = rcf_ta_trrecv_stop(ta, sid, eth_listen_csap, &caught_num);
    }


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
