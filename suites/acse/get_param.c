/** @file
 * @brief ACSE Test Suite
 *
 * ACSE Test Suite
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
    string_array_t               *get_names_resp = NULL;
    cwmp_values_array_t          *get_values_resp = NULL;
    string_array_t               *get_values;

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
    CHECK_RC(tapi_acse_get_parameter_names(ctx, TRUE, param_path));

    RING("GetParNames queued with index %u", ctx->req_id);

    CHECK_RC(tapi_acse_get_parameter_names_resp(ctx, &get_names_resp));

    RING("GetNames number %d, first name '%s'",
        (int)get_names_resp->size, get_names_resp->items[0]);

    lan_ip_conn_path = strdup(get_names_resp->items[0]);

    get_values = cwmp_str_array_alloc(lan_ip_conn_path,
                        "Enable",
                        "IPInterfaceIPAddress",
                        "IPInterfaceSubnetMask",
                        "IPInterfaceAddressingType",
                        VA_END_LIST);

    CHECK_RC(tapi_acse_get_parameter_values(ctx, get_values));
    CHECK_RC(tapi_acse_get_parameter_values_resp(ctx, &get_values_resp));

    for (i = 0; i < get_values_resp->size; i++)
    {
        char buf[1024];
        snprint_ParamValueStruct(buf, sizeof(buf),
                                 get_values_resp->items[i]);
        RING("GetParValues result [%d]: %s", i, buf);
    }

    cwmp_str_array_free(get_values);

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
