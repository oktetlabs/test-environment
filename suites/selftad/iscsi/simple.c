/** @file
 * @brief Test Environment
 *
 * iSCSI CSAP and TAPI test, twice CSAPs. 
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
 * $Id: simple.c 17763 2005-08-30 09:52:37Z konst $
 */

#define TE_TEST_NAME    "iscsi/simple"

#define TE_LOG_LEVEL 0xff

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

#include "tapi_rpc.h"

#include "tapi_test.h"
#include "tapi_tcp.h"
#include "tapi_iscsi.h"

#include "te_bufs.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"

uint8_t tx_buffer[10000];
uint8_t rx_buffer[10000];


uint8_t iscsi_login_request[] = {
0x43, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x71,
0x40, 0x00, 0x01, 0x37, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00,
0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x49, 0x6e, 0x69, 0x74, 0x69, 0x61, 0x74, 0x6f,
0x72, 0x4e, 0x61, 0x6d, 0x65, 0x3d, 0x69, 0x71,
0x6e, 0x2e, 0x31, 0x39, 0x39, 0x31, 0x2d, 0x30,
0x35, 0x2e, 0x63, 0x6f, 0x6d, 0x2e, 0x6d, 0x69,
0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x3a,
0x6d, 0x6f, 0x72, 0x69, 0x61, 0x2d, 0x76, 0x6d,
0x00, 0x53, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e,
0x54, 0x79, 0x70, 0x65, 0x3d, 0x4e, 0x6f, 0x72,
0x6d, 0x61, 0x6c, 0x00, 0x54, 0x61, 0x72, 0x67,
0x65, 0x74, 0x4e, 0x61, 0x6d, 0x65, 0x3d, 0x69,
0x71, 0x6e, 0x2e, 0x32, 0x30, 0x30, 0x34, 0x2d,
0x30, 0x31, 0x2e, 0x63, 0x6f, 0x6d, 0x3a, 0x30,
0x00, 0x41, 0x75, 0x74, 0x68, 0x4d, 0x65, 0x74,
0x68, 0x6f, 0x64, 0x3d, 0x4e, 0x6f, 0x6e, 0x65,
0x00, 0x00, 0x00, 0x00
};

int
main(int argc, char *argv[]) 
{ 
    csap_handle_t iscsi_csap = CSAP_INVALID_HANDLE;

    char  ta[32];
    char *agt_a = ta;
    char *agt_b;

    int sid;

    size_t  len = sizeof(ta);

    TEST_START; 

    {
        struct timeval now;
        gettimeofday(&now, NULL);
        srand(now.tv_usec);
    }

    
    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed: %r", rc);

    INFO("Found first TA: %s; len %d", ta, len);

    agt_a = ta;
    if (strlen(ta) + 1 >= len) 
        TEST_FAIL("There is no second Test Agent");

    agt_b = ta + strlen(ta) + 1;

    INFO("Found second TA: %s", agt_b, len);

    CHECK_RC(rcf_ta_create_session(agt_a, &sid));


    rc = tapi_iscsi_tgt_csap_create(agt_a,
                                    ISCSI_DIGEST_NONE,
                                    ISCSI_DIGEST_NONE,
                                    &iscsi_csap);
    if (rc != 0)
        TEST_FAIL("iSCSI csap 1 create failed: %r", rc); 


    rc = tapi_iscsi_send_pkt(agt_a, sid, iscsi_csap, NULL,
                             iscsi_login_request, 
                             sizeof(iscsi_login_request));
    if (rc != 0)
        TEST_FAIL("send on CSAP 1 failed: %r", rc); 


    len = sizeof(rx_buffer);
    memset(rx_buffer, 0, len);
    rc = tapi_iscsi_recv_pkt(agt_a, sid, iscsi_csap, 2000, 
                             CSAP_INVALID_HANDLE,
                             NULL, rx_buffer, &len);
    if (rc != 0)
        TEST_FAIL("recv on CSAP 1 failed: %r", rc); 


    INFO("+++++++++++ Received data: %tm", rx_buffer, len);

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(rcf_ta_csap_destroy(agt_a, sid, iscsi_csap));

    TEST_END;
}
