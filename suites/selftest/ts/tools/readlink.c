/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test for a te_readlink_fmt() function
 *
 * Testing te_readlink_fmt correctness
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page tools_readlink te_readlink_fmt test
 *
 * @objective Testing te_readlink_fmt correctness
 *
 * Produce an overlong symlink and ensure that te_readlink_fmt
 * can cope with it
 *
 * @par Test sequence:
 *
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "readlink"

#include "te_config.h"

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "tapi_test.h"
#include "te_file.h"

int
main(int argc, char **argv)
{
    char linkpath[] = "/tmp/te_link_XXXXXX";
    size_t max_name;
    char *target_path = NULL;
    char *link_contents = NULL;

    TEST_START;

    TEST_STEP("Generate a unique symlink name");
    /* mktemp is deprecated and may produce a compiler
     * warning, so we use an obviate way to generate
     * a unique direcory, remove it and the re-use the
     * name --- not ideal, but sufficient for the present
     * case
     */
    CHECK_NOT_NULL(mkdtemp(linkpath));
    CHECK_RC(rmdir(linkpath));

    TEST_STEP("Detect the maximum length of a filename");
    errno = 0;
    max_name = (size_t)pathconf("/tmp", _PC_NAME_MAX);
    if (max_name == (size_t)-1)
    {
        if (errno != 0)
            TEST_FAIL("Cannot detect max path length: %s", strerror(errno));
        max_name = _POSIX_PATH_MAX;
        WARN("undefined max path length, using POSIX default %zu", max_name);
    }

    TEST_STEP("Create a temporary file with a very long name");
    CHECK_NOT_NULL((target_path =
                    te_file_create_unique("/tmp/te_file_%0*d", NULL,
                                          (int)(max_name -
                                                sizeof("te_file_XXXXXX") +
                                                1), 1)));

    TEST_STEP("Do symlink");
    CHECK_RC(symlink(target_path, linkpath));

    TEST_STEP("Read the contents of symlink and validate it");
    CHECK_NOT_NULL((link_contents = te_readlink_fmt("%s", linkpath)));
    if (strcmp(link_contents, target_path) != 0)
    {
        TEST_VERDICT("Link read as '%s', but expected '%s'", link_contents,
                     target_path);
    }

    TEST_SUCCESS;

cleanup:
    free(link_contents);
    unlink(linkpath);
    if (target_path != NULL)
    {
        unlink(target_path);
        free(target_path);
    }

    TEST_END;
}
