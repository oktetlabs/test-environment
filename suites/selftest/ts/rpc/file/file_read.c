/** @file
 * @brief RPC Test Suite
 *
 * Read file from Agent.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

/** @page file_read Test for reading file
 *
 * @objective Demo of TAPI/RPC file reading test
 *
 * @par Scenario:
 *
 * @author Eugene Suslov <Eugene.Suslov@oktetlabs.ru>
 */

#define TE_TEST_NAME    "file_read"

#include "file_suite.h"

int
main(int argc, char **argv)
{
    char           *rfile = NULL;
    uint8_t        *buf = NULL;
    rcf_rpc_server *rpcs = NULL;
    int             fd = -1;
    uint8_t        *data = NULL;
    char           *data_str = NULL;
    const size_t    data_size = BUFSIZE;

    TEST_START;
    TEST_GET_RPCS(AGT_A, "rpcs", rpcs);

    TEST_STEP("Create file with content on TA");
    data = te_make_buf_by_len(BUFSIZE);
    rfile = tapi_file_generate_name();
    RPC_AWAIT_ERROR(rpcs);
    fd = rpc_open(rpcs, rfile, RPC_O_WRONLY | RPC_O_CREAT, 0);
    if (fd == -1)
    {
        TEST_VERDICT("rpc_open() for writing data failed");
    }
    WRITE_WHOLE_BUF(rpcs, fd, data, data_size);
    RPC_AWAIT_ERROR(rpcs);
    if (rpc_close(rpcs, fd) != 0)
    {
        TEST_VERDICT("rpc_close() failed");
    }

    TEST_STEP("Read content from the file on TA");
    if (tapi_file_read_ta(rpcs->ta, rfile, (void *)&buf) != 0)
    {
        TEST_VERDICT("tapi_file_read_ta() failed");
    }

    TEST_STEP("Print data");
    TEST_SUBSTEP("Print expected data");
    data_str = raw2string(buf, BUFSIZE);
    RING("%s", data_str);
    free(data_str);

    TEST_SUBSTEP("Print received data");
    data_str = raw2string(data, data_size);
    RING("%s", data_str);
    free(data_str);

    TEST_STEP("Check if the buffer matches initial data");
    if (memcmp(data, buf, data_size) != 0)
    {
        TEST_VERDICT("Received data doesn't match");
    }

    TEST_SUCCESS;

cleanup:

    RING("Delete the file from TA");
    CLEANUP_CHECK_RC(rcf_ta_del_file(rpcs->ta, 0, rfile));

    free(buf);
    free(data);

    TEST_END;
}
