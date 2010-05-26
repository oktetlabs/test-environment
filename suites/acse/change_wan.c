/** @file
 * @brief ACSE Test Suite
 *
 * ACSE Test Suite
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 * 
 * $Id: acse.c 64029 2010-04-27 12:06:01Z konst $
 */


#define TE_TEST_NAME "acse/change_wan"

#include "acse_suite.h"

#include "tapi_rpc_tr069.h"
#include "acse_epc.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    int r;
    int call_index;
    _cwmp__GetParameterNames            get_names;
    _cwmp__GetParameterNamesResponse   *get_names_resp;
    _cwmp__GetParameterValues           get_values;
    _cwmp__GetParameterValuesResponse  *get_values_resp;
    _cwmp__SetParameterValues           set_values;
    _cwmp__SetParameterValuesResponse  *set_values_resp;

    char *param_path = 
            "InternaetGatewayDevice.WANDevice.1.WANConnectionDevice."
            "1.WANIPConnection.";
    char *wan_ip_conn_path;

    rcf_rpc_server *rpcs_acse = NULL; 
    te_errno te_rc;
    const char *ta_acse;

    TEST_START;

    TEST_GET_STRING_PARAM(ta_acse);

    CHECK_RC(rcf_rpc_server_get(ta_acse, "acse_ctl", NULL,
                               FALSE, TRUE, FALSE, &rpcs_acse));

    CHECK_RC(tapi_acse_clear_acs(ta_acse, "A"));

    CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box", ACSE_OP_MODIFY,
          "sync_mode", 1, VA_END_LIST));

    r = rpc_cwmp_conn_req(rpcs_acse, "A", "box"); 
    RING("rc of cwmp conn req %d", r);

    te_rc = tapi_acse_wait_cr_state(ta_acse, "A", "box", CR_DONE, 10);

    memset(&get_names, 0, sizeof(get_names));
    get_names.NextLevel = 1;
    get_names.ParameterPath = &param_path;


    CHECK_RC(tapi_acse_wait_cwmp_state(ta_acse, "A", "box",
                                      CWMP_PENDING, 20));

    CHECK_RC(tapi_acse_cpe_get_parameter_names(rpcs_acse, "A", "box",
                                    &get_names, &call_index));

    RING("GetParNames queued with index %d", call_index);

    CHECK_RC(tapi_acse_cpe_get_parameter_names_resp(rpcs_acse, "A", "box",
                          10, call_index, &get_names_resp));

    if (get_names_resp == NULL)
    {
        TEST_FAIL("NULL response for GetNames ");
    }

    RING("GetNames number %d, first name '%s'",
        (int)get_names_resp->ParameterList->__size,
        get_names_resp->ParameterList->
                __ptrParameterInfoStruct[0]->Name);

    wan_ip_conn_path = strdup(get_names_resp->ParameterList->
                                __ptrParameterInfoStruct[0]->Name);

    /* TODO: make good TAPI for it.. */
    {
        struct ParameterValueList par_list;
        struct cwmp__ParameterValueStruct par_array[] = 
        {
            {".AddressingType", "Static"},
            {".ExternalIPAddress", "10.20.1.3"},
            {".SubnetMask", "255.255.255.0"},
            {".DefaultGateway", "10.20.1.1"},
            {".DNSServers", "10.20.1.1"},
        };
        size_t sz = sizeof(par_array) / sizeof(par_array[0]);
        unsigned i;

        par_list.__size = sz;
        par_list.__ptrParameterValueStruct = malloc(sizeof(void *) * sz);

        for (i = 0; i < sz; i++)
        {
            char *full_name = malloc(256);
            strcpy(full_name, wan_ip_conn_path);
            strcat(full_name, par_array[i].Name);
            par_array[i].Name = full_name;
            par_list.__ptrParameterValueStruct[i] = &par_array[i];
        }

        set_values.ParameterKey = "1";
        set_values.ParameterList = &par_list;

        CHECK_RC(tapi_acse_cpe_set_parameter_values(rpcs_acse, "A", "box",
                                        &set_values, &call_index));
        CHECK_RC(tapi_acse_cpe_set_parameter_values_resp(
                rpcs_acse, "A", "box", 20, call_index, &set_values_resp));
    }

    /* We are in sync mode, terminate CWMP session manually. */
    CHECK_RC(tapi_acse_cpe_disconnect(rpcs_acse, "A", "box"));

    CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box", ACSE_OP_MODIFY,
          "sync_mode", 0, VA_END_LIST));

    TEST_SUCCESS;

    return result;

cleanup:

    TEST_END;
}
