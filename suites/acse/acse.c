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

#include "tapi_acse.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    cwmp_get_rpc_methods_response_t *get_rpc_meth_r = NULL;
    string_array_t *methods;

    te_errno te_rc;

    tapi_acse_context_t *ctx;

    TEST_START;

    TAPI_ACSE_CTX_INIT(ctx);

    CHECK_RC(tapi_acse_get_rpc_methods(ctx));

    RING("GetRPCMethods queued with index %u", ctx->req_id);

    CHECK_RC(tapi_acse_cpe_connect(ctx));

    CHECK_RC(tapi_acse_wait_cr_state(ctx, CR_DONE));

    sleep(3);

    CHECK_RC(tapi_acse_wait_cwmp_state(ctx, CWMP_NOP));

    te_rc = tapi_acse_get_rpc_methods_resp(ctx, &methods);

    RING("rc of cwmp op check %r", te_rc);
    if (te_rc == 0 && get_rpc_meth_r != NULL)
    {
        char answer_buf[1000];
        char *p = answer_buf;
        int i;
        p += sprintf(p, "RPC methods: ");

        for (i = 0; i < methods->size; i++)
            p += sprintf(p, "'%s', ", methods->items[i]);
        RING("%s", answer_buf);
    }
    else
        TEST_FAIL("GetRPCMethodsResponse fails %r", te_rc);


    TEST_SUCCESS;
    return result;

cleanup:

    TEST_END;
}
