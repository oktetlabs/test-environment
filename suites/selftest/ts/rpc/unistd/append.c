/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Append data to created file on Agent.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page append Test for append data to file
 *
 * @objective Demo of TAPI/RPC file append test
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "append"

#include "unistd_suite.h"

#define BUFSIZE 64

int
main(int argc, char **argv)
{
    te_string rfile = TE_STRING_INIT;
    uint8_t        *buf = NULL;
    rcf_rpc_server *pco_iut = NULL;
    int             fd = -1;
    uint8_t        *data = NULL;
    const size_t    data_size = BUFSIZE + BUFSIZE;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    data = te_make_buf_by_len(BUFSIZE);

    TEST_STEP("Create a file with content on TA");
    tapi_file_make_name(&rfile);
    fd = rpc_open(pco_iut, rfile.ptr, RPC_O_WRONLY | RPC_O_CREAT, 0);
    CHECK_LENGTH(rpc_write_and_close(pco_iut, fd, data, BUFSIZE), BUFSIZE);

    TEST_STEP("Append data to the file on TA");
    fd = rpc_open(pco_iut, rfile.ptr, RPC_O_WRONLY | RPC_O_APPEND, 0);
    CHECK_LENGTH(rpc_write_and_close(pco_iut, fd, data, BUFSIZE), BUFSIZE);

    TEST_STEP("Read content from the file on TA");
    buf = tapi_calloc(1, data_size);
    fd = rpc_open(pco_iut, rfile.ptr, RPC_O_RDONLY, 0);

    CHECK_LENGTH(rpc_read(pco_iut, fd, buf, data_size), data_size);
    rpc_close(pco_iut, fd);

    TEST_STEP("Check the data");
    unistd_compare_and_fail(data, BUFSIZE, 2, buf, data_size);

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", rfile.ptr));

    free(buf);
    free(data);
    te_string_free(&rfile);

    TEST_END;
}
