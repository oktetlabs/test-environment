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


#define TE_TEST_NAME "acse/acse"

#include "acse_suite.h"

#include "tapi_acse.h"
#include "cwmp_data.h"

int
main(int argc, char *argv[])
{
    string_array_t *methods = NULL;
    char cr_url_correct[100];

    te_errno te_rc;

    tapi_acse_context_t *ctx;

    TEST_START;

    TAPI_ACSE_CTX_INIT(ctx);


    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                                  "cr_url", &cr_url_correct,
                                  VA_END_LIST));

#if 0
    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                  "cr_url", "http://1.2.3.4/123243",
                                  VA_END_LIST));
#else
    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                  "cr_url", "http://10.20.1.2:8081/123243",
                                  VA_END_LIST));
#endif
    CHECK_RC(tapi_acse_get_rpc_methods(ctx));

    RING("GetRPCMethods queued with index %u", ctx->req_id);

    CHECK_RC(tapi_acse_cpe_connect(ctx));

    CHECK_RC(tapi_acse_wait_cr_state(ctx, CR_DONE));

    sleep(3);

    CHECK_RC(tapi_acse_wait_cwmp_state(ctx, CWMP_NOP));

    te_rc = tapi_acse_get_rpc_methods_resp(ctx, &methods);

    RING("rc of cwmp op check %r", te_rc);
    if (te_rc == 0 && methods != NULL)
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
        TEST_FAIL("GetRPCMethodsResponse fails %r, methods %p",
                  te_rc, methods);


    TEST_SUCCESS;
    return result;

cleanup:

    CLEANUP_CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                  "cr_url", cr_url_correct,
                                  VA_END_LIST));

    TEST_END;
}
