/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Get file from Agent.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
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
    te_string rfile = TE_STRING_INIT;
    rcf_rpc_server *pco_iut = NULL;
    char           *contents = NULL;
    static char     expected[] = "First Second";

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Create a file on TA");

    tapi_file_make_name(&rfile);
    CHECK_RC(tapi_file_append_ta(pco_iut->ta, rfile.ptr, "NULL"));
    CHECK_RC(tapi_file_create_ta(pco_iut->ta, rfile.ptr, "First"));
    CHECK_RC(tapi_file_append_ta(pco_iut->ta, rfile.ptr, " Second"));

    TEST_STEP("Get the file from TA");
    CHECK_RC(tapi_file_read_ta(pco_iut->ta, rfile.ptr, &contents));

    TEST_STEP("Check the expected contents");
    if (strcmp(contents, expected) != 0)
    {
        TEST_VERDICT("Unexpected contents of the file: '%s' vs '%s'",
                     contents, expected);
    }

    TEST_SUCCESS;

cleanup:

    free(contents);
    CLEANUP_CHECK_RC(tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", rfile.ptr));
    te_string_free(&rfile);

    TEST_END;
}
