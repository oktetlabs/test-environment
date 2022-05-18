/** @file
 * @brief Test for a te_enum.h functions
 *
 * Testing te_enum.h correctness
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 */

/** @page tools_enum_map te_enum.h test
 *
 * @objective Testing te_enum.h correctness
 *
 * Do some sanity checks that enumeration mappings work
 * correctly
 *
 * @par Test sequence:
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "enum_map"

#include "te_config.h"

#include <string.h>

#include "tapi_test.h"
#include "te_enum.h"
#include "te_rpc_signal.h"

int
main(int argc, char **argv)
{
    static const te_enum_map mapping[] = {
        {.name = "A", .value = 1},
        {.name = "B", .value = 2},
        {.name = "C", .value = 3},
        TE_ENUM_MAP_END
    };
    te_enum_map dynamic_map[RPC_SIGUNKNOWN - RPC_SIGHUP + 2] =
        {TE_ENUM_MAP_END,};

    unsigned i;

    TEST_START;

    TEST_STEP("Checking string-to-value mapping");
    for (i = 0; mapping[i].name != NULL; i++)
    {
        int mapped = te_enum_map_from_str(mapping, mapping[i].name, -1);

        if (mapped < 0)
            TEST_VERDICT("'%s' was not found in the mapping", mapping[i].name);
        if (mapped != mapping[i].value)
        {
            TEST_VERDICT("%d value expected for '%s', but got %d",
                         mapping[i].value, mapping[i].name, mapped);
        }
    }


    TEST_STEP("Checking value-to-strng mapping");
    for (i = 0; mapping[i].name != NULL; i++)
    {
        const char *mapped = te_enum_map_from_value(mapping, mapping[i].value);

        if (strcmp(mapped, mapping[i].name) != 0)
        {
            TEST_VERDICT("'%s' value expected for '%d', but got '%s'",
                         mapping[i].name, mapping[i].value, mapped);
        }
    }

    TEST_STEP("Checking mapping of non-existing string");
    if (te_enum_map_from_str(mapping, "does not exist", -1) != -1)
        TEST_VERDICT("Non-existing string reported as found");

    TEST_STEP("Check dynamic map generation");
    te_enum_map_fill_by_conversion(dynamic_map, RPC_SIGHUP, RPC_SIGUNKNOWN,
                                   (const char *(*)(int))signum_rpc2str);
    for (i = 0; i + RPC_SIGHUP <= RPC_SIGUNKNOWN; i++)
    {
        const char *expected = signum_rpc2str(i + RPC_SIGHUP);

        if (dynamic_map[i].name == NULL)
            TEST_VERDICT("Dynamic map is not complete");

        if (dynamic_map[i].value != (int)i + RPC_SIGHUP)
        {
            TEST_VERDICT("Expected value %d, but got %d", (int)i + RPC_SIGHUP,
                         dynamic_map[i].value);
        }
        if (strcmp(dynamic_map[i].name, expected) != 0)
        {
            TEST_VERDICT("Expected '%s', but got '%s'", expected,
                         dynamic_map[i].name);
        }
    }
    if (dynamic_map[i].name != NULL)
        TEST_VERDICT("Dynamic map is not properly terminated");

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
