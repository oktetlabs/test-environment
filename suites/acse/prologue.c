/** @file
 * @brief ACSE Test Suite
 *
 * ACSE Test Suite prologue.
 *
 * Copyright (C) 2010 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Konstantin Abramenko <Konstantin Abramenko@oktetlabs.ru>
 *
 * $Id: $
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "ACSE prologue"

#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>

#include "acse_suite.h"

#include "cwmp_data.h"
#include "tapi_test.h"
#include "tapi_acse.h"


int
main(int argc, char *argv[])
{
    char *cr_url = NULL;
    int   cr_url_wait_count; 

    tapi_acse_context_t *ctx;

    TEST_START;

    TAPI_ACSE_CTX_INIT(ctx);
    
    CHECK_RC(tapi_acse_manage_acs(ctx, ACSE_OP_MODIFY,
                                  "enabled", 1, VA_END_LIST));

    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                                  "cr_url", &cr_url, VA_END_LIST));

    if (cr_url != NULL)
        RING("got ConnReq url '%s'", cr_url);
    else
        RING("got ConnReq url empty");

    cr_url_wait_count = 200;

    while (cr_url == NULL || strlen(cr_url) == 0)
    {
        sleep(1);
        CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                                      "cr_url", &cr_url, VA_END_LIST));

        RING("got ConnReq url '%s', count %d", cr_url, cr_url_wait_count);

        if ((--cr_url_wait_count) < 0)
            break;
    }

    if (cr_url == NULL || strlen(cr_url) == 0)
        TEST_FAIL("No Conn Req url on ACSE");

    CHECK_RC(tapi_acse_wait_cwmp_state(ctx, CWMP_NOP));

    TEST_SUCCESS;
    return result;

cleanup:

    TEST_END;
}
