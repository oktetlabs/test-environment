/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Resolve a relative pathname on the agent.
 *
 * Copyright (C) 2023 OKTET Labs Ltd. All rights reserved.
 */

/** @page file_resolve Test for pathname resolving
 *
 * @objective Ensure the correctness of pathname resolving.
 *
 * @param len    length of buffer for file content in bytes
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "file_resolve"

#include "te_str.h"
#include "file_suite.h"

int
main(int argc, char **argv)
{
    char           *lfile = NULL;
    te_string rfile = TE_STRING_INIT;
    te_string absfile = TE_STRING_INIT;
    char           *absfile1 = NULL;
    rcf_rpc_server *pco_iut = NULL;
    size_t          len = 0;
    void           *buf = NULL;
    char           *agent_dir = NULL;
    char           *content = NULL;

    TEST_START;
    TEST_GET_UINT_PARAM(len);
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Prepare a file");
    buf = te_make_printable_buf_by_len(len);
    CHECK_NOT_NULL(lfile = tapi_file_create(len, buf, false));

    tapi_file_make_name(&rfile);
    if ((rc = tapi_file_copy_ta(NULL, lfile, pco_iut->ta, rfile.ptr)) != 0)
    {
        TEST_VERDICT("rcf_ta_put_file() failed; errno=%r", rc);
    }

    TEST_STEP("Resolve a relative filename");
    agent_dir = tapi_cfg_base_get_ta_dir(pco_iut->ta, TAPI_CFG_BASE_TA_DIR_TMP);
    CHECK_NOT_NULL(agent_dir);

    CHECK_NOT_NULL(tapi_file_resolve_ta_pathname(&absfile, pco_iut->ta,
                                                 TAPI_CFG_BASE_TA_DIR_TMP,
                                                 rfile.ptr));
    if (te_str_strip_prefix(absfile.ptr, agent_dir) == NULL)
    {
        ERROR("'%s' does not start with '%s'", absfile.ptr, agent_dir);
        TEST_VERDICT("Relative name improperly resolved");
    }

    TEST_STEP("Resolve an absolute filename");
    absfile1 = tapi_file_resolve_ta_pathname(NULL, pco_iut->ta,
                                             TAPI_CFG_BASE_TA_DIR_AGENT,
                                             absfile.ptr);
    CHECK_NOT_NULL(absfile1);
    if (strcmp(absfile.ptr, absfile1) != 0)
    {
        ERROR("'%s' != '%s'", absfile.ptr, absfile1);
        TEST_VERDICT("Absolute name is resolved as relative");
    }

    TEST_STEP("Verify the file contents");
    CHECK_RC(tapi_file_read_ta(pco_iut->ta, absfile1, &content));
    if (strcmp(buf, content) != 0)
    {
        ERROR("'%s' != '%s'", buf, content);
        TEST_VERDICT("The read back file is different from the original");
    }


    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", rfile.ptr));

    if (unlink(lfile) != 0)
    {
        ERROR("File '%s' is not deleted", lfile);
    }

    free(lfile);
    te_string_free(&rfile);
    te_string_free(&absfile);
    free(absfile1);
    free(buf);
    free(content);
    free(agent_dir);

    TEST_END;
}
