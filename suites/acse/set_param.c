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
    cwmp_sess_state_t cwmp_state = 0;
    _cwmp__SetParameterValues          *set_values;
    _cwmp__SetParameterValuesResponse  *set_values_resp = NULL;
    _cwmp__GetParameterNamesResponse   *get_names_resp = NULL;

    char *param_path = 
            "InternetGatewayDevice.LANDevice.1.LANHostConfigManagement."
            "IPInterface.";
    char *lan_ip_conn_path;

    te_errno te_rc;
    tapi_acse_context_t *ctx; 

    TEST_START;

    TAPI_ACSE_CTX_INIT(ctx);

    CHECK_RC(tapi_acse_clear_cpe(ctx));

    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                  "sync_mode", TRUE, VA_END_LIST));

    CHECK_RC(tapi_acse_cpe_connect(ctx));

    CHECK_RC(tapi_acse_wait_cr_state(ctx, CR_DONE));

    CHECK_RC(tapi_acse_wait_cwmp_state(ctx, CWMP_PENDING));

    CHECK_RC(tapi_acse_cpe_get_parameter_names(ctx, TRUE, param_path));

    CHECK_RC(tapi_acse_cpe_get_parameter_names_resp(ctx, &get_names_resp));

    RING("GetNames number %d, first name '%s'",
        (int)get_names_resp->ParameterList->__size,
        get_names_resp->ParameterList->
                __ptrParameterInfoStruct[0]->Name);

    lan_ip_conn_path = strdup(get_names_resp->ParameterList->
                                __ptrParameterInfoStruct[0]->Name);

    set_values = cwmp_set_values_alloc("1", lan_ip_conn_path,
                    "Enable", SOAP_TYPE_xsd__boolean, 1,
                    "IPInterfaceIPAddress", SOAP_TYPE_string,
                                        "192.168.2.32",
                    VA_END_LIST);

    CHECK_RC(tapi_acse_cpe_set_parameter_values(ctx, set_values));

    te_rc = tapi_acse_cpe_set_parameter_values_resp(ctx, &set_values_resp);

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
    if (te_rc != 0)
        TEST_FAIL("Unexpected error on SetParamValues response: %r", te_rc);


    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                                          "cwmp_state", &cwmp_state,
                                          VA_END_LIST));
    if (cwmp_state != CWMP_NOP)
        CLEANUP_CHECK_RC(tapi_acse_cpe_disconnect(ctx));

    CLEANUP_CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                          "sync_mode", FALSE, VA_END_LIST));

    TEST_END;
}
