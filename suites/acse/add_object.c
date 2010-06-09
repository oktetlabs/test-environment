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


#define TE_TEST_NAME "acse/add_object"

#include "acse_suite.h"

#include "tapi_acse.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    int object_number = 0, add_status;
    _cwmp__SetParameterValues          *set_values;
    _cwmp__SetParameterValuesResponse  *set_values_resp = NULL;
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

    CHECK_RC(tapi_acse_cpe_add_object(ctx, param_path, "test")); 

    CHECK_RC(tapi_acse_cpe_add_object_resp(ctx, &object_number,
                                           &add_status));

    RING("Add object with number %d, status %d", object_number, add_status);

    lan_ip_conn_path = malloc(strlen(param_path) + 20);
    sprintf(lan_ip_conn_path, "%s%d.", param_path, object_number);

    RING("Now Set parameters for new LAN IP interface, name '%s'",
         lan_ip_conn_path);

    set_values = cwmp_set_values_alloc("test", lan_ip_conn_path,
                    "Enable", SOAP_TYPE_xsd__boolean, TRUE,
                    "IPInterfaceIPAddress", SOAP_TYPE_string,
                                        "192.168.3.84",
                    VA_END_LIST);

    CHECK_RC(tapi_acse_cpe_set_parameter_values(ctx, set_values));

    te_rc = tapi_acse_cpe_set_parameter_values_resp(ctx, &set_values_resp);

    if (TE_CWMP_FAULT == TE_RC_GET_ERROR(te_rc))
    {
        tapi_acse_log_fault(CWMP_FAULT(set_values_resp));
        TEST_FAIL("SetParameterValues failed, see details above.");
    }
    if (te_rc != 0)
        TEST_FAIL("Unexpected error on SetParamValues response: %r", te_rc);

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
