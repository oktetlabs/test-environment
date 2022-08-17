/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Minimal prologue
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page ts_prologue Prologue
 *
 * @objective Demo of minimal prologue of test suite
 *
 * Prologue is test that will be run before the package is run.
 * It is typically used to configure agents before running tests in a package.
 *
 * @note @p TEST_STEP() is required. This need to generate
 * documetation of test steps.
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "prologue"

#include "te_config.h"
#include "tapi_test.h"
#include "tapi_tags.h"

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Start dummy prologue");

    TEST_STEP("Add TRC tag");
    CHECK_RC(tapi_tags_add_tag("tag_set_by_prologue", NULL));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
