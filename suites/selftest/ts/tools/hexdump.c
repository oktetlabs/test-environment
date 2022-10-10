/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs. All rights reserved. */
/** @file
 * @brief Test for te_hex_diff_dump.h functions
 *
 * Testing hex diff dumping routines
 */

/** @page tools_hexdump te_hex_diff_dump.h test
 *
 * @objective Testing hex diff dumping routines
 *
 * Test the correctness of hex diff dumping functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/hexdiff"

#include "te_config.h"

#include "tapi_test.h"
#include "te_hex_diff_dump.h"

#define LOG_DIFF_EXP_BANNER \
    "        |=========== Expected ===========" \
    "|============ Actual ============\n"

static void
check_diff(const uint8_t *exp, size_t exp_len,
           const uint8_t *act, size_t act_len,
           size_t offset,
           const char *exp_dump)
{
    te_string dest = TE_STRING_INIT;

    te_hex_diff_dump(exp, exp_len, act, act_len, offset, &dest);
    if (strcmp(dest.ptr, exp_dump) != 0)
    {
        ERROR("Got:\n%s\nExpected:\n%s", dest.ptr, exp_dump);
        te_string_free(&dest);

        TEST_VERDICT("Unexpected dump");
    }

    te_string_free(&dest);

    LOG_HEX_DIFF_DUMP_AT(TE_LL_RING, exp, exp_len, act, act_len, offset);
}

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Test single-line dump");
    check_diff((const uint8_t []){1, 2, 3, 4, 5, 6, 7, 8}, 8,
               (const uint8_t []){1, 3, 2, 4, 5, 6, 7, 8}, 8, 0,
               LOG_DIFF_EXP_BANNER
               "00000000| 01 >02<>03< 04  05  06  07  08 "
               "| 01 >03<>02< 04  05  06  07  08 \n");

    TEST_STEP("Test short-line dump");
    check_diff((const uint8_t []){1, 2, 3, 4}, 4,
               (const uint8_t []){1, 3, 2, 4}, 4, 0,
               LOG_DIFF_EXP_BANNER
               "00000000| 01 >02<>03< 04                 "
               "| 01 >03<>02< 04                 \n");

    TEST_STEP("Test multi-line dump");
    check_diff((const uint8_t []){1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8}, 16,
               (const uint8_t []){1, 3, 2, 4, 5, 6, 7, 8,
                                  1, 3, 2, 4, 5, 6, 7, 8}, 16, 0,
        LOG_DIFF_EXP_BANNER
        "00000000| 01 >02<>03< 04  05  06  07  08 "
        "| 01 >03<>02< 04  05  06  07  08 \n"
        "00000008| 01 >02<>03< 04  05  06  07  08 "
        "| 01 >03<>02< 04  05  06  07  08 \n");

    TEST_STEP("Test multi-line dump with skip");
    check_diff((const uint8_t []){1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8}, 32,
               (const uint8_t []){1, 3, 2, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 3, 2, 4, 5, 6, 7, 8}, 32, 0,
        LOG_DIFF_EXP_BANNER
        "00000000| 01 >02<>03< 04  05  06  07  08 "
        "| 01 >03<>02< 04  05  06  07  08 \n"
        "      ...                              ...\n"
        "00000018| 01 >02<>03< 04  05  06  07  08 "
        "| 01 >03<>02< 04  05  06  07  08 \n");

    TEST_STEP("Test multi-line dump with non-zero offset");
    check_diff((const uint8_t []){1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8}, 16,
               (const uint8_t []){1, 3, 2, 4, 5, 6, 7, 8,
                                  1, 3, 2, 4, 5, 6, 7, 8}, 16, 16,
        LOG_DIFF_EXP_BANNER
        "00000010| 01 >02<>03< 04  05  06  07  08 "
        "| 01 >03<>02< 04  05  06  07  08 \n"
        "00000018| 01 >02<>03< 04  05  06  07  08 "
        "| 01 >03<>02< 04  05  06  07  08 \n");

    TEST_STEP("Test multi-line dump with partial-line offset");
    check_diff((const uint8_t []){1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8}, 16,
               (const uint8_t []){1, 3, 2, 4, 5, 6, 7, 8,
                                  1, 3, 2, 4, 5, 6, 7, 9}, 16, 1,
        LOG_DIFF_EXP_BANNER
        "00000000|     01 >02<>03< 04  05  06  07 "
        "|     01 >03<>02< 04  05  06  07 \n"
        "00000008| 08  01 >02<>03< 04  05  06  07 "
        "| 08  01 >03<>02< 04  05  06  07 \n"
        "00000010|>08<                            "
        "|>09<                            \n");

    TEST_STEP("Test unequal size dump with shorter actual data ");
    check_diff((const uint8_t []){1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8}, 32,
               (const uint8_t []){1, 3, 2, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2}, 18, 0,
        LOG_DIFF_EXP_BANNER
        "00000000| 01 >02<>03< 04  05  06  07  08 "
        "| 01 >03<>02< 04  05  06  07  08 \n"
        "      ...                              ...\n"
        "00000010| 01  02 >03<>04<>05<>06<>07<>08<"
        "| 01  02                         \n"
        "00000018|>01<>02<>03<>04<>05<>06<>07<>08<"
        "|                                \n");

    TEST_STEP("Test unequal size dump with shorter expected data ");
    check_diff((const uint8_t []){1, 3, 2, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2}, 18,
               (const uint8_t []){1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8,
                                  1, 2, 3, 4, 5, 6, 7, 8}, 32, 0,
        LOG_DIFF_EXP_BANNER
        "00000000| 01 >03<>02< 04  05  06  07  08 "
        "| 01 >02<>03< 04  05  06  07  08 \n"
        "      ...                              ...\n"
        "00000010| 01  02                         "
        "| 01  02 >03<>04<>05<>06<>07<>08<\n"
        "00000018|                                "
        "|>01<>02<>03<>04<>05<>06<>07<>08<\n");

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
