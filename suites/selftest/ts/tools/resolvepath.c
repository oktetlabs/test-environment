/** @file
 * @brief Test for a te_file_resolve_pathname() function
 *
 * Testing te_file_resolve_pathname() correctness
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 */

/** @page tools_resolvepath te_file_resolve_pathname test
 *
 * @objective Testing te_readlink_fmt correctness
 *
 * Test that a filename is properly resolved by te_file_resolve_pathname().
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/resolvepath"

#include "te_config.h"

#include <unistd.h>

#include "tapi_test.h"
#include "te_str.h"
#include "te_file.h"

static void
check_file(const char *file, const char *path, int mode, const char *basename,
           const char *expected)
{
    char *resolved = NULL;

    CHECK_RC(te_file_resolve_pathname(file, path, mode, basename, NULL));
    CHECK_RC(te_file_resolve_pathname(file, path, mode, basename, &resolved));
    CHECK_NOT_NULL(resolved);
    if (strcmp(resolved, expected) != 0)
    {
        TEST_VERDICT("'%s' is resolved to '%s', but expected '%s'", file,
                     resolved, expected);
    }
    free(resolved);
}


static void
check_nofile(const char *file, const char *path, int mode, const char *basename,
             unsigned expected_rc)
{
    te_errno rc;
    char *resolved = NULL;

    rc = te_file_resolve_pathname(file, path, mode, basename, NULL);
    if (TE_RC_GET_ERROR(rc) != expected_rc)
    {
        TEST_VERDICT("Unexpected status for '%s', expected %r, got %r",
                     file, expected_rc, rc);
    }
    rc = te_file_resolve_pathname(file, path, mode, basename, &resolved);
    if (TE_RC_GET_ERROR(rc) != expected_rc)
    {
        TEST_VERDICT("Unexpected status for '%s', expected %r, got %r",
                     file, expected_rc, rc);
    }
    if (resolved != NULL)
    {
        TEST_VERDICT("'%s' resolved to '%s' despite being absent",
                     file, resolved);
    }
}


int
main(int argc, char **argv)
{
    const char *path_env = getenv("PATH");
    char *augmented_path = te_str_concat(path_env, ":/tmp");
    char *tmpfile = NULL;
    char *tmp_basename = NULL;

    TEST_START;

    TEST_STEP("Testing absolute executable filename resolving");
    check_file("/bin/sh", path_env, X_OK, NULL, "/bin/sh");

    TEST_STEP("Testing relative executable filename resolving");
    check_file("sh", path_env, X_OK, NULL, "/bin/sh");

    CHECK_NOT_NULL(tmpfile = te_file_create_unique("/tmp/te_resolve_XXXXXX",
                                                   NULL));
    CHECK_NOT_NULL(tmp_basename = te_basename(tmpfile));

    TEST_STEP("Testing absolute filename resolving");
    check_file(tmpfile, augmented_path, F_OK, NULL, tmpfile);

    TEST_STEP("Testing relative filename resolving");
    check_file(tmpfile, augmented_path, F_OK, NULL, tmpfile);

    TEST_STEP("Testing absolute filename resolving with basename");
    check_file(tmpfile, path_env, F_OK, "/tmp", tmpfile);
    check_file(tmpfile, path_env, F_OK, tmpfile, tmpfile);
    check_file(tmpfile, NULL, F_OK, "/tmp", tmpfile);

    TEST_STEP("Testing relative filename resolving with basename");
    check_file(tmp_basename, path_env, F_OK, "/tmp", tmpfile);
    check_file(tmp_basename, path_env, F_OK, tmpfile, tmpfile);
    check_file(tmp_basename, NULL, F_OK, "/tmp", tmpfile);

    TEST_STEP("Testing absolute non-executable filename resolving");
    check_nofile(tmpfile, augmented_path, X_OK, NULL, TE_EACCES);
    check_nofile(tmpfile, path_env, X_OK, tmpfile, TE_EACCES);

    TEST_STEP("Testing relative non-executable filename resolving");
    check_nofile(tmp_basename, augmented_path, X_OK, NULL, TE_EACCES);
    check_nofile(tmp_basename, path_env, X_OK, tmpfile, TE_ENOENT);

    unlink(tmpfile);

    TEST_STEP("Testing absolute non-existing filename resolving");
    check_nofile(tmpfile, augmented_path, F_OK, NULL, TE_ENOENT);
    check_nofile(tmpfile, path_env, F_OK, tmpfile, TE_ENOENT);

    TEST_STEP("Testing relative non-existing filename resolving");
    check_nofile(tmp_basename, augmented_path, F_OK, NULL, TE_ENOENT);
    check_nofile(tmp_basename, path_env, F_OK, tmpfile, TE_ENOENT);

    TEST_SUCCESS;

cleanup:
    free(tmp_basename);
    if (tmpfile != NULL)
    {
        unlink(tmpfile);
        free(tmpfile);
    }
    free(augmented_path);

    TEST_END;
}
