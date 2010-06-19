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

#include "tapi_acse.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    cwmp_download_t download_pars;
    cwmp_download_response_t *download_resp;

    string_array_t               *get_names_resp = NULL;

    te_errno te_rc;

    cwmp_data_from_cpe_t from_cpe;

    tapi_acse_context_t *ctx;

    TEST_START;

    TAPI_ACSE_CTX_INIT(ctx);

    CHECK_RC(tapi_acse_clear_cpe(ctx));

    CHECK_RC(tapi_acse_manage_acs(ctx, ACSE_OP_MODIFY,
                                  "http_root", "/home/konst/acse_http",
                                  VA_END_LIST));

    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                  "sync_mode", TRUE,
                                  "hold_requests", TRUE,
                                  VA_END_LIST));


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


    CHECK_RC(tapi_acse_cpe_connect(ctx));

    CHECK_RC(tapi_acse_wait_cr_state(ctx, CR_DONE)); 
    CHECK_RC(tapi_acse_wait_cwmp_state(ctx, CWMP_PENDING));

    CHECK_RC(tapi_acse_download(ctx, &download_pars));

    CHECK_RC(tapi_acse_download_resp(ctx, &download_resp));

    if (download_resp != NULL)
        RING("Download status %d", (int)download_resp->Status);
    else
        TEST_FAIL("Download resp is NULL!");

    CHECK_RC(tapi_acse_get_parameter_names(ctx, TRUE,
                    "InternetGatewayDevice.LANDevice.1."
                    "LANHostConfigManagement.IPInterface."));

    CHECK_RC(tapi_acse_get_parameter_names_resp(ctx, &get_names_resp));

    CHECK_RC(tapi_acse_cpe_disconnect(ctx));

    /* Check that there is no Transfer Complete */
    te_rc = tapi_acse_get_rpc_acs(ctx, CWMP_RPC_transfer_complete,
                                  &from_cpe);
    RING("get TransferComplete rc %r", te_rc);
    if (TE_ENOENT != TE_RC_GET_ERROR(te_rc))
        TEST_FAIL("unexpected rc for attempt to get TransferComplete");

    if (1 == download_resp->Status)
    {
        /* TODO test is incomplete, Dimark client on CPE has strange
            behaviour */

#if 0
        /* Issue empty HTTP response, in sync mode */
        rpc_cwmp_op_call(rpcs_acse, "A", "box",
                         CWMP_RPC_NONE, NULL, 0, NULL);
#endif
        CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                      "sync_mode", FALSE,
                                      "hold_requests", FALSE,
                                      VA_END_LIST));

        ctx->timeout = 50;
        CHECK_RC(tapi_acse_wait_cwmp_state(ctx, CWMP_SERVE));

        ctx->timeout = 30;
        CHECK_RC(tapi_acse_wait_cwmp_state(ctx, CWMP_NOP));


        te_rc = tapi_acse_get_rpc_acs(ctx,
                                      CWMP_RPC_transfer_complete,
                                      &from_cpe);
        if (0 == te_rc)
        {
            cwmp_transfer_complete_t *tc = from_cpe.transfer_complete;
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
    CLEANUP_CHECK_RC(tapi_acse_cpe_disconnect(ctx));

    CLEANUP_CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY, 
                                          "sync_mode", FALSE,
                                          "hold_requests", FALSE, 
                                          VA_END_LIST));
    CLEANUP_CHECK_RC(tapi_acse_manage_acs(ctx, ACSE_OP_MODIFY,
                                          "http_root", "", VA_END_LIST));

    TEST_END;
}
