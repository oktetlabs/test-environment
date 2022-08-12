/** @file
 * @brief ACSE Test Suite
 *
 * ACSE Test Suite
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 */


#define TE_TEST_NAME "acse/add_object"

#include "acse_suite.h"

#include "tapi_acse.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    int object_number = 0, add_status, del_status, set_status;

    cwmp_values_array_t *set_values;
    tapi_acse_context_t *ctx;

    char *param_path =
            "InternetGatewayDevice.LANDevice.1.LANHostConfigManagement."
            "IPInterface.";
    char *lan_ip_conn_path;

    te_errno te_rc;

    TEST_START;

    TAPI_ACSE_CTX_INIT(ctx);

    CHECK_RC(tapi_acse_clear_cpe(ctx));

    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                  "sync_mode", TRUE, VA_END_LIST));

    CHECK_RC(tapi_acse_cpe_connect(ctx));

    CHECK_RC(tapi_acse_wait_cr_state(ctx, CR_DONE));

    CHECK_RC(tapi_acse_add_object(ctx, param_path, "test"));

    CHECK_RC(tapi_acse_add_object_resp(ctx, &object_number, &add_status));

    RING("Add object with number %d, status %d", object_number, add_status);

    lan_ip_conn_path = malloc(strlen(param_path) + 20);
    sprintf(lan_ip_conn_path, "%s%d.", param_path, object_number);

    RING("Now Set parameters for new LAN IP interface, name '%s'",
         lan_ip_conn_path);

    set_values = cwmp_val_array_alloc(lan_ip_conn_path,
                    "Enable", SOAP_TYPE_xsd__boolean, TRUE,
                    "IPInterfaceIPAddress", SOAP_TYPE_string,
                                        "192.168.3.85",
                    VA_END_LIST);

    CHECK_RC(tapi_acse_set_parameter_values(ctx, "Set LAN IP", set_values));

    te_rc = tapi_acse_set_parameter_values_resp(ctx, &set_status);

    if (TE_CWMP_FAULT == TE_RC_GET_ERROR(te_rc))
        TEST_FAIL("SetParameterValues failed, see details above.");

    if (te_rc != 0)
        TEST_FAIL("Unexpected error on SetParamValues response: %r", te_rc);

    /* TODO check here by some other way that LAN is added, before delete.*/

    CHECK_RC(tapi_acse_delete_object(ctx, lan_ip_conn_path, "test"));

    CHECK_RC(tapi_acse_delete_object_resp(ctx, &del_status));

    TEST_SUCCESS;

cleanup:
    {
        int cr_state;
        tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                             "cr_state", &cr_state, VA_END_LIST);
        RING("CHECK cr_state: %d", cr_state);
    }

    CLEANUP_CHECK_RC(tapi_acse_cpe_disconnect(ctx));

    CLEANUP_CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                          "sync_mode", FALSE, VA_END_LIST));

    TEST_END;
}
