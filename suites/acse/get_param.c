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


#define TE_TEST_NAME "acse/get_param"


#include "acse_suite.h"

#include "tapi_acse.h"
#include "cwmp_data.h"


#ifndef SOAP_TYPE_string
#define SOAP_TYPE_string (3)
#endif

#ifndef SOAP_TYPE_xsd__anySimpleType
#define SOAP_TYPE_xsd__anySimpleType (10)
#endif

#ifndef SOAP_TYPE_SOAP_ENC__base64
#define SOAP_TYPE_SOAP_ENC__base64 (6)
#endif

#ifndef SOAP_TYPE_time
#define SOAP_TYPE_time (98)
#endif


int
main(int argc, char *argv[])
{
    int i;
    cwmp_sess_state_t cwmp_state = 0;
    _cwmp__GetParameterValues          *get_values;
    _cwmp__GetParameterValuesResponse  *get_values_resp = NULL;
    _cwmp__GetParameterNamesResponse   *get_names_resp = NULL;

    char *param_path = 
            "InternetGatewayDevice.LANDevice.1.LANHostConfigManagement."
            "IPInterface.";
    char *lan_ip_conn_path;

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

    RING("GetParNames queued with index %u", ctx->req_id);

    CHECK_RC(tapi_acse_cpe_get_parameter_names_resp(ctx, &get_names_resp));

    RING("GetNames number %d, first name '%s'",
        (int)get_names_resp->ParameterList->__size,
        get_names_resp->ParameterList->
                __ptrParameterInfoStruct[0]->Name);

    lan_ip_conn_path = strdup(get_names_resp->ParameterList->
                                __ptrParameterInfoStruct[0]->Name);

    get_values = cwmp_get_values_alloc(lan_ip_conn_path,
                        "Enable",
                        "IPInterfaceIPAddress",
                        "IPInterfaceSubnetMask", 
                        "IPInterfaceAddressingType",
                        VA_END_LIST);

    CHECK_RC(tapi_acse_cpe_get_parameter_values(ctx, get_values));
    CHECK_RC(tapi_acse_cpe_get_parameter_values_resp(ctx,
                                                     &get_values_resp));

    for (i = 0; i < get_values_resp ->ParameterList->__size; i++)
    {
        char buf[1024];
        snprint_ParamValueStruct(buf, sizeof(buf), 
                                get_values_resp->ParameterList->
                                        __ptrParameterValueStruct[i]);
        RING("GetParValues result [%d]: %s", i, buf);
    }

    cwmp_get_values_free(get_values);

    TEST_SUCCESS;

cleanup:
    {
        int cr_state;
        tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                             "cr_state", &cr_state, VA_END_LIST);
        RING("CHECK cr_state: %d", cr_state);
    }

    CLEANUP_CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                                          "cwmp_state", &cwmp_state,
                                          VA_END_LIST));
    if (cwmp_state != CWMP_NOP)
        CLEANUP_CHECK_RC(tapi_acse_cpe_disconnect(ctx));

    CLEANUP_CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                          "sync_mode", FALSE,
                                          VA_END_LIST));

    TEST_END;
}
