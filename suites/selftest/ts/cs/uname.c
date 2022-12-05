/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Access uname info through configurator
 *
 * Access TA uname info through the Cconfigurator
 *
 */

/** @page cs-uname Access TA uname info through configurator
 *
 * @objective Check that TA reports its uname info properly.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "cs/uname"

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

#include <sys/utsname.h>

#include "te_str.h"
#include "tapi_cfg_base.h"
#include "tapi_rpc_unistd.h"
#include "tapi_test.h"
#include "tapi_env.h"

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    struct utsname info;
    struct utsname info2;

    TEST_START;

    TEST_GET_PCO(pco_iut);

    TEST_STEP("Getting uname through Configurator");
    CHECK_RC(tapi_cfg_base_get_ta_uname(pco_iut->ta, &info));

    TEST_STEP("Getting uname through RPC");
    rpc_uname(pco_iut, &info2);

#define CHECK_FIELD(_field) \
    do {                                                        \
        RING(#_field " = %s", info._field);                     \
        if (strcmp(info._field, info2._field) != 0)             \
        {                                                       \
            TEST_VERDICT("Values for %s differ: %s vs %s",      \
                         #_field, info._field, info2._field);   \
        }                                                       \
    } while(0)

    CHECK_FIELD(sysname);
    CHECK_FIELD(release);
    CHECK_FIELD(version);
    CHECK_FIELD(machine);
#undef CHECK_FIELD

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
