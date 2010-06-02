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


#define TE_TEST_NAME "acse/hold_req"

#include "acse_suite.h"

#include "tapi_rpc_tr069.h"
#include "acse_epc.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    int call_index;

    _cwmp__Download download_pars;
    _cwmp__DownloadResponse *download_resp;

    _cwmp__GetParameterNames           *get_names;
    _cwmp__GetParameterNamesResponse   *get_names_resp = NULL;

    rcf_rpc_server *rpcs_acse = NULL; 
    te_errno te_rc;

    cwmp_data_from_cpe_t from_cpe;

    const char *ta_acse;

    TEST_START;

    TEST_GET_STRING_PARAM(ta_acse);

    CHECK_RC(rcf_rpc_server_get(ta_acse, "acse_ctl", NULL,
                               FALSE, TRUE, FALSE, &rpcs_acse));

    CHECK_RC(tapi_acse_clear_cpe(ta_acse, "A", "box"));

    CHECK_RC(tapi_acse_manage_acs(ta_acse, "A", ACSE_OP_MODIFY,
          "http_root", "/home/konst/acse_http", VA_END_LIST));

    CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box", ACSE_OP_MODIFY,
          "sync_mode", 1, "hold_requests", 1, VA_END_LIST));


    memset(&download_pars, 0, sizeof(download_pars));
    download_pars.CommandKey = "test HoldRequests";
    download_pars.FileType = strdup("1 Firmware Upgrade Image");

    download_pars.URL = strdup("http://10.20.1.1:80/some_staff.bin");
    download_pars.Username = "";
    download_pars.Password = "";
    download_pars.FileSize = 0; /* TODO: fix */
    download_pars.TargetFileName = basename(download_pars.URL);
    download_pars.SuccessURL = "";
    download_pars.FailureURL = "";
    download_pars.DelaySeconds = 1;


    RING("Download queued with index %d", call_index);

    rpc_cwmp_conn_req(rpcs_acse, "A", "box");

    CHECK_RC(tapi_acse_wait_cr_state(ta_acse, "A", "box", CR_DONE, 10));

    CHECK_RC(tapi_acse_wait_cwmp_state(ta_acse, "A", "box",
                                      CWMP_PENDING, 20));

    CHECK_RC(tapi_acse_cpe_download(rpcs_acse, "A", "box",
                                    &download_pars, &call_index));

    CHECK_CWMP_RESP_RC(tapi_acse_cpe_download_resp(rpcs_acse, "A", "box",
                          10, call_index, &download_resp), download_resp);

    if (download_resp != NULL)
        RING("Download status %d", (int)download_resp->Status);
    else
        TEST_FAIL("Download resp is NULL!");

    get_names = cwmp_get_names_alloc(
        "InternetGatewayDevice.LANDevice.1.LANHostConfigManagement."
            "IPInterface.", 1);

    CHECK_RC(tapi_acse_cpe_get_parameter_names(rpcs_acse, "A", "box",
                                    get_names, &call_index));

    CHECK_CWMP_RESP_RC(
        tapi_acse_cpe_get_parameter_names_resp(rpcs_acse, "A", "box",
                      10, call_index, &get_names_resp), get_names_resp);

    /* Check that there is no Transfer Complete */
    te_rc = tapi_acse_get_rpc_acs(rpcs_acse, "A", "box", 0, 
                                  CWMP_RPC_transfer_complete,
                                  &from_cpe);
    RING(" get TransferComplete rc %r", te_rc);

    if (1 == download_resp->Status)
    {
        /* Issue empty HTTP response, in sync mode */
        rpc_cwmp_op_call(rpcs_acse, "A", "box",
                         CWMP_RPC_NONE, NULL, 0, NULL);
        CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box", ACSE_OP_MODIFY,
              "sync_mode", 0, "hold_requests", 0, VA_END_LIST));

        sleep(10);

    CHECK_RC(tapi_acse_wait_cwmp_state(ta_acse, "A", "box",
                                      CWMP_SERVE, 50));

    CHECK_RC(tapi_acse_wait_cwmp_state(ta_acse, "A", "box",
                                      CWMP_NOP, 20));


        te_rc = tapi_acse_get_rpc_acs(rpcs_acse, "A", "box", 10, 
                                      CWMP_RPC_transfer_complete,
                                      &from_cpe);
        if (0 == te_rc)
        {
            _cwmp__TransferComplete *tc = from_cpe.transfer_complete;
            RING("TransferComplete, key %s, fault: %s (%s)", 
                 tc->CommandKey, tc->FaultStruct->FaultCode, 
                 tc->FaultStruct->FaultString);
        }
        else
            RING("again check for TransferComplete return %r", te_rc);
    }


    TEST_SUCCESS;
    return result;

cleanup:

    CLEANUP_CHECK_RC(tapi_acse_cpe_disconnect(rpcs_acse, "A", "box"));

    CLEANUP_CHECK_RC(tapi_acse_manage_cpe(ta_acse, "A", "box",
                ACSE_OP_MODIFY, 
                "sync_mode", FALSE, "hold_requests", FALSE, 
                VA_END_LIST));


    TEST_END;
}
