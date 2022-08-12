/** @file
 * @brief RPC Test Suite
 *
 * Get file from Agent.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

/** @page file_get Test for getting file
 *
 * @objective Demo of TAPI/RPC file getting test
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "file_get"

#include "file_suite.h"

int
main(int argc, char **argv)
{
    char           *rfile = NULL;
    te_string       rpath = TE_STRING_INIT;
    rcf_rpc_server *rpcs = NULL;
    char           *contents = NULL;
    static char     expected[] = "First Second";

    TEST_START;
    TEST_GET_RPCS(AGT_A, "rpcs", rpcs);

    TEST_STEP("Create a file on TA");

    rfile = tapi_file_generate_name();
    CHECK_RC(te_string_append(&rpath, "%s/%s", TMP_DIR, rfile));
    CHECK_RC(tapi_file_append_ta(rpcs->ta, rpath.ptr, "NULL"));
    CHECK_RC(tapi_file_create_ta(rpcs->ta, rpath.ptr, "First"));
    CHECK_RC(tapi_file_append_ta(rpcs->ta, rpath.ptr, " Second"));

    TEST_STEP("Get the file from TA");
    CHECK_RC(tapi_file_read_ta(rpcs->ta, rpath.ptr, &contents));

    TEST_STEP("Check the expected contents");
    if (strcmp(contents, expected) != 0)
    {
        TEST_VERDICT("Unexpected contents of the file: '%s' vs '%s'",
                     contents, expected);
    }

    TEST_SUCCESS;

cleanup:

    free(contents);
    CLEANUP_CHECK_RC(rcf_ta_del_file(rpcs->ta, 0, rpath.ptr));
    te_string_free(&rpath);

    TEST_END;
}
