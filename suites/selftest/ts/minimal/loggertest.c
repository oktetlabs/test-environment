/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Minimal test
 *
 * Check formatting of messages produced by a Test Agent.
 *
 */

/** @page minimal_loggertest Logger test
 *
 * @objective Check that the @c %z format is handled in messages produced
 *            inside a Test Agent.
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME    "loggertest"

#include "te_config.h"
#include "rcf_api.h"
#include "tapi_cfg.h"
#include "tapi_test.h"
#include "te_str.h"

static te_errno
ta_size_fmt_check(const char *ta, void *cookie)
{
    char *value = NULL;
    uintmax_t size_max;

    UNUSED(cookie);

    CHECK_RC(cfg_get_string(&value, "/agent:%s/ts_lib_size_max:", ta));
    CHECK_RC(te_strtoumax(value, 10, &size_max));

    RING("Expected TA log message from %s: "
         "'TA %%zi format check: -1'", ta);
    RING("Expected TA log message from %s: "
         "'TA %%zd format check: -1'", ta);
    RING("Expected TA log message from %s: "
         "'TA %%zo format check: %jo'", ta, size_max);
    RING("Expected TA log message from %s: "
         "'TA %%zu format check: %ju'", ta, size_max);
    RING("Expected TA log message from %s: "
         "'TA %%zx format check: %jx'", ta, size_max);
    RING("Expected TA log message from %s: "
         "'TA %%zX format check: %jX'", ta, size_max);

    free(value);

    return 0;
}

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Check %%z formatting in messages produced by Test Agents");

    CHECK_RC(rcf_foreach_ta(ta_size_fmt_check, NULL));

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
