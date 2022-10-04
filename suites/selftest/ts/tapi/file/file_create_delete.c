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
    char           *filename = NULL;
    rcf_rpc_server *pco_iut = NULL;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Create a file on TA");
    filename = tapi_file_generate_name();
    if (tapi_file_create_ta(pco_iut->ta, filename, "") != 0)
    {
        TEST_VERDICT("tapi_file_create_ta() failed");
    }

    TEST_STEP("Check if the file exists");
    RPC_AWAIT_ERROR(pco_iut);
    if (rpc_access(pco_iut, filename, RPC_F_OK) != 0)
    {
        TEST_VERDICT("File doesn't exist");
    }

    TEST_STEP("Delete the file from TA");
    if (tapi_file_ta_unlink_fmt(pco_iut->ta, filename) != 0)
    {
        TEST_VERDICT("tapi_file_ta_unlink_fmt() failed");
    }

    TEST_STEP("Check if the file doesn't exist");
    RPC_AWAIT_ERROR(pco_iut);
    if (rpc_access(pco_iut, filename, RPC_F_OK) == 0)
    {
        TEST_VERDICT("File exists");
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
