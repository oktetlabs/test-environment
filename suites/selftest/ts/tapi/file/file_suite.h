/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC Test Suite
 *
 * Common definitions for rpc/file test suite.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __FILE_SUITE_H__
#define __FILE_SUITE_H__

#ifndef TEST_START_VARS
/**
 * Test suite specific variables of the test @b main() function.
 */
#define TEST_START_VARS TEST_START_ENV_VARS
#endif

#ifndef TEST_START_SPECIFIC
/**
 * Test suite specific the first actions of the test.
 */
#define TEST_START_SPECIFIC TEST_START_ENV
#endif

#ifndef TEST_END_SPECIFIC
/**
 * Test suite specific part of the last action of the test @b main()
 * function.
 */
#define TEST_END_SPECIFIC TEST_END_ENV
#endif

#include <string.h>

#include "te_defs.h"
#include "te_bufs.h"
#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_file.h"
#include "tapi_mem.h"
#include "tapi_rpc_dirent.h"
#include "tapi_rpc_unistd.h"

#define BUFSIZE 64

static inline void
file_check_exist(rcf_rpc_server *pco, const char *filename)
{
    RPC_AWAIT_ERROR(pco);
    if (rpc_access(pco, filename, RPC_F_OK) != 0)
    {
        if (RPC_ERRNO(pco) != RPC_ENOENT)
            TEST_VERDICT("%s(): Unexpected error %r", __func__, RPC_ERRNO(pco));

        ERROR_VERDICT("The expected file does not exist");
        TEST_FAIL("File '%s' does not exist on %s", filename, pco->ta);
    }
}

static inline void
file_check_not_exist(rcf_rpc_server *pco, const char *filename)
{
    RPC_AWAIT_ERROR(pco);
    if (rpc_access(pco, filename, RPC_F_OK) == 0)
    {
        ERROR_VERDICT("The file still exists");
        TEST_FAIL("File '%s' exists on %s", filename, pco->ta);
    }

    if (RPC_ERRNO(pco) != RPC_ENOENT)
        TEST_VERDICT("%s(): Unexpected error %r", __func__, RPC_ERRNO(pco));
}

static inline void
file_compare_and_fail(const char *exp_buf, const char *actual_buf)
{
    if (!te_compare_bufs(exp_buf, strlen(exp_buf) + 1, 1,
                         actual_buf, strlen(actual_buf) + 1,
                         TE_LL_ERROR))
    {
        TEST_VERDICT("Buffers do not match");
    }
}

#endif /* __FILE_SUITE_H__ */
