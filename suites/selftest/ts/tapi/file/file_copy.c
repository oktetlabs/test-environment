/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Copy file from Agent A to Agent B.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
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
    rcf_rpc_server *pco_iut = NULL;
    rcf_rpc_server *pco_tst = NULL;
    char     *src = NULL;
    char     *dst = NULL;
    int             fd = -1;
    uint8_t        *data = NULL;
    size_t          data_size = BUFSIZE;

    TEST_START;
    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_tst);

    TEST_STEP("Generate a file on TA");
    data = te_make_buf_by_len(BUFSIZE);
    src = strdup(tapi_file_generate_name());
    fd = rpc_open(pco_iut, src, RPC_O_WRONLY | RPC_O_CREAT, 0);
    CHECK_LENGTH(rpc_write_and_close(pco_iut, fd, data, data_size), data_size);

    TEST_STEP("Copy the file from TA to TA");
    dst = strdup(tapi_file_generate_name());
    if (tapi_file_copy_ta(pco_iut->ta, src, pco_tst->ta, dst) != 0)
    {
        TEST_VERDICT("tapi_file_copy_ta() failed");
    }

    TEST_STEP("Check if the file exists on TA");
    file_check_exist(pco_tst, dst);

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", src));
    CLEANUP_CHECK_RC(tapi_file_ta_unlink_fmt(pco_tst->ta, "%s", dst));

    free(data);
    free(src);
    free(dst);

    TEST_END;
}
