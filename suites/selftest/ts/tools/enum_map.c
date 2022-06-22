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
#include <sys/mman.h>

#include "tapi_test.h"
#include "te_enum.h"
#include "te_rpc_signal.h"
#include "te_rpc_sys_mman.h"

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
    static const te_enum_trn translation[] = {
        {.from = 1, .to = 0x100},
        {.from = 2, .to = 0x101},
        {.from = 3, .to = 0x102},
        TE_ENUM_TRN_END
    };
    static const te_enum_trn btranslation[] = {
        {.from = RPC_PROT_READ, .to = PROT_READ},
        {.from = RPC_PROT_WRITE, .to = PROT_WRITE},
        {.from = RPC_PROT_EXEC, .to = PROT_EXEC},
        {.from = RPC_PROT_NONE, .to = PROT_NONE},
#ifdef PROT_GROWSDOWN
        {.from = RPC_PROT_GROWSDOWN, .to = PROT_GROWSDOWN},
#endif
#ifdef PROT_GROWSUP
        {.from = RPC_PROT_GROWSUP, .to = PROT_GROWSUP},
#endif
        TE_ENUM_TRN_END
    };
    te_enum_trn dynamic_trn[RPC_SIGUNKNOWN - RPC_SIGHUP + 2] =
        {TE_ENUM_TRN_END,};

    unsigned i;
    int from_flags;
    int to_flags;

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

    TEST_STEP("Checking flag value forward translation");
    for (from_flags = 0, to_flags = 0, i = 0;
         btranslation[i].from != INT_MIN;
         i++)
    {
        int mapped = te_enum_translate_bitmap(btranslation,
                                              btranslation[i].from,
                                              FALSE);

        assert(btranslation[i].from != 0);

        if (mapped != btranslation[i].to)
        {
            TEST_VERDICT("Forward translation of %#x failed: "
                         "expected %#x, got %#x",
                         (unsigned)btranslation[i].from,
                         (unsigned)btranslation[i].to, (unsigned)mapped);
        }

        from_flags |= btranslation[i].from;
        to_flags |= btranslation[i].to;

        mapped = te_enum_translate_bitmap(btranslation, from_flags, FALSE);

        if (mapped != to_flags)
        {
            TEST_VERDICT("Forward translation of %#x failed: "
                         "expected %#x, got %#x",
                         (unsigned)btranslation[i].from,
                         (unsigned)btranslation[i].to, (unsigned)mapped);
        }
    }

    TEST_STEP("Checking flag value backward translation");
    for (from_flags = 0, to_flags = 0, i = 0;
         btranslation[i].from != INT_MIN;
         i++)
    {
        int mapped;

        if (btranslation[i].to != 0)
            continue;

        mapped = te_enum_translate_bitmap(btranslation, btranslation[i].to,
                                          TRUE);

        if (mapped != btranslation[i].from)
        {
            TEST_VERDICT("Backward translation of %#x failed: "
                         "expected %#x, got %#x",
                         (unsigned)btranslation[i].to,
                         (unsigned)btranslation[i].from, (unsigned)mapped);
        }

        from_flags |= btranslation[i].from;
        to_flags |= btranslation[i].to;

        mapped = te_enum_translate_bitmap(btranslation, to_flags, TRUE);

        if (mapped != from_flags)
        {
            TEST_VERDICT("Backward translation of %#x failed: "
                         "expected %#x, got %#x",
                         (unsigned)btranslation[i].to,
                         (unsigned)btranslation[i].from, (unsigned)mapped);
        }
    }

    TEST_STEP("Checking zero flag translation");
    if (te_enum_translate_bitmap(btranslation, 0, FALSE) != 0)
        TEST_VERDICT("Zero flags forward-translated to non-zero");
    if (te_enum_translate_bitmap(btranslation, PROT_NONE, TRUE) !=
        RPC_PROT_NONE)
        TEST_VERDICT("Zero flags backward-translated to an unexpected value");

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


    TEST_SUCCESS;

cleanup:
    TEST_END;
}
