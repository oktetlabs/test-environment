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

#include "tapi_acse.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    cwmp_sess_state_t cwmp_state = 0;
    string_array_t               *get_names_resp = NULL;
    cwmp_values_array_t          *get_values_resp = NULL;
    string_array_t               *get_values;
    cwmp_values_array_t          *set_values;
    int set_status;

    char *param_path = 
            "InternetGatewayDevice.WANDevice.1.WANConnectionDevice."
            "1.WANIPConnection.";
    char *wan_ip_conn_path;

    tapi_acse_context_t *ctx; 

    const char *old_wan_ip;
    const char *new_wan_ip = "10.20.1.4";
    size_t i;

    te_errno te_rc;

    TEST_START;
    TAPI_ACSE_CTX_INIT(ctx);

    CHECK_RC(tapi_acse_clear_cpe(ctx));

    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                  "sync_mode", 1, VA_END_LIST));

    CHECK_RC(tapi_acse_cpe_connect(ctx)); 
    CHECK_RC(tapi_acse_wait_cr_state(ctx, CR_DONE));

    CHECK_RC(tapi_acse_wait_cwmp_state(ctx, CWMP_PENDING));
    CHECK_RC(tapi_acse_get_parameter_names(ctx, TRUE, param_path));

    RING("GetParNames queued with index %u", ctx->req_id);

    CHECK_RC(tapi_acse_get_parameter_names_resp(ctx, &get_names_resp));

    RING("GetNames number %d, first name '%s'",
        (int)get_names_resp->size, get_names_resp->items[0]);

    wan_ip_conn_path = strdup(get_names_resp->items[0]);

    set_values = cwmp_val_array_alloc(wan_ip_conn_path,
                    "ExternalIPAddress", SOAP_TYPE_string, new_wan_ip,
                    "DefaultGateway", SOAP_TYPE_string, "10.20.1.1",
                    "DNSServers", SOAP_TYPE_string, "10.20.1.1",
                    VA_END_LIST);

    CHECK_RC(tapi_acse_set_parameter_values(ctx, "WAN test", set_values));

    te_rc = tapi_acse_set_parameter_values_resp(ctx, &set_status);

    if (TE_CWMP_FAULT == TE_RC_GET_ERROR(te_rc))
        TEST_FAIL("SetParameterValues failed, see details above.");

    CHECK_RC(tapi_acse_cpe_disconnect(ctx));

    sleep(10);

    CHECK_RC(tapi_acse_cpe_connect(ctx)); 

    get_values = cwmp_str_array_alloc(wan_ip_conn_path,
                                       "ExternalIPAddress",
                                       "DefaultGateway",
                                       "DNSServers",
                                       VA_END_LIST);

    CHECK_RC(tapi_acse_get_parameter_values(ctx, get_values));
    CHECK_RC(tapi_acse_get_parameter_values_resp(ctx, &get_values_resp));

    /* TODO API for check received value */
    {
        /* first value should be ExternalIPAddress */
        const char *got_wan_ip = get_values_resp->items[0]->Value;

        if (strcmp(got_wan_ip, new_wan_ip))
            TEST_FAIL("get shows value '%s' differ then was set '%s'",
                    got_wan_ip, new_wan_ip);
    }

    for (i = 0; i < get_values_resp->size; i++)
    {
        char buf[1024];
        snprint_ParamValueStruct(buf, sizeof(buf),
                                 get_values_resp->items[i]);
        RING("GetParValues result [%d]: %s", i, buf);
    }

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
                                          "sync_mode", FALSE, VA_END_LIST));

    TEST_END;
}
