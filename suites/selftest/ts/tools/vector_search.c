/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_vector.h sort and search functions
 *
 * Testing vector sorting and searching routines
 *
 */

/** @page tools_vector_search te_vector.h sort and search test
 *
 * @objective Testing vector sorting and searching routines
 *
 * Test the correctness of dynamic vector implementation.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/vector_search"

#include "te_config.h"

#include <string.h>

#include "tapi_test.h"
#include "te_vector.h"

static int
int_compare(const void *key1, const void *key2)
{
    const int *item1 = key1;
    const int *item2 = key2;

    if (*item1 < *item2)
        return -1;
    else if (*item1 > *item2)
        return 1;
    else
        return 0;
}

typedef struct key_range {
    int min;
    int max;
} key_range;

static int
range_compare(const void *key, const void *elt)
{
    const key_range *range = key;
    const int *item = elt;

    if (range->max < *item)
        return -1;
    else if (range->min > *item)
        return 1;
    else
        return 0;
}

static void
check_search(const te_vec *vector, const void *key,
             int (*search_func)(const void *, const void *),
             unsigned int exp_minpos, unsigned int exp_maxpos)
{
    unsigned int minpos;
    unsigned int maxpos;
    bool found = te_vec_search(vector, key, search_func, &minpos, &maxpos);

    if (!found)
        TEST_VERDICT("An element is not found when it should");

    if (minpos != exp_minpos)
    {
        TEST_VERDICT("Expected leftmost position is %u, but got %u",
                     exp_minpos, minpos);
    }

    if (maxpos != exp_maxpos)
    {
        TEST_VERDICT("Expected rightmost position is %u, but got %u",
                     exp_maxpos, maxpos);
    }
}

int
main(int argc, char **argv)
{
    static const unsigned int n_elements = 1000;
    te_vec vector = TE_VEC_INIT(int);
    unsigned int i;
    const int *iter;
    int prev = -1;
    int missing_key = 0;
    key_range range;

    TEST_START;

    TEST_STEP("Shuffle the initial vector");
    for (i = 0; i < n_elements; i++)
        CHECK_RC(TE_VEC_APPEND_RVALUE(&vector, int, i * 2));

    for (i = 0; i < n_elements; i++)
    {
        int p1 = rand_range(0, n_elements - 1);
        int p2 = rand_range(0, n_elements - 1);

        int *item1 = te_vec_get(&vector, p1);
        int *item2 = te_vec_get(&vector, p2);

        int tmp = *item1;

        *item1 = *item2;
        *item2 = tmp;
    }

    TEST_STEP("Sort the vector and check it is sorted");
    te_vec_sort(&vector, int_compare);
    TE_VEC_FOREACH(&vector, iter)
    {
        if (*iter < prev)
        {
            TEST_VERDICT("Sorting failed for item #%u (%d), "
                         "it is less than the previous (%d)",
                         te_vec_get_index(&vector, iter),
                         *iter, prev);
        }
        prev = *iter;
    }

    TEST_STEP("Check that existing values can be found");
    TE_VEC_FOREACH(&vector, iter)
    {
        unsigned int index = te_vec_get_index(&vector, iter);

        range.min = *iter - 1;
        range.max = *iter + 1;

        check_search(&vector, iter, int_compare, index, index);
        check_search(&vector, &range, range_compare, index, index);

        if (*iter > 0)
        {
            range.min = *iter - 2;
            check_search(&vector, &range, range_compare, index - 1, index);
        }
    }

    TEST_STEP("Do a all-overlapping range search");
    check_search(&vector, &(const key_range){-1, n_elements * 2},
                 range_compare, 0, n_elements - 1);

    TEST_STEP("Check that non-existing values are not found");
    TE_VEC_FOREACH(&vector, iter)
    {
        missing_key = *iter - 1;
        range.min = range.max = missing_key;

        if (te_vec_search(&vector, &missing_key, int_compare, NULL, NULL))
            TEST_VERDICT("Item %d is found when it should not", missing_key);

        if (te_vec_search(&vector, &range, range_compare, NULL, NULL))
            TEST_VERDICT("Item %d is found when it should not", missing_key);
    }
    missing_key = n_elements * 2;
    if (te_vec_search(&vector, &missing_key, int_compare, NULL, NULL))
        TEST_VERDICT("Item %d is found when it should not", missing_key);

    TEST_SUCCESS;

cleanup:

    te_vec_free(&vector);

    TEST_END;
}
