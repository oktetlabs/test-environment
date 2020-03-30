/** @file
 * @brief RPC Test Suite
 *
 * Put file to Agent.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 */

/** @page file_put Test for putting file
 *
 * @objective Demo of TAPI/RPC file putting test
 *
 * @param len    length of buffer for file content in bytes
 *
 * @par Scenario:
 *
 * @author Eugene Suslov <Eugene.Suslov@oktetlabs.ru>
 */

#define TE_TEST_NAME    "file_put"

#include "file_suite.h"

int
main(int argc, char **argv)
{
    char           *lfile = NULL;
    char           *rfile = NULL;
    rcf_rpc_server *rpcs = NULL;
    size_t          len = 0;
    void           *buf = NULL;

    TEST_START;
    TEST_GET_UINT_PARAM(len);
    TEST_GET_RPCS(AGT_A, "rpcs", rpcs);

    TEST_STEP("Generate a file on TEN");
    buf = te_make_buf_by_len(len);
    CHECK_NOT_NULL(lfile = tapi_file_create(len, buf, TRUE));
    RING("File '%s' is generated", lfile);

    TEST_STEP("Put the file on TA");
    rfile = tapi_file_generate_name();
    if ((rc = rcf_ta_put_file(rpcs->ta, 0, lfile, rfile)) != 0)
    {
        TEST_VERDICT("rcf_ta_put_file() failed; errno=%r", rc);
    }

    TEST_STEP("Check if the file exists on TA");
    RPC_AWAIT_ERROR(rpcs);
    if (rpc_access(rpcs, rfile, RPC_F_OK) != 0)
    {
        TEST_VERDICT("File doesn't exist on TA");
    }

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(rcf_ta_del_file(rpcs->ta, 0, rfile));

    if (unlink(lfile) != 0)
    {
        ERROR("File '%s' is not deleted", lfile);
    }

    free(lfile);
    free(buf);

    TEST_END;
}
