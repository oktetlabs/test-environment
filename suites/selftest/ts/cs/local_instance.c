/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Test Environment
 *
 * Check whether an instance exists only in the Configurator database
 */

/** @page local_instance Check whether an instance is only local
 *
 * @objective Check that Configurator tells whether an object instance
 *            exists only in its database, also while a local command
 *            sequence is open.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "cs/local_instance"

#include "te_config.h"

#include "tapi_test.h"
#include "conf_api.h"

/** Environment variable which the test adds */
#define LOCAL_INSTANCE_ENV "TE_LOCAL_INSTANCE"

/**
 * Check whether Configurator reports an instance as only local.
 *
 * @param handle    Instance handle.
 * @param expected  Expected answer.
 * @param what      Instance description, for verdicts.
 */
static void
check_local(cfg_handle handle, bool expected, const char *what)
{
    bool local;
    te_errno rc;

    rc = cfg_instance_is_local(handle, &local);
    if (rc != 0)
        TEST_VERDICT("Checking %s failed: %r", what, rc);
    if (local != expected)
        TEST_VERDICT("%s is %sreported as local", what, local ? "" : "not ");
}

int
main(int argc, char *argv[])
{
    const char *ta;
    cfg_handle agent;
    cfg_handle env;
    cfg_handle obj;
    bool local;
    bool added = false;

    TEST_START;

    TEST_PARAM_DOC(ta,
                   "Test Agent whose environment variables the test adds");
    TEST_GET_STRING_PARAM(ta);

    TEST_STEP("Check that the Test Agent instance is not local");
    CHECK_RC(cfg_find_fmt(&agent, "/agent:%s", ta));
    check_local(agent, false, "The Test Agent");

    TEST_STEP("Add an environment variable locally and check that it is "
              "local, while the local command sequence is open");
    CHECK_RC(cfg_add_instance_local_fmt(&env, CFG_VAL(STRING, "1"),
                                        "/agent:%s/env:%s", ta,
                                        LOCAL_INSTANCE_ENV));
    added = true;
    check_local(env, true, "A locally added variable");
    check_local(agent, false, "The Test Agent during a local sequence");

    TEST_STEP("Commit the variable and check that it is not local");
    CHECK_RC(cfg_commit_fmt("/agent:%s/env:%s", ta, LOCAL_INSTANCE_ENV));
    check_local(env, false, "A committed variable");

    TEST_STEP("Delete the variable and check that its handle is refused");
    CHECK_RC(cfg_del_instance(env, false));
    added = false;
    rc = cfg_instance_is_local(env, &local);
    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
        TEST_VERDICT("Checking a deleted instance returned %r", rc);

    TEST_STEP("Add the variable non-locally and check that it is not "
              "local");
    CHECK_RC(cfg_add_instance_fmt(&env, CFG_VAL(STRING, "1"),
                                  "/agent:%s/env:%s", ta,
                                  LOCAL_INSTANCE_ENV));
    added = true;
    check_local(env, false, "A variable added non-locally");

    TEST_STEP("Check that an object handle is refused");
    CHECK_RC(cfg_find_str("/agent/env", &obj));
    rc = cfg_instance_is_local(obj, &local);
    if (TE_RC_GET_ERROR(rc) != TE_EINVAL)
        TEST_VERDICT("Checking an object returned %r", rc);

    TEST_SUCCESS;

cleanup:
    if (added)
    {
        CLEANUP_CHECK_RC(cfg_del_instance_fmt(false, "/agent:%s/env:%s", ta,
                                              LOCAL_INSTANCE_ENV));
    }

    TEST_END;
}
