/** @file
 * @brief Test Environment
 *
 * iSCSI CSAP and TAPI test
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
#if 0
uint8_t iscsi_login_request[sizeof(iscsi_login_request_copy)] = {0x0,};
#endif
int
main(int argc, char *argv[]) 
{ 
    csap_handle_t iscsi_csap = CSAP_INVALID_HANDLE;

    char  ta[32];
    char *agt_a = ta;
    char *agt_b;

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
    
#if 0    
    {
        asn_value_p segment_data;
        int key_num; 
        char *key_name;
        int key_index;
        iscsi_key_values values;

        uint32_t  len;

        if ((rc = bin_data2asn(iscsi_login_request_copy + 48,
                               sizeof(iscsi_login_request_copy) - 48,
                               &segment_data)) != 0)
        {
            TEST_FAIL("bin_data2asn failed:  %r", rc);
        }

        memcpy(iscsi_login_request, iscsi_login_request_copy, 
               sizeof(iscsi_login_request_copy));
        printf("sizeof iscsi pdu %d, %d", 
               sizeof(iscsi_login_request_copy), 
               sizeof(iscsi_login_request));
        
        len = sizeof(iscsi_login_request) - 48;

        if ((rc = asn2bin_data(segment_data, 
                               iscsi_login_request + 48, &len)) != 0)
            TEST_FAIL("asn2bin_data failed");
    }
#endif    

    rc = tapi_iscsi_csap_create(agt_a, 0, &iscsi_csap);
    if (rc != 0)
        TEST_FAIL("iSCSI csap create failed: %r", rc); 

#if 0
    len = 200;
    te_fill_buf(tx_buffer, len);
    INFO("+++++++++++ Prepared data: %tm", tx_buffer, len);
#endif
    {
        int i;
        size_t portion = 2;
        for (i = 0; i < sizeof (iscsi_login_request); i+= portion)
        {
            rc = tapi_iscsi_send_pkt(agt_a, 0, iscsi_csap, NULL,
                                     iscsi_login_request + i, portion);
            if (rc != 0)
                TEST_FAIL("send on CSAP failed: %r", rc); 
        }
    }

    len = sizeof(rx_buffer);
    memset(rx_buffer, 0, len);
    rc = tapi_iscsi_recv_pkt(agt_a, 0, iscsi_csap, 2000, 
                              CSAP_INVALID_HANDLE, NULL, 
                              rx_buffer, &len);
    if (rc != 0)
        TEST_FAIL("recv on CSAP failed: %r", rc); 

    INFO("+++++++++++ Received data: %tm", rx_buffer, len);

#if 1
    {
        asn_value_p segment_data;
        int key_num; 
        char *key_name;
        int key_index;
        
        iscsi_key_values values;
        int key_values_num;
        int key_values_index;

        iscsi_key_value_type type;

        char *str_value;
        int   int_value;
        

        if ((rc = bin_data2asn(rx_buffer + 48,
                               sizeof(iscsi_login_request) - 48,
                               &segment_data)) != 0:147
            )
        {
            TEST_FAIL("bin_data2asn failed:  %r", rc);
        }

        if ((key_num = tapi_iscsi_get_key_num(segment_data)) == -1)
            TEST_FAIL("Cannot get key number");

        printf("\nkey number is %d\n", key_num);

        for (key_index = 0; key_index < key_num; key_index++)
        {
            if ((key_name = tapi_iscsi_get_key_name(segment_data, 
                                                    key_index)) == NULL)
                TEST_FAIL("Cannot get key name");
            printf("\nkey_name is %s\n", key_name);
            if ((values = tapi_iscsi_get_key_values(segment_data, 
                                                    key_index)) == NULL)
                TEST_FAIL("Cannot get key values");

            if ((key_values_num = tapi_iscsi_get_key_values_num(values
                                                                )) == -1)
            {
                TEST_FAIL("Cannot get values num");
            }
            printf("\nvalues num is %d\n", key_values_num);

            for (key_values_index = 0; 
                 key_values_index < key_values_num; 
                 key_values_index++)
            {
                if ((type = tapi_iscsi_get_key_value_type(values, 
                                key_values_index)) 
                    == iscsi_key_value_type_invalid)
                    TEST_FAIL("Cannot get type");         
                switch (type)
                {
                    case iscsi_key_value_type_int:
                    case iscsi_key_value_type_hex:    
                    {    
                        if ((rc = tapi_iscsi_get_int_key_value(values, 
                                      key_values_index, &int_value)) != 0)
                            TEST_FAIL("cannot get int value");
                        printf("\nget int value %d\n", int_value);
                        break;
                    }    
                    case iscsi_key_value_type_string:
                    {
                        if ((rc = tapi_iscsi_get_string_key_value(values, 
                                      key_values_index, &str_value)) != 0)
                            TEST_FAIL("cannot get string value");
                        printf("\nget string value %s\n", str_value);
                        break;
                    }
                }
            }
        }
    }
#endif    

#if 0
    /* temporary check, while there is dummy 'target' on TA */
    rc = memcmp(tx_buffer, rx_buffer, len);
    if (rc != 0)
        TEST_FAIL("iSCSI: sent and received data differ, rc = %d", rc);
#endif


    TEST_SUCCESS;

cleanup:

    if (iscsi_csap != CSAP_INVALID_HANDLE)
        rcf_ta_csap_destroy(agt_a, 0, iscsi_csap);

    TEST_END;
}
