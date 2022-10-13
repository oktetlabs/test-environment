/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Create file on Agent.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page file_create_delete Test for file creation
 *
 * @objective Demo of TAPI/RPC file creation test
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "file_create_delete"

#include "file_suite.h"

int
main(int argc, char **argv)
{
    te_string filename = TE_STRING_INIT;
    rcf_rpc_server *pco_iut = NULL;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Create a file on TA");
    tapi_file_make_name(&filename);
    if (tapi_file_create_ta(pco_iut->ta, filename.ptr, "%s", "") != 0)
    {
        TEST_VERDICT("tapi_file_create_ta() failed");
    }

    TEST_STEP("Check if the file exists");
    file_check_exist(pco_iut, filename.ptr);

    TEST_STEP("Delete the file from TA");
    if (tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", filename.ptr) != 0)
    {
        TEST_VERDICT("tapi_file_ta_unlink_fmt() failed");
    }

    TEST_STEP("Check if the file is really deleted");
    file_check_not_exist(pco_iut, filename.ptr);

    TEST_SUCCESS;

cleanup:

    te_string_free(&filename);
    TEST_END;
}
