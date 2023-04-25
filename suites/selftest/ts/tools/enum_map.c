/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test for a te_enum.h functions
 *
 * Testing te_enum.h correctness
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
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
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "enum_map"

#include "te_config.h"

#include <string.h>

#include "tapi_test.h"
#include "te_enum.h"
#include "te_rpc_signal.h"

/* Bits for the first bitmask. */
#define ENUM_MAP_MASK_A_BITS_A 0x1
#define ENUM_MAP_MASK_A_BITS_B 0x2
#define ENUM_MAP_MASK_A_BITS_C 0xC

/* Bits for second bitmask. */
#define ENUM_MAP_MASK_B_BITS_A 0xC
#define ENUM_MAP_MASK_B_BITS_B 0x2
#define ENUM_MAP_MASK_B_BITS_C 0x1

/* Unknown bits that is not to be used in any masks. */
#define ENUM_MAP_MASK_BITS__UNKNOWN 0x10

/* Overlapped bit. */
#define ENUM_MAP_MASK_BITS__OVERLAPPED (ENUM_MAP_MASK_A_BITS_A | \
                                        ENUM_MAP_MASK_A_BITS_B | \
                                        ENUM_MAP_MASK_A_BITS_C)

typedef te_errno (*action_fn)(unsigned int i);

static te_errno
action1(unsigned int i)
{
    return i == 0 ? 0 : TE_EINVAL;
}

static te_errno
action2(unsigned int i)
{
    return i == 1 ? 0 : TE_EINVAL;
}

static te_errno
action3(unsigned int i)
{
    return i == 2 ? 0 : TE_EINVAL;
}

static te_errno
unknown_action(unsigned int i)
{
    UNUSED(i);
    return TE_ENOENT;
}

static void
check_prefix_strip(void)
{
    static const te_enum_map mapping[] = {
        {.name = "ERROR", .value = 1},
        {.name = "WARNING", .value = 2},
        {.name = "NOTE", .value = 3},
        {.name = "NOTICE", .value = 4},
        {.name = "TRACE", .value = 5},
        {.name = "TRACEALL", .value = 6},
        TE_ENUM_MAP_END
    };

    static const struct {
        const char *input;
        te_bool exact_match;
        const char *expected;
        int exp_val;
    } tests[] = {
        {NULL, TRUE, NULL, -1},
        {NULL, FALSE, NULL, -1},
        {"", TRUE, "", -1},
        {"", FALSE, "", -1},
        {"ERROR", TRUE, "", 1},
        {"ERROR", FALSE, "", 1},
        {"ERR", TRUE, "ERR", -1},
        {"ERR", FALSE, "", 1},
        {"WARNING:", TRUE, ":", 2},
        {"WARN", FALSE, "", 2},
        {"NOTE", TRUE, "", 3},
        {"NOTICE", TRUE, "", 4},
        {"NOT", TRUE, "NOT", -1},
        {"NOT", FALSE, "", 3},
        {"NOTI", FALSE, "", 4},
        {"TRACE0", TRUE, "0", 5},
        {"TRACEA", TRUE, "A", 5},
        {"TRACEALL", TRUE, "", 6},
        {"TRACE", FALSE, "", 5},
        {"TRACEA", FALSE, "", 6},
    };
    unsigned int i;

    for (i = 0; i < TE_ARRAY_LEN(tests); i++)
    {
        char *next = NULL;
        int val = te_enum_parse_longest_match(mapping, -1,
                                              tests[i].exact_match,
                                              tests[i].input,
                                              &next);

        if (tests[i].expected == NULL)
        {
            if (next != NULL)
                TEST_VERDICT("Non-NULL output for NULL input");
        }
        else
        {
            if (next == NULL)
                TEST_VERDICT("NULL output for non-NULL input");

            if (strcmp(next, tests[i].expected) != 0)
            {
                ERROR("Expected '%s' for '%s', got '%s'",
                      tests[i].expected, tests[i].input, next);
                TEST_VERDICT("Unexpected string tail");
            }
        }

        if (val != tests[i].exp_val)
        {
            ERROR("Expected %d for '%s', got %d",
                  tests[i].exp_val, te_str_empty_if_null(tests[i].input),
                  val);
            TEST_VERDICT("Unexpected mapped value");
        }
    }
}

int
main(int argc, char **argv)
{
    static const te_enum_map mapping[] = {
        {.name = "A", .value = 1},
        {.name = "B", .value = 2},
        {.name = "C", .value = 3},
        TE_ENUM_MAP_END
    };
    static const TE_ENUM_MAP_ACTION(action_fn) actions[] = {
        {.name = "A", .action = action1},
        {.name = "B", .action = action2},
        {.name = "C", .action = action3},
        TE_ENUM_MAP_END
    };
    te_enum_map dynamic_map[RPC_SIGUNKNOWN - RPC_SIGHUP + 2] =
        {TE_ENUM_MAP_END,};
    static const te_enum_trn translation[] = {
        {.from = 1, .to = 0x100},
        {.from = 2, .to = 0x101},
        {.from = 3, .to = 0x102},
        TE_ENUM_TRN_END
    };
    te_enum_trn dynamic_trn[RPC_SIGUNKNOWN - RPC_SIGHUP + 2] =
        {TE_ENUM_TRN_END,};
    static te_enum_bitmask_conv mask_conv_map[] = {
        {.bits_from = ENUM_MAP_MASK_A_BITS_A,
         .bits_to = ENUM_MAP_MASK_B_BITS_A},
        {.bits_from = ENUM_MAP_MASK_A_BITS_B,
         .bits_to = ENUM_MAP_MASK_B_BITS_B},
        {.bits_from = ENUM_MAP_MASK_A_BITS_C,
         .bits_to = ENUM_MAP_MASK_B_BITS_C},
        TE_ENUM_BITMASK_CONV_END
    };
    static uint64_t masks_a[] = {
        ENUM_MAP_MASK_A_BITS_A,
        ENUM_MAP_MASK_A_BITS_B,
        ENUM_MAP_MASK_A_BITS_C,
        ENUM_MAP_MASK_A_BITS_A | ENUM_MAP_MASK_A_BITS_B,
        ENUM_MAP_MASK_A_BITS_B | ENUM_MAP_MASK_A_BITS_C,
        ENUM_MAP_MASK_A_BITS_A | ENUM_MAP_MASK_A_BITS_B |
        ENUM_MAP_MASK_A_BITS_C,
    };
    static uint64_t masks_b[] = {
        ENUM_MAP_MASK_B_BITS_A,
        ENUM_MAP_MASK_B_BITS_B,
        ENUM_MAP_MASK_B_BITS_C,
        ENUM_MAP_MASK_B_BITS_A | ENUM_MAP_MASK_B_BITS_B,
        ENUM_MAP_MASK_B_BITS_B | ENUM_MAP_MASK_B_BITS_C,
        ENUM_MAP_MASK_B_BITS_A | ENUM_MAP_MASK_B_BITS_B |
        ENUM_MAP_MASK_B_BITS_C,
    };

    unsigned i;
    te_errno status = 0;

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

    TEST_STEP("Checking mapping of non-existing values");
    if (te_enum_map_from_any_value(mapping, -1, NULL) != NULL)
        TEST_VERDICT("Non-existing value reported as found");

    TEST_STEP("Checking longest prefix stripping");
    check_prefix_strip();

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

    TEST_STEP("Checking string-to-action mapping");
    for (i = 0; actions[i].name != NULL; i++)
    {
        TE_ENUM_DISPATCH(actions, unknown_action, actions[i].name, status, i);
        CHECK_RC(status);
    }
    TE_ENUM_DISPATCH(actions, unknown_action, "does not exist", status, 0);
    if (status!= TE_ENOENT)
        TEST_VERDICT("Non-existing string reported as found");

    TEST_STEP("Checking enum value translation");
    for (i = 0; translation[i].from != INT_MIN; i++)
    {
        int mapped = te_enum_translate(translation, translation[i].from,
                                       FALSE, -1);

        if (mapped != translation[i].to)
        {
            TEST_VERDICT("Forward translation of %d failed: "
                         "expected %d, got %d", translation[i].from,
                         translation[i].to, mapped);
        }
        mapped = te_enum_translate(translation, translation[i].to, TRUE, -1);

        if (mapped != translation[i].from)
        {
            TEST_VERDICT("Backward translation of %d failed: "
                         "expected %d, got %d", translation[i].to,
                         translation[i].from, mapped);
        }
    }

    TEST_STEP("Checking unknown value translation");
    if (te_enum_translate(translation, INT_MAX, FALSE, -1) != -1)
        TEST_VERDICT("Unknown value forward-translated as it is known");
    if (te_enum_translate(translation, INT_MAX, TRUE, -1) != -1)
        TEST_VERDICT("Unknown value backward-translated as it is known");

    TEST_STEP("Check dynamic translation generation");
    te_enum_trn_fill_by_conversion(dynamic_trn, RPC_SIGHUP, RPC_SIGUNKNOWN,
                                   (int (*)(int))signum_rpc2h);
    for (i = 0; i + RPC_SIGHUP <= RPC_SIGUNKNOWN; i++)
    {
        int translated = signum_rpc2h(i + RPC_SIGHUP);

        if (dynamic_trn[i].from == INT_MIN)
            TEST_VERDICT("Dynamic translation is not complete");

        if (dynamic_trn[i].from != (int)i + RPC_SIGHUP)
        {
            TEST_VERDICT("Expected source value %d, but got %d",
                         (int)i + RPC_SIGHUP, dynamic_trn[i].from);
        }
        if (dynamic_trn[i].to != translated)
        {
            TEST_VERDICT("Expected destination value '%d', but got '%d'",
                         translated, dynamic_trn[i].to);
        }
    }
    if (dynamic_trn[i].from != INT_MIN)
        TEST_VERDICT("Dynamic translation is not properly terminated");

    TEST_STEP("Checking bitmasks conversion");
    assert(sizeof(masks_a) == sizeof(masks_b));
    for (i = 0; i < sizeof(masks_a) / sizeof(masks_a[0]); i++)
    {
        uint64_t converted;

        CHECK_RC(te_enum_bitmask_convert(mask_conv_map, masks_a[i],
                                         FALSE, &converted));
        if (converted != masks_b[i])
        {
            TEST_VERDICT("Forward converstion of %" PRIu64 " failed: "
                         "expected %" PRIu64 ", got %" PRIu64, masks_a[i],
                         masks_b[i], converted);
        }

        CHECK_RC(te_enum_bitmask_convert(mask_conv_map, masks_b[i],
                                         TRUE, &converted));
        if (converted != masks_a[i])
        {
            TEST_VERDICT("Backward converstion of %" PRIu64" failed: "
                         "expected %" PRIu64", got %" PRIu64, masks_b[i],
                         masks_a[i], converted);
        }
    }

    TEST_STEP("Checking forward conversion of a bitmask with unknown bit");
    if (te_enum_bitmask_convert(mask_conv_map,
                                (masks_a[0] | ENUM_MAP_MASK_BITS__UNKNOWN),
                                FALSE, NULL) != TE_ERANGE)
    {
        TEST_VERDICT("Unknown bit forward-converted as it is known");
    }

    TEST_STEP("Checking backward conversion of a bitmask with unknown bit");
    if (te_enum_bitmask_convert(mask_conv_map,
                                (masks_b[0] | ENUM_MAP_MASK_BITS__UNKNOWN),
                                TRUE, NULL) != TE_ERANGE)
    {
        TEST_VERDICT("Unknown bit backward-converted as it is known");
    }

    TEST_STEP("Checking bitmasks conversion using maps with overlapped bits");
    /* Left-side bits are overlapped. */
    mask_conv_map[0].bits_from = ENUM_MAP_MASK_BITS__OVERLAPPED;
    status = TE_EINVAL;
    status &= te_enum_bitmask_convert(mask_conv_map, masks_a[0], FALSE, NULL);
    status &= te_enum_bitmask_convert(mask_conv_map, masks_b[0], TRUE, NULL);

    /* Right-side bits are overlapped. */
    mask_conv_map[0].bits_from = ENUM_MAP_MASK_A_BITS_A;
    mask_conv_map[0].bits_to = ENUM_MAP_MASK_BITS__OVERLAPPED;
    status &= te_enum_bitmask_convert(mask_conv_map, masks_a[0], FALSE, NULL);
    status &= te_enum_bitmask_convert(mask_conv_map, masks_b[0], TRUE, NULL);

    if (status != TE_EINVAL)
        TEST_VERDICT("Maps with overlapped bits were processed as valid");

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
