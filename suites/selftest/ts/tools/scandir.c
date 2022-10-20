/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for a te_file_scandir() function
 *
 * Testing te_file_scandir() correctness
 */

/** @page tools_scandir te_file_scandir test
 *
 * @objective Testing te_file_scandir() correctness.
 *
 * Test that file name are processed properly by te_file_scandir().
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/scandir"

#include "te_config.h"

#include <unistd.h>
#include <fnmatch.h>

#include "tapi_test.h"
#include "te_str.h"
#include "te_file.h"

typedef struct match_data {
    const char *dirname;
    unsigned int count;
} match_data;

static te_errno
check_match(const char *pattern, const char *pathname, void *data)
{
    match_data *m = data;
    size_t dirlen = strlen(m->dirname);

    if (strncmp(pathname, m->dirname, dirlen) != 0 ||
        pathname[dirlen] != '/')
    {
        ERROR("'%s' does not start with '%s'", pathname, m->dirname);
        return TE_E_INVALID_NAME;
    }

    if (fnmatch(pattern, pathname + dirlen + 1,
                FNM_PATHNAME | FNM_PERIOD) != 0)
    {
        ERROR("'%s' does not match '%s'", pathname, pattern);
        return TE_E_BAD_PATHNAME;
    }
    m->count++;

    return 0;
}

static te_errno
do_remove(const char *pattern, const char *pathname, void *data)
{
    UNUSED(pattern);
    UNUSED(data);

    if (unlink(pathname) == 0)
        return 0;

    return TE_OS_RC(TE_MODULE_NONE, errno);
}

static void
check_scandir(unsigned int n_files,
              const char *prefix, const char *suffix,
              const char *nomatch_prefix, const char *nomatch_suffix,
              const char *pattern)
{
    char tmpdir[] = "/tmp/te_scandir_XXXXXX";
    unsigned int i;
    match_data data = {tmpdir, 0};

    CHECK_NOT_NULL(mkdtemp(tmpdir));
    for (i = 0; i < n_files; i++)
    {
        char *fname;

        CHECK_NOT_NULL(fname = te_file_create_unique("%s/%s",
                                                     suffix, tmpdir, prefix));
        free(fname);

        CHECK_NOT_NULL(fname = te_file_create_unique("%s/%s",
                                                     nomatch_suffix, tmpdir,
                                                     nomatch_prefix));
        free(fname);
    }

    CHECK_RC(te_file_scandir(tmpdir, check_match, &data, "%s", pattern));
    if (data.count != n_files)
    {
        TEST_VERDICT("%u files should match '%s', but %u reported", n_files,
                     pattern, data.count);
    }

    CHECK_RC(te_file_scandir(tmpdir, do_remove, NULL, NULL));
    CHECK_RC(rmdir(tmpdir));
}

int
main(int argc, char **argv)
{
    unsigned int n_files;
    TEST_START;

    TEST_GET_UINT_PARAM(n_files);

    TEST_STEP("Checking pathnames with different suffices");
    check_scandir(n_files, "", ".json", "", ".c", "*.json");

    TEST_STEP("Checking pathnames with different prefixes");
    check_scandir(n_files, "prefix", ".json", "badprefix", ".json",
                  "prefix*.json");

    TEST_STEP("Checking hidden pathnames");
    check_scandir(n_files, "", "", ".", "", "*");
    check_scandir(n_files, ".", "", "", "", "*");

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
