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

#define TE_TEST_NAME    "iscsi/test_tapi"

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
uint8_t rx_buffer[1000000];

#if 0
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
#else

uint8_t iscsi_login_request[] = {
#if 0
0x00, 0x11, 0x2f, 0x6b, 0x22, 0xca, 0x00, 0x0e, 
0x0c, 0x4d, 0x47, 0x02, 0x08, 0x00, 0x45, 0x00,  
0x01, 0x4c, 0xe5, 0x4e, 0x40, 0x00, 0x40, 0x06, 
0x88, 0xde, 0xc0, 0xa8, 0x25, 0x22, 0xc0, 0xa8, 
0x25, 0x0c, 0x80, 0x3a, 0x0c, 0xbc, 0x5b, 0x24, 
0x88, 0xbc, 0xf7, 0xeb, 0xb9, 0x22, 0x80, 0x18,
0x00, 0xb7, 0x73, 0xc0, 0x00, 0x00, 0x01, 0x01, 
0x08, 0x0a, 0x00, 0xda, 0x64, 0x5e, 0x23, 0x21,
0x3e, 0xd7, 
#endif
0x43, 0x87, 0x00, 0x00, 0x00, 0x00, 
0x00, 0xe5, 0x80, 0x12, 0x34, 0x56, 0x78, 0x9a, 
0x00, 0x00, 0x00, 0x01, 0x5b, 0x38, 0x00, 0x02, 
0x00, 0x00, 0x00, 0x00, 0x56, 0xce, 0x00, 0x00,
0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x48, 0x65, 0x61, 0x64, 0x65, 0x72, 
0x44, 0x69, 0x67, 0x65, 0x73, 0x74, 0x3d, 0x43,
0x52, 0x43, 0x33, 0x32, 0x43, 0x2c, 0x4e, 0x6f, 
0x6e, 0x65, 0x00, 0x44, 0x61, 0x74, 0x61, 0x44,
0x69, 0x67, 0x65, 0x73, 0x74, 0x3d, 0x43, 0x52, 
0x43, 0x33, 0x32, 0x43, 0x2c, 0x4e, 0x6f, 0x6e,
0x65, 0x00, 0x49, 0x6e, 0x69, 0x74, 0x69, 0x61, 
0x6c, 0x52, 0x32, 0x54, 0x3d, 0x4e, 0x6f, 0x00,
0x4d, 0x61, 0x78, 0x42, 0x75, 0x72, 0x73, 0x74, 
0x4c, 0x65, 0x6e, 0x67, 0x74, 0x68, 0x3d, 0x33,
0x32, 0x37, 0x36, 0x38, 0x00, 0x46, 0x69, 0x72, 
0x73, 0x74, 0x42, 0x75, 0x72, 0x73, 0x74, 0x4c,
0x65, 0x6e, 0x67, 0x74, 0x68, 0x3d, 0x31, 0x36, 
0x33, 0x38, 0x34, 0x00, 0x44, 0x65, 0x66, 0x61,
0x75, 0x6c, 0x74, 0x54, 0x69, 0x6d, 0x65, 0x32, 
0x57, 0x61, 0x69, 0x74, 0x3d, 0x31, 0x30, 0x00,
0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x54, 
0x69, 0x6d, 0x65, 0x32, 0x52, 0x65, 0x74, 0x61,
0x69, 0x6e, 0x3d, 0x36, 0x00, 0x4d, 0x61, 0x78, 
0x4f, 0x75, 0x74, 0x73, 0x74, 0x61, 0x6e, 0x64,
0x69, 0x6e, 0x67, 0x52, 0x32, 0x54, 0x3d, 0x34, 
0x00, 0x44, 0x61, 0x74, 0x61, 0x50, 0x44, 0x55,
0x49, 0x6e, 0x4f, 0x72, 0x64, 0x65, 0x72, 0x3d, 
0x4e, 0x6f, 0x00, 0x44, 0x61, 0x74, 0x61, 0x53,
0x65, 0x71, 0x75, 0x65, 0x6e, 0x63, 0x65, 0x49, 
0x6e, 0x4f, 0x72, 0x64, 0x65, 0x72, 0x3d, 0x4e,
0x6f, 0x00, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x52, 
0x65, 0x63, 0x6f, 0x76, 0x65, 0x72, 0x79, 0x4c,
0x65, 0x76, 0x65, 0x6c, 0x3d, 0x30, 0x00, 0x00, 
0x00, 0x00
};
#endif
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
        

        if ((rc = bin_data2asn(iscsi_login_request + 48,
                               sizeof(iscsi_login_request) - 48,
                               &segment_data)) != 0
            )
        {
            TEST_FAIL("bin_data2asn failed:  %r", rc);
        }

        if ((key_num = tapi_iscsi_get_key_num(segment_data)) == -1)
            TEST_FAIL("Cannot get key number");

        for (key_index = 0; key_index < key_num; key_index++)
        {
            if ((key_name = tapi_iscsi_get_key_name(segment_data, 
                                                    key_index)) == NULL)
                TEST_FAIL("Cannot get key name");
            if ((values = tapi_iscsi_get_key_values(segment_data, 
                                                    key_index)) == NULL)
                TEST_FAIL("Cannot get key values");

            if ((key_values_num = tapi_iscsi_get_key_values_num(values
                                                                )) == -1)
            {
                TEST_FAIL("Cannot get values num");
            }

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
                        break;
                    }    
                    case iscsi_key_value_type_string:
                    {
                        if ((rc = tapi_iscsi_get_string_key_value(values, 
                                      key_values_index, &str_value)) != 0)
                            TEST_FAIL("cannot get string value");
                        break;
                    }
                }
            }
        }

        {
            iscsi_key_values values;
            rc = tapi_iscsi_add_new_key(segment_data, "NewKey", 
                                        key_num - 1);
            if (rc < 0)
            {
                ERROR("tapi_iscsi_add_new_key() failed, rc = %d", rc);
            }
            values = tapi_iscsi_key_values_create(2, 
                                                  iscsi_key_value_type_int,
                                                  239,
                                iscsi_key_value_type_string,
                                                  "renata's API is "
                                                  "not working");
            if (values == NULL)
            {
                ERROR("tapi_iscsi_key_values_create() failed, rc = %d", rc);
            }
            tapi_iscsi_set_key_values(segment_data, key_num - 1,
                                      values);
            if (rc < 0)
            {
                ERROR("tapi_iscsi_set_key_values() failed, rc = %d", rc);
            }
        }

        /***********************************/
        if ((key_num = tapi_iscsi_get_key_num(segment_data)) == -1)
            TEST_FAIL("Cannot get key number");


        for (key_index = 0; key_index < key_num; key_index++)
        {
            if ((key_name = tapi_iscsi_get_key_name(segment_data, 
                                                    key_index)) == NULL)
                TEST_FAIL("Cannot get key name");
            if ((values = tapi_iscsi_get_key_values(segment_data, 
                                                    key_index)) == NULL)
                TEST_FAIL("Cannot get key values");

            if ((key_values_num = tapi_iscsi_get_key_values_num(values
                                                                )) == -1)
            {
                TEST_FAIL("Cannot get values num");
            }

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
                        break;
                    }    
                    case iscsi_key_value_type_string:
                    {
                        if ((rc = tapi_iscsi_get_string_key_value(values, 
                                      key_values_index, &str_value)) != 0)
                            TEST_FAIL("cannot get string value");
                        break;
                    }
                }
            }
        }
        /****************************************/
        
    }
#endif    
    TEST_SUCCESS;

cleanup:

    TEST_END;
}
