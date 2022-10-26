/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Testing change tracking API
 *
 * Testing change tracking API
 */

/** @page cs-changed Testing change tracking API
 *
 * @objective Check that data change tracking works properly
 *
 * @par Scenario:
 */

#define TE_TEST_NAME "cs/changed"

#include "te_config.h"
#include "te_str.h"
#include "tapi_cfg_changed.h"
#include "tapi_test.h"

#define CHANGE_TAG "test"

typedef struct statistics {
    size_t count;
    size_t min;
    size_t max;
    size_t sum;
} statistics;

static te_errno
do_stat(const char *tag, size_t start, size_t len, void *ctx)
{
    statistics *stat = ctx;

    if (strcmp(tag, CHANGE_TAG) != 0)
        TEST_VERDICT("Unexpected tag '%s'", tag);

    if (start + len < start)
        TEST_VERDICT("Overflow detected");

    stat->count++;
    stat->sum += start;
    if (start < stat->min)
        stat->min = start;
    if (start + len > stat->max)
        stat->max = start + len;

    return TE_EAGAIN;
}

static void
check_region_stat(statistics expected)
{
    statistics actual = {.count = 0, .min = SIZE_MAX, .max = 0, .sum = 0};

    CHECK_RC(tapi_cfg_changed_process_regions(CHANGE_TAG, do_stat, &actual));

    if (expected.count != actual.count)
    {
        TEST_VERDICT("Actual number of regions (%zu) is different "
                     "from the expected one (%zu)", actual.count,
                     expected.count);
    }

    if (expected.min != actual.min)
    {
        TEST_VERDICT("Actual lowest changed position (%zu) is different "
                     "from the expected one (%zu)", actual.min,
                     expected.min);
    }

    if (expected.max != actual.max)
    {
        TEST_VERDICT("Actual highest changed position (%zu) is different "
                     "from the expected one (%zu)", actual.max,
                     expected.max);
    }

    if (expected.sum != actual.sum)
    {
        TEST_VERDICT("Actual sum of starting positions (%zu) is different "
                     "from the expected one (%zu)", actual.sum,
                     expected.sum);
    }
}

static te_errno
stub(const char *tag, size_t start, size_t len, void *ctx)
{
    UNUSED(tag);
    UNUSED(start);
    UNUSED(len);
    UNUSED(ctx);

    return 0;
}

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Testing simple adding of regions");
    CHECK_RC(tapi_cfg_changed_add_region(CHANGE_TAG, 0, 100));
    CHECK_RC(tapi_cfg_changed_add_region(CHANGE_TAG, 1000, 10000));
    CHECK_RC(tapi_cfg_changed_add_region(CHANGE_TAG, 100000, SIZE_MAX));

    check_region_stat((statistics){.count = 3, .min = 0, .max = SIZE_MAX,
                                   .sum = 1000 + 100000});

    TEST_STEP("Testing simple region processing");
    check_region_stat((statistics){.count = 3, .min = 0, .max = SIZE_MAX,
                                   .sum = 1000 + 100000});
    CHECK_RC(tapi_cfg_changed_process_regions(CHANGE_TAG, stub, NULL));
    check_region_stat((statistics){.count = 0, .min = SIZE_MAX, .max = 0,
                                   .sum = 0});

    TEST_STEP("Testing simple removing of regions");
    CHECK_RC(tapi_cfg_changed_add_region(CHANGE_TAG, 0, 100));
    CHECK_RC(tapi_cfg_changed_add_region(CHANGE_TAG, 1000, 10000));
    CHECK_RC(tapi_cfg_changed_add_region(CHANGE_TAG, 100000, SIZE_MAX));
    CHECK_RC(tapi_cfg_changed_remove_region(CHANGE_TAG, 0));
    CHECK_RC(tapi_cfg_changed_remove_region(CHANGE_TAG, 1000));
    CHECK_RC(tapi_cfg_changed_remove_region(CHANGE_TAG, 100000));
    CHECK_RC(tapi_cfg_changed_remove_region(CHANGE_TAG, 1000000));
    check_region_stat((statistics){.count = 0, .min = SIZE_MAX, .max = 0,
                                   .sum = 0});

    TEST_STEP("Testing region resizing");
    CHECK_RC(tapi_cfg_changed_add_region(CHANGE_TAG, 0, 100));
    CHECK_RC(tapi_cfg_changed_add_region(CHANGE_TAG, 0, 200));
    check_region_stat((statistics){.count = 1, .min = 0, .max = 200, .sum = 0});
    CHECK_RC(tapi_cfg_changed_add_region(CHANGE_TAG, 0, 100));
    check_region_stat((statistics){.count = 1, .min = 0, .max = 200, .sum = 0});

    TEST_STEP("Testing tag clearing");
    CHECK_RC(tapi_cfg_changed_clear_tag(CHANGE_TAG));
    check_region_stat((statistics){.count = 0, .min = SIZE_MAX, .max = 0,
                                   .sum = 0});

    TEST_STEP("Testing adding of overlapping regions");
    CHECK_RC(tapi_cfg_changed_add_region_overlap(CHANGE_TAG, 1000, 10000));
    CHECK_RC(tapi_cfg_changed_add_region_overlap(CHANGE_TAG, 2000, 2000));
    check_region_stat((statistics){.count = 1, .min = 1000, .max = 11000,
                                   .sum = 1000});
    CHECK_RC(tapi_cfg_changed_add_region_overlap(CHANGE_TAG, 100, 1000));
    check_region_stat((statistics){.count = 2, .min = 100, .max = 11000,
                                   .sum = 100 + 1000});
    CHECK_RC(tapi_cfg_changed_add_region_overlap(CHANGE_TAG, 10000, 2000));
    check_region_stat((statistics){.count = 2, .min = 100, .max = 12000,
                                   .sum = 100 + 1000});
    CHECK_RC(tapi_cfg_changed_add_region_overlap(CHANGE_TAG, 0, 100));
    check_region_stat((statistics){.count = 3, .min = 0, .max = 12000,
                                   .sum = 100 + 1000});
    CHECK_RC(tapi_cfg_changed_add_region_overlap(CHANGE_TAG, 20000, 10000));
    check_region_stat((statistics){.count = 4, .min = 0, .max = 30000,
                                   .sum = 100 + 1000 + 20000});
    CHECK_RC(tapi_cfg_changed_add_region_overlap(CHANGE_TAG, 0, SIZE_MAX));
    check_region_stat((statistics){.count = 4, .min = 0, .max = SIZE_MAX,
                                   .sum = 100 + 1000 + 20000});
    CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, "/local:/changed:%s",
                            CHANGE_TAG));

    TEST_STEP("Testing removal of overlapping regions");
    CHECK_RC(tapi_cfg_changed_remove_region_overlap(CHANGE_TAG,
                                                    100000, 100000));
    check_region_stat((statistics){.count = 5, .min = 0, .max = SIZE_MAX,
                                   .sum = 100 + 1000 + 20000 + 200000});
    CHECK_RC(tapi_cfg_changed_remove_region_overlap(CHANGE_TAG, 0, 10));
    check_region_stat((statistics){.count = 5, .min = 10, .max = SIZE_MAX,
                                   .sum = 10 + 100 + 1000 + 20000 + 200000});
    /* remove an already removed region for the second time */
    CHECK_RC(tapi_cfg_changed_remove_region_overlap(CHANGE_TAG, 0, 10));
    check_region_stat((statistics){.count = 5, .min = 10, .max = SIZE_MAX,
                                   .sum = 10 + 100 + 1000 + 20000 + 200000});

    CHECK_RC(tapi_cfg_changed_remove_region_overlap(CHANGE_TAG, 100, 19900));
    check_region_stat((statistics){.count = 3, .min = 10, .max = SIZE_MAX,
                                   .sum = 10 + 20000 + 200000});

    CHECK_RC(tapi_cfg_changed_remove_region_overlap(CHANGE_TAG, 1000000,
                                                    SIZE_MAX));
    check_region_stat((statistics){.count = 3, .min = 10, .max = 1000000,
                                   .sum = 10 + 20000 + 200000});

    CHECK_RC(tapi_cfg_changed_remove_region_overlap(CHANGE_TAG, 0, SIZE_MAX));
    check_region_stat((statistics){.count = 0, .min = SIZE_MAX, .max = 0,
                                   .sum = 0});

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
