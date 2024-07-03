/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test Environment
 *
 * Check set and rollback in Configurator
 */

/** @page set_restore Access /proc/sys through configurator
 *
 * @objective Check configurator set and rollback by accessing /proc/sys
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "cs/set_restore"

#ifndef TEST_START_VARS
#define TEST_START_VARS TEST_START_ENV_VARS
#endif

#ifndef TEST_START_SPECIFIC
#define TEST_START_SPECIFIC TEST_START_ENV
#endif

#ifndef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC TEST_END_ENV
#endif

#include "te_config.h"

#include "tapi_test.h"
#include "tapi_env.h"
#include "conf_api.h"

typedef enum rollback_type {
    NONE,
    BACKUP,
    BACKUP_NOHISTORY,
} rollback_type;

/**
 * The list of values allowed for parameter of type @p rollback_type.
 */
#define ROLLBACK_TYPE_MAPPING_LIST \
        { "none", NONE },          \
        { "backup", BACKUP },      \
        { "nohistory", BACKUP_NOHISTORY }

/**
 * Get the value of parameter of type @p rollback_type.
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type @p rollback_type.
 */
#define TEST_GET_ROLLBACK_TYPE_PARAM(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, ROLLBACK_TYPE_MAPPING_LIST)

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;

    char *backup = NULL;
    const char *oid_name = NULL;

    bool restore;
    rollback_type rollback;

    int32_t old_value = -1;
    int32_t new_value = -1;
    int32_t cur_value = -1;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_STRING_PARAM(oid_name);
    TEST_GET_BOOL_PARAM(restore);
    TEST_GET_ROLLBACK_TYPE_PARAM(rollback);

    TEST_STEP("Create and verify configuration backup");
    CHECK_RC(cfg_create_backup(&backup));
    CHECK_RC(cfg_verify_backup(backup));

    TEST_STEP("Change existing configuration");
    CHECK_RC(cfg_get_int32(&old_value, "/agent:%s%s:",
                           pco_iut->ta, oid_name));

    new_value = old_value + 1;
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(INT32, new_value),
                                  "/agent:%s%s:",
                                  pco_iut->ta, oid_name));

    CHECK_RC(cfg_get_int32(&cur_value, "/agent:%s%s:",
                           pco_iut->ta, oid_name));
    if (new_value != cur_value)
    {
        TEST_FAIL("Incorrect /agent:%s%s: value. "
                  "It should be '%d', but it's '%d'",
                  pco_iut->ta, oid_name, new_value, cur_value);
    }

    TEST_STEP("Check that the backup verification fails after "
              "the configuration change");
    rc = cfg_verify_backup(backup);
    if (rc == 0)
        TEST_FAIL("Current configuration should be different from backup");

    if (restore)
    {
        TEST_STEP("Restore configuration manually");
        CHECK_RC(cfg_set_instance_fmt(CFG_VAL(INT32, old_value),
                                      "/agent:%s%s:",
                                      pco_iut->ta, oid_name));
    }

    switch (rollback)
    {
        case NONE:
            break;

        case BACKUP:
            TEST_STEP("Restore configuration from backup");
            CHECK_RC(cfg_restore_backup(backup));
            break;

        case BACKUP_NOHISTORY:
            TEST_STEP("Restore configuration from backup");
            CHECK_RC(cfg_restore_backup_nohistory(backup));
            break;

        default:
            TEST_FAIL("Unknown rollback_type '%d'", rollback);
            break;
    }

    TEST_STEP("Check that the backup verification succeeds");
    rc = cfg_verify_backup(backup);
    if (rc != 0)
    {
        CHECK_RC(cfg_restore_backup(backup));
        TEST_FAIL("Failed to verify backup");
    }

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(cfg_release_backup(&backup));
    free(backup);

    TEST_END;
}
