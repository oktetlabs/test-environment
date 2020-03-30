/** @file
 * @brief RPC Test Suite
 *
 * Get file from Agent.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 */

/** @page file_get Test for getting file
 *
 * @objective Demo of TAPI/RPC file getting test
 *
 * @par Scenario:
 *
 * @author Eugene Suslov <Eugene.Suslov@oktetlabs.ru>
 */

#define TE_TEST_NAME    "file_get"

#include "file_suite.h"

int
main(int argc, char **argv)
{
    char           *lfile = NULL;
    char           *rfile = NULL;
    te_string       rpath = TE_STRING_INIT;
    rcf_rpc_server *rpcs = NULL;

    TEST_START;
    TEST_GET_RPCS(AGT_A, "rpcs", rpcs);

    TEST_STEP("Create a file on TA");

    rfile = tapi_file_generate_name();
    CHECK_RC(te_string_append(&rpath, "%s/%s", TMP_DIR, rfile));
    if (tapi_file_create_ta(rpcs->ta, rpath.ptr, "") != 0)
    {
        TEST_VERDICT("tapi_file_create_ta() failed");
    }

    TEST_STEP("Get the file from TA");
    lfile = tapi_file_generate_name();
    if ((rc = rcf_ta_get_file(rpcs->ta, 0, rpath.ptr, lfile)) != 0)
    {
        TEST_VERDICT("rcf_ta_get_file() failed; errno=%r", rc);
    }

    TEST_STEP("Check if the file exists on TEN");
    if (access(lfile, F_OK) != 0)
    {
        TEST_VERDICT("File doesn't exist on TEN");
    }

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(rcf_ta_del_file(rpcs->ta, 0, rpath.ptr));

    if (unlink(lfile) != 0)
    {
        ERROR("File '%s' is not deleted", lfile);
    }

    te_string_free(&rpath);

    TEST_END;
}
