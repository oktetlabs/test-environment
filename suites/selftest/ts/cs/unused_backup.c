/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Testing configuration backups usage
 */

/** @page cs-unused_backup Unused configuration backup
 *
 * @objective Check that unused configuration backup does not
 *            break configuration tree handling.
 *
 * @par Scenario:
 */

#define TE_TEST_NAME "cs/unused_backup"

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

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;

    char *bkp1 = NULL;
    char *bkp2 = NULL;
    char *bkp3 = NULL;

    TEST_START;

    TEST_GET_PCO(pco_iut);

    TEST_STEP("Create the first configuration backup.");
    CHECK_RC(cfg_create_backup(&bkp1));
    RING("Created configuration backup 1: %s", bkp1);

    TEST_STEP("Set both properties of @b incr_obj to @c 1 and then to "
              "@c 2, to achieve configuration state which Configurator "
              "can roll back only via commands history.");

    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT32, 1),
                                  "/agent:%s/selftest:/incr_obj:/a:",
                                  pco_iut->ta));
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT32, 1),
                                  "/agent:%s/selftest:/incr_obj:/b:",
                                  pco_iut->ta));
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT32, 2),
                                  "/agent:%s/selftest:/incr_obj:/a:",
                                  pco_iut->ta));
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT32, 2),
                                  "/agent:%s/selftest:/incr_obj:/b:",
                                  pco_iut->ta));

    TEST_STEP("Create the second configuration backup.");
    CHECK_RC(cfg_create_backup(&bkp2));
    RING("Created configuration backup 2: %s", bkp2);

    TEST_STEP("In a single commit, set both properties of @b commit_obj to "
              "@c 1. Configurator can roll this change back only by "
              "restoring configuration state from backup without using "
              "history of commands.");

    CHECK_RC(cfg_set_instance_local_fmt(
                                    CFG_VAL(UINT32, 1),
                                    "/agent:%s/selftest:/commit_obj:/a:",
                                    pco_iut->ta));
    CHECK_RC(cfg_set_instance_local_fmt(
                                    CFG_VAL(UINT32, 1),
                                    "/agent:%s/selftest:/commit_obj:/b:",
                                    pco_iut->ta));
    CHECK_RC(cfg_commit_fmt("/agent:%s/selftest:/commit_obj:",
                            pco_iut->ta));

    TEST_STEP("Create the third configuration backup.");
    CHECK_RC(cfg_create_backup(&bkp3));
    RING("Created configuration backup 3: %s", bkp3);

    /*
     * Configurator usually ignores errors when restoring state from
     * history, so that everything that is possible to restore is
     * restored. However for unknown reason it stops the process if
     * it encounters a command asking to set an instance which is not
     * even present in Configurator database - unless it is the final
     * state restoration at Configurator shutdown. See the code in
     * cfg_dh_restore_backup_ext(), handling of CFG_SET.
     * So here such an instance is changed which will disappear from
     * configuration database after restoring the state to backup 2,
     * making it impossible to use history of commands after backup 2
     * if this history is kept due to hanging unused backup 3
     * (see bug 12563).
     */

    TEST_STEP("Set @b commit_obj_dep instance to @c 1 - after change of "
              "@b commit_obj state it should be available. Once state is "
              "rolled back to backup 2, this instance will disappear "
              "again, making the following history of commands unusable.");
    CHECK_RC(cfg_synchronize_fmt(TRUE, "/agent:%s/selftest:",
                                 pco_iut->ta));
    CHECK_RC(cfg_set_instance_fmt(CFG_VAL(UINT32, 1),
                                  "/agent:%s/selftest:/commit_obj_dep:",
                                  pco_iut->ta));

    TEST_STEP("Restore backup 2 without using history of commands, "
              "skipping backup 3.");
    rc = cfg_restore_backup_nohistory(bkp2);
    if (rc != 0)
        TEST_VERDICT("Failed to restore the second backup.");

    CHECK_RC(cfg_synchronize_fmt(TRUE, "/agent:%s/selftest:",
                                 pco_iut->ta));

    TEST_STEP("Restore backup 1 (assuming that history of commands will be "
              "used).");
    rc = cfg_restore_backup(bkp1);
    if (rc != 0)
        TEST_VERDICT("Failed to restore the first backup.");

    TEST_SUCCESS;

cleanup:

    free(bkp1);
    free(bkp2);
    free(bkp3);

    TEST_END;
}
