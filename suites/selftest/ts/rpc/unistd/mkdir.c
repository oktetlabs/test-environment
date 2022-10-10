/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Create and delete directory with files on Agent.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page mkdir Test for directory creation and deletion
 *
 * @objective Demo of TAPI/RPC directory creation and deletion test
 *
 * @param number    number of files in created directory
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "mkdir"

#include "unistd_suite.h"

static inline size_t
create_files(size_t nfiles, rcf_rpc_server *rpcs, const char *path)
{
    te_string filename = TE_STRING_INIT_RESERVE(RCF_MAX_PATH);
    size_t    i = 0;
    size_t    ncreated = 0;

    for (i = 0; i < nfiles; i++)
    {
        te_string_reset(&filename);
        CHECK_RC(te_string_append(&filename, "%s/%zu", path, i));
        if (tapi_file_create_ta(rpcs->ta, filename.ptr, "%s", "") != 0)
            break;
        ncreated++;
        filename.len = 0;
    }
    te_string_free(&filename);

    return ncreated;
}

static inline int
remove_files(rcf_rpc_server *rpcs, const char *path)
{
    rpc_dir_p   dirp = rpc_opendir(rpcs, path);
    rpc_dirent *dent = NULL;

    while (TRUE)
    {
        dent = rpc_readdir(rpcs, dirp);
        if (dent == NULL)
            break;

        if (strcmp(dent->d_name, ".") != 0 && strcmp(dent->d_name, "..") != 0)
        {
            tapi_file_ta_unlink_fmt(rpcs->ta, "%s/%s", path, dent->d_name);
        }
        free(dent);
    }

    return rpc_closedir(rpcs, dirp);
}

int
main(int argc, char **argv)
{
    char           *rdir = NULL;
    rcf_rpc_server *pco_iut = NULL;
    size_t          nfiles = 0;

    TEST_START;
    TEST_GET_PCO(pco_iut);
    TEST_GET_UINT_PARAM(nfiles);

    TEST_STEP("Create a directory on TA");
    rdir = strdup(tapi_file_generate_name());
    rpc_mkdir(pco_iut, rdir, 0);

    TEST_STEP("Create files in the directory");
    if (create_files(nfiles, pco_iut, rdir) != nfiles)
        TEST_VERDICT("Files aren't created");

    TEST_SUCCESS;

cleanup:

    TEST_STEP("Remove the directory");
    if (remove_files(pco_iut, rdir) != 0)
        TEST_VERDICT("Directory isn't removed");

    rpc_rmdir(pco_iut, rdir);

    TEST_STEP("Check if the directory is deleted");
    RPC_AWAIT_ERROR(pco_iut);
    if (rpc_access(pco_iut, rdir, RPC_F_OK) == 0)
    {
        TEST_VERDICT("The removed directory still exists");
    }
    else if (RPC_ERRNO(pco_iut) != RPC_ENOENT)
    {
        TEST_VERDICT("access() failed with an unexpected error: %r",
                     RPC_ERRNO(pco_iut));
    }

    TEST_END;
}
