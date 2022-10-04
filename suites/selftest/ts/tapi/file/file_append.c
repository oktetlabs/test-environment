/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Append data to created file on Agent.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page file_append Test for append data to file
 *
 * @objective Demo of TAPI/RPC file append test
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "file_append"

#include "file_suite.h"

int
main(int argc, char **argv)
{
    char           *rfile = NULL;
    uint8_t        *buf = NULL;
    rcf_rpc_server *pco_iut = NULL;
    int             fd = -1;
    uint8_t        *data1 = NULL;
    uint8_t        *data2 = NULL;
    char           *data_str = NULL;
    const size_t    data_size = BUFSIZE + BUFSIZE;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    data1 = te_make_buf_by_len(BUFSIZE);
    data2 = te_make_buf_by_len(BUFSIZE);

    TEST_STEP("Create a file with content on TA");
    rfile = tapi_file_generate_name();
    fd = rpc_open(pco_iut, rfile, RPC_O_WRONLY | RPC_O_CREAT, 0);
    CHECK_LENGTH(rpc_write(pco_iut, fd, data1, BUFSIZE), BUFSIZE);
    rpc_close(pco_iut, fd);


    TEST_STEP("Append data to the file on TA");
    fd = rpc_open(pco_iut, rfile, RPC_O_WRONLY | RPC_O_APPEND, 0);
    CHECK_LENGTH(rpc_write(pco_iut, fd, data2, BUFSIZE), BUFSIZE);
    rpc_close(pco_iut, fd);

    TEST_STEP("Read content from the file on TA");
    buf = (uint8_t *)tapi_calloc(1, data_size);
    fd = rpc_open(pco_iut, rfile, RPC_O_RDONLY, 0);

    CHECK_LENGTH(rpc_read(pco_iut, fd, buf, data_size), data_size);
    rpc_close(pco_iut, fd);

    TEST_STEP("Print data");
    TEST_SUBSTEP("Print expected data");
    data_str = raw2string(data1, BUFSIZE);
    RING("initial: %s", data_str);
    free(data_str);
    data_str = raw2string(data2, BUFSIZE);
    RING("appended: %s", data_str);
    free(data_str);

    TEST_SUBSTEP("Print received data");
    data_str = raw2string(buf, data_size);
    RING("%s", data_str);
    free(data_str);

    TEST_STEP("Check if the buffer matches initial + appended data");
    if (memcmp(data1, buf, BUFSIZE) != 0 ||
        memcmp(data2, buf + BUFSIZE, BUFSIZE) != 0)
    {
        TEST_VERDICT("Written data doesn't match");
    }

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(rcf_ta_del_file(pco_iut->ta, 0, rfile));

    free(buf);
    free(data1);
    free(data2);

    TEST_END;
}
