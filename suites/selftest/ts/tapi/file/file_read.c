/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Read file from Agent.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page file_read Test for reading file
 *
 * @objective Demo of TAPI/RPC file reading test
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "file_read"

#include "file_suite.h"

int
main(int argc, char **argv)
{
    char           *rfile = NULL;
    char           *buf = NULL;
    rcf_rpc_server *pco_iut = NULL;
    int             fd = -1;
    char           *data = NULL;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Create file with content on TA");
    data = te_make_printable_buf_by_len(BUFSIZE);
    rfile = tapi_strdup(tapi_file_generate_name());
    fd = rpc_open(pco_iut, rfile, RPC_O_WRONLY | RPC_O_CREAT, 0);

    CHECK_LENGTH(rpc_write_and_close(pco_iut, fd, data, BUFSIZE - 1),
                 BUFSIZE - 1);

    TEST_STEP("Read content from the file on TA");
    if (tapi_file_read_ta(pco_iut->ta, rfile, &buf) != 0)
    {
        TEST_VERDICT("tapi_file_read_ta() failed");
    }

    TEST_STEP("Check data");
    file_compare_and_fail(data, buf);

    TEST_SUCCESS;

cleanup:

    RING("Delete the file from TA");
    CLEANUP_CHECK_RC(tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", rfile));

    free(buf);
    free(data);
    free(rfile);

    TEST_END;
}
