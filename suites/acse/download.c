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

#include "tapi_rpc_tr069.h"
#include "acse_epc.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    int r;
    int call_index;
    int cr_state;
    _cwmp__Download download_pars;
    _cwmp__DownloadResponse *download_resp;

    rcf_rpc_server *rpcs_acse = NULL; 
    te_errno te_rc;
    const char *ta_acse;
    const char *file_type;
    const char *url;

    TEST_START;

    TEST_GET_STRING_PARAM(ta_acse);
    TEST_GET_STRING_PARAM(file_type);
    TEST_GET_STRING_PARAM(url);

    CHECK_RC(rcf_rpc_server_get(ta_acse, "acse_ctl", NULL,
                               FALSE, TRUE, FALSE, &rpcs_acse));

    CHECK_RC(tapi_acse_clear_cpe(ta_acse, "A", "box"));

    CHECK_RC(tapi_acse_manage_acs(ta_acse, "A", ACSE_OP_MODIFY,
          "http_root", "/home/konst/acse_http", VA_END_LIST));

    memset(&download_pars, 0, sizeof(download_pars));
    download_pars.CommandKey = "SW upgrade";
    download_pars.FileType = strdup(file_type);

    download_pars.URL = strdup(url);
    download_pars.Username = "";
    download_pars.Password = "";
    download_pars.FileSize = 0; /* TODO: fix */
    download_pars.TargetFileName = basename(url);
    download_pars.SuccessURL = "";
    download_pars.FailureURL = "";

    CHECK_RC(tapi_acse_cpe_download(rpcs_acse, "A", "box",
                                    &download_pars, &call_index));

    RING("Download queued with index %d", call_index);

    r = rpc_cwmp_conn_req(rpcs_acse, "A", "box");

    RING("rc of cwmp conn req %d", r);

    cr_state = -1;

    CHECK_RC(tapi_acse_wait_cr_state(ta_acse, "A", "box", CR_DONE, 10));

    /* time to establish CWMP session */
    sleep(3);

    CHECK_RC(tapi_acse_wait_cwmp_state(ta_acse, "A", "box", CWMP_NOP, 20));

    te_rc = tapi_acse_cpe_download_resp(rpcs_acse, "A", "box",
                          10, call_index, &download_resp);

    RING("rc of download_resp: %r", te_rc);
    if (0 == te_rc && download_resp != NULL)
    {
        RING("Download status %d", (int)download_resp->Status);
    }
    else if (TE_CWMP_FAULT == TE_RC_GET_ERROR(te_rc))
    {
        _cwmp__Fault *f = (_cwmp__Fault *)download_resp;
        RING("Fault detected: %s(%s)", f->FaultCode, f->FaultString);
    }
    else 
        TEST_FAIL("Download unexpected fail %r", te_rc);

    if (0 == te_rc && 1 == download_resp->Status)
    { 
        /* Try wait for TransferComplete */
        cwmp_data_from_cpe_t from_cpe;
        sleep(10); /* time to pass file */
        te_rc = tapi_acse_get_rpc_acs(rpcs_acse, "A", "box", 30, 
                                      CWMP_RPC_transfer_complete,
                                      &from_cpe);
        if (0 == te_rc)
        {
            _cwmp__TransferComplete *tc = from_cpe.transfer_complete;
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
