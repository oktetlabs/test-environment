/** @file
 * @brief Test for OID manipulation routines
 *
 * Testing OID parsing and comparison correctness
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 */

/** @page cs-oid Test for OID manipulation routines
 *
 * @objective Testing OID parsing and comparison correctness
 *
 * Do some sanity checks for OID manipulation routines
 *
 * @par Test sequence:
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 */

#define TE_TEST_NAME    "cs/oid"

#include "te_config.h"

#include <string.h>

#include "tapi_test.h"
#include "conf_oid.h"
#include "te_rpc_signal.h"

static const cfg_oid object_oid = CFG_OBJ_OID_LITERAL({"agent"},
                                                      {"interface"});
int
main(int argc, char **argv)
{
    cfg_oid *inst_oid;

    TEST_START;

    TEST_STEP("Checking a whole-OID match");
    CHECK_NOT_NULL(inst_oid =
                   cfg_convert_oid_str("/agent:Agt_A/interface:eth0"));
    if (!cfg_oid_match(inst_oid, &object_oid, FALSE))
        TEST_VERDICT("OID expected to match but it did not");
    cfg_free_oid(inst_oid);

    CHECK_NOT_NULL(inst_oid =
                   cfg_convert_oid_str("/agent:Agt_A"));
    if (cfg_oid_match(inst_oid, &object_oid, FALSE))
        TEST_VERDICT("A shorter OID expected not to match but it did");
    cfg_free_oid(inst_oid);

    CHECK_NOT_NULL(inst_oid =
                   cfg_convert_oid_str("/agent:Agt_A/route:1.2.3.4|24"));
    if (cfg_oid_match(inst_oid, &object_oid, FALSE))
        TEST_VERDICT("OID expected not to match but it did");
    cfg_free_oid(inst_oid);

    CHECK_NOT_NULL(inst_oid =
                   cfg_convert_oid_str("/agent:Agt_A/interface:eth0/status:"));
    if (cfg_oid_match(inst_oid, &object_oid, FALSE))
    {
        TEST_VERDICT("OID prefix matched though a whole OID match "
                     "was requested");
    }
    cfg_free_oid(inst_oid);

    TEST_STEP("Checking a prefix OID match");
    CHECK_NOT_NULL(inst_oid =
                   cfg_convert_oid_str("/agent:Agt_A/interface:eth0"));
    if (!cfg_oid_match(inst_oid, &object_oid, TRUE))
        TEST_VERDICT("OID expected to match but it did not");
    cfg_free_oid(inst_oid);

    CHECK_NOT_NULL(inst_oid =
                   cfg_convert_oid_str("/agent:Agt_A"));
    if (cfg_oid_match(inst_oid, &object_oid, TRUE))
        TEST_VERDICT("A shorter OID expected not to match but it did");
    cfg_free_oid(inst_oid);

    CHECK_NOT_NULL(inst_oid =
                   cfg_convert_oid_str("/agent:Agt_A/route:1.2.3.4|24"));
    if (cfg_oid_match(inst_oid, &object_oid, TRUE))
        TEST_VERDICT("OID expected not to match but it did");
    cfg_free_oid(inst_oid);

    CHECK_NOT_NULL(inst_oid =
                   cfg_convert_oid_str("/agent:Agt_A/interface:eth0/status:"));
    if (!cfg_oid_match(inst_oid, &object_oid, TRUE))
    {
        TEST_VERDICT("OID prefix did not match though a prefix OID match "
                     "was requested");
    }
    cfg_free_oid(inst_oid);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
