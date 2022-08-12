/** @file
 * @brief RPC Test Suite
 *
 * Copy file from Agent A to Agent B.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

/** @page file_copy Test for remote file copy
 *
 * @objective Demo of TAPI/RPC file repote copy test
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "file_copy"

#include "file_suite.h"

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_src = NULL;
    rcf_rpc_server *pco_dst = NULL;
    const char     *src = NULL;
    const char     *dst = NULL;
    te_string       src_path = TE_STRING_INIT;
    te_string       dst_path = TE_STRING_INIT;
    int             fd = -1;
    uint8_t        *data = NULL;
    size_t          data_size = BUFSIZE;

    TEST_START;
    TEST_GET_RPCS(AGT_A, "pco_src", pco_src);
    TEST_GET_RPCS(AGT_B, "pco_dst", pco_dst);

    TEST_STEP("Generate a file on TA");
    data = te_make_buf_by_len(BUFSIZE);
    src = tapi_file_generate_name();
    CHECK_RC(te_string_append(&src_path, "%s/%s", TMP_DIR, src));
    fd = rpc_open(pco_src, src_path.ptr, RPC_O_WRONLY | RPC_O_CREAT, 0);
    WRITE_WHOLE_BUF(pco_src, fd, data, data_size);
    RPC_AWAIT_ERROR(pco_src);
    if (rpc_close(pco_src, fd) != 0)
    {
        TEST_VERDICT("rpc_close() failed");
    }

    TEST_STEP("Copy the file from TA to TA");
    dst = tapi_file_generate_name();
    CHECK_RC(te_string_append(&dst_path, "%s/%s", TMP_DIR, dst));
    if (tapi_file_copy_ta(pco_src->ta, src_path.ptr,
                          pco_dst->ta, dst_path.ptr) != 0)
    {
        TEST_VERDICT("tapi_file_copy_ta() failed");
    }

    TEST_STEP("Check if the file exists on TA");
    RPC_AWAIT_ERROR(pco_dst);
    if (rpc_access(pco_dst, dst_path.ptr, RPC_F_OK) != 0)
    {
        TEST_VERDICT("File doesn't exist on TA");
    }

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(rcf_ta_del_file(pco_src->ta, 0, src_path.ptr));
    CLEANUP_CHECK_RC(rcf_ta_del_file(pco_dst->ta, 0, dst_path.ptr));

    free(data);
    te_string_free(&src_path);
    te_string_free(&dst_path);

    TEST_END;
}
