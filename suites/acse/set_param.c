/** @file
 * @brief ACSE Test Suite
 *
 * ACSE Test Suite
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 */


#define TE_TEST_NAME "acse/set_param"

#include "acse_suite.h"

#include "tapi_acse.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    int set_status;
    cwmp_sess_state_t cwmp_state = 0;
    cwmp_values_array_t          *set_values;
    string_array_t               *get_names_resp = NULL;

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

    CHECK_RC(tapi_acse_get_parameter_names(ctx, TRUE, param_path));

    CHECK_RC(tapi_acse_get_parameter_names_resp(ctx, &get_names_resp));

    RING("GetNames number %d, first name '%s'",
        (int)get_names_resp->size, get_names_resp->items[0]);

    lan_ip_conn_path = strdup(get_names_resp->items[0]);

    set_values = cwmp_val_array_alloc(lan_ip_conn_path,
                        "Enable", SOAP_TYPE_boolean, TRUE,
                        "IPInterfaceIPAddress", SOAP_TYPE_string,
                                        "192.168.2.31",
                        VA_END_LIST);

    CHECK_RC(tapi_acse_set_parameter_values(ctx, "test", set_values));

    te_rc = tapi_acse_set_parameter_values_resp(ctx, &set_status);

    if (TE_CWMP_FAULT == TE_RC_GET_ERROR(te_rc))
        TEST_FAIL("SetParameterValues failed, see details above.");

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
