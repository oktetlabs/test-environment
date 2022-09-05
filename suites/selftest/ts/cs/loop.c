/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Testing Block Device Configuration TAPI
 *
 * Testing Block Device Configuration TAPI.
 */

/** @page cs-loop Testing Loop Block Device Configuration TAPI
 *
 * @objective Check that Loop Block Device Configuration TAPI works properly.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "cs/loop"

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
#include "tapi_cfg_block.h"
#include "tapi_test.h"
#include "tapi_env.h"

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    const char *blockdev;
    char *filename = NULL;
    char *chk_filename = NULL;
    static const char filename_template[] = "te_loop_XXXXXX";
    tarpc_off_t length = 0;
    int fd = -1;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_STRING_PARAM(blockdev);
    TEST_GET_VALUE_BIN_UNIT_PARAM(length);

    TEST_STEP("Initialize loop devices");
    CHECK_RC(tapi_cfg_block_initialize_loop(pco_iut->ta));

    CHECK_RC(tapi_cfg_block_grab(pco_iut->ta, blockdev));

    CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, "/agent:%s", pco_iut->ta));

    if (!tapi_cfg_block_is_loop(pco_iut->ta, blockdev))
        TEST_VERDICT("%s is not a loop device on %s", blockdev, pco_iut->ta);

    CHECK_RC(tapi_cfg_block_loop_get_backing_file(pco_iut->ta, blockdev,
                                                  &chk_filename));
    if (chk_filename != NULL)
    {
        ERROR("'%s' is attached to %s", chk_filename, blockdev);
        TEST_VERDICT("%s on %s has an attached backing file",
                     blockdev, pco_iut->ta);
    }

    fd = rpc_mkstemp(pco_iut, filename_template, &filename);
    rpc_ftruncate(pco_iut, fd, length);

    CHECK_RC(tapi_cfg_block_loop_set_backing_file(pco_iut->ta, blockdev,
                                                  filename));

    CHECK_RC(tapi_cfg_block_loop_get_backing_file(pco_iut->ta, blockdev,
                                                  &chk_filename));
    if (chk_filename == NULL)
    {
        TEST_VERDICT("No file is attached to %s on %s",
                     blockdev, pco_iut->ta);
    }
    if (strcmp(filename, chk_filename) != 0)
    {
        ERROR("The attached file should be '%s', but it's '%s'",
              filename, chk_filename);
        TEST_VERDICT("Unexpected attached file for %s on %s",
                     blockdev, pco_iut->ta);
    }

    free(chk_filename);
    chk_filename = NULL;

    CHECK_RC(tapi_cfg_block_loop_set_backing_file(pco_iut->ta, blockdev, NULL));
    CHECK_RC(tapi_cfg_block_loop_get_backing_file(pco_iut->ta, blockdev,
                                                  &chk_filename));
    if (chk_filename != NULL)
    {
        ERROR("'%s' is still attached to %s", chk_filename, blockdev);
        TEST_VERDICT("%s on %s has an attached backing file",
                     blockdev, pco_iut->ta);
    }


    TEST_SUCCESS;

cleanup:

    if (fd > 0)
        rpc_close(pco_iut, fd);
    if (filename != NULL)
    {
        CLEANUP_CHECK_RC(tapi_cfg_block_loop_set_backing_file(pco_iut->ta,
                                                              blockdev, NULL));
        rpc_unlink(pco_iut, filename);
        free(filename);
    }
    free(chk_filename);

    TEST_END;
}
