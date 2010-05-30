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


#define TE_TEST_NAME "acse/set_param"

#include "acse_suite.h"

#include "tapi_rpc_tr069.h"
#include "acse_epc.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    int r;
    int call_index;
    cwmp_sess_state_t cwmp_state = 0;
    _cwmp__GetParameterNames            get_names;
    _cwmp__GetParameterNamesResponse   *get_names_resp = NULL;
    _cwmp__GetParameterValues           get_values;
    _cwmp__GetParameterValuesResponse  *get_values_resp = NULL;
    _cwmp__SetParameterValues           set_values;
    _cwmp__SetParameterValuesResponse  *set_values_resp = NULL;

    char *param_path = 
            "InternetGatewayDevice.LANDevice.1.LANHostConfigManagement."
            "IPInterface.";
    char *lan_ip_conn_path;

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

    memset(&get_names, 0, sizeof(get_names));
    get_names.NextLevel = 1;
    get_names.ParameterPath = &param_path;


    CHECK_RC(tapi_acse_wait_cwmp_state(ta_acse, "A", "box",
                                      CWMP_PENDING, 20));

    CHECK_RC(tapi_acse_cpe_get_parameter_names(rpcs_acse, "A", "box",
                                    &get_names, &call_index));

    RING("GetParNames queued with index %d", call_index);

    te_rc = tapi_acse_cpe_get_parameter_names_resp(rpcs_acse, "A", "box",
                          10, call_index, &get_names_resp);
    if (TE_CWMP_FAULT == TE_RC_GET_ERROR(te_rc))
    {
        _cwmp__Fault *f = (_cwmp__Fault *)get_names_resp;
        TEST_FAIL("CWMP Fault received: %s(%s)",
                    f->FaultCode, f->FaultString);
    }
    if (te_rc != NULL)
        TEST_FAIL("Unexpected error on GetParamNames response: %r", te_rc);

    if (get_names_resp == NULL)
    {
        TEST_FAIL("NULL response for GetNames ");
    }

    RING("GetNames number %d, first name '%s'",
        (int)get_names_resp->ParameterList->__size,
        get_names_resp->ParameterList->
                __ptrParameterInfoStruct[0]->Name);

    lan_ip_conn_path = strdup(get_names_resp->ParameterList->
                                __ptrParameterInfoStruct[0]->Name);

    /* TODO: make good TAPI for it.. */
    {
        size_t i;
        struct ParameterNames par_names;
        char **names_array = calloc(4, sizeof(char*));
        par_names.__size = 4;
        par_names.__ptrstring = names_array;
        names_array[0] = calloc(256, 1);
        sprintf(names_array[0],
                "%s%s", lan_ip_conn_path, "Enable");
        names_array[1] = calloc(256, 1);
        sprintf(names_array[1],
                "%s%s", lan_ip_conn_path, "IPInterfaceIPAddress");
        names_array[2] = calloc(256, 1);
        sprintf(names_array[2],
                "%s%s", lan_ip_conn_path, "IPInterfaceSubnetMask");
        names_array[3] = calloc(256, 1);
        sprintf(names_array[3],
                "%s%s", lan_ip_conn_path, "IPInterfaceAddressingType");
        get_values.ParameterNames_ = &par_names;

        CHECK_RC(tapi_acse_cpe_get_parameter_values(rpcs_acse, "A", "box",
                                        &get_values, &call_index));
        te_rc = tapi_acse_cpe_get_parameter_values_resp(
                rpcs_acse, "A", "box", 20, call_index, &get_values_resp);
        if (TE_CWMP_FAULT == TE_RC_GET_ERROR(te_rc))
        {
            _cwmp__Fault *f = (_cwmp__Fault *)get_values_resp;
            ERROR("CWMP Fault received: %s(%s)",
                    f->FaultCode, f->FaultString);
            TEST_FAIL("GetParameterValues failed");
        }

    if (te_rc != NULL)
        TEST_FAIL("Unexpected error on GetParamValues response: %r", te_rc);

        for (i = 0; i < get_values_resp ->ParameterList->__size; i++)
        {
            RING("GetParamValues [%u] resp: %s = %s", i, 
                get_values_resp ->ParameterList->
                        __ptrParameterValueStruct[i]->Name,
                get_values_resp ->ParameterList->
                        __ptrParameterValueStruct[i]->Value);
        }
    }

    /* TODO: make good TAPI for it.. */
    {
        struct ParameterValueList par_list;
        struct cwmp__ParameterValueStruct par_array[] = 
        {
            /* {"Enable", "1"}, */
            {"IPInterfaceIPAddress", "192.168.3.16"},
        };
        size_t sz = sizeof(par_array) / sizeof(par_array[0]);
        unsigned i;

        par_list.__size = sz;
        par_list.__ptrParameterValueStruct = malloc(sizeof(void *) * sz);

        for (i = 0; i < sz; i++)
        {
            char *full_name = malloc(256);
            strcpy(full_name, lan_ip_conn_path);
            strcat(full_name, par_array[i].Name);
            par_array[i].Name = full_name;
            par_list.__ptrParameterValueStruct[i] = &par_array[i];
        }

        set_values.ParameterKey = "1";
        set_values.ParameterList = &par_list;

        CHECK_RC(tapi_acse_cpe_set_parameter_values(rpcs_acse, "A", "box",
                                        &set_values, &call_index));

        te_rc = tapi_acse_cpe_set_parameter_values_resp(
                rpcs_acse, "A", "box", 20, call_index, &set_values_resp);
        if (TE_CWMP_FAULT == TE_RC_GET_ERROR(te_rc))
        {
            _cwmp__Fault *f = (_cwmp__Fault *)set_values_resp;
            size_t f_s = f->__sizeSetParameterValuesFault;
            size_t i;


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
    if (te_rc != NULL)
        TEST_FAIL("Unexpected error on SetParamValues response: %r", te_rc);
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
