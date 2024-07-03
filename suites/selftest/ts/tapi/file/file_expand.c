/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Create file on Agent.
 *
 * Copyright (C) 2023 OKTET Labs Ltd. All rights reserved.
 */

/** @page file_expand Test for templated file generation
 *
 * @objective Demo of TAPI/RPC file creation test
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "file_expand"

#include "file_suite.h"
#include "te_file.h"

int
main(int argc, char **argv)
{
    te_string filename = TE_STRING_INIT;
    te_string local = TE_STRING_INIT;
    rcf_rpc_server *pco_iut = NULL;
    const char *template;
    const char *varname;
    const char *value;
    const char *expected;
    te_kvpair_h kvpairs;
    char *remote = NULL;
    const char *tmpdir = getenv("TMPDIR");

    if (tmpdir == NULL)
        tmpdir = "/tmp";
    te_kvpair_init(&kvpairs);

    TEST_START;
    TEST_GET_STRING_PARAM(template);
    TEST_GET_STRING_PARAM(varname);
    TEST_GET_OPT_STRING_PARAM(value);
    TEST_GET_OPT_STRING_PARAM(expected);
    TEST_GET_PCO(pco_iut);

    CHECK_RC(te_kvpair_add(&kvpairs, varname, "%s",
                           te_str_empty_if_null(value)));
    tapi_file_make_name(&filename);

    TEST_STEP("Testing remote file expansion");
    CHECK_RC(tapi_file_expand_kvpairs(pco_iut->ta, template, NULL, &kvpairs,
                                      "%s", filename.ptr));
    CHECK_RC(tapi_file_read_ta(pco_iut->ta, filename.ptr, &remote));
    if (strcmp(expected, remote) != 0)
    {
        ERROR("Expected content: '%s', actual '%s'", expected, remote);
        TEST_VERDICT("Unexpected remote expansion");
    }

    TEST_STEP("Testing local file expansion");
    CHECK_RC(tapi_file_expand_kvpairs(NULL, template, NULL, &kvpairs,
                                      "%s/%s", tmpdir, filename.ptr));
    CHECK_RC(te_file_read_string(&local, false, 0, "%s/%s", tmpdir,
                                 filename.ptr));
    if (strcmp(expected, local.ptr) != 0)
    {
        ERROR("Expected content: '%s', actual '%s'", expected, local.ptr);
        TEST_VERDICT("Unexpected local expansion");
    }

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", filename.ptr));
    CLEANUP_CHECK_RC(te_unlink_fmt("%s/%s", tmpdir, filename.ptr));
    free(remote);
    te_string_free(&filename);
    te_string_free(&local);
    te_kvpair_fini(&kvpairs);
    TEST_END;
}
