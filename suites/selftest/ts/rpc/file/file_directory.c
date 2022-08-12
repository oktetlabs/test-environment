/** @file
 * @brief RPC Test Suite
 *
 * Create and delete directory with files on Agent.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

/** @page file_directory Test for directory creation and deletion
 *
 * @objective Demo of TAPI/RPC directory creation and deletion test
 *
 * @param number    number of files in created directory
 *
 * @par Scenario:
 *
 * @author Eugene Suslov <Eugene.Suslov@oktetlabs.ru>
 */

#define TE_TEST_NAME    "file_directory"

#include "file_suite.h"

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
        if (tapi_file_create_ta(rpcs->ta, filename.ptr, "") != 0)
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
    RPC_AWAIT_ERROR(rpcs);
    rpc_dir_p   dirp = rpc_opendir(rpcs, path);
    rpc_dirent *dent = NULL;

    while (TRUE)
    {
        RPC_AWAIT_ERROR(rpcs);
        dent = rpc_readdir(rpcs, dirp);
        if (dent == NULL)
            break;

        if (strcmp(dent->d_name, ".") != 0 && strcmp(dent->d_name, "..") != 0)
        {
            tapi_file_ta_unlink_fmt(rpcs->ta, "%s/%s", path, dent->d_name);
        }
        free(dent);
    }

    RPC_AWAIT_ERROR(rpcs);
    return rpc_closedir(rpcs, dirp);
}

int
main(int argc, char **argv)
{
    char           *rdir = NULL;
    te_string       rpath = TE_STRING_INIT;
    rcf_rpc_server *rpcs = NULL;
    size_t          nfiles = 0;

    TEST_START;
    TEST_GET_RPCS(AGT_A, "rpcs", rpcs);
    TEST_GET_UINT_PARAM(nfiles);

    TEST_STEP("Create a directory on TA");
    rdir = tapi_file_generate_name();
    CHECK_RC(te_string_append(&rpath, "%s/%s", TMP_DIR, rdir));
    RPC_AWAIT_ERROR(rpcs);
    if (rpc_mkdir(rpcs, rpath.ptr, 0) != 0)
    {
        TEST_VERDICT("rpc_mkdir() failed", rpath.ptr);
    }

    TEST_STEP("Create files in the directory");
    if (create_files(nfiles, rpcs, rpath.ptr) != nfiles)
    {
        TEST_VERDICT("Files aren't created");
    }

    TEST_SUCCESS;

cleanup:

    TEST_STEP("Remove the directory");
    if (remove_files(rpcs, rpath.ptr) != 0)
    {
        TEST_VERDICT("Directory isn't removed");
    }
    RPC_AWAIT_ERROR(rpcs);
    if (rpc_rmdir(rpcs, rpath.ptr) != 0)
    {
        TEST_VERDICT("rpc_rmdir() failed");
    }

    TEST_STEP("Check if the directory doesn't exist");
    RPC_AWAIT_ERROR(rpcs);
    if (rpc_access(rpcs, rpath.ptr, RPC_F_OK) == 0)
    {
        TEST_VERDICT("Directory still exists on TA");
    }

    te_string_free(&rpath);

    TEST_END;
}
