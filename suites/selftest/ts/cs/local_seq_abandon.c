/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Test Environment
 *
 * Leave a local command sequence open in Configurator
 */

/** @page local_seq_abandon Leave a local command sequence open
 *
 * @objective Fail with a local command sequence open, so that
 *            @ref local_seq_check can check that Configurator rolls
 *            it back.
 *
 * The test fails between a local change and its commit, as a test does
 * when something goes wrong in the middle of a configuration change. It
 * is expected to fail. Before the local change it makes a committed one,
 * which Tester has to undo through Configurator.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "cs/local_seq_abandon"

#include "te_config.h"

#include "tapi_test.h"
#include "conf_api.h"

/** Environment variable which is added and committed */
#define LOCAL_SEQ_COMMITTED "TE_LOCAL_SEQ_COMMITTED"

/** Environment variable which is added locally and not committed */
#define LOCAL_SEQ_LOCAL "TE_LOCAL_SEQ_LOCAL"

int
main(int argc, char *argv[])
{
    const char *ta;

    TEST_START;

    TEST_PARAM_DOC(ta,
                   "Test Agent whose environment variables the test adds");
    TEST_GET_STRING_PARAM(ta);

    TEST_STEP("Add an environment variable");
    CHECK_RC(cfg_add_instance_fmt(NULL, CFG_VAL(STRING, "1"),
                                  "/agent:%s/env:%s", ta,
                                  LOCAL_SEQ_COMMITTED));

    TEST_STEP("Add another environment variable locally, which starts "
              "a local command sequence");
    CHECK_RC(cfg_add_instance_local_fmt(NULL, CFG_VAL(STRING, "1"),
                                        "/agent:%s/env:%s", ta,
                                        LOCAL_SEQ_LOCAL));

    TEST_STEP("Fail without committing the local change");
    TEST_FAIL("The test fails with a local command sequence open");

cleanup:
    TEST_END;
}
