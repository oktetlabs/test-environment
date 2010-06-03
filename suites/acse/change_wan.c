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
 * $Id$
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
    size_t i;
    cwmp_sess_state_t cwmp_state = 0;
    _cwmp__GetParameterNames           *get_names;
    _cwmp__GetParameterNamesResponse   *get_names_resp;
    _cwmp__GetParameterValues          *get_values;
    _cwmp__GetParameterValuesResponse  *get_values_resp;
    _cwmp__SetParameterValues          *set_values;
    _cwmp__SetParameterValuesResponse  *set_values_resp;

    char *param_path = 
            "InternetGatewayDevice.WANDevice.1.WANConnectionDevice."
            "1.WANIPConnection.";
    char *wan_ip_conn_path;

    rcf_rpc_server *rpcs_acse = NULL; 
    te_errno te_rc;
    const char *ta_acse;

    TEST_START;

    TEST_GET_STRING_PARAM(ta_acse);

    CHECK_RC(rcf_rpc_server_get(ta_acse, "acse_ctl", NULL,
                               FALSE, TRUE, FALSE, &rpcs_acse));

    CHECK_RC(tapi_acse_clear_cpe(ta_acse, "A", "box"));

    CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box", ACSE_OP_MODIFY,
          "sync_mode", 1, VA_END_LIST));

    r = rpc_cwmp_conn_req(rpcs_acse, "A", "box"); 
    RING("rc of cwmp conn req %d", r);

    te_rc = tapi_acse_wait_cr_state(ta_acse, "A", "box", CR_DONE, 10);

    get_names = cwmp_get_names_alloc(param_path, 1);


    CHECK_RC(tapi_acse_wait_cwmp_state(ta_acse, "A", "box",
                                      CWMP_PENDING, 20));
    CHECK_RC(tapi_acse_cpe_get_parameter_names(rpcs_acse, "A", "box",
                                     get_names, &call_index));

    RING("GetParNames queued with index %d", call_index);

    CHECK_CWMP_RESP_RC(
         tapi_acse_cpe_get_parameter_names_resp(rpcs_acse, "A", "box",
                      10, call_index, &get_names_resp), get_names_resp);

    RING("GetNames number %d, first name '%s'",
        (int)get_names_resp->ParameterList->__size,
        get_names_resp->ParameterList->
                __ptrParameterInfoStruct[0]->Name);

    wan_ip_conn_path = strdup(get_names_resp->ParameterList->
                                __ptrParameterInfoStruct[0]->Name);

    set_values = cwmp_set_values_alloc("1", wan_ip_conn_path,
                "ExternalIPAddress", SOAP_TYPE_string, "10.20.1.3",
                "DefaultGateway", SOAP_TYPE_string, "10.20.1.1",
                "DNSServers", SOAP_TYPE_string, "10.20.1.1",
                    VA_END_LIST);

    CHECK_RC(tapi_acse_cpe_set_parameter_values(rpcs_acse, "A", "box",
                                    set_values, &call_index));


    te_rc = tapi_acse_cpe_set_parameter_values_resp(
            rpcs_acse, "A", "box", 20, call_index, &set_values_resp);

    if (TE_CWMP_FAULT == TE_RC_GET_ERROR(te_rc))
    {
        _cwmp__Fault *f = (_cwmp__Fault *)set_values_resp;
        size_t f_s = f->__sizeSetParameterValuesFault;


        ERROR("CWMP Fault received: %s(%s), arr len %d",
                f->FaultCode, f->FaultString, (int)f_s);
        for (i = 0; i < f_s; i++)
            ERROR("SetValue Fault [%d], Name '%s', Err %s(%s)",
                (int)i,
                f->SetParameterValuesFault[i].ParameterName,
                f->SetParameterValuesFault[i].FaultCode,
                f->SetParameterValuesFault[i].FaultString);
        
        TEST_FAIL("SetParameterValues failed, see details above.");
    }

    CHECK_RC(tapi_acse_cpe_disconnect(rpcs_acse, "A", "box"));

    sleep(10);

    rpc_cwmp_conn_req(rpcs_acse, "A", "box"); 

    get_values = cwmp_get_values_alloc(wan_ip_conn_path,
                                       "ExternalIPAddress",
                                       "DefaultGateway",
                                       "DNSServers",
                                       VA_END_LIST);

    CHECK_RC(tapi_acse_cpe_get_parameter_values(rpcs_acse, "A", "box",
                                    get_values, &call_index));
    CHECK_CWMP_RESP_RC(
                tapi_acse_cpe_get_parameter_values_resp(
                    rpcs_acse, "A", "box", 20, call_index,
                    &get_values_resp),
                get_values_resp);


    for (i = 0; i < get_values_resp->ParameterList->__size; i++)
    {
        char buf[1024];
        snprint_ParamValueStruct(buf, sizeof(buf), 
                                get_values_resp->ParameterList->
                                        __ptrParameterValueStruct[i]);
        RING("GetParValues result [%d]: %s", i, buf);
    }

    TEST_SUCCESS;

cleanup:
    {
        int cr_state;
        tapi_acse_manage_cpe(ta_acse, "A", "box",
                ACSE_OP_OBTAIN, "cr_state", &cr_state, VA_END_LIST);
        RING("CHECK cr_state: %d", cr_state);
    }

    CLEANUP_CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box",
                ACSE_OP_OBTAIN, "cwmp_state", &cwmp_state, VA_END_LIST));
    if (cwmp_state != CWMP_NOP)
        CLEANUP_CHECK_RC(tapi_acse_cpe_disconnect(rpcs_acse, "A", "box"));

    CLEANUP_CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box",
                ACSE_OP_MODIFY, "sync_mode", FALSE, VA_END_LIST));

    TEST_END;
}
