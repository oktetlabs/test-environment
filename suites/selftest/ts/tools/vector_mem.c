/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs. All rights reserved. */
/** @file
 * @brief Test for vector memory management.
 *
 * Testing vector memory management.
 */

/** @page tools_vector_mem te_vector.h test (memory management)
 *
 * @objective Testing vector memory management.
 *
 * Test the correctness of memory management in
 * vector functions.
 *
 * The test is more useful when run under valgrind.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/vector_mem"

#include "te_config.h"

#include <string.h>

#include "tapi_test.h"
#include "te_vector.h"

static te_vec_item_destroy_fn count_destroy;
static unsigned int destroy_cnt;
static void
count_destroy(const void *item)
{
    UNUSED(item);
    destroy_cnt++;
}

static void
check_zeroes(const te_vec *vec, size_t start, size_t count)
{
    size_t i;

    for (i = start; i < start + count; i++)
    {
        int value = TE_VEC_GET(int, vec, i);

        if (value != 0)
        {
            ERROR("Unexpected value at index %zu: %d", i, value);
            TEST_VERDICT("Element not zeroed");
        }
    }
}

static void
check_replace_extend(unsigned int max_elements)
{
    static const int value = 1;
    te_vec vector = TE_VEC_INIT_DESTROY(int, count_destroy);
    unsigned int index = rand_range(0, max_elements - 1);
    int real_value;

    destroy_cnt = 0;
    te_vec_replace(&vector, index, &value);
    real_value = TE_VEC_GET(int, &vector, index);
    if (real_value != value)
    {
        TEST_VERDICT("Unexpected value for %u: %d != %d",
                     index, real_value, value);
    }
    if (te_vec_size(&vector) != index + 1)
    {
        TEST_VERDICT("Unexpected vector size: %zu != %u",
                     te_vec_size(&vector), index + 1);
    }
    if (destroy_cnt > 0)
        TEST_VERDICT("A destructor called unexpectedly");
    check_zeroes(&vector, 0, index);

    destroy_cnt = 0;
    te_vec_free(&vector);
    if (destroy_cnt != index + 1)
    {
        TEST_VERDICT("Destructor called %u times instead of %u",
                     destroy_cnt, index + 1);
    }
}

static void
check_append_replace(void)
{
    te_vec vector = TE_VEC_INIT_DESTROY(int, count_destroy);
    int value1 = rand();
    int value2 = rand();
    int value3 = rand();

    destroy_cnt = 0;
    TE_VEC_APPEND(&vector, value1);
    TE_VEC_APPEND(&vector, value2);
    te_vec_append(&vector, NULL);

    te_vec_replace(&vector, 1, &value3);
    if (te_vec_size(&vector) != 3)
    {
        TEST_VERDICT("Unexpected vector length: %zu instead of 3",
                     te_vec_size(&vector));
    }
    if (destroy_cnt != 1)
    {
        TEST_VERDICT("Destructor called %u times instead of 1",
                     destroy_cnt);
    }

    if (TE_VEC_GET(int, &vector, 0) != value1)
    {
        ERROR("Unexpected value for item 0: %d instead of %d",
              TE_VEC_GET(int, &vector, 0), value1);
        TEST_VERDICT("Value changed unexpectedly");
    }

    if (TE_VEC_GET(int, &vector, 1) != value3)
    {
        ERROR("Unexpected value for item 1: %d instead of %d",
              TE_VEC_GET(int, &vector, 1), value3);
        TEST_VERDICT("Value has not changed as expected");
    }

    check_zeroes(&vector, 2, 1);

    te_vec_free(&vector);
}

static void
check_remove(unsigned int max_elements)
{
    unsigned int n = rand_range(1, max_elements);
    unsigned int remove_idx_end = rand_range(0, n - 1);
    unsigned int remove_idx_start = rand_range(0, remove_idx_end);
    unsigned int remove_count = remove_idx_end - remove_idx_start + 1;
    te_vec vector = TE_VEC_INIT_DESTROY(int, count_destroy);

    te_vec_append_array(&vector, NULL, n);
    check_zeroes(&vector, 0, n);

    destroy_cnt = 0;
    te_vec_remove(&vector, remove_idx_start, remove_count);
    if (te_vec_size(&vector) != n - remove_count)
    {
        ERROR("Vector size after removel should be %u, but is %zu",
              n - remove_count, te_vec_size(&vector));
        TEST_VERDICT("Improper number of removed elements");
    }
    if (destroy_cnt != remove_count)
    {
        TEST_VERDICT("Destructor called %u times instead of %u",
                     destroy_cnt, remove_count);
    }

    te_vec_free(&vector);
}

static void
check_remove_tail(unsigned int max_elements)
{
    unsigned int n = rand_range(1, max_elements);
    unsigned int remove_idx = rand_range(0, n - 1);
    te_vec vector = TE_VEC_INIT_DESTROY(int, count_destroy);

    te_vec_append_array(&vector, NULL, n);
    check_zeroes(&vector, 0, n);

    destroy_cnt = 0;
    te_vec_remove(&vector, remove_idx, SIZE_MAX);
    if (te_vec_size(&vector) != remove_idx)
    {
        ERROR("Vector size after removel should be %u, but is %zu",
              remove_idx, te_vec_size(&vector));
        TEST_VERDICT("Improper number of removed elements");
    }
    if (destroy_cnt != n - remove_idx)
    {
        TEST_VERDICT("Destructor called %u times instead of %u",
                     destroy_cnt, n - remove_idx);
    }

    te_vec_free(&vector);
}

static void
check_transfer(unsigned int max_elements)
{
    static const int value1 = 1;
    unsigned int n = rand_range(1, max_elements);
    unsigned int to_transfer = rand_range(0, n - 1);
    int dest = 0;
    te_vec vector = TE_VEC_INIT_DESTROY(int, count_destroy);

    destroy_cnt = 0;
    te_vec_append_array(&vector, NULL, n);
    TE_VEC_GET(int, &vector, to_transfer) = value1;
    te_vec_transfer(&vector, to_transfer, &dest);
    if (dest != value1)
    {
        TEST_VERDICT("Unexpected value transferred: %d instead of %d",
                     dest, value1);
    }
    if (destroy_cnt != 0)
        TEST_VERDICT("A destructor called unexpectedly");
    if (TE_VEC_GET(int, &vector, to_transfer) != 0)
        TEST_VERDICT("Transferred element not zeroed");

    te_vec_transfer(&vector, to_transfer, NULL);
    if (destroy_cnt != 1)
        TEST_VERDICT("A destructor called %u times instead of 1", destroy_cnt);

    te_vec_free(&vector);
}

static void
check_transfer_append(unsigned int max_elements)
{
    static const int value1 = 1;
    unsigned int n = rand_range(1, max_elements);
    unsigned int to_transfer = rand_range(0, n - 1);
    te_vec vector = TE_VEC_INIT_DESTROY(int, count_destroy);
    te_vec dest_vec = TE_VEC_INIT(int);

    destroy_cnt = 0;
    te_vec_append_array(&vector, NULL, n);
    TE_VEC_GET(int, &vector, to_transfer) = value1;

    te_vec_transfer_append(&vector, to_transfer, SIZE_MAX, &dest_vec);
    check_zeroes(&vector, 0, n);

    if (destroy_cnt != 0)
        TEST_VERDICT("A destructor called unexpectedly");
    if (te_vec_size(&dest_vec) != n - to_transfer)
    {
        TEST_VERDICT("Destination vector should have %u elemenents, "
                     " but has %zu", n - to_transfer, te_vec_size(&dest_vec));
    }

    if (TE_VEC_GET(int, &dest_vec, 0) != value1)
    {
        TEST_VERDICT("First element of destination vector is %d",
                     TE_VEC_GET(int, &dest_vec, 0));
    }
    check_zeroes(&dest_vec, 1, n - to_transfer - 1);

    te_vec_transfer_append(&vector, to_transfer, SIZE_MAX, NULL);
    if (destroy_cnt != n - to_transfer)
    {
        TEST_VERDICT("A destructor called %u times instead of %u", destroy_cnt,
                     n - to_transfer);
    }

    te_vec_free(&vector);
    te_vec_free(&dest_vec);
}


int
main(int argc, char **argv)
{
    unsigned int i;
    unsigned int n_iterations;
    unsigned int max_elements;

    TEST_START;

    TEST_GET_UINT_PARAM(n_iterations);
    TEST_GET_UINT_PARAM(max_elements);

    TEST_STEP("Checking element replacement with extension");
    for (i = 0; i < n_iterations; i++)
        check_replace_extend(max_elements);

    TEST_STEP("Checking element replacement");
    check_append_replace();

    TEST_STEP("Checking element removal");
    for (i = 0; i < n_iterations; i++)
        check_remove(max_elements);

    TEST_STEP("Checking tail removal");
    for (i = 0; i < n_iterations; i++)
        check_remove_tail(max_elements);

    TEST_STEP("Checking element transferral");
    for (i = 0; i < n_iterations; i++)
        check_transfer(max_elements);

    TEST_STEP("Checking element bulk transferral");
    for (i = 0; i < n_iterations; i++)
        check_transfer_append(max_elements);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
