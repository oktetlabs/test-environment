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


#define TE_TEST_NAME "acse/download"

#include "acse_suite.h"

#include "acse_epc.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    cwmp_download_t download_pars;
    cwmp_download_response_t *download_resp;

    te_errno te_rc;
    const char *file_type;
    const char *url;

    tapi_acse_context_t *ctx;

    TEST_START;


    TEST_GET_STRING_PARAM(file_type);
    TEST_GET_STRING_PARAM(url);

    TAPI_ACSE_CTX_INIT(ctx);

    CHECK_RC(tapi_acse_clear_cpe(ctx));

    CHECK_RC(tapi_acse_manage_acs(ctx, ACSE_OP_MODIFY,
                                  "http_root", "/home/konst/acse_http",
                                  VA_END_LIST));

    /* TODO API for fill download params */
    memset(&download_pars, 0, sizeof(download_pars));
    download_pars.CommandKey = "SW upgrade";
    download_pars.FileType = strdup(file_type);

    download_pars.URL = strdup(url);
    download_pars.Username = "";
    download_pars.Password = "";
    download_pars.FileSize = 0; /* TODO: fix? */
    download_pars.TargetFileName = basename(url);
    download_pars.SuccessURL = "";
    download_pars.FailureURL = "";

    CHECK_RC(tapi_acse_cpe_download(ctx, &download_pars));

    CHECK_RC(tapi_acse_cpe_connect(ctx));

    CHECK_RC(tapi_acse_wait_cr_state(ctx, CR_DONE));

    /* time to establish CWMP session */
    sleep(3);

    CHECK_RC(tapi_acse_wait_cwmp_state(ctx, CWMP_NOP));

    te_rc = tapi_acse_cpe_download_resp(ctx, &download_resp);

    RING("rc of download_resp: %r", te_rc);
    if (0 == te_rc && download_resp != NULL)
    {
        RING("Download status %d", (int)download_resp->Status);
    }
    else if (TE_CWMP_FAULT == TE_RC_GET_ERROR(te_rc))
    {
        cwmp_fault_t *f = (cwmp_fault_t *)download_resp;
        RING("Fault detected: %s(%s)", f->FaultCode, f->FaultString);
    }
    else 
        TEST_FAIL("Download unexpected fail %r", te_rc);

    if (0 == te_rc && 1 == download_resp->Status)
    { 
        /* Try wait for TransferComplete */
        cwmp_data_from_cpe_t from_cpe;
        ctx->timeout = 40;
        te_rc = tapi_acse_get_rpc_acs(ctx, CWMP_RPC_transfer_complete,
                                      &from_cpe);
        if (0 == te_rc)
        {
            cwmp_transfer_complete_t *tc = from_cpe.transfer_complete;
            RING("TransferComplete, key %s, fault: %s (%s)", 
                 tc->CommandKey, tc->FaultStruct->FaultCode, 
                 tc->FaultStruct->FaultString);
        }
    }
    /* TODO check test result according with test parameters: with 
    different types of download and different sources normal expected
    results are differ. */


    TEST_SUCCESS;
    return result;

cleanup:

    TEST_END;
}
