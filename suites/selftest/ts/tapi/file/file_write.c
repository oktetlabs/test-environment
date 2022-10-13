/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Write file to Agent.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page file_write Test for writing file
 *
 * @objective Demo of TAPI/RPC file writing test
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "file_write"

#include "file_suite.h"

int
main(int argc, char **argv)
{
    te_string rfile = TE_STRING_INIT;
    char           *buf = NULL;
    rcf_rpc_server *pco_iut = NULL;
    int             fd = -1;
    char           *data = NULL;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Write data to a file on TA");
    data = te_make_printable_buf_by_len(BUFSIZE);
    tapi_file_make_name(&rfile);
    if (tapi_file_create_ta(pco_iut->ta, rfile.ptr, "%s", data) != 0)
    {
        TEST_VERDICT("tapi_file_create_ta() failed");
    }

    TEST_STEP("Read content from the file on TA");
    buf = tapi_calloc(1, BUFSIZE);
    fd = rpc_open(pco_iut, rfile.ptr, RPC_O_RDONLY, 0);
    /* terminating zero is provided by tapi_calloc */
    CHECK_LENGTH(rpc_read(pco_iut, fd, buf, BUFSIZE - 1), BUFSIZE - 1);
    rpc_close(pco_iut, fd);

    TEST_STEP("Check data");
    file_compare_and_fail(data, buf);

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", rfile.ptr));

    free(buf);
    free(data);
    te_string_free(&rfile);

    TEST_END;
}
