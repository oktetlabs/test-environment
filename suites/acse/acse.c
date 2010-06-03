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


#define TE_TEST_NAME "acse/acse"

#include "acse_suite.h"

#include "tapi_rpc_tr069.h"
#include "acse_epc.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    char cwmp_buf[1024];
    unsigned int u;
    int r;
    int call_index;
    int cr_state;
    te_cwmp_rpc_cpe_t cwmp_rpc = CWMP_RPC_get_rpc_methods;
    _cwmp__GetRPCMethodsResponse *get_rpc_meth_r = NULL;

    rcf_rpc_server *rpcs_acse = NULL; 
    te_errno te_rc;
    const char *ta_acse;

    TEST_START;

    TEST_GET_STRING_PARAM(ta_acse);

    te_rc = rcf_rpc_server_get(ta_acse, "acse_ctl", NULL,
                               FALSE, TRUE, FALSE, &rpcs_acse);

    CHECK_RC(te_rc);

    CHECK_RC(tapi_acse_cpe_get_rpc_methods(rpcs_acse, "A", "box",
                                           &call_index));

    RING("GetRPCMethods queued with index %d", call_index);

    r = rpc_cwmp_conn_req(rpcs_acse, "A", "box");

    RING("rc of cwmp conn req %d", r);

    cr_state = -1;

    CHECK_RC(tapi_acse_wait_cr_state(ta_acse, "A", "box", CR_DONE, 10));

    sleep(3);

    CHECK_RC(tapi_acse_wait_cwmp_state(ta_acse, "A", "box", CWMP_NOP, 10));

    te_rc = tapi_acse_cpe_get_rpc_methods_resp(rpcs_acse, "A", "box",
                          TRUE, call_index, &get_rpc_meth_r);

    RING("rc of cwmp op check %r", te_rc);
    if (te_rc == 0 && get_rpc_meth_r != NULL)
    {
        MethodList *mlist;
        char answer_buf[1000] = "";
        char *p = answer_buf;
        p += sprintf(p, "RPC methods: ");

        if ((mlist = get_rpc_meth_r->MethodList_)
            != NULL)
        {
            int i;
            for (i = 0; i < mlist->__size; i++)
                p += sprintf(p, "'%s', ", mlist->__ptrstring[i]);
        }
        RING("%s", answer_buf);
    }
    else
        TEST_FAIL("GetRPCMethodsResponse fails %r", te_rc);


    TEST_SUCCESS;
    return result;

cleanup:

    TEST_END;
}
