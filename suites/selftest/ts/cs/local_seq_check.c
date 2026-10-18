/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Test Environment
 *
 * Check the rollback of a local command sequence left open
 */

/** @page local_seq_check Check the rollback of a local command sequence
 *
 * @objective Check that Configurator rolls back the local command
 *            sequence which @ref local_seq_abandon left open, accepts
 *            requests after it, and that a rollback leaves in the
 *            database what the Test Agent has.
 *
 * The test runs right after @ref local_seq_abandon. If the sequence were
 * still open, Configurator would refuse the requests of this test, and
 * Tester would not run it at all, as it could not restore the
 * configuration after @ref local_seq_abandon.
 *
 * The restore from history, which a rollback uses, does not undo a set
 * which is not committed. The test sets a value locally and then fails a
 * local command, which rolls the sequence back: the value must read back
 * as the agent has it, not as it was set locally.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "cs/local_seq_check"

#include "te_config.h"

#include "tapi_test.h"
#include "conf_api.h"

/** Environment variable which local_seq_abandon adds and commits */
#define LOCAL_SEQ_COMMITTED "TE_LOCAL_SEQ_COMMITTED"

/** Environment variable which local_seq_abandon adds locally */
#define LOCAL_SEQ_LOCAL "TE_LOCAL_SEQ_LOCAL"

/** Environment variable which this test adds */
#define LOCAL_SEQ_CHECK "TE_LOCAL_SEQ_CHECK"

/** Value the variable is added with */
#define LOCAL_SEQ_CHECK_VAL "1"

/** Value the variable is set to locally */
#define LOCAL_SEQ_CHECK_LOCAL_VAL "2"

/** Object which does not exist: a local addition under it fails */
#define LOCAL_SEQ_NO_OBJECT "te_local_seq_no_object"

/**
 * Check that an environment variable is not in the Configurator database.
 *
 * @param ta    Test Agent.
 * @param name  Name of the variable.
 * @param what  How local_seq_abandon added it, for verdicts.
 */
static void
check_env_absent(const char *ta, const char *name, const char *what)
{
    cfg_handle handle;
    te_errno rc;

    rc = cfg_find_fmt(&handle, "/agent:%s/env:%s", ta, name);
    if (rc == 0)
        TEST_VERDICT("The variable added %s is not rolled back", what);
    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        TEST_VERDICT("Failed to look for the variable added %s: %r",
                     what, rc);
    }
}

int
main(int argc, char *argv[])
{
    const char *ta;
    char *value = NULL;
    bool added = false;

    TEST_START;

    TEST_PARAM_DOC(ta,
                   "Test Agent whose environment variables the test checks");
    TEST_GET_STRING_PARAM(ta);

    TEST_STEP("Check that the variables added by the previous test are "
              "gone");
    check_env_absent(ta, LOCAL_SEQ_LOCAL, "locally");
    check_env_absent(ta, LOCAL_SEQ_COMMITTED, "with a commit");

    TEST_STEP("Add and read an environment variable");
    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, LOCAL_SEQ_CHECK_VAL),
                              "/agent:%s/env:%s", ta, LOCAL_SEQ_CHECK);
    if (rc != 0)
        TEST_VERDICT("Failed to add an environment variable: %r", rc);
    added = true;

    rc = cfg_get_instance_string_fmt(&value, "/agent:%s/env:%s", ta,
                                     LOCAL_SEQ_CHECK);
    if (rc != 0)
        TEST_VERDICT("Failed to read an environment variable: %r", rc);

    TEST_STEP("Set it locally, then fail a local command: an addition "
              "under an object which does not exist. Configurator rolls "
              "the sequence back");
    CHECK_RC(cfg_set_instance_local_fmt(CFG_VAL(STRING,
                                                LOCAL_SEQ_CHECK_LOCAL_VAL),
                                        "/agent:%s/env:%s", ta,
                                        LOCAL_SEQ_CHECK));
    rc = cfg_add_instance_local_fmt(NULL, CFG_VAL(STRING, "x"),
                                    "/agent:%s/%s:", ta,
                                    LOCAL_SEQ_NO_OBJECT);
    if (rc == 0)
        TEST_VERDICT("A local addition under an unknown object succeeded");
    RING("The local addition failed, as expected: %r", rc);

    TEST_STEP("Check that the variable reads back as the agent has it, "
              "not as it was set locally");
    free(value);
    value = NULL;
    rc = cfg_get_instance_string_fmt(&value, "/agent:%s/env:%s", ta,
                                     LOCAL_SEQ_CHECK);
    if (rc != 0)
        TEST_VERDICT("Failed to read the variable after the rollback: %r",
                     rc);
    if (strcmp(value, LOCAL_SEQ_CHECK_VAL) != 0)
    {
        RING("The variable reads '%s' after the rollback", value);
        TEST_VERDICT("The value set locally is still in the database "
                     "after the rollback");
    }

    TEST_STEP("Delete the variable");
    added = false;
    rc = cfg_del_instance_fmt(false, "/agent:%s/env:%s", ta,
                              LOCAL_SEQ_CHECK);
    if (rc != 0)
        TEST_VERDICT("Failed to delete an environment variable: %r", rc);

    TEST_SUCCESS;

cleanup:
    if (added)
    {
        CLEANUP_CHECK_RC(cfg_del_instance_fmt(false, "/agent:%s/env:%s", ta,
                                              LOCAL_SEQ_CHECK));
    }
    free(value);

    TEST_END;
}
